/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological
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

#include "AudioEndpoint.h"

#include <interfaces/IBluetooth.h>
#include <interfaces/IBluetoothAudio.h>
#include <bluetooth/audio/bluetooth_audio.h>

namespace WPEFramework {

namespace Plugin {

    class DecoupledJob : private Core::WorkerPool::JobType<DecoupledJob&> {
        friend class Core::ThreadPool::JobType<DecoupledJob&>;

    public:
        using Job = std::function<void()>;

        DecoupledJob(const DecoupledJob&) = delete;
        DecoupledJob& operator=(const DecoupledJob&) = delete;

        DecoupledJob()
            : Core::WorkerPool::JobType<DecoupledJob&>(*this)
            , _lock()
            , _job(nullptr)
        {
        }

        ~DecoupledJob() = default;

    public:
        void Submit(const Job& job)
        {
            _lock.Lock();
            _job = job;
            JobType::Submit();
            ASSERT(JobType::IsIdle() == false);
            _lock.Unlock();
        }
        void Schedule(const Job& job, const uint32_t defer)
        {
            _lock.Lock();
            _job = job;
            JobType::Reschedule(Core::Time::Now().Add(defer));
            ASSERT(JobType::IsIdle() == false);
            _lock.Unlock();
        }
        void Revoke()
        {
            _lock.Lock();
            _job = nullptr;
            JobType::Revoke();
            _lock.Unlock();
        }

    private:
        void Dispatch()
        {
            _lock.Lock();
            Job DoTheJob = _job;
            _job = nullptr;
            _lock.Unlock();
            DoTheJob();
        }

    private:
        Core::CriticalSection _lock;
        Job _job;
    }; // class DecoupledJob

    static Core::NodeId Designator(const Exchange::IBluetooth::IDevice* device)
    {
        ASSERT(device != nullptr);

        return (Bluetooth::Address(device->LocalId().c_str()).NodeId(static_cast<Bluetooth::Address::type>(device->Type()), 0, 0));
    }
    static Core::NodeId Designator(const Exchange::IBluetooth::IDevice* device, const uint16_t psm)
    {
        ASSERT(device != nullptr);

        return (Bluetooth::Address(device->RemoteId().c_str()).NodeId(static_cast<Bluetooth::Address::type>(device->Type()), 0 /* must be zero */, psm));
    }

    class SignallingServer : public Bluetooth::AVDTP::Server {
    public:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Interface(0 /* HCI0 */)
                , PSM(Bluetooth::AVDTP::PSM)
                , InactivityTimeoutMs(5 * 60 * 1000)
            {
                Add(_T("interface"), &Interface);
                Add(_T("psm"), &PSM);
                Add(_T("inactivitytimeout"), &InactivityTimeoutMs);

            }
            ~Config() = default;

        public:
            Core::JSON::DecUInt8 Interface;
            Core::JSON::DecUInt8 PSM;
            Core::JSON::DecUInt32 InactivityTimeoutMs;
        }; // class Config

    public:
        template<typename SOCKETSERVER>
        class Channel : public Bluetooth::AVDTP::Socket {
        public:
            Channel() = delete;
            Channel(const Channel&) = delete;
            Channel& operator=(const Channel&) = delete;
            ~Channel() = default;

            Channel(const SOCKET& connector, const Core::NodeId& remoteNode, Core::SocketServerType<Channel<SOCKETSERVER>>* server)
                : Bluetooth::AVDTP::Socket(connector, remoteNode)
                , _socketServer(static_cast<SOCKETSERVER*>(server))
                , _lastActivity(0)
            {
                // Created by the socket server.
                ASSERT(server != nullptr);
            }
            Channel(const Core::NodeId& localNode, const Core::NodeId& remoteNode, SOCKETSERVER* server)
                : Bluetooth::AVDTP::Socket(localNode, remoteNode)
                , _socketServer(server)
                , _lastActivity(0)
            {
                // Created by the signalling server.
                ASSERT(server != nullptr);
            }

