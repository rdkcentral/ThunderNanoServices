#include "ResourceMonitor.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(ResourceMonitor, 1, 0);

    const string ResourceMonitor::Initialize(PluginHost::IShell* service)
    {
        string message;
        _connectionId = 0;

        ASSERT(_service == nullptr);
        ASSERT(_monitor == nullptr);

        _service = service;

        Config config;
        config.FromString(_service->ConfigLine());
        _skipURL = static_cast<uint32_t>(_service->WebPrefix().length());

        _monitor = _service->Root<Exchange::IResourceMonitor>(_connectionId, 2000, _T("ResourceMonitorImplementation"));

        if (_monitor == nullptr) {
            _service = nullptr;
            message = _T("ResourceMonitor could not be instantiated.");
        } else {
            _monitor->Configure(service);
        }

        return message;
    }

    void ResourceMonitor::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == _service);

        _monitor->Release();
        
        if (_connectionId != 0) {
            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            // The process can disappear in the meantime...
            if (connection != nullptr) {
                // Connection is still there.
                connection->Terminate();
                connection->Release();
            }
        }

        _service = nullptr;
    }

    string ResourceMonitor::Information() const
    {
        return "";
    }

    /* static */ Core::ProxyPoolType<Web::TextBody> ResourceMonitor::webBodyFactory(4);
}
}
