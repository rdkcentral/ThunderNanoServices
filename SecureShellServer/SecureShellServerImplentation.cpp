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

#include "Module.h"
#include <interfaces/ISecureShellServer.h>
#include <libdropbear.h>


namespace WPEFramework {
namespace Plugin {

class ClientSessionImpl : public Exchange::ISecureShellServer::IClientSession {
public:
        ClientSessionImpl() = delete;
        ClientSessionImpl(const ClientSessionImpl&) = delete;
        ClientSessionImpl& operator=(const ClientSessionImpl&) = delete;

        class IteratorImpl : public Exchange::ISecureShellServer::IClientSession::IIterator {
        public:
            IteratorImpl() = delete;
            IteratorImpl(const IteratorImpl&) = delete;
            IteratorImpl& operator=(const IteratorImpl&) = delete;

            IteratorImpl(const std::list<ClientSessionImpl*>& container)
            {
                std::list<ClientSessionImpl*>::const_iterator index = container.begin();
                while (index != container.end()) {
                    Exchange::ISecureShellServer::IClientSession* element = (*index);
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
            virtual uint32_t Count() const override
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
            virtual Exchange::ISecureShellServer::IClientSession* Current()
            {
                ASSERT(IsValid() == true);
                Exchange::ISecureShellServer::IClientSession* result = nullptr;
                result = (*_iterator);
                ASSERT(result != nullptr);
                result->AddRef();
                return (result);
            }

            BEGIN_INTERFACE_MAP(IteratorImpl)
            INTERFACE_ENTRY(Exchange::ISecureShellServer::IClientSession::IIterator)
            END_INTERFACE_MAP

        private:
            uint32_t _index;
            std::list<Exchange::ISecureShellServer::IClientSession*> _list;
            std::list<Exchange::ISecureShellServer::IClientSession*>::iterator _iterator;
        };
    public:
        ClientSessionImpl(const string& ipaddress, const string& timestamp, const int remoteid)
            : _ipaddress(ipaddress)
            , _timestamp(timestamp)
            , _remoteid(remoteid)
        {
        }
        ~ClientSessionImpl()
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
        virtual int RemoteId() const
        {
            return (_remoteid);
        }


        BEGIN_INTERFACE_MAP(ClientSessionImpl)
        INTERFACE_ENTRY(Exchange::ISecureShellServer::IClientSession)
        END_INTERFACE_MAP

    private:
        std::string _ipaddress;
        std::string _timestamp;
        int _remoteid;
};

class SecureShellImplementation : public Exchange::ISecureShellServer {

public:
    SecureShellImplementation()=default;
    SecureShellImplementation(const SecureShellImplementation&) = delete;
    SecureShellImplementation& operator= (const SecureShellImplementation&) = delete;

    ~SecureShellImplementation() override
    {
        Deactivate_dropbear();
    }

    uint Activate_dropbear(const string& inputConfigs) override
    {
        return  activate_dropbear(inputConfigs.c_str());
    }
    
    uint Deactivate_dropbear() override
    {
        return deactivate_dropbear();
    }

    uint GetActiveSessionsCount()
    {
        return get_active_sessions_count();
    }

    Exchange::ISecureShellServer::IClientSession::IIterator* ActiveClientsIter() override
    {
        std::list<ClientSessionImpl*> local_clients;
	    Exchange::ISecureShellServer::IClientSession::IIterator* iter=nullptr;

        uint count = get_active_sessions_count();
        TRACE(Trace::Information, (_T("Get details of (%d)active SSH client sessions managed by Dropbear service"), count));

        if (count>0)
            {
                struct client_info *info = new client_info[count];
                get_active_sessions_info(info, count);

                for(int32_t i=0; i<count; i++)
                {
                    TRACE(Trace::Information, (_T("Count: %d index: %d pid: %d IP: %s Timestamp: %s"),
                                                count, i, info[i].pid, info[i].ipaddress, info[i].timestamp));

                    local_clients.push_back(Core::Service<ClientSessionImpl>::Create<ClientSessionImpl>(info[i].ipaddress,
                                            info[i].timestamp, info[i].pid));
                }
                delete[] info;

                if (local_clients.empty() == false) {
                        iter = Core::Service<ClientSessionImpl::IteratorImpl>::Create<ISecureShellServer::IClientSession::IIterator>(local_clients);
                        TRACE(Trace::Information, (_T("Currently total %d sessions are active"), iter->Count()));
                }
                local_clients.clear(); // Clear all elements before we re-populate it with new elements
            }

            return iter;
        }

        uint CloseClientSession(const int pid) override 
        {
            return close_client_session(pid);
        }

    BEGIN_INTERFACE_MAP(SecureShellImplementation)
    INTERFACE_ENTRY(Exchange::ISecureShellServer)
    END_INTERFACE_MAP

    };

    SERVICE_REGISTRATION(SecureShellImplementation, 1, 0);

} // namespace plugin

}