        public:
            uint64_t LastActivity() const {
                return (_lastActivity);
            }

        private:
            // Socket overrides
            void OnSignal(const Bluetooth::AVDTP::Signal& signal, const Bluetooth::AVDTP::Socket::ResponseHandler& handler) override
            {
                TRACE(ServerFlow, (_T("Incoming AVDTP signal %d from remote client %s"), signal.Id(), RemoteNode().HostName().c_str()));
                _socketServer->OnSignal(this, signal, handler);
                _lastActivity = Core::Time::Now().Ticks();
            }
            void OnPacket(const uint8_t packet[], const uint16_t length) override
            {
                _socketServer->OnPacket(this, packet, length);
                _lastActivity = Core::Time::Now().Ticks();
            }
            void Operational(const bool isRunning) override
            {
                if (isRunning == true) {
                    TRACE(ServerFlow, (_T("Incoming AVDTP connection from %s"), RemoteNode().HostName().c_str()));
                }
                else {
                    TRACE(ServerFlow, (_T("Closed AVDTP connection to %s"), RemoteNode().HostName().c_str()));
                }

                _socketServer->Operational(this, isRunning);
                _lastActivity = Core::Time::Now().Ticks();
            }

        private:
            SOCKETSERVER* _socketServer;
            mutable uint64_t _lastActivity;
        }; // class Channel

        class SocketServer;
        using PeerConnection = Channel<SocketServer>;

        class SocketServer : public Core::SocketServerType<PeerConnection> {
        public:
            class Timer {
                // Timer used for disconnecting stale AVDTP connections.
            public:
                Timer() = delete;
                Timer(const Timer&) = delete;
                Timer(Timer&&) = default;
                Timer& operator=(const Timer&) = delete;
                ~Timer() = default;

                Timer(SocketServer& server)
                    : _socketServer(server)
                {
                }

            public:
                uint64_t Timed(const uint64_t scheduledTime)
                {
                    return (_socketServer.EvaluateConnections(scheduledTime));
                }

            private:
                SocketServer& _socketServer;
            }; // class Timer

        public:
            static constexpr uint32_t OpenTimeoutMs = 2000;
            static constexpr uint32_t CloseTimeoutMs = 2000;
            static constexpr uint32_t ConnectionEvaluationPeriodMs = 1000;

        public:
            friend class Channel<SocketServer>;

            SocketServer(const SocketServer&) = delete;
            SocketServer& operator=(const SocketServer&) = delete;

            SocketServer(SignallingServer& parent, const uint32_t inactivityTimeoutMs)
                : Core::SocketServerType<Channel<SocketServer>>()
                , _parent(parent)
                , _inactivityTimeoutUs(inactivityTimeoutMs * Core::Time::TicksPerMillisecond)
                , _connectionEvaluationTimer(Core::Thread::DefaultStackSize(), _T("SignallingConnectionChecker"))
            {
            }
            ~SocketServer()
            {
                Stop();
            }

        public:
            uint32_t Start(const uint16_t interface, const uint16_t psm)
            {
                uint32_t result = Core::ERROR_NONE;

                const Core::NodeId node(Bluetooth::Address(interface).NodeId(Bluetooth::Address::BREDR_ADDRESS, 0 /* cid */, psm));

                LocalNode(node);

                result = Open(OpenTimeoutMs);

                if (result == Core::ERROR_NONE) {
                    if (_inactivityTimeoutUs != 0) {
                        Core::Time NextTick = Core::Time::Now();
                        NextTick.Add(ConnectionEvaluationPeriodMs);

                        _connectionEvaluationTimer.Schedule(NextTick.Ticks(), Timer(*this));
                    }
                }

                return (result);
            }
            uint32_t Stop()
            {
                TRACE(ServerFlow, (_T("Stopping AVDTP server...")));

                if (Close(CloseTimeoutMs) != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Failed to stop AVDTP server!")));
                }

                return (Core::ERROR_NONE);
            }

