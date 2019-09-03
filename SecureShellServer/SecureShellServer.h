#pragma once

#include "Module.h"
#include <interfaces/ISecureShellServer.h>
#include <interfaces/json/JsonData_SecureShellServer.h>

#include "SecureShellServerImplementation.h"

namespace WPEFramework {
namespace Plugin {

    class SecureShellServer : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::ISecureShellServer, public PluginHost::JSONRPC {
    public:
        class Data : public Core::JSON::Container {
        public:
            Data()
                : Core::JSON::Container()
                , SessionInfo()
		, ActiveCount()
            {
                Add(_T("sessioninfo"), &SessionInfo);
                Add(_T("activecount"), &ActiveCount);
            }

            virtual ~Data()
            {
            }

        public:
            Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoResultData> SessionInfo;
	    Core::JSON::DecUInt32 ActiveCount;
        };

	class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , InputParameters()
            {
                Add(_T("inputparameters"), &InputParameters);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String InputParameters;
        };

    public:
        SecureShellServer()
        {
            RegisterAll();
        }

        ~SecureShellServer() override
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(SecureShellServer)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(Exchange::ISecureShellServer)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IWeb methods
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        // SecureShellServer methods
        uint32_t GetSessionsCount();
        uint32_t GetSessionsInfo(Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoResultData>& response);
        uint32_t CloseClientSession(const std::string& clientpid);

    private:
        SecureShellServer(const SecureShellServer&) = delete;
        SecureShellServer& operator=(const SecureShellServer&) = delete;

        void RegisterAll();
        void UnregisterAll();
        
	uint32_t endpoint_getactivesessionscount(Core::JSON::DecUInt32& response);
	uint32_t endpoint_getactivesessionsinfo(Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoResultData>& response);
	uint32_t endpoint_closeclientsession(const JsonData::SecureShellServer::CloseclientsessionParamsData& params);

        SecureShellServerImplementation _implementation;

        uint8_t _skipURL = 0;
        std::string _InputParameters;
        PluginHost::IShell* _service;
    };

} // namespace Plugin
} // namespace WPEFramework
