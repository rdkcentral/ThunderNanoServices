#pragma once

#include "Module.h"
#include "interfaces/IRemoteHostExample.h"
#include <interfaces/json/JsonData_RemoteHostExample.h>

namespace WPEFramework {
namespace Plugin {

    class RemoteHostExample : public PluginHost::IPlugin, public PluginHost::JSONRPC, public Exchange::IRemoteHostExample::ITimeListener {
    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , Name()
            {
                Add(_T("name"), &Name);
            }

            ~Config()
            {
            }

        public:
            Core::JSON::String Name;
        };

        RemoteHostExample(const RemoteHostExample&) = delete;
        RemoteHostExample& operator=(const RemoteHostExample&) = delete;

        RemoteHostExample()
            : _shell(nullptr)
        {
            RegisterAll();
        }

        virtual ~RemoteHostExample()
        {
            if (_shell) {
                _shell->Release();
                _shell = nullptr;
            }

            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(RemoteHostExample)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(Exchange::IRemoteHostExample::ITimeListener)
            INTERFACE_ENTRY(PluginHost::JSONRPC)
            INTERFACE_AGGREGATE(Exchange::IRemoteHostExample, _implementation)
            INTERFACE_AGGREGATE(RPC::IRemoteLinker, _implementation)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IRemoteHostExample::ITimeListener methods
        // -------------------------------------------------------------------------------------------------------
        uint32_t TimeUpdate(string time) override;
    private:
        //   JSONRPC methods
        // -------------------------------------------------------------------------------------------------------
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_greet(const JsonData::RemoteHostExample::GreetParamsData& params, JsonData::RemoteHostExample::GreetResultData& response);

    private:
        Exchange::IRemoteHostExample* _implementation;
        PluginHost::IShell* _shell;
    };

} // namespace Plugin
} // namespace WPEFramework