#include "DeviceIdentification.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceIdentification, 1, 0);

    /* virtual */ const string DeviceIdentification::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        ASSERT(_idProvider == nullptr);
        
        _idProvider = Core::Service<IdentityProvider>::Create<IdentityProvider>();
        
        service->SubSystems()->Set(PluginHost::ISubSystem::IDENTIFIER, _idProvider);

        return (string());
    }

    /* virtual */ void DeviceIdentification::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        service->SubSystems()->Set(PluginHost::ISubSystem::NOT_IDENTIFIER, nullptr);

        if (_idProvider != nullptr) {
            delete _idProvider;
            _idProvider = nullptr;
        }
    }

    /* virtual */ string DeviceIdentification::Information() const
    {
        // No additional info to report.
        return (string());
    }

} // namespace Plugin
} // namespace WPEFramework
