// ==========================================================================
// TestPluginTest — validates that the test runtime can load a plugin
// and interact with it via both COM-RPC and JSON-RPC.
//
// COM-RPC tests use QueryInterface<ITestPlugin> to call methods directly.
// JSON-RPC tests use ThunderTestRuntime::Invoke() and JSONRPCLink.
//
// The TestPlugin is built as a shared library and placed in
// ${CMAKE_BINARY_DIR}/test_plugins/. The test passes that directory as
// the systemPath so the embedded server can dlopen it.
// ==========================================================================

#include "Module.h"
#include <test_support/ThunderTestRuntime.h>
#include <qa_interfaces/ITestPlugin.h>
#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>
#include <string>

#ifndef TEST_PLUGIN_PATH
#error "TEST_PLUGIN_PATH must be defined by CMake"
#endif

namespace Thunder {
namespace TestCore {
namespace Tests {

    class TestPluginTest : public ::testing::Test {
    protected:
        static ThunderTestRuntime _runtime;

        static void SetUpTestSuite()
        {
            ThunderTestRuntime::PluginConfig dummyConfig;
            dummyConfig.Callsign = "TestPlugin";
            dummyConfig.ClassName = "TestPlugin";
            dummyConfig.Locator = "libThunderTestPlugin.so";
            dummyConfig.StartMode = PluginHost::IShell::startmode::ACTIVATED;

            std::vector<ThunderTestRuntime::PluginConfig> plugins;
            plugins.push_back(dummyConfig);

            uint32_t result = _runtime.Initialize(plugins, TEST_PLUGIN_PATH);
            ASSERT_EQ(result, Core::ERROR_NONE) << "Failed to initialize runtime with TestPlugin";
        }

        static void TearDownTestSuite()
        {
            _runtime.Deinitialize();
        }
    };

    ThunderTestRuntime TestPluginTest::_runtime;

    // ==================================================================
    // Plugin lifecycle
    // ==================================================================

    TEST_F(TestPluginTest, PluginIsActivated)
    {
        auto shell = _runtime.GetShell("TestPlugin");
        EXPECT_TRUE(shell.IsValid()) << "TestPlugin IShell must be available";
        if (shell.IsValid()) {
            EXPECT_EQ(shell->State(), PluginHost::IShell::state::ACTIVATED);
        }
    }

    // ==================================================================
    // COM-RPC path (QueryInterface<ITestPlugin>)
    // ==================================================================

    TEST_F(TestPluginTest, COMRPC_QueryInterfaceSucceeds)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr) << "QueryInterface<ITestPlugin> must succeed";
        iface->Release();
    }

    TEST_F(TestPluginTest, COMRPC_EchoReturnsInput)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        string output;
        uint32_t result = iface->Echo("hello", output);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(output, "hello");

