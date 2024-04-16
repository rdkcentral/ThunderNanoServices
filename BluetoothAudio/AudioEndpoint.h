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
#include "DebugTracing.h"
#include <bluetooth/audio/codecs/SBC.h>

namespace Thunder {

namespace Plugin {

namespace A2DP {

    inline Bluetooth::A2DP::IAudioCodec* AllocCodec(uint8_t codec, const Bluetooth::Buffer& params)
    {
        switch (codec) {
        case Bluetooth::A2DP::SBC::CODEC_TYPE:
            return (new Bluetooth::A2DP::SBC(params));
        default:
            break;
        }

        return (nullptr);
    }

    class AudioEndpointData : public Bluetooth::AVDTP::StreamEndPointData {
    public:
        AudioEndpointData() = delete;
        AudioEndpointData(const AudioEndpointData&) = delete;
        AudioEndpointData& operator=(const AudioEndpointData&) = delete;
        ~AudioEndpointData() override = default;

        template<typename SEP>
        AudioEndpointData(SEP&& sep)
            : Bluetooth::AVDTP::StreamEndPointData(std::forward<SEP>(sep))
            , _codec(nullptr)
            , _contentProtection(nullptr)
        {
            if (DeserializeTransportCapabilities() == true) {
                DeserializeCodecCapabilities();
                DeserializeCopyProtectionCapabilities();
            }

            if (_codec == nullptr) {
                TRACE(Trace::Error, (_T("Invalid endpoint")));
            }
        }

    public:
        const Bluetooth::A2DP::IAudioCodec* Codec() const {
            return (_codec.get());
        }
        Bluetooth::A2DP::IAudioCodec* Codec(){
            return (_codec.get());
        }
        const Bluetooth::A2DP::IAudioContentProtection* ContentProtection() const {
            return (_contentProtection.get());
        }
        Bluetooth::A2DP::IAudioContentProtection* ContentProtection() {
            return (_contentProtection.get());
        }

    private:
        bool DeserializeTransportCapabilities()
        {
            bool result = true;

            auto it = Capabilities().find(Service::MEDIA_TRANSPORT);

            if (it == Capabilities().end()) {
                TRACE(ServerFlow, (_T("Endpoint %02x does not support MEDIA_TRANSPORT category!"), Id()));
                result = false;
            }

            return (result);
        }
        void DeserializeCodecCapabilities()
        {
            using Service = Bluetooth::AVDTP::StreamEndPoint::Service;

            ASSERT(_codec == nullptr);

            auto it = Capabilities().find(Service::MEDIA_CODEC);

            if (it == Capabilities().end()) {
                TRACE(ServerFlow, (_T("Endpoint 0x%02x does not support MEDIA_CODEC category!"), Id()));
            }
            else {
                const Service& caps = (*it).second;

                if (caps.Params().size() >= 2) {
                    Bluetooth::DataRecord config(caps.Params());
                    uint8_t mediaType{};
                    config.Pop(mediaType);

                    if (mediaType == Bluetooth::A2DP::IAudioCodec::MEDIA_TYPE) {
                        uint8_t codecType{};
                        config.Pop(codecType);

                        _codec.reset(AllocCodec(codecType, caps.Params()));
                        ASSERT(_codec.get() != nullptr);

                        if (_codec == nullptr) {
                            TRACE(ServerFlow, (_T("Endpoint 0x%02x supports an unknown audio codec [0x%02x]"),
                                                    Id(), codecType));
                        }
                    }
                    else {
                        TRACE(ServerFlow, (_T("Endpoint media type is not AUDIO! [0x%02x]"), mediaType));
                    }
                }
            }
        }
        void DeserializeCopyProtectionCapabilities()
        {
            using Service = Bluetooth::AVDTP::StreamEndPoint::Service;

            enum a2dp_contentprotection : uint16_t {
                NONE    = 0x0000,
                DTCP    = 0x0001,
                SCMS_T  = 0x0002
            };

            ASSERT(_contentProtection == nullptr);

            // Optional content protection capability
            if (_codec != nullptr) {
                auto it = Capabilities().find(Service::CONTENT_PROTECTION);

                if (it == Capabilities().end()) {
                    TRACE(ServerFlow, (_T("Endpoint 0x%02x does not support CONTENT_PROTECTION category"), Id()));
                }
                else {
                    const Service& caps = (*it).second;

                    if (caps.Params().size() >= 2) {
                        Bluetooth::DataRecordLE config(caps.Params());

                        a2dp_contentprotection cpType{};
                        config.Pop(cpType);

                        if (cpType == a2dp_contentprotection::NONE) {
                            TRACE(ServerFlow, (_T("Endpoint 0x%02x uses no copy protection"), Id()));
                        }
                        else {
                            //
                            // TODO Add support for copy protection?
                            ///
                            TRACE(ServerFlow, (_T("Endpoint 0x%02x supports unknown copy protection! [0x%04x]"),
                                                    Id(), static_cast<uint8_t>(cpType)));
                        }
                    }
                }
            }
        }

