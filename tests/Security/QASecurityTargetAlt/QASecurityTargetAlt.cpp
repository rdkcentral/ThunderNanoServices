#include "QASecurityTargetAlt.h"

namespace Thunder {
namespace Plugin {

namespace {

static Metadata<QASecurityTargetAlt> metadata(
    // Version
    1, 0, 0,
    // Preconditions
    { PluginHost::ISubSystem::SECURITY },
    // Terminations
    {},
    // Controls
    {}
);

} // namespace


void QASecurityTargetAlt::RegisterAll()
{
    Register<void, void>(
        _T("echo"),
        &QASecurityTargetAlt::endpoint_echo,
        this);
}


void QASecurityTargetAlt::UnregisterAll()
{
    Unregister(_T("echo"));
}


uint32_t QASecurityTargetAlt::endpoint_echo()
{
    return Core::ERROR_NONE;
}


const string QASecurityTargetAlt::Initialize(
    PluginHost::IShell* service VARIABLE_IS_NOT_USED)
{
    ASSERT(service != nullptr);

    RegisterAll();

    return {};
}


void QASecurityTargetAlt::Deinitialize(
    PluginHost::IShell* service VARIABLE_IS_NOT_USED)
{
    ASSERT(service != nullptr);

    UnregisterAll();
}


string QASecurityTargetAlt::Information() const
{
    return _T("QA Security Target");
}

void QASecurityTargetAlt::Inbound(
    Web::Request& request VARIABLE_IS_NOT_USED)
{
    // No special processing is required.
}

Core::ProxyType<Web::Response> QASecurityTargetAlt::Process(
    const Web::Request& request)
{
    Core::ProxyType<Web::Response> response =
        Core::ProxyType<Web::Response>::Create();

    if (request.Path == "/Service/QASecurityTargetAlt/http") {
        response->ErrorCode = Web::STATUS_OK;
    } else {
        response->ErrorCode = Web::STATUS_NOT_FOUND;
    }

    return response;
}

} // namespace Plugin
} // namespace Thunder
