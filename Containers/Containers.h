#pragma once

#include "Module.h"
#include "interfaces/json/JsonData_Containers.h"

namespace WPEFramework {
namespace Plugin {

    class Containers : public PluginHost::IPlugin, PluginHost::JSONRPC {
    public:
        Containers(const Containers&) = delete;
        Containers& operator=(const Containers&) = delete;

        Containers()
        {
            RegisterAll();
        }

        virtual ~Containers()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(Containers)
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
        //   JSONRPC methods
        // -------------------------------------------------------------------------------------------------------
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_stop(const JsonData::Containers::StopParamsData& params);
        uint32_t get_containers(Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_networks(const string& index, Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_ip(const string& index, Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_memory(const string& index, Core::JSON::String& response) const;
        uint32_t get_status(const string& index, Core::JSON::EnumType<JsonData::Containers::StatusType>& response) const;
        uint32_t get_cpu(const string& index, Core::JSON::String& response) const;
        uint32_t get_log(const string& index, Core::JSON::String& response) const;
        uint32_t get_config(const string& index, Core::JSON::String& response) const;
    };

} // namespace Plugin
} // namespace WPEFramework