    private:
        std::unique_ptr<Bluetooth::A2DP::IAudioCodec> _codec;
        std::unique_ptr<Bluetooth::A2DP::IAudioContentProtection> _contentProtection;
    };

    class AudioEndpoint : public Bluetooth::AVDTP::StreamEndPoint {
    public:
        using Service = Bluetooth::AVDTP::StreamEndPoint::Service;

        struct IHandler {
            virtual ~IHandler() { }

            virtual uint32_t OnSetConfiguration(AudioEndpoint& endpoint, Bluetooth::AVDTP::Socket* channel) = 0;
            virtual uint32_t OnAcquire() = 0;
            virtual uint32_t OnRelinquish() = 0;
            virtual uint32_t OnSetSpeed(const int8_t speed) = 0;
        };

    public:
        AudioEndpoint() = delete;
        AudioEndpoint(const AudioEndpoint&) = delete;
        AudioEndpoint& operator=(const AudioEndpoint&) = delete;
        ~AudioEndpoint() override = default;

        AudioEndpoint(IHandler& handler, const uint8_t id, const bool sink,
                      Bluetooth::A2DP::IAudioCodec* codec,
                      Bluetooth::A2DP::IAudioContentProtection* cp = nullptr)

            : Bluetooth::AVDTP::StreamEndPoint(id, (sink? Bluetooth::AVDTP::StreamEndPoint::SINK : Bluetooth::AVDTP::StreamEndPoint::SOURCE), Bluetooth::AVDTP::StreamEndPoint::AUDIO)
            , _lock()
            , _handler(&handler)
            , _remote(nullptr)
            , _codec(codec)
            , _contentProtection(cp)
            , _contentProtectionEnabled(false)
        {
            ASSERT(_handler != nullptr);
            ASSERT(_codec != nullptr);
            ASSERT(id != 0);

            // An audio endpoint is reqired to have MediaTransport and MediaCodec capabilities.
            Add(StreamEndPoint::capability, Service::MEDIA_TRANSPORT);
            Add(StreamEndPoint::capability, Service::MEDIA_CODEC, CodecCapabilities());

            if (_contentProtection != nullptr) {
                // Optionally...
                Add(StreamEndPoint::capability, Service::CONTENT_PROTECTION, ContentProtectionCapabilities());
            }
        }

    public:
        const Bluetooth::A2DP::IAudioCodec* Codec() const {
            return (_codec.get());
        }
        Bluetooth::A2DP::IAudioCodec* Codec(){
            return (_codec.get());
        }
        const Bluetooth::A2DP::IAudioContentProtection* ContentProtection() const {
            return (_contentProtection.get());
        }
        Bluetooth::A2DP::IAudioContentProtection* ContentProtection() {
            return (_contentProtection.get());
        }

