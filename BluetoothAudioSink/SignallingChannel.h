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
#include "AudioCodec.h"
#include "AudioCodecSBC.h"
#include "AudioContentProtection.h"

#include <interfaces/IBluetoothAudio.h>
#include <interfaces/JBluetoothAudioSink.h>

namespace WPEFramework {

namespace A2DP {

    class SignallingChannel : public Bluetooth::AVDTPSocket {
    private:
        static constexpr uint16_t DefaultMTU = 672;
        static constexpr uint16_t OpenTimeout = 2000; // ms
        static constexpr uint16_t CloseTimeout = 5000;
        static constexpr uint16_t DiscoverTimeout = 2000;

    private:
        class SignallingFlow {
        public:
            ~SignallingFlow() = default;
            SignallingFlow() = delete;
            SignallingFlow(const SignallingFlow&) = delete;
            SignallingFlow& operator=(const SignallingFlow&) = delete;
            SignallingFlow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            explicit SignallingFlow(const string& text)
                : _text(Core::ToString(text))
            {
            }

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
        }; // class SignallingFlow

    public:
        class AudioEndpoint {
        private:
            static constexpr uint16_t CommandTimeout = 2000;

        public:
            using StatusNotifyFn = std::function<void(const Exchange::IBluetoothAudioSink::status)>;

        public:
            AudioEndpoint() = delete;
            AudioEndpoint& operator=(const AudioEndpoint&) = delete;

            AudioEndpoint(AVDTPSocket& socket, const Bluetooth::AVDTPProfile::StreamEndPoint& sep,
                          uint8_t sourceSEID, const StatusNotifyFn& notify)
                : _acpSeid(sep.SEID())
                , _intSeid(sourceSEID)
                , _socket(socket)
                , _command()
                , _notify(notify)
                , _codec(nullptr)
                , _cp(nullptr)
                , _cpEnabled(false)
            {
                ParseCapabilities(sep);
            }

            ~AudioEndpoint()
            {
                if (_cp != nullptr) {
                    delete _cp;
                }
                if (_codec != nullptr) {
                    delete _codec;
                }
            }

        public:
            uint8_t SEID() const
            {
                return (_acpSeid);
            }
            IAudioCodec* Codec() const
            {
                return (_codec);
            }
            IAudioContentProtection* ContentProtection() const
            {
                return (_cp);
            }
            void Configuration(IAudioCodec::Format& format, bool& cpEnabled)
            {
                ASSERT(_codec != nullptr);
                _codec->Configuration(format);
                cpEnabled = _cpEnabled;
            }

            void Configure(const IAudioCodec::Format& format, const bool enableCP = false);

            void Open()
            {
                CmdOpen();
            }
            void Close()
            {
                CmdClose();
            }
            void Start()
            {
                CmdStart();
            }
            void Stop()
            {
                CmdSuspend();
            }

        public:
            void CmdSetConfiguration(Bluetooth::AVDTPSocket::SEPConfiguration& config)
            {
                _command.SetConfiguration(SEID(), _intSeid, config);
                _socket.Execute(CommandTimeout, _command, [&](const AVDTPSocket::Command& cmd) {
                    if ((cmd.Status() == Core::ERROR_NONE) && (cmd.Result().Status() == AVDTPSocket::Command::Signal::SUCCESS)) {
                        // Read it back...
                        CmdGetConfiguration();
                    } else {
                        TRACE(Trace::Error, (_T("Failed to set endpoint configuration, SEID 0x%02x"), SEID()));
                    }
                });
            }
            void CmdGetConfiguration()
            {
                _command.GetConfiguration(SEID());
                _socket.Execute(CommandTimeout, _command, [&](const AVDTPSocket::Command& cmd) {
                    if ((cmd.Status() == Core::ERROR_NONE) && (cmd.Result().Status() == AVDTPSocket::Command::Signal::SUCCESS)) {
                        cmd.Result().ReadConfiguration([this](const uint8_t category, const Bluetooth::Buffer& data) {
                            switch(category) {
                            case Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::CONTENT_PROTECTION:
                                // TODO
                                break;
                            case Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::MEDIA_CODEC:
                                // TODO
                                break;
                            }
                        });

                        _notify(Exchange::IBluetoothAudioSink::CONFIGURED);
                    } else {
                        TRACE(Trace::Error, (_T("Failed to read endpoint configuration, SEID 0x%02x"), SEID()));
                    }
                });
            }
            void CmdOpen()
            {
                _command.Open(SEID());
                _socket.Execute(CommandTimeout, _command, [&](const AVDTPSocket::Command& cmd) {
                    if ((cmd.Status() == Core::ERROR_NONE) && (cmd.Result().Status() == AVDTPSocket::Command::Signal::SUCCESS)) {
                        _notify(Exchange::IBluetoothAudioSink::OPEN);
                    } else {
                        TRACE(Trace::Error, (_T("Failed to open endpoint, SEID 0x%02x"), SEID()));
                    }
                });
            }
            void CmdClose()
            {
                _command.Close(SEID());
                _socket.Execute(CommandTimeout, _command, [&](const AVDTPSocket::Command& cmd) {
                    if ((cmd.Status() == Core::ERROR_NONE) && (cmd.Result().Status() == AVDTPSocket::Command::Signal::SUCCESS)) {
                        _notify(Exchange::IBluetoothAudioSink::CONFIGURED);
                    } else {
                        TRACE(Trace::Error, (_T("Failed to close endpoint, SEID 0x%02x"), SEID()));
                    }
                });
            }
            void CmdStart()
            {
                _command.Start(SEID());
                _socket.Execute(CommandTimeout, _command, [&](const AVDTPSocket::Command& cmd) {
                    if ((cmd.Status() == Core::ERROR_NONE) && (cmd.Result().Status() == AVDTPSocket::Command::Signal::SUCCESS)) {
                        _notify(Exchange::IBluetoothAudioSink::STREAMING);
                    } else {
                        TRACE(Trace::Error, (_T("Failed to start endpoint, SEID 0x%02x"), SEID()));
                    }
                });
            }
            void CmdSuspend()
            {
                _command.Start(SEID());
                _socket.Execute(CommandTimeout, _command, [&](const AVDTPSocket::Command& cmd) {
                    if ((cmd.Status() == Core::ERROR_NONE) && (cmd.Result().Status() == AVDTPSocket::Command::Signal::SUCCESS)) {
                        _notify(Exchange::IBluetoothAudioSink::OPEN);
                    } else {
                        TRACE(Trace::Error, (_T("Failed to suspend endpoint, SEID 0x%02x"), SEID()));
                    }
                });
            }

