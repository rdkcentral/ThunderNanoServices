#pragma once

#include "Module.h"
#include <interfaces/IDropbearServer.h>
#include <interfaces/json/JsonData_DropbearServer.h>

#include "DropbearServerImplementation.h"

namespace WPEFramework {
namespace Plugin {

    class DropbearServer : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::IDropbearServer, public PluginHost::JSONRPC {
    public:
        class Data : public Core::JSON::Container {
        public:
            Data()
                : Core::JSON::Container()
                , SessionInfo()
                , HostKeys()
		, PortFlag()
		, PortNumber()
		, TotalCount()
		, ActiveCount()
            {
                Add(_T("sessioninfo"), &SessionInfo);
                Add(_T("hostkeys"), &HostKeys);
                Add(_T("portflag"), &PortFlag);
                Add(_T("portnumber"), &PortNumber);
                Add(_T("totalcount"), &TotalCount);
                Add(_T("activecount"), &ActiveCount);
            }

            virtual ~Data()
            {
            }

        public:
            Core::JSON::ArrayType<JsonData::DropbearServer::SessioninfoResultData> SessionInfo;
	    Core::JSON::String HostKeys;
	    Core::JSON::String PortFlag;
	    Core::JSON::String PortNumber;
	    Core::JSON::DecUInt32 TotalCount;
	    Core::JSON::DecUInt32 ActiveCount;
        };

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
        uint32_t StartService(const std::string& host_keys, const std::string& port_flag, const std::string& port);
        uint32_t StopService() override;
        uint32_t GetTotalSessions();
        uint32_t GetSessionsCount();
        uint32_t GetSessionsInfo(Core::JSON::ArrayType<JsonData::DropbearServer::SessioninfoResultData>& response);
        uint32_t CloseClientSession(uint32_t client_pid);

    private:
        DropbearServer(const DropbearServer&) = delete;
        DropbearServer& operator=(const DropbearServer&) = delete;

        void RegisterAll();
        void UnregisterAll();
        
    	uint32_t endpoint_startservice(const JsonData::DropbearServer::StartserviceParamsData& params);
        uint32_t endpoint_stopservice();
	uint32_t endpoint_gettotalsessions(Core::JSON::DecUInt32& response);
	uint32_t endpoint_getactivesessionscount(Core::JSON::DecUInt32& response);
	uint32_t endpoint_getactivesessionsinfo(Core::JSON::ArrayType<JsonData::DropbearServer::SessioninfoResultData>& response);
	uint32_t endpoint_closeclientsession(Core::JSON::DecUInt32& pid);

        DropbearServerImplementation _implemetation;

        uint8_t _skipURL = 0;
    };

} // namespace Plugin
} // namespace WPEFramework
