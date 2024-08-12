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

namespace Thunder {

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
        class ChannelType : public Bluetooth::AVDTP::Socket {
        public:
            ChannelType() = delete;
            ChannelType(const ChannelType&) = delete;
            ChannelType& operator=(const ChannelType&) = delete;

            ChannelType(const SOCKET& connector, const Core::NodeId& remoteNode, Core::SocketServerType<ChannelType<SOCKETSERVER>>* server)
                : Bluetooth::AVDTP::Socket(connector, remoteNode)
                , _socketServer(static_cast<SOCKETSERVER*>(server))
                , _lastActivity(0)
                , _latch(false)
            {
                ASSERT(server != nullptr);
                TRACE(ServerFlow, (_T("Connection channel to %s constructed by server"), remoteNode.HostAddress().c_str()));

            }
            ChannelType(const Core::NodeId& localNode, const Core::NodeId& remoteNode, SOCKETSERVER* server)
                : Bluetooth::AVDTP::Socket(localNode, remoteNode)
                , _socketServer(server)
                , _lastActivity(0)
                , _latch(false)
            {
                ASSERT(server != nullptr);
                TRACE(ServerFlow, (_T("Connection channel to %s constructed by client"), remoteNode.HostAddress().c_str()));
            }
            ~ChannelType() override
            {
                TRACE(ServerFlow, (_T("Connection channel to %s destructed"), RemoteNode().HostAddress().c_str()));
            }

        public:
            uint64_t LastActivity() const {
                return (_lastActivity);
            }
            bool IsLatched() const {
                return (_latch);
            }

        public:
            void Latch(const bool latched)
            {
                // Is not closeable even if inactive?
                _latch = latched;
            }

        private:
            // Socket overrides
            void OnSignal(const Bluetooth::AVDTP::Signal& signal, const Bluetooth::AVDTP::Socket::ResponseHandler& handler) override
            {
                TRACE(ServerFlow, (_T("Incoming AVDTP signal %d from remote client %s"), signal.Id(), RemoteNode().HostName().c_str()));

                _socketServer->OnSignal(signal, handler);
                _lastActivity = Core::Time::Now().Ticks();
            }
            void OnPacket(const uint8_t packet[], const uint16_t length) override
            {
                // TRACE(ServerFlow, (_T("Incoming RTP packet %d from remote client %s"), signal.Id(), RemoteNode().HostName().c_str()));

                _socketServer->OnPacket(packet, length);
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
            bool _latch;
        }; // class ChannelType

        class SocketServer;
        using PeerConnection = ChannelType<SocketServer>;

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
            friend class ChannelType<SocketServer>;

            SocketServer(const SocketServer&) = delete;
            SocketServer& operator=(const SocketServer&) = delete;

            SocketServer(SignallingServer& parent, const uint32_t inactivityTimeoutMs)
                : Core::SocketServerType<ChannelType<SocketServer>>()
                , _parent(parent)
                , _inactivityTimeoutUs(inactivityTimeoutMs * Core::Time::TicksPerMillisecond)
                , _connectionEvaluationTimer(Core::Thread::DefaultStackSize(), _T("SignallingConnectionChecker"))
                , _peerConnections()
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
                if (Close(CloseTimeoutMs) != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Failed to stop AVDTP server!")));
                }

                return (Core::ERROR_NONE);
            }

        public:
            Core::ProxyType<PeerConnection> Create(const Core::NodeId& local, const Core::NodeId& remote)
            {
                return (_peerConnections.Instance<PeerConnection>(remote.HostName(), local, remote, this));
            }
            Core::ProxyType<PeerConnection> Find(const Core::NodeId& node)
            {
                Core::ProxyType<PeerConnection> connection;

                auto index(Clients());
                while (index.Next() == true) {
                    if (index.Client()->RemoteNode().HostName() == node.HostName()) {
                        connection = index.Client();
                        break;
                    }
                }

                if (connection.IsValid() == false) {
                    connection = _peerConnections.Find(node.HostName());
                }

                return (connection);
            }

