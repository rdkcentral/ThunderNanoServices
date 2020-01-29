#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class InputSwitch : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        class Data : public Core::JSON::Container {
        public:
            Data()
                : Core::JSON::Container()
                , Callsign()
                , Enabled()
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("enabled"), &Enabled);
            }
            Data(const Data& copy)
                : Core::JSON::Container()
                , Callsign(copy.Callsign)
                , Enabled(copy.Enabled)
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("enabled"), &Enabled);
            }
            virtual ~Data()
            {
            }

            Data& operator= (const Data& rhs)
            {
                Callsign = rhs.Callsign;
                Enabled = rhs.Enabled;

                return (*this);
            }

        public:
            Core::JSON::String Callsign;
            Core::JSON::Boolean Enabled;
        };

    public:
        InputSwitch(const InputSwitch&) = delete;
        InputSwitch& operator=(const InputSwitch&) = delete;

        InputSwitch()
            : _skipURL(0)
            , _handler()
        {
            // RegisterAll();
        }

        virtual ~InputSwitch()
        {
            // UnregisterAll();
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
        bool FindChannel(const string& name);

        // JsonRpc
        void RegisterAll();
        void UnregisterAll();

    private:
        uint8_t _skipURL;
        PluginHost::IPCUserInput::Iterator _handler;
    };

} // namespace Plugin
} // namespace WPEFramework
