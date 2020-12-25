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

#include "DHCPServer.h"
#include <interfaces/json/JsonData_DHCPServer.h>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DHCPServer, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<DHCPServer::Data>> jsonDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<DHCPServer::Data::Server>> jsonServerDataFactory(1);

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
    DHCPServer::DHCPServer()
        : _skipURL(0)
        , _servers()
    {
        RegisterAll();
    }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

    /* virtual */ DHCPServer::~DHCPServer()
    {
        UnregisterAll();
    }

    /* virtual */ const string DHCPServer::Initialize(PluginHost::IShell* service)
    {
        string result;
        Config config;
        config.FromString(service->ConfigLine());

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        Core::NodeId dns(config.DNS.Value().c_str());
        Core::JSON::ArrayType<Config::Server>::Iterator index(config.Servers.Elements());

        _persistentPath = service->PersistentPath();

        Core::Directory directory(_persistentPath.c_str());
        if (directory.CreatePath() == false) {
            TRACE(Trace::Error, (_T("Could not create DHCPServer persistent path")));
            _persistentPath = "";
        }

        while (index.Next() == true) {
            if (index.Current().Interface.IsSet() == true) {
                auto server = _servers.emplace(std::piecewise_construct,
                    std::make_tuple(index.Current().Interface.Value()),
                    std::make_tuple(
                        config.Name.Value(),
                        index.Current().Interface.Value(),
                        index.Current().PoolStart.Value(),
                        index.Current().PoolSize.Value(),
                        index.Current().Router.Value(),
                        dns,
                        std::bind(&DHCPServer::OnNewIPRequest, this, std::placeholders::_1, std::placeholders::_2)));

                if (server.second == true) {
                    LoadLeases(server.first->first, server.first->second);
                }
            }
        }

        // On success return empty, to indicate there is no error text.
        return (result);
    }

    /* virtual */ void DHCPServer::Deinitialize(PluginHost::IShell* service)
    {
        std::map<const string, DHCPServerImplementation>::iterator index(_servers.begin());

        while (index != _servers.end()) {
            index->second.Close();
            index++;
        }

        _servers.clear();
    }

    /* virtual */ string DHCPServer::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void DHCPServer::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response>
    DHCPServer::Process(const Web::Request& request)
    {

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::TextSegmentIterator index(
            Core::TextFragment(request.Path, _skipURL, static_cast<uint16_t>(request.Path.length() - _skipURL)),
            false,
            '/');

        // By definition skip the first slash.
        index.Next();

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported request for the [DHCPServer] service.");

        if (request.Verb == Web::Request::HTTP_GET) {

            if (index.Next() == true) {
                std::map<const string, DHCPServerImplementation>::iterator server(_servers.find(index.Current().Text()));

                if (server != _servers.end()) {
                    Core::ProxyType<Data::Server> info(jsonDataFactory.Element());

                    info->Set(server->second);
                    result->Body(info);
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = "OK";
                }
            } else {
                Core::ProxyType<Data> info(jsonDataFactory.Element());
                std::map<const string, DHCPServerImplementation>::iterator servers(_servers.begin());

                while (servers != _servers.end()) {

                    info->Servers.Add().Set(servers->second);
                    servers++;
                }

                result->Body(info);
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";
            }
        } else if (request.Verb == Web::Request::HTTP_PUT) {

            if (index.Next() == true) {
                std::map<const string, DHCPServerImplementation>::iterator server(_servers.find(index.Current().Text()));

                if ((server != _servers.end()) && (index.Next() == true)) {

                    if (index.Current() == _T("Activate")) {

                        if (server->second.Open() == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = "OK";
                        } else {
                            result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                            result->Message = "Could not activate the server";
                        }
                    } else if (index.Current() == _T("Deactivate")) {

                        if (server->second.Close() == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = "OK";
                        } else {
                            result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                            result->Message = "Could not activate the server";
                        }
                    }
                }
            }
        }

        return result;
    }

    void DHCPServer::SaveLeases(const string& interface, const DHCPServerImplementation& dhcpServer) const
    {

        if (_persistentPath.empty() == false) {
            Core::JSON::ArrayType<Data::Server::Lease> leasesList;

            // Saving to file might take a while, and lifetime of DHCPServerImplementation::Iterator
            // object must be deterministic and short (<10ms), so we scope it 
            {
                DHCPServerImplementation::Iterator leases = dhcpServer.Leases();
                while(leases.Next() && (leases.IsValid() == true)) {
                    leasesList.Add().Set(leases.Current());
                }        
            }

            Core::File leasesFile(_persistentPath + interface + ".json");

            if (leasesFile.Create() == true) {
                leasesList.IElement::ToFile(leasesFile);
                leasesFile.Close();
            } else {
                TRACE(Trace::Error, (_T("Could not save leases in permanent storage area.\n")));
            }
        }
    }

    void DHCPServer::LoadLeases(const string& interface, DHCPServerImplementation& dhcpServer) 
    {

        if (_persistentPath.empty() == false) {
            Core::File leasesFile(_persistentPath + interface + ".json");

            if (leasesFile.Open(true) == true) {
                Core::JSON::ArrayType<Data::Server::Lease> leases;

                Core::OptionalType<Core::JSON::Error> error;
                leases.IElement::FromFile(leasesFile, error);
                if (error.IsSet() == true) {
                    SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                }
                leasesFile.Close();

                auto iterator = leases.Elements();
                while ((iterator.Next() == true) && (iterator.IsValid() == true)) {
                    dhcpServer.AddLease(iterator.Current().Get());
                }
            } 
        }
    }

    void DHCPServer::OnNewIPRequest(const string& interface, const DHCPServerImplementation::Lease* lease) 
    {
        TRACE(Trace::Information, ("DHCP server granted address %s on interface %s", lease->Address().HostAddress().c_str(), interface.c_str()));

        auto dhcpServer = _servers.find(interface);
        if (dhcpServer != _servers.end()) {
            SaveLeases(interface, dhcpServer->second);
        }
    }

} // namespace Plugin
} // namespace WPEFramework