        private:
            // Channel callbacks
            void Operational(PeerConnection* channel, const bool isRunning)
            {
                Core::ProxyType<PeerConnection> connection = Find(channel->RemoteNode());

                if (connection.IsValid() == true) {
                    _parent.Operational(connection, isRunning);
                }
            }
            void OnSignal(const Bluetooth::AVDTP::Signal& signal, const Bluetooth::AVDTP::Socket::ResponseHandler& handler)
            {
                _parent.OnSignal(signal, handler);
            }
            void OnPacket(const uint8_t packet[], const uint16_t length)
            {
                _parent.OnPacket(packet, length);
            }

        private:
            // Timer callbacks
            uint64_t EvaluateConnections(const uint64_t timeNow)
            {
                // Evaluates if any of the created connections can be closed and/or destructed.

                Core::Time NextTick(Core::Time::Now());
                NextTick.Add(ConnectionEvaluationPeriodMs);

                // Forget closed ones.
                Cleanup();

                auto index(Clients());

                while (index.Next() == true) {
                    // If the endpoint is being open, it must not time out.
                    if (index.Client()->IsLatched() == false) {

                        const uint64_t lastActivity = index.Client()->LastActivity();
                        const uint64_t elapsed = (timeNow > lastActivity? timeNow - lastActivity : 0);

                        if (elapsed >= _inactivityTimeoutUs) {
                            TRACE(ServerFlow, (_T("Closing inactive AVDTP connection to %s"), index.Client()->RemoteNode().HostName().c_str()));
                            index.Client()->Close(0);
                        }
                    }
                }

                _peerConnections.Visit([this, &timeNow](const string& address, Core::ProxyType<PeerConnection>& connection) {
                    if ((connection->IsOpen() == true) && (connection->IsLatched() == false)) {
                        const uint64_t lastActivity = connection->LastActivity();
                        const uint64_t elapsed = (timeNow > lastActivity? timeNow - lastActivity : 0);

                        if (elapsed >= _inactivityTimeoutUs) {
                            TRACE(ServerFlow, (_T("Closing inactive AVDTP peer connection to %s"), address.c_str()));
                            connection->Close(0);
                        }
                    }
                });

                return (NextTick.Ticks());
            }