        private:
            // Channel callbacks
            void Operational(PeerConnection* channel, const bool isRunning)
            {
                _parent.Operational(channel, isRunning);
            }

        private:
            void OnSignal(PeerConnection* channel, const Bluetooth::AVDTP::Signal& signal, const Bluetooth::AVDTP::Socket::ResponseHandler& handler)
            {
                _parent.OnSignal(channel, signal, handler);
            }
            void OnPacket(PeerConnection* channel, const uint8_t packet[], const uint16_t length)
            {
                _parent.OnPacket(channel, packet, length);
            }

        private:
            // Timer callbacks
            uint64_t EvaluateConnections(const uint64_t timeNow)
            {
                Core::Time NextTick(Core::Time::Now());
                NextTick.Add(ConnectionEvaluationPeriodMs);

                // Forget closed ones.
                Cleanup();

                auto index(Clients());

                while (index.Next() == true) {
                    // If the endpoint is being open, it must not time out.
                    if (index.Client()->IsBusy() == false) {

                        const uint64_t lastActivity = index.Client()->LastActivity();
                        const uint64_t elapsed = (timeNow > lastActivity? timeNow - lastActivity : 0);

                        if (elapsed >= _inactivityTimeoutUs) {
                            TRACE(ServerFlow, (_T("Closing inactive AVDTP connection to %s"), index.Client()->RemoteNode().HostName().c_str()));
                            index.Client()->Close(0);
                        }
                    }
                }

                return (NextTick.Ticks());
            }

        private:
            SignallingServer& _parent;
            uint64_t _inactivityTimeoutUs;
            Core::TimerType<Timer> _connectionEvaluationTimer;
        }; // Class SocketServer

    public:
        SignallingServer(const SignallingServer&) = delete;
        SignallingServer& operator=(const SignallingServer&) = delete;

    private:
        SignallingServer()
            : _socketServer()
            , _lock()
            , _peerConnection()
            , _endpoints()
            , _transportEndpoint(nullptr)
        {
        }
        ~SignallingServer()
        {
            Stop();
        }

    private:
        // SocketServer callbacks
        void Operational(PeerConnection* channel, const bool isRunning)
        {
            printf("OPERA %d\n", isRunning);
            ASSERT(channel != nullptr);

            _lock.Lock();

            if (isRunning == true) {
                for (auto& ep : _endpoints) {
                    A2DP::AudioEndpoint& endpoint = ep.second;

                    if (endpoint.DeviceAddress() == channel->RemoteNode().HostAddress()) {
                        if (endpoint.IsFree() == false) {
                            if (channel->Type() == Bluetooth::AVDTP::Socket::SIGNALLING) {
                                channel->Type(Bluetooth::AVDTP::Socket::TRANSPORT);
                                endpoint.Operational(*channel, true);
                                _transportEndpoint = &endpoint;
                            }
                            else {
                                TRACE(Trace::Error, (_T("Unsupported AVDTP channel!")));
                            }
                        }

                        break;
                    }
                }
            }
            else {
                if (channel->Type() == Bluetooth::AVDTP::Socket::TRANSPORT) {
                    ASSERT(_transportEndpoint != nullptr);
                    _transportEndpoint->Operational(*channel, false);
                    _transportEndpoint = nullptr;
                }
                else if (channel->Type() == Bluetooth::AVDTP::Socket::SIGNALLING) {
                    for (auto& ep : _endpoints) {
                        A2DP::AudioEndpoint& endpoint = ep.second;

                        if (endpoint.DeviceAddress() == channel->RemoteNode().HostAddress()) {
                            if (endpoint.IsFree() == false) {
                                endpoint.Operational(*channel, false);
                            }

                            break;
                        }
                    }
                }
                else {
                    // Should be unreachable...
                    ASSERT(false);
                }
            }

            _lock.Unlock();
        }
        void OnSignal(PeerConnection* /* channel */, const Bluetooth::AVDTP::Signal& signal, const Bluetooth::AVDTP::Socket::ResponseHandler& handler)
        {
            Server::OnSignal(signal, handler);
        }
        void OnPacket(PeerConnection* /* channel */, const uint8_t packet[], const uint16_t length)
        {
            _lock.Lock();

            ASSERT(_transportEndpoint != nullptr);

            if  (_transportEndpoint != nullptr) {
                _transportEndpoint->OnPacket(packet, length);
            }

            _lock.Unlock();
        }

