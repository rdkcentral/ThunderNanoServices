#pragma once

#include "Module.h"
#include <interfaces/json/JsonData_InputSwitch.h>

namespace WPEFramework {
namespace Plugin {

    class InputSwitch : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
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
            Core::JSON::ArrayType<JsonData::InputSwitch::AddressesData> Addresses;
            JsonData::InputSwitch::SysteminfoData SystemInfo;
            JsonData::InputSwitch::SocketinfoData Sockets;
        };

    private:
        InputSwitch(const InputSwitch&) = delete;
        InputSwitch& operator=(const InputSwitch&) = delete;

        uint32_t addresses(const Core::JSON::String& parameters, Core::JSON::ArrayType<JsonData::InputSwitch::AddressesData>& response)
        {
            AddressInfo(response);
            return (Core::ERROR_NONE);
        }
        uint32_t system(const Core::JSON::String& parameters, JsonData::InputSwitch::SysteminfoData& response)
        {
            SysInfo(response);
            return (Core::ERROR_NONE);
        }
        uint32_t sockets(const Core::JSON::String& parameters, JsonData::InputSwitch::SocketinfoData& response)
        {
            SocketPortInfo(response);
            return (Core::ERROR_NONE);
        }

    public:
        InputSwitch()
            : _skipURL(0)
            , _service(nullptr)
            , _subSystem(nullptr)
            , _systemId()
            , _deviceId()
        {
            RegisterAll();
        }

        virtual ~InputSwitch()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(InputSwitch)
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
        uint32_t get_systeminfo(JsonData::InputSwitch::SysteminfoData& response) const;
        uint32_t get_addresses(Core::JSON::ArrayType<JsonData::InputSwitch::AddressesData>& response) const;
        uint32_t get_socketinfo(JsonData::InputSwitch::SocketinfoData& response) const;

        void SysInfo(JsonData::InputSwitch::SysteminfoData& systemInfo) const;
        void AddressInfo(Core::JSON::ArrayType<JsonData::InputSwitch::AddressesData>& addressInfo) const;
        void SocketPortInfo(JsonData::InputSwitch::SocketinfoData& socketPortInfo) const;
        string GetDeviceId() const;

    private:
        uint8_t _skipURL;
        PluginHost::IShell* _service;
        PluginHost::ISubSystem* _subSystem;
        string _systemId;
        mutable string _deviceId;
    };

} // namespace Plugin
} // namespace WPEFramework
