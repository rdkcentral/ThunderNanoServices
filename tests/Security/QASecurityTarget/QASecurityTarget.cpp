#include "QASecurityTarget.h"

namespace Thunder {
namespace Plugin {

namespace {

static Metadata<QASecurityTarget> metadata(
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


void QASecurityTarget::RegisterAll()
{
    Register<void, void>(
        _T("echo"),
        &QASecurityTarget::endpoint_echo,
        this);
}


void QASecurityTarget::UnregisterAll()
{
    Unregister(_T("echo"));
}


uint32_t QASecurityTarget::endpoint_echo()
{
    return Core::ERROR_NONE;
}


const string QASecurityTarget::Initialize(
    PluginHost::IShell* service)
{
    ASSERT(service != nullptr);

    RegisterAll();

    return {};
}


void QASecurityTarget::Deinitialize(
    PluginHost::IShell* service)
{
    ASSERT(service != nullptr);

    UnregisterAll();
}


string QASecurityTarget::Information() const
{
    return _T("QA Security Target");
}

void QASecurityTarget::Inbound(
    Web::Request& request)
{
    // No special processing is required.
}

Core::ProxyType<Web::Response> QASecurityTarget::Process(
    const Web::Request& request)
{
    Core::ProxyType<Web::Response> response =
        Core::ProxyType<Web::Response>::Create();

    if (request.Path == "/Service/QASecurityTarget/http") {
        response->ErrorCode = Web::STATUS_OK;
    } else {
        response->ErrorCode = Web::STATUS_NOT_FOUND;
    }

    return response;
}

} // namespace Plugin
} // namespace Thunder