    public:
        static SignallingServer& Instance()
        {
            static SignallingServer server;
            return (server);
        }

    public:
        void Add(const bool sink, Bluetooth::A2DP::IAudioCodec* codec, A2DP::AudioEndpoint::IHandler& handler)
        {
            _lock.Lock();

            uint8_t id = (static_cast<uint8_t>(_endpoints.size()) + 1);

            auto it = _endpoints.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(id),
                                        std::forward_as_tuple(handler, id, sink, codec)).first;

            TRACE(Trace::Information, (_T("New audio %s endpoint #%d (codec %02x)"), (sink? "sink": "source"), (*it).second.Id(), (*it).second.Codec()->Type()));

            _lock.Unlock();
        }

    public:
        void Start(const uint16_t hciInterface, const uint8_t psm, const uint32_t inactivityTimeoutMs)
        {
            TRACE(ServerFlow, (_T("Starting AVDTP server on HCI interface %d (PSM %d), timeout %d ms"), hciInterface, psm, inactivityTimeoutMs));

            _lock.Lock();

            if (_socketServer.IsValid() == false) {
                _socketServer = Core::ProxyType<SocketServer>::Create(*this, inactivityTimeoutMs);

                if (_socketServer.IsValid() == true) {
                    if (_socketServer->Start(hciInterface, psm) != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to start the signalling server")));

                        _socketServer.Release();
                    }
                    else {
                        TRACE(Trace::Information, (_T("Started signalling server (listening on HCI interface %d, L2CPAP PSM %d)"), hciInterface, psm));
                    }
                }
            }

            _lock.Unlock();
        }
        void Stop()
        {
            _lock.Lock();

            if (_socketServer.IsValid() == true) {
                _socketServer->Stop();
                _socketServer.Release();

                TRACE(Trace::Information, (_T("Stopped AVDTP Server")));
            }

            _lock.Unlock();
        }

    public:
        Bluetooth::AVDTP::Socket* Create(const Core::NodeId& local, const Core::NodeId& remote)
        {
            Bluetooth::AVDTP::Socket* conn = nullptr;

            _lock.Lock();

            if ((_socketServer.IsValid() == true) && (_peerConnection == nullptr)) {
                _peerConnection.reset(new (std::nothrow) PeerConnection(local, remote, _socketServer.operator->()));
                ASSERT(_peerConnection.get() != nullptr);

                conn = _peerConnection.get();
            }

            _lock.Unlock();

            return (conn);
        }
        void Destroy(Bluetooth::AVDTP::Socket* connection)
        {
            _lock.Lock();

            if ((_socketServer.IsValid() == true) && (_peerConnection.get() == connection)) {
                _peerConnection.reset();
            }

            _lock.Unlock();
        }

    private:
        bool WithEndpoint(const uint8_t id, const std::function<void(Bluetooth::AVDTP::StreamEndPoint&)>& inspectCb) override
        {
            bool result = false;

            _lock.Lock();

            auto it = _endpoints.find(id);

            if (it != _endpoints.end()) {
                inspectCb(it->second);
                result = true;
            }

            _lock.Unlock();

            return (result);
        }

    private:
        Core::ProxyType<SocketServer> _socketServer;
        Core::CriticalSection _lock;
        std::unique_ptr<PeerConnection> _peerConnection;
        std::map<uint8_t, A2DP::AudioEndpoint> _endpoints;
        A2DP::AudioEndpoint* _transportEndpoint;
    };

} // namespace Plugin

}