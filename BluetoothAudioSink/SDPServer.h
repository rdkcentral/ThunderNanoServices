/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 RDK Management
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

namespace WPEFramework {

namespace SDP {

    template<typename FLOW>
    void Dump(const Bluetooth::SDP::Tree& tree)
    {
        using namespace Bluetooth::SDP;

        uint16_t cnt = 1;
        for (auto const& service : tree.Services()) {
            string name = "<unknown>";

            TRACE_GLOBAL(FLOW, (_T("Service #%i '%s'"), cnt++, name.c_str()));

            TRACE_GLOBAL(FLOW, (_T("  Handle:")));
            TRACE_GLOBAL(FLOW, (_T("    - 0x%08x"), service.Handle()));

            if (service.ServiceClassIDList() != nullptr) {
                TRACE_GLOBAL(FLOW, (_T("  Service Class ID List:")));
                for (auto const& clazz : service.ServiceClassIDList()->Classes()) {
                    TRACE_GLOBAL(FLOW, (_T("    - %s '%s'"),
                                clazz.Type().ToString().c_str(), clazz.Name().c_str()));
                }
            }

            if (service.BrowseGroupList() != nullptr) {
                TRACE_GLOBAL(FLOW, (_T("  Browse Group List:")));
                for (auto const& group : service.BrowseGroupList()->Classes()) {
                    TRACE_GLOBAL(FLOW, (_T("    - %s '%s'"),
                                group.Type().ToString().c_str(), group.Name().c_str()));
                }
            }

            if (service.ProtocolDescriptorList() != nullptr) {
                TRACE_GLOBAL(FLOW, (_T("  Protocol Descriptor List:")));
                for (auto const& protocol : service.ProtocolDescriptorList()->Protocols()) {
                    TRACE_GLOBAL(FLOW, (_T("    - %s '%s', parameters: %s"),
                                protocol.first.Type().ToString().c_str(), protocol.first.Name().c_str(),
                                Bluetooth::DataRecord(protocol.second).ToString().c_str()));
                }
            }

            if (service.LanguageBaseAttributeIDList() != nullptr) {
                TRACE_GLOBAL(FLOW, (_T("  Language Base Attribute IDs:")));
                for (auto const& lb : service.LanguageBaseAttributeIDList()->LanguageBases()) {
                    TRACE_GLOBAL(FLOW, (_T("    - 0x%04x, 0x%04x, 0x%04x"),
                                lb.Language(), lb.Charset(), lb.Base()));
                }
            }

            if (service.ProfileDescriptorList() != nullptr) {
                TRACE_GLOBAL(FLOW, (_T("  Profile Descriptor List:")));
                for (auto const& profile : service.ProfileDescriptorList()->Profiles()) {
                    TRACE_GLOBAL(FLOW, (_T("    - %s '%s', version: %d.%d"),
                                profile.first.Type().ToString().c_str(), profile.first.Name().c_str(),
                                (profile.second.Version() >> 8), (profile.second.Version() & 0xFF)));
                }
            }

            if (service.Attributes().empty() == false) {
#ifdef __DEBUG__
                TRACE_GLOBAL(FLOW, (_T("  All attributes:")));
#else
                TRACE_GLOBAL(FLOW, (_T("  Unknown attributes:")));
#endif
                for (auto const& attribute : service.Attributes()) {
                    TRACE_GLOBAL(FLOW, (_T("    - %04x '%s', value: %s"),
                                attribute.Id(), attribute.Name().c_str(),
                                Bluetooth::DataRecord(attribute.Value()).ToString().c_str()));
                }
            }
        }
    }

    class ServiceDiscoveryServer : public Bluetooth::SDP::Tree {

        class Implementation;

        class SDPServerFlow {
        public:
            SDPServerFlow() = delete;
            SDPServerFlow(const SDPServerFlow&) = delete;
            SDPServerFlow& operator=(const SDPServerFlow&) = delete;
            SDPServerFlow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            explicit SDPServerFlow(const string& text)
                : _text(Core::ToString(text))
            {
            }
            ~SDPServerFlow() = default;

