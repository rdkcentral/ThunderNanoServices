#include "ProcessMonitor.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(ProcessMonitor, 1, 0);

/* virtual */const string ProcessMonitor::Initialize(PluginHost::IShell *service)
{
    _config.FromString(service->ConfigLine());
    _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

    _localNotification->Open(service, _config.ExitTimeout.Value());

    service->Register(_localNotification);
    service->Register(&_remoteNotification);

    return (_T(""));
}

/* virtual */void ProcessMonitor::Deinitialize(PluginHost::IShell *service)
{
    _localNotification->Close();

    service->Unregister(_localNotification);
    service->Unregister(&_remoteNotification);
}

/* virtual */string ProcessMonitor::Information() const
{
    return (nullptr);
}

/* virtual */void ProcessMonitor::Inbound(Web::Request &request)
{
}

/* virtual */Core::ProxyType<Web::Response> ProcessMonitor::Process(
        const Web::Request &request)
{
    Core::ProxyType<Web::Response> result(
            PluginHost::Factories::Instance().Response());

    result->ErrorCode = Web::STATUS_OK;
    result->Message = "OK";

    return (result);
}
}
}