    public:
        uint32_t Configure(const Bluetooth::A2DP::IAudioCodec::StreamFormat& format, const string& settings, const bool enableCP = false)
        {
            using Service = Bluetooth::AVDTP::StreamEndPoint::Service;

            ASSERT(_handler != nullptr);

            TRACE(ServerFlow, (_T("Configuring endpoint %d by client"), Id()));

            _lock.Lock();

            // Configure Media Transport
            Add(Service::MEDIA_TRANSPORT);

            // Configure codec
            if (_codec != nullptr) {
                if (_codec->Configure(format, settings) == Core::ERROR_NONE) {
                    uint8_t config[16];
                    const uint16_t length = _codec->Serialize(false, config, sizeof(config));

                    Add(Service::MEDIA_CODEC, Bluetooth::Buffer(config, length));
                }
            }

            // Configure content protection if enabled and supported
            if ((_contentProtection != nullptr) && (enableCP == true)) {
                if (_contentProtection->Configure(settings) == Core::ERROR_NONE) {
                    uint8_t config[16];
                    uint8_t length = _contentProtection->Serialize(false, config, sizeof(config));

                    Add(Service::CONTENT_PROTECTION, Bluetooth::Buffer(config, length));
                    _contentProtectionEnabled = true;
                }
            }
            else {
                _contentProtectionEnabled = false;
            }

            uint32_t result = _remote.SetConfiguration(*this, RemoteId());

            if (result == Core::ERROR_NONE) {
                uint8_t failed{};
                result = OnSetConfiguration(failed, nullptr);
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Start()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            TRACE(ServerFlow, (_T("Starting endpoint %d by client"), Id()));

            _lock.Lock();

            if (State() == StreamEndPoint::OPENED) {

                result = _remote.Start(*this);

                if (result == Core::ERROR_NONE) {
                    result = OnStart();
                }
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Suspend()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            TRACE(ServerFlow, (_T("Suspending endpoint %d by client"), Id()));

            if (State() == StreamEndPoint::STARTED) {

                result = _remote.Suspend(*this);

                if (result == Core::ERROR_NONE) {
                    result = OnSuspend();
                }
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Open()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            TRACE(ServerFlow, (_T("Opening endpoint %d by client"), Id()));

            _lock.Lock();

            if (State() == StreamEndPoint::CONFIGURED) {

                result = _remote.Open(*this);

                if (result == Core::ERROR_NONE) {
                    result = OnOpen();
                }
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Close()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            TRACE(ServerFlow, (_T("Closing endpoint %d by client"), Id()));

            _lock.Lock();

            if ((State() == StreamEndPoint::OPENED) || (State() == StreamEndPoint::STARTED)) {

                result = _remote.Close(*this);

                if (result == Core::ERROR_NONE) {
                    result = OnClose();
                }
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Abort()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            TRACE(ServerFlow, (_T("Aborting endpoint %d by client"), Id()));

            _lock.Lock();

            if ((State() == StreamEndPoint::CONFIGURED) || (State() == StreamEndPoint::OPENED) || (State() == StreamEndPoint::STARTED)) {

                result = _remote.Abort(*this);
                if (result == Core::ERROR_NONE) {
                    result = OnAbort();
                }
            }

            _lock.Unlock();

            return (result);
        }
        void Disconnected()
        {
            _lock.Lock();

            if ((State() != Bluetooth::AVDTP::StreamEndPoint::IDLE)
                    && (State() != Bluetooth::AVDTP::StreamEndPoint::CLOSING)
                    && (State() != Bluetooth::AVDTP::StreamEndPoint::ABORTING)) {

                TRACE(ServerFlow, (_T("Endpoint %d device disconnected unexpectedly!"), Id()));

                // Deallocate the sink and teardown internal data, no point in closing the remote
                // device as it's not longer reachable.
                OnAbort();
            }

            _lock.Unlock();

        }
        void ClientChannel(Bluetooth::AVDTP::Socket* channel)
        {
            _remote.Channel(channel);
        }

    private:
        // AVDTP::IStreamEndPointControl overrides
        uint32_t OnSetConfiguration(uint8_t& failedCategory, Bluetooth::AVDTP::Socket* channel) override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            failedCategory = 0;

            _lock.Lock();

            if (State() == StreamEndPoint::IDLE) {
                result = Core::ERROR_BAD_REQUEST;

                auto const& it = Configuration().find(Service::MEDIA_TRANSPORT);

                if (it == Configuration().end()) {
                    TRACE(ServerFlow, (_T("Configuration request is missing MEDIA_TRANSPORT category")));
                }
                else {
                    auto const& it = Configuration().find(Service::MEDIA_CODEC);

                    if (it == Configuration().end()) {
                        TRACE(ServerFlow, (_T("Configuration request is missing MEDIA_CODEC category")));
                    }
                    else {
                        const Bluetooth::Buffer& config = (*it).second.Params();

                        if (Codec()->Configure(config.data(), config.size()) == Core::ERROR_NONE) {

                            if (_handler->OnSetConfiguration(*this, channel) == Core::ERROR_NONE) {

                                TRACE(ServerFlow, (_T("Endpoint %d configured (with codec %02x)"), Id(), Codec()->Type()));

                                State(StreamEndPoint::CONFIGURED);

                                result = Core::ERROR_NONE;
                            }
                        }
                        else {
                            failedCategory = Service::MEDIA_CODEC;

                            TRACE(ServerFlow, (_T("Requested MEDIA_CODEC configuration is unsupported or invalid for codec %02x"), Codec()->Type()));
                        }
                    }
                }
            }

            _lock.Unlock();

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to configure endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }
        uint32_t OnReconfigure(uint8_t& failedCategory) override
        {
            return (OnSetConfiguration(failedCategory, nullptr));
        }
        uint32_t OnOpen() override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (State() == StreamEndPoint::CONFIGURED) {
                result = _handler->OnAcquire();

                if (result == Core::ERROR_NONE) {
                    TRACE(ServerFlow, (_T("Endpoint %d opened"), Id()));
                    State(StreamEndPoint::OPENED);
                }
            }

            _lock.Unlock();

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to open endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }
        uint32_t OnClose() override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if ((State() == StreamEndPoint::OPENED) || (State() == StreamEndPoint::STARTED)) {

                State(StreamEndPoint::CLOSING);

                result = _handler->OnRelinquish();

                TRACE(ServerFlow, (_T("Endpoint %d closed"), Id()));

                // Go back in state machine.
                State(StreamEndPoint::IDLE);
            }

            _lock.Unlock();

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to gracefully close endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }
        uint32_t OnStart() override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (State() == StreamEndPoint::OPENED) {
                result = _handler->OnSetSpeed(100);

                if (result == Core::ERROR_NONE) {
                    TRACE(ServerFlow, (_T("Endpoint %d started"), Id()));

                    State(StreamEndPoint::STARTED);
                }
            }

            _lock.Unlock();

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to start endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }
        uint32_t OnSuspend() override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (State() == StreamEndPoint::STARTED) {
                result = _handler->OnSetSpeed(0);

                if (result == Core::ERROR_NONE) {
                    TRACE(ServerFlow, (_T("Endpoint %d suspended"), Id()));

                    // Go back in state machine.
                    State(StreamEndPoint::OPENED);
                }
            }

            _lock.Unlock();

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to suspend endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }
        uint32_t OnAbort() override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if (State() != StreamEndPoint::IDLE) {
                State(StreamEndPoint::ABORTING);

                result = _handler->OnRelinquish();

                TRACE(ServerFlow, (_T("Endpoint %d aborted"), Id()));

                // Reset state machine.
                State(StreamEndPoint::IDLE);
            }

