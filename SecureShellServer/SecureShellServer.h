/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#include "Module.h"
#include <interfaces/ISecureShellServer.h>
#include <interfaces/json/JsonData_SecureShellServer.h>




namespace WPEFramework {
namespace Plugin {

    class SecureShellServer : public PluginHost::IPlugin, public PluginHost::IWeb , public PluginHost::JSONRPC {
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
            Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoData> SessionInfo;
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
        : _skipURL(0)
        , _connectionId(0)
        , _InputParameters()
        ,_secureShellServer(nullptr)
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
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::ISecureShellServer,_secureShellServer)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IWeb methods
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        
    private:
        SecureShellServer(const SecureShellServer&) = delete;
        SecureShellServer& operator=(const SecureShellServer&) = delete;

        
        void RegisterAll();
        void UnregisterAll();
        
        uint32_t endpoint_activesessionscount(Core::JSON::DecUInt32& response);
        uint32_t endpoint_activesessionsinfo(Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoData>& response);
        uint32_t endpoint_closeclientsession(const JsonData::SecureShellServer::CloseclientcessionParamsData& params);

        uint8_t _skipURL;
        uint32_t _connectionId;
        std::string _InputParameters;
        Exchange::ISecureShellServer* _secureShellServer;


        uint32_t  GetActiveSessionsInfo(std::list<JsonData::SecureShellServer::SessioninfoData>& activesSessions);
    };

} // namespace Plugin
} // namespace WPEFramework
