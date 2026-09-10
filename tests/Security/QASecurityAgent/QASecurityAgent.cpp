#include "QASecurityAgent.h"

namespace Thunder {
namespace Plugin {

namespace {

class SecurityInformation : public PluginHost::ISubSystem::ISecurity {
public:
    explicit SecurityInformation(const string& callsign)
        : _callsign(callsign)
    {
    }

    SecurityInformation(const SecurityInformation&) = delete;
    SecurityInformation& operator=(const SecurityInformation&) = delete;

    ~SecurityInformation() override = default;

    BEGIN_INTERFACE_MAP(SecurityInformation)
        INTERFACE_ENTRY(PluginHost::ISubSystem::ISecurity)
    END_INTERFACE_MAP

public:
    string Callsign() const override
    {
        return _callsign;
    }

private:
    string _callsign;
};

static Metadata<QASecurityAgent> metadata(
    1, 0, 0,
    {},
    {},
    { PluginHost::ISubSystem::SECURITY }
);

} // namespace

QASecurityContext::Policy QASecurityAgent::_policy;

void QASecurityAgent::SetPolicy(
    const QASecurityContext::Policy& policy)
{
    _policy = policy;
}

const string QASecurityAgent::Initialize(
    PluginHost::IShell* service)
{
    ASSERT(service != nullptr);

    /*
     * IMPORTANT:
     *
     * Do NOT publish the SECURITY subsystem here.
     *
     * Calling SubSystems()->Set(SECURITY, ...) from Initialize()
     * causes Thunder's ServiceMap::Security(true) to run while this
     * plugin is still in ACTIVATION.
     *
     * We register the notification here and publish SECURITY from
     * Notification::Activated(), after Thunder has moved the plugin
     * to ACTIVATED.
     */

    service->Register(
        &_notification,
        service->Callsign());

    return {};
}

void QASecurityAgent::Deinitialize(
    PluginHost::IShell* service)
{
    ASSERT(service != nullptr);

    service->Unregister(
        &_notification,
        service->Callsign());
}

string QASecurityAgent::Information() const
{
    return _T("QA Security Agent");
}

/*
 * Called by Thunder after the plugin has reached ACTIVATED.
 */
void QASecurityAgent::Notification::Activated(
    const string& callsign VARIABLE_IS_NOT_USED,
    PluginHost::IShell* plugin)
{
    ASSERT(plugin != nullptr);

    /*
     * This notification was registered against this plugin's
     * callsign, so callsign should be the QASecurityAgent callsign.
     */

    if (plugin == nullptr) {
        return;
    }

    PluginHost::ISubSystem* subsystems =
        plugin->SubSystems();

    if (subsystems == nullptr) {
        SYSLOG(
            Logging::Startup,
            (_T("QASecurityAgent: Unable to obtain subsystem interface")));
        return;
    }

    Core::Sink<SecurityInformation> information(
        plugin->Callsign());

    const Core::hresult result =
        subsystems->Set(
            PluginHost::ISubSystem::SECURITY,
            &information);

    subsystems->Release();

    if (result != Core::ERROR_NONE) {
        SYSLOG(
            Logging::Startup,
            (_T("QASecurityAgent: Failed to register Security subsystem, error=%u"),
             result));
    } else {
        SYSLOG(
            Logging::Startup,
            (_T("QASecurityAgent: Security subsystem registered after activation")));
    }
}

void QASecurityAgent::Notification::Deactivated(
    const string& callsign VARIABLE_IS_NOT_USED,
    PluginHost::IShell* plugin VARIABLE_IS_NOT_USED)
{
    /*
     * Thunder manages the subsystem lifecycle during plugin
     * deactivation. Nothing is required here.
     */
}

void QASecurityAgent::Notification::Unavailable(
    const string& callsign VARIABLE_IS_NOT_USED,
    PluginHost::IShell* plugin VARIABLE_IS_NOT_USED)
{
    /*
     * Nothing required for the QA plugin.
     */
}

bool QASecurityAgent::EncodeToken(
    const TokenPolicy policy,
    string& token)
{
    switch (policy) {
    case TokenPolicy::PATH_ALLOW:
        token = _T("PATH_ALLOW");
        break;

    case TokenPolicy::HTTP_ALLOW:
        token = _T("HTTP_ALLOW");
        break;

    case TokenPolicy::JSONRPC_ALLOW:
        token = _T("JSONRPC_ALLOW");
        break;

    case TokenPolicy::ALL_ALLOW:
        token = _T("ALL_ALLOW");
        break;

    case TokenPolicy::DENY:
    default:
        token = _T("DENY");
        break;
    }

    return true;
}

bool QASecurityAgent::DecodeToken(
    const string& token,
    TokenPolicy& policy)
{
    if (token == _T("PATH_ALLOW")) {
        policy = TokenPolicy::PATH_ALLOW;

    } else if (token == _T("HTTP_ALLOW")) {
        policy = TokenPolicy::HTTP_ALLOW;

    } else if (token == _T("JSONRPC_ALLOW")) {
        policy = TokenPolicy::JSONRPC_ALLOW;

    } else if (token == _T("ALL_ALLOW")) {
        policy = TokenPolicy::ALL_ALLOW;

    } else if (token == _T("DENY")) {
        policy = TokenPolicy::DENY;

    } else {
        return false;
    }

    return true;
}

Core::hresult QASecurityAgent::CreateToken(
    const uint16_t length,
    const uint8_t buffer[],
    string& token)
{
    TokenPolicy policy = TokenPolicy::DENY;

    if ((buffer != nullptr) && (length != 0)) {
        const string payload(
            reinterpret_cast<const char*>(buffer),
            length);

        if (payload == _T("PATH_ALLOW")) {
            policy = TokenPolicy::PATH_ALLOW;

        } else if (payload == _T("HTTP_ALLOW")) {
            policy = TokenPolicy::HTTP_ALLOW;

        } else if (payload == _T("JSONRPC_ALLOW")) {
            policy = TokenPolicy::JSONRPC_ALLOW;

        } else if (payload == _T("ALL_ALLOW")) {
            policy = TokenPolicy::ALL_ALLOW;
        }
    }

    return (
        EncodeToken(policy, token)
            ? Core::ERROR_NONE
            : Core::ERROR_GENERAL);
}

PluginHost::ISecurity* QASecurityAgent::Officer(
    const string& token)
{
    TokenPolicy tokenPolicy;

    if (DecodeToken(token, tokenPolicy) == false) {
        return nullptr;
    }

    QASecurityContext::Policy policy;

    switch (tokenPolicy) {
    case TokenPolicy::PATH_ALLOW:
        policy.path =
            QASecurityContext::Decision::ALLOW;
        break;

    case TokenPolicy::HTTP_ALLOW:
        policy.http =
            QASecurityContext::Decision::ALLOW;
        break;

    case TokenPolicy::JSONRPC_ALLOW:
        policy.http =
            QASecurityContext::Decision::ALLOW;
        policy.jsonrpc =
            QASecurityContext::Decision::ALLOW;
        break;

    case TokenPolicy::ALL_ALLOW:
        policy.path =
            QASecurityContext::Decision::ALLOW;

        policy.http =
            QASecurityContext::Decision::ALLOW;

        policy.jsonrpc =
            QASecurityContext::Decision::ALLOW;
        break;

    case TokenPolicy::DENY:
    default:
        break;
    }

    return Core::ServiceType<QASecurityContext>::Create<QASecurityContext>(
        token,
        policy);
}

} // namespace Plugin
} // namespace Thunder
