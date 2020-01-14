#pragma once

#include "Module.h"
#include <interfaces/IDeviceIdentification.h>
#include <interfaces/json/JsonData_DeviceIdentification.h>

namespace WPEFramework {
namespace Plugin {

    class DeviceIdentification : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        DeviceIdentification(const DeviceIdentification&) = delete;
        DeviceIdentification& operator=(const DeviceIdentification&) = delete;

        DeviceIdentification()
            : _deviceId()
            , _device(nullptr)
            , _identifier(nullptr)
            , _connectionId(0)
        {
        }

        virtual ~DeviceIdentification()
        {
        }

        BEGIN_INTERFACE_MAP(DeviceIdentification)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t get_deviceidentification(JsonData::DeviceIdentification::DeviceidentificationData& response) const;

        string GetDeviceId() const;
        void Info(JsonData::DeviceIdentification::DeviceidentificationData&) const;

    private:
        string _deviceId;
        Exchange::IDeviceProperties* _device;
        const PluginHost::ISubSystem::IIdentifier* _identifier;

        uint32_t _connectionId;
    };

} // namespace Plugin
} // namespace WPEFramework
