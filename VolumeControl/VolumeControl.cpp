#include "VolumeControl.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(VolumeControl, 1, 0);

    const string VolumeControl::Initialize(PluginHost::IShell* service)
    {
        ASSERT (_service == nullptr);
        ASSERT (service != nullptr);

        _service = service;
        _service->Register(&_connectionNotification);

        string result;
        _implementation = _service->Root<Exchange::IVolumeControl>(_connectionId, 2000, _T("VolumeControlImplementation"));
        if (_implementation == nullptr) {
            result = _T("Couldn't create volume control instance");
        } else {
          _implementation->Register(&_volumeNotification);
        }

        return (result);
    }

    void VolumeControl::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        _service->Unregister(&_connectionNotification);
        _implementation->Unregister(&_volumeNotification);

        if (_implementation->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

            ASSERT(_connectionId != 0);

            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            // The process can disappear in the meantime...
            if (connection != nullptr) {
                // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                connection->Terminate();
                connection->Release();
            }
        }

        _service = nullptr;
        _implementation = nullptr;
    }

    string VolumeControl::Information() const
    {
        return string();
    }

    void VolumeControl::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);
            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin
} // namespace WPEFramework
