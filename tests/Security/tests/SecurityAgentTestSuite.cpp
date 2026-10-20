#include "Module.h"
#include <test_support/ThunderTestRuntime.h>

#include "QASecurityContext.h"

#include <gtest/gtest.h>
#include <plugins/IPlugin.h>
#include <websocket/websocket.h>

namespace Thunder {
namespace TestCore {
namespace Tests {

namespace {

static constexpr uint16_t SECURITY_TEST_PORT = 12350;

/*
 * ============================================================================
 * Test fixture
 * ============================================================================
 */

class QASecurityAgentTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        ThunderTestRuntime::PluginConfig securityAgent;
        securityAgent.Callsign = QASECURITYAGENT_TEST_CALLSIGN;
        securityAgent.ClassName = QASECURITYAGENT_TEST_CLASSNAME;
        securityAgent.Locator = QASECURITYAGENT_TEST_LOCATOR;
        securityAgent.Resumed = false;

        ThunderTestRuntime::PluginConfig securityTarget;
        securityTarget.Callsign = QASECURITYTARGET_TEST_CALLSIGN;
        securityTarget.ClassName = QASECURITYTARGET_TEST_CLASSNAME;
        securityTarget.Locator = QASECURITYTARGET_TEST_LOCATOR;
        securityTarget.Resumed = false;

        const uint32_t result = _runtime.Initialize(
            { securityAgent, securityTarget },
            QASECURITYAGENT_TEST_PLUGIN_PATH,
            "",
            SECURITY_TEST_PORT);

        ASSERT_EQ(result, Core::ERROR_NONE);
    }

    static void TearDownTestSuite()
    {
        _runtime.Deinitialize();
    }

    void SetUp() override
    {
        _authenticate = _runtime.QueryInterfaceByCallsign<PluginHost::IAuthenticate>(QASECURITYAGENT_TEST_CALLSIGN);
        ASSERT_NE(_authenticate, nullptr);
    }

    void TearDown() override
    {
        if (_authenticate != nullptr) {
            _authenticate->Release();
            _authenticate = nullptr;
        }
    }

protected:
    static ThunderTestRuntime _runtime;
    PluginHost::IAuthenticate* _authenticate { nullptr };
};

ThunderTestRuntime QASecurityAgentTest::_runtime;

/*
 * ============================================================================
 * JSON-RPC WebSocket client
 * ============================================================================
 *
 * The client is used for the real transport-level security tests.
 *
 * Security is supplied through:
 *
 *     ?token=<token>
 *
 * and the JSON-RPC request is sent over the established WebSocket.
 *
 * We intentionally do not copy Core::JSONRPC::Message because Thunder
 * deletes its copy assignment operator.
 */

class JSONRPCMessageFactory : public Core::ProxyPoolType<Core::JSONRPC::Message> {
public:
    JSONRPCMessageFactory() = delete;
    JSONRPCMessageFactory(const JSONRPCMessageFactory&) = delete;
    JSONRPCMessageFactory& operator=(const JSONRPCMessageFactory&) = delete;

    explicit JSONRPCMessageFactory(const uint32_t number)
        : Core::ProxyPoolType<Core::JSONRPC::Message>(number)
    {
    }

    ~JSONRPCMessageFactory() = default;

    Core::ProxyType<Core::JSON::IElement> Element(const string&)
    {
        return Core::ProxyType<Core::JSON::IElement>(Core::ProxyPoolType<Core::JSONRPC::Message>::Element());
    }
};

struct ClientFactoryBase {
protected:
    JSONRPCMessageFactory _objectFactory;

    explicit ClientFactoryBase(const uint32_t count)
        : _objectFactory(count)
    {
    }
};

class SecurityWebSocketClient
    : private ClientFactoryBase
    , public Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, JSONRPCMessageFactory&, Core::JSON::IElement> {
private:
    using BaseClass = Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, JSONRPCMessageFactory&, Core::JSON::IElement>;

public:
    SecurityWebSocketClient(const string& path, const string& query, const Core::NodeId& remoteNode)
        : ClientFactoryBase(5)
        , BaseClass(5, _objectFactory, path, _T("jsonrpc"), query, _T(""), false, true, false,
              remoteNode.AnyInterface(), remoteNode, 1024, 1024)
    {
    }

    ~SecurityWebSocketClient() override = default;

    void Received(Core::ProxyType<Core::JSON::IElement>&) { }
    void Send(Core::ProxyType<Core::JSON::IElement>&) { }
    void StateChange() override { }
    bool IsIdle() const override { return true; }
};

/*
 * ============================================================================
 * HTTP client
 * ============================================================================
 */

class HTTPResponseFactory : public Core::ProxyPoolType<Web::Response> {
public:
    HTTPResponseFactory() = delete;
    HTTPResponseFactory(const HTTPResponseFactory&) = delete;
    HTTPResponseFactory& operator=(const HTTPResponseFactory&) = delete;

