#include "DeviceIdentification.h"
#include "IdentityProvider.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceIdentification, 1, 0);

    /* virtual */ const string DeviceIdentification::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_device == nullptr);

        string message;

        _device = service->Root<Exchange::IDeviceProperties>(_connectionId, 2000, _T("DeviceImplementation"));
        if (_device != nullptr) {

            _identifier = _device->QueryInterface<PluginHost::ISubSystem::IIdentifier>();
            if (_identifier == nullptr) {

                _device->Release();
                _device = nullptr;
            } else {
                _deviceId = GetDeviceId();
                if (_deviceId.empty() != true) {
                    service->SubSystems()->Set(PluginHost::ISubSystem::IDENTIFIER, _device);
                }
            }
        }

        if (_device == nullptr) {
            message = _T("DeviceIdentification plugin could not be instantiated.");
        }

        return message;
    }

    /* virtual */ void DeviceIdentification::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_device != nullptr);

        ASSERT(_identifier != nullptr);
        if (_identifier != nullptr) {
            if (_deviceId.empty() != true) {
                service->SubSystems()->Set(PluginHost::ISubSystem::IDENTIFIER, nullptr);
                _deviceId.clear();
            }
            _identifier->Release();
            _identifier = nullptr;
        }

        ASSERT(_device != nullptr);
        if (_device != nullptr) {
            _device->Release();
            _device = nullptr;
        }

        _connectionId = 0;
    }

    /* virtual */ string DeviceIdentification::Information() const
    {
        // No additional info to report.
        return (string());
    }

    string DeviceIdentification::GetDeviceId() const
    {
        string result;
        ASSERT(_identifier != nullptr);

        if (_identifier != nullptr) {
            uint8_t myBuffer[64];

            myBuffer[0] = _identifier->Identifier(sizeof(myBuffer) - 1, &(myBuffer[1]));

            if (myBuffer[0] != 0) {
                result = Core::SystemInfo::Instance().Id(myBuffer, ~0);
            }
        }

        return result;
    }

    void DeviceIdentification::Info(JsonData::DeviceIdentification::DeviceidentificationData& deviceInfo) const
    {
        deviceInfo.Firmwareversion = _device->FirmwareVersion();
        deviceInfo.Chipset = _device->Chipset();

        if (_deviceId.empty() != true) {
            deviceInfo.Deviceid = _deviceId;
        }
    }

} // namespace Plugin
} // namespace WPEFramework