        public:
            const char* Data() const
            {
                return (_text.c_str());
            }
            uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        }; // class SDPServerFlowh

        class ClientConnection : public Bluetooth::SDP::ServerSocket {
        public:
            ClientConnection() = delete;
            ClientConnection(const ClientConnection&) = delete;
            ClientConnection& operator=(const ClientConnection&) = delete;

            ClientConnection(const SOCKET& connector, const Core::NodeId& remoteNode, Core::SocketServerType<ClientConnection>* server)
                : Bluetooth::SDP::ServerSocket(connector, remoteNode)
                , _server(*static_cast<Implementation*>(server))
            {
                ASSERT(server != nullptr);
            }

            ~ClientConnection() = default;

        private:
            void Operational() override
            {
                TRACE(SDPServerFlow, (_T("Incoming connection from %s"), RemoteId().c_str()));
            }
            void Inoperational() override
            {
                TRACE(SDPServerFlow, (_T("Closed connection to %s"), RemoteId().c_str()));
            }

            void Services(const Bluetooth::UUID& uuid, std::list<uint32_t>& serviceHandles) override
            {
                // Provides all service handles that realize the particular service class ID in this server's SDP tree.
                // Used for serializing SDP_ServiceSearchRequest and SDP_ServiceSearchAttributeRequest (combined with Serialize()).
                TRACE(SDPServerFlow, (_T("Incoming service query for UUID %s for remote client %s..."),
                                      uuid.ToString().c_str(), RemoteId().c_str()));

                _server.Tree().Lock();
                for (auto const& service : _server.Tree().Services()) {
                    if (service.Search(uuid) == true) {
                        serviceHandles.push_back(service.Handle());
                        TRACE(SDPServerFlow, (_T("Found service 0x%08x"), service.Handle()));
                    }
                }
                _server.Tree().Unlock();

                TRACE(SDPServerFlow, (_T("Found %i services"), serviceHandles.size()));
            }

            void Serialize(const uint32_t serviceHandle, const std::pair<uint16_t, uint16_t>& range, std::function<void(const uint16_t id, const Bluetooth::Buffer& buffer)> Store) const override
            {
                using AttributeDescriptor = Bluetooth::SDP::Service::AttributeDescriptor;

                // Serializes attribute data for a service.
                // Used for serializing SDP_ServiceAttributeRequest and SDP_ServiceSearchAttributeRequest (combined with Services()).
                TRACE(SDPServerFlow, (_T("Incoming attribute range 0x%04x-0x%04x query for service 0x%08x for remote client %s..."),
                                        range.first, range.second, serviceHandle, RemoteId().c_str()));

                _server.Tree().Lock();
                const Bluetooth::SDP::Service* service = _server.Tree().Find(serviceHandle);
                if (service != nullptr) {
                    // Firstly, see if any universal attributes overlap with the range.
                    for (uint id = std::max(static_cast<uint16_t>(AttributeDescriptor::ServiceRecordHandle), range.first);
                            id <= std::min(static_cast<uint16_t>(AttributeDescriptor::IconURL), range.second); id++)
                    {
                        Store(id, service->Serialize(id));
                    }

                    // Secondly, see if the custom attributes are in the range.
                    for (auto const& attr : service->Attributes()) {
                        if ((attr.Id() >= std::max(static_cast<uint16_t>(0x100), range.first)) && (attr.Id() <= range.second)) {
                            Store(attr.Id(), attr.Value());
                        }
                    }

                }
                _server.Tree().Unlock();

            }

        private:
            Implementation& _server;
        }; // class ClientConnection

        class Implementation : public Core::SocketServerType<ClientConnection> {
        public:
            class Timer {
                // Timer used for disconnecting stale SDP connections.
            public:
                Timer() = delete;
                Timer(const Timer&) = delete;
                Timer(Timer&&) = default;
                Timer& operator=(const Timer&) = delete;