            _lock.Unlock();

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to gracefully abort endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }
        uint32_t OnSecurityControl() override
        {
            return (Core::ERROR_UNAVAILABLE);
        }

    private:
        Bluetooth::Buffer CodecCapabilities() const
        {
            // Convenience method to serialize codec capabilities frame.
            ASSERT(_codec != nullptr);

            uint8_t caps[16];
            const uint16_t length = Codec()->Serialize(true, caps, sizeof(caps));
            return (Bluetooth::Buffer(caps, length));
        }
        Bluetooth::Buffer ContentProtectionCapabilities() const
        {
            // Convenience method to serialize copy protection capabilities frame.
            ASSERT(_contentProtection != nullptr);

            uint8_t caps[16];
            const uint16_t length = ContentProtection()->Serialize(true, caps, sizeof(caps));
            return (Bluetooth::Buffer(caps, length));
        }

    public:
        Core::CriticalSection _lock;
        IHandler* _handler;
        Bluetooth::AVDTP::Client _remote;
        std::unique_ptr<Bluetooth::A2DP::IAudioCodec> _codec;
        std::unique_ptr<Bluetooth::A2DP::IAudioContentProtection> _contentProtection;
        bool _contentProtectionEnabled;
    }; // class AudioEndpoint

} // namespace A2DP

} // namespace Plugin

}