        iface->Release();
    }

    TEST_F(TestPluginTest, COMRPC_GreetReturnsMessage)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        string message;
        uint32_t result = iface->Greet("Thunder", message);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(message, "Hello, Thunder!");

        iface->Release();
    }

    TEST_F(TestPluginTest, COMRPC_GreetDefaultsToWorld)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        string message;
        uint32_t result = iface->Greet("", message);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(message, "Hello, World!");

        iface->Release();
    }

    TEST_F(TestPluginTest, COMRPC_EchoEmptyString)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        string output;
        uint32_t result = iface->Echo("", output);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_TRUE(output.empty());

        iface->Release();
    }

    // ==================================================================
    // JSON-RPC path (full designator via Invoke)
    // ==================================================================

    TEST_F(TestPluginTest, JSONRPC_EchoReturnsInput)
    {
        string response;
        uint32_t result = _runtime.Invoke("TestPlugin.echo", R"({"input":"hello"})", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "echo returned: " << result;
        EXPECT_FALSE(response.empty());
        EXPECT_NE(response.find("hello"), string::npos) << "response: " << response;
    }

    TEST_F(TestPluginTest, JSONRPC_GreetReturnsMessage)
    {
        string response;
        uint32_t result = _runtime.Invoke("TestPlugin.greet", R"({"name":"Thunder"})", response);
        EXPECT_EQ(result, Core::ERROR_NONE) << "greet returned: " << result;
        EXPECT_NE(response.find("Hello, Thunder!"), string::npos) << "response: " << response;
    }

    TEST_F(TestPluginTest, JSONRPC_UnknownMethodReturnsError)
    {
        string response;
        uint32_t result = _runtime.Invoke("TestPlugin.nonexistent", "{}", response);
        EXPECT_EQ(result, Core::ERROR_UNKNOWN_KEY);
    }

    // ==================================================================
    // JSON-RPC path (JSONRPCLink — callsign-bound)
    // ==================================================================

    TEST_F(TestPluginTest, JSONRPC_EchoViaLink)
    {
        auto* link = _runtime.CreateJSONRPCLink("TestPlugin");
        ASSERT_NE(link, nullptr);

        string response;
        uint32_t result = link->Invoke("echo", R"({"input":"linked"})", response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_NE(response.find("linked"), string::npos) << "response: " << response;

        delete link;
    }

    TEST_F(TestPluginTest, JSONRPC_GreetViaLink)
    {
        auto* link = _runtime.CreateJSONRPCLink("TestPlugin");
        ASSERT_NE(link, nullptr);

        string response;
        uint32_t result = link->Invoke("greet", R"({"name":"Link"})", response);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_NE(response.find("Hello, Link!"), string::npos) << "response: " << response;

        delete link;
    }

    // ==================================================================
    // COM-RPC event path (INotification sink)
    // ==================================================================

    class GreetingNotificationSink : public QualityAssurance::ITestPlugin::INotification {
    public:
        GreetingNotificationSink() = default;
        ~GreetingNotificationSink() override = default;

        void OnGreeting(const string& message) override
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _lastMessage = message;
            _count++;
            _cv.notify_one();
        }

        bool WaitForEvent(uint32_t timeoutMs = 2000)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            return _cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return _count > 0; });
        }

        string LastMessage()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            return _lastMessage;
        }

        uint32_t Count()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            return _count;
        }

        void Reset()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _lastMessage.clear();
            _count = 0;
        }

        BEGIN_INTERFACE_MAP(GreetingNotificationSink)
            INTERFACE_ENTRY(QualityAssurance::ITestPlugin::INotification)
        END_INTERFACE_MAP

    private:
        std::mutex _mutex;
        std::condition_variable _cv;
        string _lastMessage;
        uint32_t _count = 0;
    };

    TEST_F(TestPluginTest, COMRPC_NotificationOnGreet)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        Core::SinkType<GreetingNotificationSink> sink;

        uint32_t result = iface->Register(&sink);
        EXPECT_EQ(result, Core::ERROR_NONE);

        string message;
        result = iface->Greet("EventTest", message);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(message, "Hello, EventTest!");

        EXPECT_TRUE(sink.WaitForEvent()) << "Notification was not received within timeout";
        EXPECT_EQ(sink.LastMessage(), "Hello, EventTest!");
        EXPECT_EQ(sink.Count(), 1u);

        result = iface->Unregister(&sink);
        EXPECT_EQ(result, Core::ERROR_NONE);

        iface->Release();
    }

    TEST_F(TestPluginTest, COMRPC_NoNotificationAfterUnregister)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        Core::SinkType<GreetingNotificationSink> sink;

        uint32_t result = iface->Register(&sink);
        EXPECT_EQ(result, Core::ERROR_NONE);

        // First greet — should trigger notification
        string message;
        result = iface->Greet("First", message);
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_TRUE(sink.WaitForEvent());
        EXPECT_EQ(sink.Count(), 1u);

        // Unregister, then greet again — should NOT trigger notification
        result = iface->Unregister(&sink);
        EXPECT_EQ(result, Core::ERROR_NONE);

        sink.Reset();
        result = iface->Greet("Second", message);
        EXPECT_EQ(result, Core::ERROR_NONE);

        // Give a short window to catch a spurious notification
        EXPECT_FALSE(sink.WaitForEvent(200)) << "Notification received after Unregister";
        EXPECT_EQ(sink.Count(), 0u);

        iface->Release();
    }

    TEST_F(TestPluginTest, COMRPC_MultipleGreetsTriggerMultipleNotifications)
    {
        auto* iface = _runtime.GetInterface<QualityAssurance::ITestPlugin>("TestPlugin");
        ASSERT_NE(iface, nullptr);

        Core::SinkType<GreetingNotificationSink> sink;

        uint32_t result = iface->Register(&sink);
        EXPECT_EQ(result, Core::ERROR_NONE);

        string message;
        iface->Greet("One", message);
        iface->Greet("Two", message);
        iface->Greet("Three", message);

        // Wait for at least some events, then check count
        sink.WaitForEvent();
        EXPECT_EQ(sink.Count(), 3u);
        EXPECT_EQ(sink.LastMessage(), "Hello, Three!");

        result = iface->Unregister(&sink);
        EXPECT_EQ(result, Core::ERROR_NONE);

        iface->Release();
    }

    // ==================================================================
    // JSON-RPC event path (Subscribe via JSONRPCLink)
    // ==================================================================

    TEST_F(TestPluginTest, JSONRPC_SubscribeReceivesEvent)
    {
        auto* link = _runtime.CreateJSONRPCLink("TestPlugin");
        ASSERT_NE(link, nullptr);

        std::mutex mtx;
        std::condition_variable cv;
        string receivedParams;
        bool eventFired = false;

        uint32_t result = link->Subscribe("ongreeting", [&](const string& params) {
            std::lock_guard<std::mutex> lock(mtx);
            receivedParams = params;
            eventFired = true;
            cv.notify_one();
        });
        EXPECT_EQ(result, Core::ERROR_NONE) << "Subscribe failed";

        // Trigger the event via JSON-RPC greet call
        string response;
        result = link->Invoke("greet", R"({"name":"Subscriber"})", response);
        EXPECT_EQ(result, Core::ERROR_NONE);

        {
            std::unique_lock<std::mutex> lock(mtx);
            bool received = cv.wait_for(lock, std::chrono::milliseconds(2000), [&] { return eventFired; });
            EXPECT_TRUE(received) << "JSON-RPC event not received within timeout";
            EXPECT_NE(receivedParams.find("Hello, Subscriber!"), string::npos)
                << "event params: " << receivedParams;
        }

        link->Unsubscribe("ongreeting");
        delete link;
    }

    TEST_F(TestPluginTest, JSONRPC_NoEventAfterUnsubscribe)
    {
        auto* link = _runtime.CreateJSONRPCLink("TestPlugin");
        ASSERT_NE(link, nullptr);

        std::mutex mtx;
        std::condition_variable cv;
        bool eventFired = false;

        uint32_t result = link->Subscribe("ongreeting", [&](const string& params) {
            std::lock_guard<std::mutex> lock(mtx);
            eventFired = true;
            cv.notify_one();
        });
        EXPECT_EQ(result, Core::ERROR_NONE);

        link->Unsubscribe("ongreeting");

        // Trigger the event — should NOT be received
        string response;
        link->Invoke("greet", R"({"name":"Ghost"})", response);

        {
            std::unique_lock<std::mutex> lock(mtx);
            bool received = cv.wait_for(lock, std::chrono::milliseconds(200), [&] { return eventFired; });
            EXPECT_FALSE(received) << "JSON-RPC event received after Unsubscribe";
        }

        delete link;
    }

} // namespace Tests
} // namespace TestCore
} // namespace Thunder
