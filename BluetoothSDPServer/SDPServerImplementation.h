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

#include "DebugTracing.h"

namespace Thunder {

namespace Plugin {

namespace SDP {

    class ServiceDiscoveryServer : public Bluetooth::SDP::Tree {

        static constexpr uint32_t SocketOpenTimeoutMs = 1000;

        template<typename SERVER>
        class PeerConnection : public Bluetooth::SDP::ServerSocket {
        public:
            PeerConnection() = delete;
            PeerConnection(const PeerConnection&) = delete;
            PeerConnection& operator=(const PeerConnection&) = delete;
            ~PeerConnection() = default;

            PeerConnection(const SOCKET& connector, const Core::NodeId& remoteNode, Core::SocketServerType<PeerConnection<SERVER>>* server)
                : Bluetooth::SDP::ServerSocket(connector, remoteNode)
                , _server(*static_cast<SERVER*>(server))
                , _lastActivity(0)
            {
                ASSERT(server != nullptr);
            }

        public:
            uint64_t LastActivity() const {
                return (_lastActivity);
            }

        public:
            void OnPDU(const Bluetooth::SDP::ServerSocket& socket, const Bluetooth::SDP::PDU& request, const Bluetooth::SDP::ServerSocket::ResponseHandler& handler) override
            {
                TRACE(Verbose, (_T("Incoming SDP request %d from remote client %s"),
                        request.Type(), RemoteNode().HostName().c_str()));

                // Just forward the request to the server.
                _server.OnPDU(socket, request, handler);

                _lastActivity = Core::Time::Now().Ticks();
            }

        private:
            void Operational(const bool upAndRunning) override
            {
                if (upAndRunning == true) {
                    _lastActivity = Core::Time::Now().Ticks();

                    TRACE(Verbose, (_T("Incoming SDP connection from %s"), RemoteId().c_str()));
                }
                else {
                    TRACE(Verbose, (_T("Closed SDP connection to %s"), RemoteId().c_str()));
                }
            }

        private:
            SERVER& _server;
            mutable uint64_t _lastActivity;
        }; // class PeerConnection

        class Implementation : public Core::SocketServerType<PeerConnection<Implementation>>,
                               public Bluetooth::SDP::Server {
        public:
            class Timer {
                // Timer used for disconnecting stale SDP connections.
            public:
                Timer() = delete;
                Timer(const Timer&) = delete;
                Timer(Timer&&) = default;
                Timer& operator=(const Timer&) = delete;
                ~Timer() = default;

                explicit Timer(Implementation& server)
                    : _server(server)
                {
                }

            public:
                uint64_t Timed(const uint64_t scheduledTime) {
                    return (_server.EvaluateConnections(scheduledTime));
                }

            private:
                Implementation& _server;
            }; // class Timer

        public:
            Implementation(const Implementation&) = delete;
            Implementation& operator=(const Implementation&) = delete;

            Implementation(ServiceDiscoveryServer& parent, const uint32_t inactivityTimeoutMs)
                : Core::SocketServerType<PeerConnection<Implementation>>()
                , Bluetooth::SDP::Server()
                , _parent(parent)
                , _inactivityTimeoutUs(inactivityTimeoutMs * Core::Time::TicksPerMillisecond)
                , _connectionEvaluationPeriodMs(100)
                , _connectionEvaluationTimer(Core::Thread::DefaultStackSize(), _T("SDPConnectionChecker"))
            {
            }
            ~Implementation()
            {
                Stop();
            }

        public:
            // Bluetooth::SDP::Server overrides
            void WithServiceTree(const std::function<void(const Bluetooth::SDP::Tree&)>& inspectCb) override
            {
                // The lib server implementation with look up the service tree as required.
                _parent.WithServiceTree(inspectCb);
            }

        public:
            uint32_t Start(const uint8_t interface, const uint8_t psm)
            {
                uint32_t result = Core::ERROR_NONE;

                TRACE(Verbose, (_T("Starting SDP Server...")));

                Core::NodeId node(Bluetooth::Address(interface).NodeId(Bluetooth::Address::BREDR_ADDRESS, 0, psm));

                LocalNode(node);

                if (Open(SocketOpenTimeoutMs) == Core::ERROR_NONE) {

                    // Set up inactivity evaluation timer.
                    if ((_connectionEvaluationPeriodMs != 0) && (_inactivityTimeoutUs != 0)) {
                        Core::Time NextTick = Core::Time::Now();
                        NextTick.Add(_connectionEvaluationPeriodMs);

                        _connectionEvaluationTimer.Schedule(NextTick.Ticks(), Timer(*this));
                    }

                    TRACE(Verbose, (_T("SDP server is now listening on HCI interface %d (%s), L2CAP PSM %d"),
                            node.QualifiedName().c_str(), interface, psm));
                }
                else {
                    result = Core::ERROR_GENERAL;
                }

                return (result);
            }
            uint32_t Stop()
            {
                TRACE(Verbose, (_T("Stopping SDP Server...")));

                Close(Core::infinite);

                return (Core::ERROR_NONE);
            }

        private:
            uint64_t EvaluateConnections(const uint64_t timeNow)
            {
                Core::Time NextTick(Core::Time::Now());
                NextTick.Add(_connectionEvaluationPeriodMs);

                // Forget closed ones.
                Cleanup();

                auto index = Clients();

                while (index.Next() == true) {
                    if (timeNow - index.Client()->LastActivity() >= _inactivityTimeoutUs) {
                        // Close inactive connections
                        TRACE(Verbose, (_T("Closing inactive connection to %s"), index.Client()->RemoteId().c_str()));
                        index.Client()->Close(0);
                    }
                }

                return (NextTick.Ticks());
            }

        private:
            ServiceDiscoveryServer& _parent;
            uint64_t _inactivityTimeoutUs;
            uint32_t _connectionEvaluationPeriodMs;
            Core::TimerType<Timer> _connectionEvaluationTimer;
        }; // Class Implementation

    public:
        ServiceDiscoveryServer(const ServiceDiscoveryServer&) = delete;
        ServiceDiscoveryServer& operator=(const ServiceDiscoveryServer&) = delete;

        ServiceDiscoveryServer()
            : _server()
            , _lock()
        {
        }
        ~ServiceDiscoveryServer()
        {
            Stop();
        }

    public:
        void Start(const uint8_t interface, const uint8_t psm, const uint32_t inactivityTimeoutMs)
        {
            if (_server.IsValid() == false) {
                _server = Core::ProxyType<Implementation>::Create(*this, inactivityTimeoutMs);

                if (_server->Start(interface, psm) == Core::ERROR_NONE) {
                    TRACE(Trace::Information, (_T("Started Bluetooth SDP Server, providing %i services"), Services().size()));

                    Dump<Verbose>(*this);
                }
                else {
                    TRACE(Trace::Error, (_T("Failed to start Bluetooth SDP server")));
                    _server.Release();
                }
            }
        }
        void Stop()
        {
            if (_server.IsValid() == true) {
                _server->Stop();
                _server.Release();

                TRACE(Trace::Information, (_T("Stopped Bluetooth SDP server")));
            }
        }

    public:
        void WithServiceTree(const std::function<void(Bluetooth::SDP::Tree&)>& inspectCb)
        {
            ASSERT(inspectCb != nullptr);

            _lock.Lock();
            inspectCb(*this);
            _lock.Unlock();
        }

    private:
        Core::ProxyType<Implementation> _server;
        mutable Core::CriticalSection _lock;
    }; //class ServiceDiscoveryServer

} // namespace SDP

} // namespace Plugin

}