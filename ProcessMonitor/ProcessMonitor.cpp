#include "ProcessMonitor.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(ProcessMonitor, 1, 0);

const string ProcessMonitor::Initialize(PluginHost::IShell* service)
{
    Config config;
    config.FromString(service->ConfigLine());

    _notification.Open(service, config.ExitTimeout.Value());

    return (_T(""));
}

void ProcessMonitor::Deinitialize(PluginHost::IShell* service)
{
    _notification.Close();
}

string ProcessMonitor::Information() const
{
    string emptyString;
    return emptyString;
}
}
}