        private:
            void ParseCapabilities(const Bluetooth::AVDTPProfile::StreamEndPoint& sep);

        private:
            uint8_t _acpSeid;
            uint8_t _intSeid;
            AVDTPSocket& _socket;
            AVDTPSocket::Command _command;
            StatusNotifyFn _notify;
            IAudioCodec* _codec;
            IAudioContentProtection* _cp;
            bool _cpEnabled;
        }; // class AudioEndpoint

    public:
        using DiscoveryCompleteCb = std::function<void(std::list<AudioEndpoint>&)>;
        using UpdatedCb = std::function<void()>;

        SignallingChannel() = delete;
        SignallingChannel(const SignallingChannel&) = delete;
        SignallingChannel& operator=(const SignallingChannel&) = delete;

        SignallingChannel(const uint8_t seid, const UpdatedCb updatedCb,
                          const Core::NodeId& localNode, const Core::NodeId& remoteNode, const uint16_t mtu = DefaultMTU)
            : Bluetooth::AVDTPSocket(localNode, remoteNode, mtu)
            , _updatedCb(updatedCb)
            , _seid(seid)
            , _lock()
            , _status(Exchange::IBluetoothAudioSink::UNASSIGNED)
            , _profile()
            , _audioEndpoints()
            , _discoveryComplete()
        {
            Status(Exchange::IBluetoothAudioSink::DISCONNECTED);
        }
        ~SignallingChannel() override
        {
            Disconnect();
        }

    private:
        bool Initialize() override
        {
            return (true);
        }
        void Operational() override
        {
            TRACE(SignallingFlow, (_T("Bluetooth AVDTP signalling channel is operational")));
            Status(Exchange::IBluetoothAudioSink::IDLE);
        }

    public:
        uint8_t SEID()
        {
            return (_seid);
        }
        void Discover(const DiscoveryCompleteCb& discoveryComplete)
        {
            _discoveryComplete = discoveryComplete;
            _profile.Discover(DiscoverTimeout, *this, [&](const uint32_t result) {
                if (result == Core::ERROR_NONE) {
                    TRACE(SignallingFlow, (_T("Stream endpoint discovery complete")));
                    DumpProfile();

                    for (auto const& sep : _profile.StreamEndPoints()) {
                        if ((sep.MediaType() == Bluetooth::AVDTPProfile::StreamEndPoint::AUDIO)
                                && (sep.ServiceType() == Bluetooth::AVDTPProfile::StreamEndPoint::SINK)) {

                            _audioEndpoints.emplace_back(*this, sep, SEID(), [this](const Exchange::IBluetoothAudioSink::status status) {
                                Status(status);
                            });
                        }
                    }

                    if (_audioEndpoints.empty() == true) {
                        TRACE(Trace::Information, (_T("No audio sink stream endpoints available")));
                    } else {
                        _discoveryComplete(_audioEndpoints);
                    }
                }
            });
        }

    public:
        uint32_t Connect(const Core::NodeId& remoteNode)
        {
            RemoteNode(remoteNode);

            uint32_t result = Open(OpenTimeout);
            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to open AVDTP signalling socket [%d]"), result));
            } else {
                TRACE(SignallingFlow, (_T("Successfully opened AVDTP signalling socket")));
            }

            return (result);
        }
        uint32_t Disconnect()
        {
            uint32_t result = Core::ERROR_NONE;

            if (IsOpen() == true) {
                result = Close(CloseTimeout);
                if (result != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Failed to close AVDTP signalling socket [%d]"), result));
                } else {
                    TRACE(SignallingFlow, (_T("Successfully closed AVDTP signalling socket")));
                }
            }

            Status(Exchange::IBluetoothAudioSink::DISCONNECTED);

            return (result);
        }
        Exchange::IBluetoothAudioSink::status Status() const
        {
            return (_status);
        }

    private:
        void Status(const Exchange::IBluetoothAudioSink::status newStatus)
        {
            _lock.Lock();

            if (_status != newStatus) {
                _status = newStatus;
                Core::EnumerateType<JsonData::BluetoothAudioSink::StatusType> value(static_cast<Exchange::IBluetoothAudioSink::status>(_status));
                TRACE(Trace::Information, (_T("Audio sink status: %s"), (value.IsSet()? value.Data() : "(undefined)")));
                _updatedCb();
            }

            _lock.Unlock();
        }
        void DumpProfile() const;

    private:
        UpdatedCb _updatedCb;
        uint8_t _seid;
        Core::CriticalSection _lock;
        Exchange::IBluetoothAudioSink::status _status;
        Bluetooth::AVDTPProfile _profile;
        std::list<AudioEndpoint> _audioEndpoints;
        DiscoveryCompleteCb _discoveryComplete;
    }; // class SignallingChannel

} // namespace A2DP

}
