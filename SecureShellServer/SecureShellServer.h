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

#include "Module.h"
#include <interfaces/ISecureShellServer.h>
#include <interfaces/json/JsonData_SecureShellServer.h>

#include <libdropbear.h>


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

	class ClientImpl : public ISecureShellServer::IClient {
        private:
            ClientImpl() = delete;
            ClientImpl(const ClientImpl&) = delete;
            ClientImpl& operator=(const ClientImpl&) = delete;

	public:
            class IteratorImpl : public Exchange::ISecureShellServer::IClient::IIterator {
            private:
                IteratorImpl() = delete;
                IteratorImpl(const IteratorImpl&) = delete;
                IteratorImpl& operator=(const IteratorImpl&) = delete;


            public:
                IteratorImpl(const std::list<ClientImpl*>& container)
                {
                    std::list<ClientImpl*>::const_iterator index = container.begin();
                    while (index != container.end()) {
                        ISecureShellServer::IClient* element = (*index);
                        element->AddRef();
                        _list.push_back(element);
                        index++;
                    }
                }
                virtual ~IteratorImpl()
                {
                    while (_list.size() != 0) {
                        _list.front()->Release();
                        _list.pop_front();
                    }
                }

            public:
		virtual uint32_t Count()
                {
                   return (static_cast<uint32_t>(_list.size()));
                }
                virtual void Reset() override
                {
                    _index = 0;
                }
                virtual bool IsValid() const override
                {
                    return ((_index != 0) && (_index <= _list.size()));
                }
                virtual bool Next() override
                {
                    if (_index == 0) {
                        _index = 1;
                        _iterator = _list.begin();
                    } else if (_index <= _list.size()) {
                        _index++;
                        _iterator++;
                    }
                    return (IsValid());
                }
                virtual ISecureShellServer::IClient* Current()
                {
                    ASSERT(IsValid() == true);
                    ISecureShellServer::IClient* result = nullptr;
                    result = (*_iterator);
                    ASSERT(result != nullptr);
                    result->AddRef();
                    return (result);
                }

                BEGIN_INTERFACE_MAP(IteratorImpl)
                INTERFACE_ENTRY(ISecureShellServer::IClient::IIterator)
                END_INTERFACE_MAP

            private:
                uint32_t _index;
                std::list<ISecureShellServer::IClient*> _list;
                std::list<ISecureShellServer::IClient*>::iterator _iterator;
            };
        public:
            ClientImpl(const string& ipaddress, const string& timestamp, const string& remoteid)
                : _ipaddress(ipaddress)
                , _timestamp(timestamp)
                , _remoteid(remoteid)
            {
            }
            ~ClientImpl()
            {
            }

        public:
            virtual string IpAddress() const
            {
                return (_ipaddress);
            }
            virtual string TimeStamp() const
            {
                return (_timestamp);
            }
            virtual string RemoteId() const
            {
                return (_remoteid);
            }
            virtual void Close()
            {
                TRACE(Trace::Information, (_T("closing client session with _remoteid: %s"), _remoteid.c_str()));
                close_client_session(std::stoi(_remoteid));
            }

            BEGIN_INTERFACE_MAP(ClientImpl)
            INTERFACE_ENTRY(ISecureShellServer::IClient)
            END_INTERFACE_MAP

        private:
            std::string _ipaddress;
            std::string _timestamp;
            std::string _remoteid;
        };

    public:
        SecureShellServer()
        : _skipURL(0)
        , _InputParameters()
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

        //  ISecureShellServer methods
        virtual ISecureShellServer::IClient::IIterator* Clients() override;

        // SecureShellServer methods
        uint32_t GetSessionsCount(ISecureShellServer::IClient::IIterator* iter);
        uint32_t GetSessionsInfo(Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoResultData>& sessioninfo);
        uint32_t CloseClientSession(ISecureShellServer::IClient* client);

    private:
        SecureShellServer(const SecureShellServer&) = delete;
        SecureShellServer& operator=(const SecureShellServer&) = delete;

        ISecureShellServer::IClient::IIterator* SessionsInfo();

        void RegisterAll();
        void UnregisterAll();
        
        uint32_t endpoint_getactivesessionscount(Core::JSON::DecUInt32& response);
        uint32_t endpoint_getactivesessionsinfo(Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoResultData>& response);
        uint32_t endpoint_closeclientsession(const JsonData::SecureShellServer::SessioninfoResultData& params);

        uint8_t _skipURL;
        std::string _InputParameters;
    };

} // namespace Plugin
} // namespace WPEFramework
