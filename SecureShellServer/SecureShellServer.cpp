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
 
#include "SecureShellServer.h"

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<SecureShellServer> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::PLATFORM, subsystem::NETWORK },
            // Terminations
            {},
            // Controls
            {}
        );
    }

    const string SecureShellServer::Initialize(PluginHost::IShell* service)
    {
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        Config config;
        config.FromString(service->ConfigLine());
        _InputParameters = config.InputParameters.Value();

        TRACE(Trace::Information, (_T("Starting Dropbear Service with options as: %s"), _InputParameters.c_str()));
        // TODO: Check the return value and based on that change result
        activate_dropbear(const_cast<char*>(_InputParameters.c_str()));

        Exchange::JSecureShellServer::Register(*this, this);

        return string();
    }

    void SecureShellServer::Deinitialize(PluginHost::IShell* service)
    {
        // Deinitialize what we initialized..
        Exchange::JSecureShellServer::Unregister(*this);
        TRACE(Trace::Information, (_T("Stoping Dropbear Service")));
        deactivate_dropbear(); //TODO: Check the return value and based on that change result
    }

    string SecureShellServer::Information() const
    {
        // No additional info to report.
        return string();
    }

    Exchange::ISecureShellServer::IClient::IIterator* SecureShellServer::SessionsInfo()
    {
        std::list<ClientImpl*> local_clients;
	    Exchange::ISecureShellServer::IClient::IIterator* iter = nullptr;

        int32_t count = get_active_sessions_count();
        TRACE(Trace::Information, (_T("Get details of (%d)active SSH client sessions managed by Dropbear service"), count));

	    if (count > 0) {
            struct client_info *info = static_cast<struct client_info*>(::malloc(sizeof(struct client_info) * count));

            get_active_sessions_info(info, count);

            for (int32_t i = 0;  i< count; i++) {
                TRACE(Trace::Information, (_T("Count: %d index: %d pid: %d IP: %s Timestamp: %s"),
                                            count, i, info[i].pid, info[i].ipaddress, info[i].timestamp));

                local_clients.push_back(Core::ServiceType<SecureShellServer::ClientImpl>::Create<ClientImpl>(info[i].ipaddress,
                                        info[i].timestamp, std::to_string(info[i].pid)));
            }
            ::free(info);

            if (local_clients.empty() == false) {
                    iter = Core::ServiceType<ClientImpl::IteratorImpl>::Create<ISecureShellServer::IClient::IIterator>(local_clients);
                    TRACE(Trace::Information, (_T("Currently total %d sessions are active"), iter->Count()));
            }
            local_clients.clear(); // Clear all elements before we re-populate it with new elements
	    }

	    return iter;
    }

    Core::hresult SecureShellServer::ActiveSessionsCount(uint32_t& count) const
    {
        Exchange::ISecureShellServer::IClient::IIterator* it = SessionsInfo();

        if (it != nullptr) {
            count = it->Count();
            it->Release();
        }
        else {
            count = 0;
        }

        TRACE(Trace::Information, (_T("Total number of active SSH client sessions managed by Dropbear service: %d"), count));

        return (Core::ERROR_NONE);
    }

    Core::hresult SecureShellServer::ActiveSessionsInfo(Exchange::ISSHSessions::IClientIterator*& clients) const
    {
        Core::hresult result = Core::ERROR_GENERAL;
        std::list<Exchange::ISSHSessions::Client> clientList;
	    Exchange::ISecureShellServer::IClient::IIterator* it = SessionsInfo();

        if (it != nullptr) {
            uint32_t index = 0;
            it->Reset();

            while(it->Next()) {
                TRACE(Trace::Information, (_T("Count: %d index:%d pid: %s IP: %s Timestamp: %s"),
                                           it->Count(), index++,
                                           it->Current()->RemoteId().c_str(),
                                           it->Current()->IpAddress().c_str(),
                                           it->Current()->TimeStamp().c_str()));

                Exchange::ISSHSessions::Client client;
                client.pid = it->Current()->RemoteId();
                client.ipAddress = it->Current()->IpAddress();
                client.timeStamp = it->Current()->TimeStamp();
                clientList.push_back(std::move(client));
            }
            it->Release();
        }

        if (clientList.empty() == false) {
            using Iterator = Exchange::ISSHSessions::IClientIterator;
            clients = Core::ServiceType<RPC::IteratorType<Iterator>>::Create<Iterator>(std::move(clientList));
            result = Core::ERROR_NONE;
        }
        else {
            clients = nullptr;
        }

        return (result);
    }

    Core::hresult SecureShellServer::CloseSession(uint32_t& pid) const
    {
        ClientImpl* client = Core::ServiceType<SecureShellServer::ClientImpl>::Create<ClientImpl>(std::to_string(pid));
        client->Close();

        return (Core::ERROR_NONE);
    }

    /*virtual*/ Exchange::ISecureShellServer::IClient::IIterator* SecureShellServer::Clients()
    {
        return SecureShellServer::SessionsInfo();
    }

} // namespace Plugin
} // namespace Thunder
