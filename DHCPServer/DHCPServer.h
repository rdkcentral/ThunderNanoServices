/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "DHCPServerImplementation.h"
#include <interfaces/json/JsonData_DHCPServer.h>
#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class DHCPServer : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        class Data : public Core::JSON::Container {
        private:
            Data(Data const& other) = delete;
            Data& operator=(Data const& other) = delete;

        public:
            class Server : public Core::JSON::Container {
            private:
                Server& operator=(const Server&) = delete;

            public:
                class Lease : public Core::JSON::Container {
                private:
                    Lease& operator=(const Lease&) = delete;

                public:
                    Lease()
                        : Core::JSON::Container()
                        , Name()
                        , IPAddress()
                        , Expires()
                    {
                        Add(_T("name"), &Name);
                        Add(_T("ip"), &IPAddress);
                        Add(_T("expires"), &Expires);
                    }
                    Lease(const Lease& copy)
                        : Core::JSON::Container()
                        , Name(copy.Name)
                        , IPAddress(copy.IPAddress)
                        , Expires(copy.Expires)
                    {
                        Add(_T("name"), &Name);
                        Add(_T("ip"), &IPAddress);
                        Add(_T("expires"), &Expires);
                    }
                    virtual ~Lease()
                    {
                    }

                public:
                    void Set(const DHCPServerImplementation::Lease& lease)
                    {
                        Name = lease.Id().Text();
                        IPAddress = lease.Address().HostAddress();
                        Expires = lease.Expiration();
                    }

                    DHCPServerImplementation::Lease Get() const 
                    {
                        in_addr ip;
                        inet_aton(IPAddress.Value().c_str(), &ip);
                        ip.s_addr = ntohl(ip.s_addr); // Change back to host :)

                        // Convert identifier to bytes
                        uint8_t buffer[DHCPServerImplementation::Identifier::maxLength];
                        uint16_t identifierLength = Core::FromHexString(Name.Value(), buffer, DHCPServerImplementation::Identifier::maxLength);
                        DHCPServerImplementation::Identifier identifier(buffer, static_cast<uint8_t>(identifierLength));

                        return DHCPServerImplementation::Lease(identifier, ip.s_addr, Expires.Value());
                    }

                public:
                    Core::JSON::String Name;
                    Core::JSON::String IPAddress;
                    Core::JSON::DecUInt64 Expires;
                };

            public:
                Server()
                    : Core::JSON::Container()
                    , Interface()
                    , Begin()
                    , End()
                    , Router()
                    , Active(false)
                    , Leases()
                {
                    Add(_T("interface"), &Interface);
                    Add(_T("begin"), &Begin);
                    Add(_T("end"), &End);
                    Add(_T("router"), &Router);
                    Add(_T("active"), &Active);
                    Add(_T("leases"), &Leases);
                }
                Server(const Server& copy)
                    : Core::JSON::Container()
                    , Interface(copy.Interface)
                    , Begin(copy.Begin)
                    , End(copy.End)
                    , Router(copy.Router)
                    , Active(copy.Active)
                    , Leases()
                {
                    Add(_T("interface"), &Interface);
                    Add(_T("begin"), &Begin);
                    Add(_T("end"), &End);
                    Add(_T("router"), &Router);
                    Add(_T("active"), &Active);
                    Add(_T("leases"), &Leases);
                }
                virtual ~Server()
                {
                }

            public:
                void Set(const DHCPServerImplementation& server)
                {
                    Interface = server.Interface();
                    Begin = server.BeginPool().HostAddress();
                    End = server.EndPool().HostAddress();
                    Router = server.Router().HostAddress();
                    Active = server.IsActive();

                    DHCPServerImplementation::Iterator index(server.Leases());

                    while (index.Next() == true) {
                        Leases.Add().Set(index.Current());
                    }
                }

            public:
                Core::JSON::String Interface;
                Core::JSON::String Begin;
                Core::JSON::String End;
                Core::JSON::String Router;
                Core::JSON::Boolean Active;
                Core::JSON::ArrayType<Lease> Leases;
            };

        public:
            Data()
            {
                Add(_T("servers"), &Servers);
            }
            virtual ~Data()
            {
            }

        public:
            Core::JSON::ArrayType<Server> Servers;
        };

    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            class Server : public Core::JSON::Container {
            private:
                Server& operator=(const Server&) = delete;

            public:
                Server()
                    : Core::JSON::Container()
                    , Interface()
                    , PoolStart(0)
                    , PoolSize(0)
                    , Router(0)
                    , Active(false)
                {
                    Add(_T("interface"), &Interface);
                    Add(_T("poolstart"), &PoolStart);
                    Add(_T("poolsize"), &PoolSize);
                    Add(_T("router"), &Router);
                    Add(_T("active"), &Active);
                }
                Server(const Server& copy)
                    : Core::JSON::Container()
                    , Interface(copy.Interface)
                    , PoolStart(copy.PoolStart)
                    , PoolSize(copy.PoolSize)
                    , Router(copy.Router)
                    , Active(copy.Active)
                {
                    Add(_T("interface"), &Interface);
                    Add(_T("poolstart"), &PoolStart);
                    Add(_T("poolsize"), &PoolSize);
                    Add(_T("router"), &Router);
                    Add(_T("active"), &Active);
                }
                virtual ~Server()
                {
                }

            public:
                Core::JSON::String Interface;
                Core::JSON::DecUInt32 PoolStart;
                Core::JSON::DecUInt32 PoolSize;
                Core::JSON::DecUInt32 Router;
                Core::JSON::Boolean Active;
            };

        public:
            Config()
                : Core::JSON::Container()
                , Name()
                , DNS()
                , Servers()
            {
                Add(_T("name"), &Name);
                Add(_T("dns"), &DNS);
                Add(_T("servers"), &Servers);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Name;
            Core::JSON::String DNS;
            Core::JSON::ArrayType<Server> Servers;
        };

    private:
        DHCPServer(const DHCPServer&) = delete;
        DHCPServer& operator=(const DHCPServer&) = delete;

    public:
        DHCPServer();
        virtual ~DHCPServer();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(DHCPServer)
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

        // JsonRpc
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_activate(const JsonData::DHCPServer::ActivateParamsInfo& params);
        uint32_t endpoint_deactivate(const JsonData::DHCPServer::ActivateParamsInfo& params);
        uint32_t get_status(const string& index, Core::JSON::ArrayType<JsonData::DHCPServer::ServerData>& response) const;

        // Lease permanent storage
        // -------------------------------------------------------------------------------------------------------
        void SaveLeases(const string& interface, const DHCPServerImplementation& dhcpServer) const;
        void LoadLeases(const string& interface, DHCPServerImplementation& dhcpServer);

        // Callbacks
        void OnNewIPRequest(const string& interface, const DHCPServerImplementation::Lease* lease);
    private:
        uint16_t _skipURL;
        std::map<const string, DHCPServerImplementation> _servers;
        std::string _persistentPath;
    };

} // namespace Plugin
}