        private:
            SignallingServer& _parent;
            uint64_t _inactivityTimeoutUs;
            Core::TimerType<Timer> _connectionEvaluationTimer;
            Core::ProxyMapType<string, PeerConnection> _peerConnections;
        }; // Class SocketServer

    public:
        struct IObserver {
            virtual ~IObserver() {};
            virtual void Operational(const Core::ProxyType<PeerConnection>& channel, const bool isRunning) = 0;
        };

        struct IMediaReceiver {
            virtual ~IMediaReceiver() {};
            virtual void Packet(const uint8_t packet[], const uint16_t length) = 0;
        };

    public:
        SignallingServer(const SignallingServer&) = delete;
        SignallingServer& operator=(const SignallingServer&) = delete;

        friend class Core::SingletonType<SignallingServer>;

    private:
        SignallingServer()
            : _socketServer()
            , _lock()
            , _observersLock()
            , _endpoints()
            , _observers()
            , _mediaReceiver(nullptr)
        {
        }
        ~SignallingServer()
        {
            Clear();
        }

    public:
        static SignallingServer& Instance()
        {
            static SignallingServer& server = Core::SingletonType<SignallingServer>::Instance();
            return (server);
        }
        void Clear()
        {
            Stop();
            _observers.clear();
            _endpoints.clear();
            _mediaReceiver = nullptr;
        }

    public:
        uint8_t Add(const bool sink, Bluetooth::A2DP::IAudioCodec* codec, A2DP::AudioEndpoint::IHandler& handler)
        {
            _lock.Lock();

            uint8_t id = (static_cast<uint8_t>(_endpoints.size()) + 1);

            auto it = _endpoints.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(id),
                                        std::forward_as_tuple(handler, id, sink, codec)).first;

            TRACE(Trace::Information, (_T("New audio %s endpoint #%d (codec %02x)"), (sink? "sink": "source"), (*it).second.Id(), (*it).second.Codec()->Type()));

            _lock.Unlock();

            return (id);
        }

    public:
        void Start(const uint16_t hciInterface, const uint8_t psm, const uint32_t inactivityTimeoutMs, IMediaReceiver& receiver)
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
                        _mediaReceiver = &receiver;
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
                _mediaReceiver = nullptr;

                TRACE(Trace::Information, (_T("Stopped AVDTP Server")));
            }

            _lock.Unlock();
        }

    public:
        void Register(IObserver* observer)
        {
            ASSERT(observer != nullptr);

            // Register a new AVDTP connection observer.

            _observersLock.Lock();

            auto it = std::find(_observers.begin(), _observers.end(), observer);
            ASSERT(it == _observers.end());

            if (it == _observers.end()) {
                _observers.push_back(observer);
            }
            _observersLock.Unlock();
        }
        void Unregister(const IObserver* observer)
        {
            ASSERT(observer != nullptr);

            // Unregister an AVDTP connection observer.

            _observersLock.Lock();

            auto it = std::find(_observers.begin(), _observers.end(), observer);
            ASSERT(it != _observers.end());

            if (it != _observers.end()) {
                _observers.erase(it);
            }

            _observersLock.Unlock();
        }

    public:
        Core::ProxyType<PeerConnection> Create(const Core::NodeId& local, const Core::NodeId& remote)
        {
            // Creates a new AVDTP connection.

            Core::ProxyType<PeerConnection> connection;

            _lock.Lock();

            if (_socketServer.IsValid() == true) {
                connection = _socketServer->Create(local, remote);
            }

            _lock.Unlock();

            return (connection);
        }
        Core::ProxyType<PeerConnection> Claim(const Core::NodeId& remote)
        {
            // Claims an existing AVDTP connection.

            Core::ProxyType<PeerConnection> connection;

            _lock.Lock();

            if (_socketServer.IsValid() == true) {
                connection = _socketServer->Find(remote);
            }

            _lock.Unlock();

            return (connection);
        }

    public:
        A2DP::AudioEndpoint* Endpoint(const uint8_t id)
        {
            A2DP::AudioEndpoint* result = nullptr;

            _lock.Lock();

            auto it = _endpoints.find(id);

            if (it != _endpoints.end()) {
                result = &((*it).second);
            }

            _lock.Unlock();

            return (result);
        }

    private:
        // SocketServer callbacks
        void Operational(const Core::ProxyType<PeerConnection>& channel, const bool isRunning)
        {
            ASSERT(channel.IsValid() == true);

            _observersLock.Lock();

            for (auto& observer : _observers) {
                observer->Operational(channel, isRunning);
            }

            _observersLock.Unlock();
        }
        void OnSignal(const Bluetooth::AVDTP::Signal& signal, const Bluetooth::AVDTP::Socket::ResponseHandler& handler)
        {
            Server::OnSignal(signal, handler);
        }
        void OnPacket(const uint8_t packet[], const uint16_t length)
        {
            ASSERT(_mediaReceiver != nullptr);

            _mediaReceiver->Packet(packet, length);
        }

    private:
        // AVDTP::Server overrides
        bool Visit(const uint8_t id, const std::function<void(Bluetooth::AVDTP::StreamEndPoint&)>& visitorCb) override
        {
            bool result = false;

            _lock.Lock();

            auto it = _endpoints.find(id);

            if (it != _endpoints.end()) {
                visitorCb((*it).second);
                result = true;
            }

            _lock.Unlock();

            return (result);
        }

    private:
        Core::ProxyType<SocketServer> _socketServer;
        Core::CriticalSection _lock;
        Core::CriticalSection _observersLock;
        std::map<uint8_t, A2DP::AudioEndpoint> _endpoints;
        std::list<IObserver*> _observers;
        IMediaReceiver* _mediaReceiver;
    }; // class SignallingServer

} // namespace Plugin

}
