/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

#include "Tracing.h"

namespace WPEFramework {

namespace SDP {

    class ServiceDiscoveryServer : public Bluetooth::SDP::Tree {

        class Implementation;

        class ClientConnection : public Bluetooth::SDP::ServerSocket {
        public:
            ClientConnection() = delete;
            ClientConnection(const ClientConnection&) = delete;
            ClientConnection& operator=(const ClientConnection&) = delete;

            ClientConnection(const SOCKET& connector, const Core::NodeId& remoteNode, Core::SocketServerType<ClientConnection>* server)
                : Bluetooth::SDP::ServerSocket(connector, remoteNode)
                , _server(*static_cast<Implementation*>(server))
                , _lastActivity(0)
            {
                ASSERT(server != nullptr);
            }

            ~ClientConnection() = default;

        public:
            uint64_t LastActivity() const
            {
                return (_lastActivity);
            }

        private:
            void Operational(const bool upAndRunning) override
            {
                if (upAndRunning == true) {
                    _lastActivity = Core::Time::Now().Ticks();
                    TRACE(Tracing::Verbose, (_T("Incoming connection from %s"), RemoteId().c_str()));
                } else {
                    TRACE(Tracing::Verbose, (_T("Closed connection to %s"), RemoteId().c_str()));
                }
            }

            void Services(const Bluetooth::UUID& uuid, std::list<uint32_t>& serviceHandles) override
            {
                // Provides all service handles that realize the particular service class ID in this server's SDP tree.
                // Used for serializing SDP_ServiceSearchRequest and SDP_ServiceSearchAttributeRequest (combined with Serialize()).
                TRACE(Tracing::Verbose, (_T("Incoming service query for UUID %s for remote client %s..."),
                                      uuid.ToString().c_str(), RemoteId().c_str()));

                _server.Tree().Lock();
                for (auto const& service : _server.Tree().Services()) {
                    if (service.Search(uuid) == true) {
                        serviceHandles.push_back(service.Handle());
                        TRACE(Tracing::Verbose, (_T("Found service 0x%08x '%s'"), service.Handle(), service.Name().c_str()));
                    }
                }
                _server.Tree().Unlock();

                TRACE(Tracing::Verbose, (_T("Found %i matching services"), serviceHandles.size()));

                _lastActivity = Core::Time::Now().Ticks();
            }

            void Serialize(const uint32_t serviceHandle, const std::pair<uint16_t, uint16_t>& range, std::function<void(const uint16_t id, const Bluetooth::Buffer& buffer)> Store) const override
            {
                using AttributeDescriptor = Bluetooth::SDP::Service::AttributeDescriptor;
                uint16_t count = 0;

                // Serializes attribute data for a service.
                // Used for serializing SDP_ServiceAttributeRequest and SDP_ServiceSearchAttributeRequest (combined with Services()).
                TRACE(Tracing::Verbose, (_T("Incoming attribute range 0x%04x-0x%04x query for service 0x%08x for remote client %s..."),
                                        range.first, range.second, serviceHandle, RemoteId().c_str()));

                _server.Tree().Lock();
                const Bluetooth::SDP::Service* service = _server.Tree().Find(serviceHandle);
                if (service != nullptr) {
                    // Firstly, see if any universal attributes overlap with the range.
                    for (uint id = std::max(static_cast<uint16_t>(AttributeDescriptor::ServiceRecordHandle), range.first);
                            id <= std::min(static_cast<uint16_t>(AttributeDescriptor::IconURL), range.second); id++)
                    {
                        Store(id, service->Serialize(id));
                        count++;
                    }

                    // Secondly, see if the custom attributes are in the range.
                    for (auto const& attr : service->Attributes()) {
                        if ((attr.Id() >= std::max(static_cast<uint16_t>(0x100), range.first)) && (attr.Id() <= range.second)) {
                            Store(attr.Id(), attr.Value());
                            count++;
                        }
                    }

                }
                _server.Tree().Unlock();

                _lastActivity = Core::Time::Now().Ticks();

                TRACE(Tracing::Verbose, (_T("Found %i matching attributes"), count));
            }

        private:
            Implementation& _server;
            mutable uint64_t _lastActivity;
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
                , _inactivityTimeout(inactivityTimeout * Core::Time::TicksPerMillisecond /* us */)
                , _connectionEvaluationPeriod(100 /* ms */)
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

                    Tracing::Dump<Tracing::Verbose>(Tree());
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
                    if (timeNow - index.Client()->LastActivity() >= _inactivityTimeout) {
                        // Close inactive connections
                        TRACE(Tracing::Verbose, (_T("Closing inactive connection to %s"), index.Client()->RemoteId().c_str()));
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