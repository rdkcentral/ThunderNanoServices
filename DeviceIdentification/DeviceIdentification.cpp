#include "DeviceIdentification.h"
#include "IdentityProvider.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceIdentification, 1, 0);

    /* virtual */ const string DeviceIdentification::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        service->SubSystems()->Set(PluginHost::ISubSystem::IDENTIFIER, this);

        return (string());
    }

    /* virtual */ void DeviceIdentification::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
    }

    /* virtual */ string DeviceIdentification::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual*/ uint8_t DeviceIdentification::Identifier(const uint8_t length, uint8_t buffer[]) const
    {
        uint8_t result = 0;
        unsigned char length_error;
        const unsigned char* identity = GetIdentity(&length_error);

        if (identity != nullptr) {
            result = length_error;
            ::memcpy(buffer, identity, (result > length ? length : result));
        }
        else {
            SYSLOG(Logging::Notification, (_T("System identity can not be determined. Error: [%d]!"), length_error));
        }

        return (result);
    }

} // namespace Plugin
} // namespace WPEFramework
