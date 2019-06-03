#ifndef DEVICEINFO_DEVICEINFO_H
#define DEVICEINFO_DEVICEINFO_H

#include "Module.h"
#include <interfaces/json/JsonData_DeviceInfo.h>

namespace WPEFramework {
namespace Plugin {

    class DeviceInfo : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        class Data : public Core::JSON::Container {
        public:
            Data()
                : Core::JSON::Container()
                , Addresses()
                , SystemInfo()
            {
                Add(_T("addresses"), &Addresses);
                Add(_T("systeminfo"), &SystemInfo);
                Add(_T("sockets"), &Sockets);
            }

            virtual ~Data()
            {
            }

        public:
            Core::JSON::ArrayType<JsonData::DeviceInfo::AddressesParamsData> Addresses;
            JsonData::DeviceInfo::SysteminfoParamsData SystemInfo;
            JsonData::DeviceInfo::SocketinfoParamsData Sockets;
        };

    private:
        DeviceInfo(const DeviceInfo&) = delete;
        DeviceInfo& operator=(const DeviceInfo&) = delete;

        uint32_t addresses(const Core::JSON::String& parameters, Core::JSON::ArrayType<JsonData::DeviceInfo::AddressesParamsData>& response)
        {
            AddressInfo(response);
            return (Core::ERROR_NONE);
        }
        uint32_t system(const Core::JSON::String& parameters, JsonData::DeviceInfo::SysteminfoParamsData& response)
        {
            SysInfo(response);
            return (Core::ERROR_NONE);
        }
        uint32_t sockets(const Core::JSON::String& parameters, JsonData::DeviceInfo::SocketinfoParamsData& response)
        {
            SocketPortInfo(response);
            return (Core::ERROR_NONE);
        }

    public:
        DeviceInfo()
            : _skipURL(0)
            , _service(nullptr)
            , _subSystem(nullptr)
            , _idProvider(nullptr)
            , _systemId()
            , _deviceId()
        {
            RegisterAll();
        }

        virtual ~DeviceInfo()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(DeviceInfo)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        // JsonRpc
        void RegisterAll();
        void UnregisterAll();
        uint32_t get_systeminfo(JsonData::DeviceInfo::SysteminfoParamsData& response) const;
        uint32_t get_addresses(Core::JSON::ArrayType<JsonData::DeviceInfo::AddressesParamsData>& response) const;
        uint32_t get_socketinfo(JsonData::DeviceInfo::SocketinfoParamsData& response) const;

        void SysInfo(JsonData::DeviceInfo::SysteminfoParamsData& systemInfo) const;
        void AddressInfo(Core::JSON::ArrayType<JsonData::DeviceInfo::AddressesParamsData>& addressInfo) const;
        void SocketPortInfo(JsonData::DeviceInfo::SocketinfoParamsData& socketPortInfo) const;
        string GetDeviceId() const;

        class IdentityProvider : public PluginHost::ISubSystem::IIdentifier {
        public:
            IdentityProvider();
            virtual ~IdentityProvider(){
                if (_identifier != nullptr) {
                    delete (_identifier);
                }
            };

            IdentityProvider(const IdentityProvider&) = delete;
            IdentityProvider& operator=(const IdentityProvider&) = delete;

            BEGIN_INTERFACE_MAP(IdentityProvider)
                INTERFACE_ENTRY(PluginHost::ISubSystem::IIdentifier)
            END_INTERFACE_MAP

            virtual uint8_t Identifier(const uint8_t length, uint8_t buffer[]) const override{
                uint8_t result = 0;

                if (_identifier != nullptr) {
                    result = _identifier[0];
                    ::memcpy(buffer, &(_identifier[1]), (result > length ? length : result));
                }

                return (result);
            }
        private:
            uint8_t* _identifier;
        };

    private:
        uint8_t _skipURL;
        PluginHost::IShell* _service;
        PluginHost::ISubSystem* _subSystem;
        IdentityProvider* _idProvider;
        string _systemId;
        mutable string _deviceId;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // DEVICEINFO_DEVICEINFO_H
