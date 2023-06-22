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

    class SignallingServer {
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
        struct IHandler {
            virtual ~IHandler() {}
            virtual bool Accept(Bluetooth::AVDTP::ServerSocket* channel) = 0;
            virtual void Operational(const bool running, Bluetooth::AVDTP::ServerSocket* channel) = 0;
            virtual void OnSignal(const Bluetooth::AVDTP::Signal& /* signal */, const Bluetooth::AVDTP::ServerSocket::ResponseHandler& /* handler */) { }
            virtual void OnPacket(const uint8_t* /* packet */, const uint16_t /* length */) { }
            virtual bool IsBusy() const = 0;
        };

    public:
        template<typename SERVER>
        class Channel : public Bluetooth::AVDTP::ServerSocket {
        public:
            Channel() = delete;
            Channel(const Channel&) = delete;
            Channel& operator=(const Channel&) = delete;
            ~Channel() = default;

            Channel(const SOCKET& connector, const Core::NodeId& remoteNode, Core::SocketServerType<Channel<SERVER>>* server)
                : Bluetooth::AVDTP::ServerSocket(connector, remoteNode)
                , _server(static_cast<SERVER*>(server))
                , _handler(nullptr)
                , _lastActivity(0)
            {
                ASSERT(server != nullptr);
            }
            Channel(const Core::NodeId& localNode, const Core::NodeId& remoteNode, IHandler* handler)
                : Bluetooth::AVDTP::ServerSocket(localNode, remoteNode)
                , _server(nullptr)
                , _handler(handler)
                , _lastActivity(0)
            {
                ASSERT(handler != nullptr);
            }

        public:
            uint64_t LastActivity() const {
                return (_lastActivity);
            }
            bool IsBusy() const {
                if (_handler != nullptr) {
                    // This is safe, once set handler always outlives this channel.
                    return (_handler->IsBusy());
                }
                else {
                    return (false);
                }
            }

        public:
            void Handler(IHandler* handler)
            {
                ASSERT(handler != nullptr);
                ASSERT(_handler == nullptr);

                _handler = handler;
            }

        private:
            void OnSignal(const Bluetooth::AVDTP::Signal& signal, const Bluetooth::AVDTP::ServerSocket::ResponseHandler& handler) override
            {
                TRACE(ServerFlow, (_T("Incoming AVDTP signal %d from remote client %s"), signal.Id(), RemoteNode().HostName().c_str()));

                ASSERT(_handler != nullptr);

                _handler->OnSignal(signal, handler);

                _lastActivity = Core::Time::Now().Ticks();
            }
            void OnPacket(const uint8_t packet[], const uint16_t length) override
            {
                ASSERT(_handler != nullptr);

                _handler->OnPacket(packet, length);

                _lastActivity = Core::Time::Now().Ticks();
            }

        private:
            void Operational(const bool running) override
            {
                if (running == true) {
                    _lastActivity = Core::Time::Now().Ticks();

                    TRACE(ServerFlow, (_T("Incoming AVDTP connection from %s"), RemoteNode().HostName().c_str()));
                }
                else {
                    TRACE(ServerFlow, (_T("Closed AVDTP connection to %s"), RemoteNode().HostName().c_str()));
                }

                if (_server != nullptr) {
                    _server->Connected(running, *this);
                }

                if (_handler != nullptr) {
                    _handler->Operational(running, static_cast<Bluetooth::AVDTP::ServerSocket*>(this));
                }
            }

        private:
            SERVER* _server;
            IHandler* _handler;
            mutable uint64_t _lastActivity;
        }; // class Channel

        class Implementation;
        using PeerConnection = Channel<Implementation>;

        class Implementation : public Core::SocketServerType<PeerConnection> {
        public:
            class Timer {
                // Timer used for disconnecting stale AVDTP connections.
            public:
                Timer() = delete;
                Timer(const Timer&) = delete;
                Timer(Timer&&) = default;
                Timer& operator=(const Timer&) = delete;
                ~Timer() = default;

                Timer(Implementation& server)
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
            static constexpr uint32_t OpenTimeoutMs = 2000;
            static constexpr uint32_t CloseTimeoutMs = 2000;
            static constexpr uint32_t ConnectionEvaluationPeriodMs = 100;

        public:
            Implementation(const Implementation&) = delete;
            Implementation& operator=(const Implementation&) = delete;

            Implementation(SignallingServer& parent, const uint32_t inactivityTimeoutMs)
                : Core::SocketServerType<Channel<Implementation>>()
                , _parent(parent)
                , _inactivityTimeoutUs(inactivityTimeoutMs * Core::Time::TicksPerMillisecond)
                , _connectionEvaluationTimer(Core::Thread::DefaultStackSize(), _T("SignallingConnectionChecker"))
            {
            }
            ~Implementation()
            {
                Stop();
            }

        public:
            void Connected(const bool running, PeerConnection& connection)
            {
                // Relay the notification up
                _parent.Connected(running, connection);
            }

        public:
            uint32_t Start(const uint16_t interface, const uint16_t psm)
            {
                uint32_t result = Core::ERROR_NONE;

                const Core::NodeId node(Bluetooth::Address(interface).NodeId(Bluetooth::Address::BREDR_ADDRESS, 0 /* cid */, psm));

                LocalNode(node);

                if (Open(OpenTimeoutMs) == Core::ERROR_NONE) {

                    if (_inactivityTimeoutUs != 0) {
                        Core::Time NextTick = Core::Time::Now();
                        NextTick.Add(ConnectionEvaluationPeriodMs);

                        _connectionEvaluationTimer.Schedule(NextTick.Ticks(), Timer(*this));
                    }
                }
                else {
                    result = Core::ERROR_GENERAL;
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
            uint64_t EvaluateConnections(const uint64_t timeNow)
            {
                // This will close and destroy inactive connections.

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
        }; // Class Implementation

    public:
        SignallingServer(const SignallingServer&) = delete;
        SignallingServer& operator=(const SignallingServer&) = delete;

    private:
        // The signalling server is shared between sink and source functionality.
        // While it is the core component of source, it may be also needed for sink,
        // whenever the sink device initiates the connection (this is optionally allowed by the spec).
        SignallingServer()
            : _server()
            , _lock()
            , _sink(nullptr)
            , _source(nullptr)
            , _peerConnection()
        {
        }

        ~SignallingServer()
        {
            Stop();
        }

    public:
        static SignallingServer& Instance()
        {
            static SignallingServer server;
            return (server);
        }

    public:
        void Start(const uint16_t hciInterface, const uint8_t psm, const uint32_t inactivityTimeoutMs)
        {
            _lock.Lock();

            TRACE(ServerFlow, (_T("Starting AVDTP server on HCI interface %d (PSM %d), timeout %d ms"), hciInterface, psm, inactivityTimeoutMs));

            if (_server.IsValid() == false) {
                _server = Core::ProxyType<Implementation>::Create(*this, inactivityTimeoutMs);

                if (_server.IsValid() == true) {
                    if (_server->Start(hciInterface, psm) != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to start the signalling server")));

                        _server.Release();
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

            if (_server.IsValid() == true) {
                _server->Stop();
                _server.Release();

                TRACE(Trace::Information, (_T("Stopped AVDTP Server")));
            }

            _lock.Unlock();
        }
        Bluetooth::AVDTP::ServerSocket* Create(const Core::NodeId& local, const Core::NodeId& remote, IHandler* handler)
        {
            Bluetooth::AVDTP::ServerSocket* conn = nullptr;

            _lock.Lock();

            if ((_server.IsValid() == true) && (_peerConnection == nullptr)) {
                _peerConnection.reset(new (std::nothrow) PeerConnection(local, remote, handler));
                ASSERT(_peerConnection.get() != nullptr);

                conn = _peerConnection.get();
            }

            _lock.Unlock();

            return (conn);
        }
        void Destroy(Bluetooth::AVDTP::ServerSocket* connection)
        {
            _lock.Lock();

            if ((_server.IsValid() == true) && (_peerConnection.get() == connection)) {
                _peerConnection.reset();
            }

            _lock.Unlock();
        }
        void Register(IHandler *handler, const bool sink = false)
        {
            _lock.Lock();

            if (sink == true) {
                TRACE(ServerFlow, (_T("Registered a sink handler")));
                ASSERT(_sink == nullptr);
                _sink = handler;
            }
            else {
                TRACE(ServerFlow, (_T("Registered a source handler")));
                ASSERT(_source == nullptr);
                _source = handler;
            }

            _lock.Unlock();
        }
        void Unregister(IHandler *handler)
        {
            _lock.Lock();

            if (handler == _sink) {
                _sink = nullptr;
            }
            else if (handler == _source) {
                _source = nullptr;
            }

            _lock.Unlock();
        }

    private:
        void Connected(const bool running, PeerConnection& channel)
        {
            _lock.Lock();

            if (running == true) {

                bool accepted = false;

                // The priority is for the sink since it may know the device already (i.e. the assigned device).

                if (_sink != nullptr) {
                    if (_sink->Accept(static_cast<Bluetooth::AVDTP::ServerSocket*>(&channel)) == true) {
                        TRACE(ServerFlow, (_T("AVDTP connection '%s' accepted by sink"), channel.RemoteNode().HostName().c_str()));
                        channel.Handler(_sink);
                        accepted = true;
                    }
                }

                if ((accepted == false) && (_source != nullptr)) {
                    if (_source->Accept(static_cast<Bluetooth::AVDTP::ServerSocket*>(&channel)) == true) {
                        TRACE(ServerFlow, (_T("AVDTP connection '%s' accepted by source"), channel.RemoteNode().HostName().c_str()));
                        channel.Handler(_source);
                        accepted = true;
                    }
                }

                if (accepted == false) {
                    TRACE(ServerFlow, (_T("AVDTP connection '%s' not accepted, closing"), channel.RemoteNode().HostName().c_str()));

                    //if (channel.Close(1000) != Core::ERROR_NONE) {
                    //    TRACE(Trace::Error, (_T("Failed to close connection")));
                   // }
                }
            }
            else {
                TRACE(ServerFlow, (_T("AVDTP connection '%s' closed"), channel.RemoteNode().HostName().c_str()));
            }

            _lock.Unlock();
        }

    private:
        Core::ProxyType<Implementation> _server;
        Core::CriticalSection _lock;
        IHandler* _sink;
        IHandler* _source;
        std::unique_ptr<PeerConnection> _peerConnection;
    };

} // namespace Plugin

}