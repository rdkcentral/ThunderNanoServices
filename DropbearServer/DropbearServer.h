#pragma once

#include "Module.h"
#include <interfaces/IDropbearServer.h>
#include <interfaces/json/JsonData_DropbearServer.h>

#include "DropbearServerImplementation.h"

namespace WPEFramework {
namespace Plugin {

    class DropbearServer : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::IDropbearServer, public PluginHost::JSONRPC {
    public:
        DropbearServer()
        {
            RegisterAll();
        }

        ~DropbearServer() override
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(DropbearServer)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(Exchange::IDropbearServer)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IWeb methods
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        // DropbearServer methods
        uint32_t StartService(const std::string& argv) override;
        uint32_t StopService() override;

    private:
        DropbearServer(const DropbearServer&) = delete;
        DropbearServer& operator=(const DropbearServer&) = delete;

        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_startservice(const JsonData::DropbearServer::StartserviceParamsData& params);
        uint32_t endpoint_stopservice();

        DropbearServerImplementation _implemetation;

        uint8_t _skipURL = 0;
    };

} // namespace Plugin
} // namespace WPEFramework