                Timer(Implementation& server)
                    : _server(server)
                {
                }
                ~Timer() = default;

            public:
                uint64_t Timed(const uint64_t scheduledTime)
                {
                    return (_server.EvaluateConnections(scheduledTime));
                }

            private:
                Implementation& _server;
            }; // class Timer

        public:
            Implementation(const Implementation&) = delete;
            Implementation& operator=(const Implementation&) = delete;

            Implementation(const Bluetooth::SDP::Tree& tree, const uint64_t inactivityTimeout)
                : Core::SocketServerType<ClientConnection>()
                , _tree(tree)
                , _inactivityTimeout(inactivityTimeout)
                , _connectionEvaluationPeriod(inactivityTimeout / 2)
                , _connectionEvaluationTimer(Core::Thread::DefaultStackSize(), _T("SDPConnectionChecker"))
            {
            }
            ~Implementation()
            {
                Close(1000);
            }

        public:
            uint32_t Start(const uint16_t interface)
            {
                uint32_t result = Core::ERROR_NONE;

                Core::NodeId node(Bluetooth::Address(interface).NodeId(Bluetooth::Address::BREDR_ADDRESS, 0, Bluetooth::SDP::ServerSocket::SDP_PSM /* a well-known PSM */));

                LocalNode(node);

                if (Open(2000) == Core::ERROR_NONE) {
                    if ((_connectionEvaluationPeriod != 0) && (_inactivityTimeout != 0)) {
                        Core::Time NextTick = Core::Time::Now();
                        NextTick.Add(_connectionEvaluationPeriod);
                        _connectionEvaluationTimer.Schedule(NextTick.Ticks(), Timer(*this));
                    }

                    TRACE(Trace::Information, (_T("Started SDP Server, listening on %s, providing %i services"), node.QualifiedName().c_str(), Tree().Services().size()));
                    Dump<SDPServerFlow>(Tree());
                } else {
                    result = Core::ERROR_GENERAL;
                }

                return (result);
            }
            uint32_t Stop()
            {
                TRACE(Trace::Information, (_T("Stopping SDP Server")));
                Close(Core::infinite);
                return (Core::ERROR_NONE);
            }
            const Bluetooth::SDP::Tree& Tree() const
            {
                return (_tree);
            }

        private:
            uint64_t EvaluateConnections(const uint64_t timeNow)
            {
                Core::Time NextTick(Core::Time::Now());
                NextTick.Add(_connectionEvaluationPeriod);

                // Forget closed ones.
                Cleanup();

                auto index(Clients());
                while (index.Next() == true) {
                    if (timeNow - index.Client()->LastActivity() > _inactivityTimeout) {
                        // Close inactive connections
                        TRACE(SDPServerFlow, (_T("Closing inactive connection to %s"), index.Client()->RemoteId().c_str()));
                        index.Client()->Close(0);
                    }
                }

                return (NextTick.Ticks());
            }

        private:
            const Bluetooth::SDP::Tree& _tree;
            uint64_t _inactivityTimeout;
            uint32_t _connectionEvaluationPeriod;
            Core::TimerType<Timer> _connectionEvaluationTimer;
        }; // Class Implementation

    public:
        ServiceDiscoveryServer(const ServiceDiscoveryServer&) = delete;
        ServiceDiscoveryServer& operator=(const ServiceDiscoveryServer&) = delete;
        ServiceDiscoveryServer() = default;
        ~ServiceDiscoveryServer()
        {
            Stop();
        }

    public:
        void Start(const uint16_t interface = 0, const uint32_t inactivityTimeout = 10000)
        {
            if (_server.IsValid() == false) {
                _server = Core::ProxyType<Implementation>::Create(*this, inactivityTimeout);
                _server->Start(interface);
            }
        }
        void Stop()
        {
            if (_server.IsValid() == true) {
                _server->Stop();
                _server.Release();
            }
        }

    private:
        Core::ProxyType<Implementation> _server;
    }; //class ServiceDiscoveryServer

} // namespace SDP

}