    explicit HTTPResponseFactory(const uint32_t number)
        : Core::ProxyPoolType<Web::Response>(number)
    {
    }

    ~HTTPResponseFactory() = default;

    Core::ProxyType<Web::Response> Element()
    {
        return Core::ProxyPoolType<Web::Response>::Element();
    }
};

class SecurityHTTPClient
    : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Core::ProxyPoolType<Web::Response>&> {
private:
    using BaseClass = Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Core::ProxyPoolType<Web::Response>&>;

    static constexpr uint32_t MAX_WAIT_TIME_MS = 2000;

public:
    SecurityHTTPClient() = delete;
    SecurityHTTPClient(const SecurityHTTPClient&) = delete;
    SecurityHTTPClient& operator=(const SecurityHTTPClient&) = delete;

    SecurityHTTPClient(const Core::NodeId& remoteNode, const string& path, const string& token)
        : BaseClass(5, _responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 2048)
        , _path(path)
        , _token(token)
        , _responseReceived(false, false)
    {
    }

    ~SecurityHTTPClient() override
    {
        EXPECT_EQ(Close(MAX_WAIT_TIME_MS), Core::ERROR_NONE);
    }

    void LinkBody(Core::ProxyType<Web::Response>&) override { }

    void Received(Core::ProxyType<Web::Response>& response) override
    {
        _statusCode = response->ErrorCode;
        EXPECT_EQ(_responseReceived.Unlock(), Core::ERROR_NONE);
    }

    virtual void Send(const Core::ProxyType<Web::Request>& request) override
    {
        EXPECT_EQ(request->Verb, Web::Request::HTTP_GET);
    }

    void StateChange() override { } 

    uint32_t Execute(const uint32_t timeoutMs)
    {
        _statusCode = 0;

        Core::ProxyType<Web::Request> request = Core::ProxyType<Web::Request>::Create();

        request->Verb = Web::Request::HTTP_GET;
        request->Path = _path;
        request->MajorVersion = 1;
        request->MinorVersion = 1;
        request->WebToken = Web::Authorization(Web::Authorization::BEARER, _token);

        const uint32_t result = Open(timeoutMs);

        if (result != Core::ERROR_NONE) {
            return result;
        }

        if (Submit(request) == false) {
            Close(MAX_WAIT_TIME_MS);
            return Core::ERROR_GENERAL;
        }

        return _responseReceived.Lock(timeoutMs);
    }

    uint16_t StatusCode() const
    {
        return _statusCode;
    }

private:
    string _path;
    string _token;

    Core::Event _responseReceived;

    uint16_t _statusCode { 0 };

    static Core::ProxyPoolType<Web::Response> _responseFactory;
};

Core::ProxyPoolType<Web::Response> SecurityHTTPClient::_responseFactory(5);

/*
 * ============================================================================
 * Authentication interface
 * ============================================================================
 */

TEST_F(QASecurityAgentTest, AuthenticationInterfaceAvailable)
{
    ASSERT_NE(_authenticate, nullptr);
}

/*
 * ============================================================================
 * CreateToken
 * ============================================================================
 *
 * Verify every supported input generates the expected token.
 */

TEST_F(QASecurityAgentTest, CreateTokenPolicies)
{
    {
        const uint8_t payload[] = { 'P', 'A', 'T', 'H', '_', 'A', 'L', 'L', 'O', 'W' };
        string token;

        ASSERT_EQ(_authenticate->CreateToken(sizeof(payload), payload, token), Core::ERROR_NONE);
        EXPECT_EQ(token, "PATH_ALLOW");
    }

    {
        const uint8_t payload[] = { 'H', 'T', 'T', 'P', '_', 'A', 'L', 'L', 'O', 'W' };
        string token;

        ASSERT_EQ(_authenticate->CreateToken(sizeof(payload), payload, token), Core::ERROR_NONE);
        EXPECT_EQ(token, "HTTP_ALLOW");
    }

    {
        const uint8_t payload[] = { 'J', 'S', 'O', 'N', 'R', 'P', 'C', '_', 'A', 'L', 'L', 'O', 'W' };
        string token;

        ASSERT_EQ(_authenticate->CreateToken(sizeof(payload), payload, token), Core::ERROR_NONE);
        EXPECT_EQ(token, "JSONRPC_ALLOW");
    }

    {
        const uint8_t payload[] = { 'A', 'L', 'L', '_', 'A', 'L', 'L', 'O', 'W' };
        string token;

        ASSERT_EQ(_authenticate->CreateToken(sizeof(payload), payload, token), Core::ERROR_NONE);
        EXPECT_EQ(token, "ALL_ALLOW");
    }

    {
        const uint8_t payload[] = { 'U', 'N', 'K', 'N', 'O', 'W', 'N' };
        string token;

        ASSERT_EQ(_authenticate->CreateToken(sizeof(payload), payload, token), Core::ERROR_NONE);
        EXPECT_EQ(token, "DENY");
    }
}

/*
 * ============================================================================
 * Officer
 * ============================================================================
 *
 * Every valid token must produce a security context.
 */

TEST_F(QASecurityAgentTest, OfficerReturnsSecurityContext)
{
    const char* tokens[] = {
        "PATH_ALLOW",
        "HTTP_ALLOW",
        "JSONRPC_ALLOW",
        "ALL_ALLOW",
        "DENY"
    };

    for (const char* token : tokens) {
        PluginHost::ISecurity* security = _authenticate->Officer(token);

        ASSERT_NE(security, nullptr) << "Officer() returned nullptr for token: " << token;
        EXPECT_EQ(security->Token(), token);

        security->Release();
    }
}

/*
 * ============================================================================
 * Officer token -> policy mapping
 * ============================================================================
 */

TEST_F(QASecurityAgentTest, OfficerTokenPolicies)
{
    // PATH_ALLOW
    {
        PluginHost::ISecurity* security = _authenticate->Officer("PATH_ALLOW");
        ASSERT_NE(security, nullptr);

        EXPECT_TRUE(security->Allowed("/Service/QASecurityTarget"));
        EXPECT_FALSE(security->Allowed(Web::Request()));
        EXPECT_FALSE(security->Allowed(Core::JSONRPC::Message()));

        security->Release();
    }

    // HTTP_ALLOW
    {
        PluginHost::ISecurity* security = _authenticate->Officer("HTTP_ALLOW");
        ASSERT_NE(security, nullptr);

        EXPECT_FALSE(security->Allowed("/Service/QASecurityTarget"));
        EXPECT_TRUE(security->Allowed(Web::Request()));
        EXPECT_FALSE(security->Allowed(Core::JSONRPC::Message()));

        security->Release();
    }

    // JSONRPC_ALLOW
    //
    // JSON-RPC is transported over HTTP in the daemon.
    // Therefore the token must allow both the HTTP-level
    // request and the JSON-RPC-level message.
    {
        PluginHost::ISecurity* security = _authenticate->Officer("JSONRPC_ALLOW");
        ASSERT_NE(security, nullptr);

        EXPECT_FALSE(security->Allowed("/Service/QASecurityTarget"));
        EXPECT_TRUE(security->Allowed(Web::Request()));
        EXPECT_TRUE(security->Allowed(Core::JSONRPC::Message()));

        security->Release();
    }

    // ALL_ALLOW
    {
        PluginHost::ISecurity* security = _authenticate->Officer("ALL_ALLOW");
        ASSERT_NE(security, nullptr);

        EXPECT_TRUE(security->Allowed("/Service/QASecurityTarget"));
        EXPECT_TRUE(security->Allowed(Web::Request()));
        EXPECT_TRUE(security->Allowed(Core::JSONRPC::Message()));

        security->Release();
    }

    // DENY
    {
        PluginHost::ISecurity* security = _authenticate->Officer("DENY");
        ASSERT_NE(security, nullptr);

        EXPECT_FALSE(security->Allowed("/Service/QASecurityTarget"));
        EXPECT_FALSE(security->Allowed(Web::Request()));
        EXPECT_FALSE(security->Allowed(Core::JSONRPC::Message()));

        security->Release();
    }
}

/*
 * ============================================================================
 * Invalid token
 * ============================================================================
 */

TEST_F(QASecurityAgentTest, InvalidTokenIsRejected)
{
    PluginHost::ISecurity* security = _authenticate->Officer("INVALID_TOKEN");
    EXPECT_EQ(security, nullptr);
}

/*
 * ============================================================================
 * Security target availability
 * ============================================================================
 */

TEST_F(QASecurityAgentTest, SecurityTargetIsAvailable)
{
    PluginHost::IPlugin* target = _runtime.QueryInterfaceByCallsign<PluginHost::IPlugin>(QASECURITYTARGET_TEST_CALLSIGN);
    ASSERT_NE(target, nullptr);

    target->Release();
}

/*
 * ============================================================================
 * Real WebSocket PATH_ALLOW
 * ============================================================================
 */

TEST_F(QASecurityAgentTest, WebSocketPathAllow)
{
    SecurityWebSocketClient client(
        "/Service/QASecurityTarget",
        "token=PATH_ALLOW",
        Core::NodeId("127.0.0.1", SECURITY_TEST_PORT));

    EXPECT_EQ(client.Open(5000), Core::ERROR_NONE);
    EXPECT_TRUE(client.IsOpen());
    EXPECT_EQ(client.Close(5000), Core::ERROR_NONE);
}

/*
 * ============================================================================
 * Real WebSocket PATH_DENY
 * ============================================================================
 */

TEST_F(QASecurityAgentTest, WebSocketPathDenied)
{
    SecurityWebSocketClient client(
        "/Service/QASecurityTarget",
        "token=DENY",
        Core::NodeId("127.0.0.1", SECURITY_TEST_PORT));

    const uint32_t result = client.Open(1000);

    EXPECT_NE(result, Core::ERROR_NONE);
    EXPECT_FALSE(client.IsOpen());
    EXPECT_EQ(client.Close(1000), Core::ERROR_NONE);
}

/*
 * ============================================================================
 * Direct QASecurityContext policy tests
 * ============================================================================
 */

TEST(QASecurityContextTest, IndividualPolicies)
{
    using Context = Plugin::QASecurityContext;

    // PATH only
    {
        Context::Policy policy;
        policy.path = Context::Decision::ALLOW;

        Context context("PATH", policy);

        EXPECT_TRUE(context.Allowed("/QASecurityTarget"));
        EXPECT_FALSE(context.Allowed(Web::Request()));
        EXPECT_FALSE(context.Allowed(Core::JSONRPC::Message()));

        EXPECT_EQ(context.PathCalls(), 1);
        EXPECT_EQ(context.HttpCalls(), 1);
        EXPECT_EQ(context.JsonRpcCalls(), 1);
    }

    // HTTP only
    {
        Context::Policy policy;
        policy.http = Context::Decision::ALLOW;

        Context context("HTTP", policy);

        EXPECT_FALSE(context.Allowed("/QASecurityTarget"));
        EXPECT_TRUE(context.Allowed(Web::Request()));
        EXPECT_FALSE(context.Allowed(Core::JSONRPC::Message()));

        EXPECT_EQ(context.PathCalls(), 1);
        EXPECT_EQ(context.HttpCalls(), 1);
        EXPECT_EQ(context.JsonRpcCalls(), 1);
    }

    // JSON-RPC only
    {
        Context::Policy policy;
        policy.jsonrpc = Context::Decision::ALLOW;

        Context context("JSONRPC", policy);

        EXPECT_FALSE(context.Allowed("/QASecurityTarget"));
        EXPECT_FALSE(context.Allowed(Web::Request()));
        EXPECT_TRUE(context.Allowed(Core::JSONRPC::Message()));

        EXPECT_EQ(context.PathCalls(), 1);
        EXPECT_EQ(context.HttpCalls(), 1);
        EXPECT_EQ(context.JsonRpcCalls(), 1);
    }

    // DENY
    {
        Context::Policy policy;
        Context context("DENY", policy);

        EXPECT_FALSE(context.Allowed("/QASecurityTarget"));
        EXPECT_FALSE(context.Allowed(Web::Request()));
        EXPECT_FALSE(context.Allowed(Core::JSONRPC::Message()));

        EXPECT_EQ(context.PathCalls(), 1);
        EXPECT_EQ(context.HttpCalls(), 1);
        EXPECT_EQ(context.JsonRpcCalls(), 1);
    }

    // ALL
    {
        Context::Policy policy;
        policy.path = Context::Decision::ALLOW;
        policy.http = Context::Decision::ALLOW;
        policy.jsonrpc = Context::Decision::ALLOW;

        Context context("ALL", policy);

        EXPECT_TRUE(context.Allowed("/QASecurityTarget"));
        EXPECT_TRUE(context.Allowed(Web::Request()));
        EXPECT_TRUE(context.Allowed(Core::JSONRPC::Message()));

        EXPECT_EQ(context.PathCalls(), 1);
        EXPECT_EQ(context.HttpCalls(), 1);
        EXPECT_EQ(context.JsonRpcCalls(), 1);
    }
}

/*
 * ============================================================================
 * HTTP tests
 * ============================================================================
 */

TEST_F(QASecurityAgentTest, HTTPAllow)
{
    SecurityHTTPClient client(
        Core::NodeId("127.0.0.1", SECURITY_TEST_PORT),
        "/Service/QASecurityTarget/http",
        "HTTP_ALLOW");

    ASSERT_EQ(client.Execute(2000), Core::ERROR_NONE);
    EXPECT_EQ(client.StatusCode(), Web::STATUS_OK);
}

TEST_F(QASecurityAgentTest, HTTPDenied)
{
    SecurityHTTPClient client(
        Core::NodeId("127.0.0.1", SECURITY_TEST_PORT),
        "/Service/QASecurityTarget/http",
        "DENY");

    ASSERT_EQ(client.Execute(2000), Core::ERROR_NONE);
    EXPECT_EQ(client.StatusCode(), Web::STATUS_UNAUTHORIZED);
}

} // namespace
} // namespace Tests
} // namespace TestCore
} // namespace Thunder