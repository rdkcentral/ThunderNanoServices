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

namespace WPEFramework {

namespace Plugin {

namespace A2DP {

    template<typename HANDLER, typename FLOW>
    class AudioEndpoint : public Bluetooth::AVDTP::StreamEndPoint {
    public:
        using Service = Bluetooth::AVDTP::StreamEndPoint::Service;

    public:
        AudioEndpoint() = delete;
        AudioEndpoint(const AudioEndpoint&) = delete;
        AudioEndpoint& operator=(const AudioEndpoint&) = delete;
        ~AudioEndpoint() override = default;

        // Initialize endpoint from client
        template<typename SEP>
        AudioEndpoint(HANDLER& handler, const uint8_t intSeid, SEP&& sep)
            : Bluetooth::AVDTP::StreamEndPoint(std::forward<SEP>(sep))
            , _handler(handler)
            , _codec(nullptr)
            , _contentProtection(nullptr)
            , _contentProtectionEnabled(false)
            , _intSeid(intSeid)
        {
            if (DeserializeTransportCapabilities() == true) {
                DeserializeCodecCapabilities();
                DeserializeCopyProtectionCapabilities();
            }

            if (_codec == nullptr) {
                TRACE(Trace::Error, (_T("Invalid endpoint")));
            }
        }

        // Initialize endpoint from server
        AudioEndpoint(HANDLER& handler, const uint8_t id,
                        const bool sink,
                        Bluetooth::A2DP::IAudioCodec* codec,
                        Bluetooth::A2DP::IAudioContentProtection* cp = nullptr)

            : Bluetooth::AVDTP::StreamEndPoint(id, (sink? Bluetooth::AVDTP::StreamEndPoint::SINK : Bluetooth::AVDTP::StreamEndPoint::SOURCE), Bluetooth::AVDTP::StreamEndPoint::AUDIO)
            , _handler(handler)
            , _codec(codec)
            , _contentProtection(cp)
            , _contentProtectionEnabled(false)
            , _intSeid(0)
        {
            ASSERT(_codec != nullptr);
            ASSERT(id != 0);

            // An audio endpoint is reqired to have MediaTransport and MediaCodec capabilities.
            Add(StreamEndPoint::capability, Service::MEDIA_TRANSPORT);
            Add(StreamEndPoint::capability, Service::MEDIA_CODEC, CodecCapabilities());

            if (_contentProtection != nullptr) {
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

            const uint32_t result = (_handler.SetConfiguration(*this, _intSeid));

            if (result == Core::ERROR_NONE) {
                State(StreamEndPoint::CONFIGURED);
            }

            return (result);
        }
        uint32_t Start() override
        {
            const uint32_t result = _handler.Start(*this);

            if (result== Core::ERROR_NONE) {
                State(StreamEndPoint::STARTED);
            }

            return (result);
        }
        uint32_t Suspend() override
        {
            const uint32_t result = _handler.Suspend(*this);

            if (result == Core::ERROR_NONE) {
                State(StreamEndPoint::OPENED);
            }

            return (result);
        }
        uint32_t Open() override
        {
            const uint32_t result = _handler.Open(*this);

            if (result == Core::ERROR_NONE) {
                State(StreamEndPoint::OPENED);
            }

            return (result);
        }
        uint32_t Close() override
        {
            const uint32_t result = _handler.Close(*this);

            if (result == Core::ERROR_NONE) {
                State(StreamEndPoint::IDLE);
            }

            return (result);
        }

    public:
        // AVDTP::StreamEndPoint overrides
        uint32_t OnConfigure(const uint8_t intSeid, uint8_t& failedCategory) override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            failedCategory = 0;

            if (State() == StreamEndPoint::IDLE) {
                result = Core::ERROR_BAD_REQUEST;

                auto const& it = Configuration().find(Service::MEDIA_TRANSPORT);

                if (it == Configuration().end()) {
                    TRACE(FLOW, (_T("Configuration request is missing MEDIA_TRANSPORT category")));
                }
                else {
                    auto const& it = Configuration().find(Service::MEDIA_CODEC);

                    if (it == Configuration().end()) {
                        TRACE(FLOW, (_T("Configuration request is missing MEDIA_CODEC category")));
                    }
                    else {
                        const Bluetooth::Buffer& config = (*it).second.Params();

                        if (Codec()->Configure(config.data(), config.size()) == Core::ERROR_NONE) {
                            _intSeid = intSeid;

                            if (_handler.OnSetConfiguration(*this) == Core::ERROR_NONE) {

                                TRACE(FLOW, (_T("Endpoint %d configured (with codec %02x, initiator SEID %02x)"),
                                                        Id(), Codec()->Type(), intSeid));

                                State(StreamEndPoint::CONFIGURED);
                                result = Core::ERROR_NONE;
                            }
                        }
                        else {
                            failedCategory = Service::MEDIA_CODEC;

                            TRACE(FLOW, (_T("Requested MEDIA_CODEC configuration is unsupported or invalid for codec %02x"),
                                                        Codec()->Type()));
                        }
                    }
                }
            }

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to configure endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }
        uint32_t OnOpen() override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            if (State() == StreamEndPoint::CONFIGURED) {
                if (_handler.OnAcquire(*this) == Core::ERROR_NONE) {
                    TRACE(FLOW, (_T("Endpoint %d opened"), Id()));
                    State(StreamEndPoint::OPENED);
                }

                result = Core::ERROR_NONE;
            }

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to open endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }
        uint32_t OnClose() override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            if ((State() == StreamEndPoint::OPENED) || (State() == StreamEndPoint::STARTED)) {

                State(StreamEndPoint::CLOSING);

                result = _handler.OnRelinquish(*this);

                if (result == Core::ERROR_NONE) {
                    TRACE(FLOW, (_T("Endpoint %d closed"), Id()));

                    // Go back in state machine.
                    State(StreamEndPoint::IDLE);
                }
            }

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to close endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }
        uint32_t OnStart() override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            if (State() == StreamEndPoint::OPENED) {
                result = _handler.OnSetSpeed(*this, 100);

                if (result == Core::ERROR_NONE) {
                    TRACE(FLOW, (_T("Endpoint %d started"), Id()));
                    State(StreamEndPoint::STARTED);
                }
            }

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to start endpoint %d [%d]"), Id(), result));
            }
            return (result);
        }
        uint32_t OnSuspend() override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            if (State() == StreamEndPoint::STARTED) {
                result = _handler.OnSetSpeed(*this, 0);

                if (result == Core::ERROR_NONE) {
                    TRACE(FLOW, (_T("Endpoint %d suspended"), Id()));

                    // Go back in state machine.
                    State(StreamEndPoint::OPENED);
                }
            }

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to suspend endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }
        uint32_t OnAbort() override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            if (State() != StreamEndPoint::IDLE) {
                State(StreamEndPoint::ABORTING);

                _handler.OnRelinquish(*this);

                TRACE(FLOW, (_T("Endpoint %d aborted"), Id()));

                // Reset state machine.
                State(StreamEndPoint::IDLE);
                result = Core::ERROR_NONE;
            }

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to abort endpoint %d [%d]"), Id(), result));
            }

            return (result);
        }

    public:
        void Disconnected()
        {
            TRACE(FLOW, (_T("Endpoint %d device disconnected"), Id()));

            // Deallocate the sink and teardown internal data, no point in closing the remote
            // device as it's not longer reachable.

            _handler.OnRelinquish(*this);

            // Reset state machine.
            State(StreamEndPoint::IDLE);
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

    private:
        bool DeserializeTransportCapabilities()
        {
            bool result = true;

            auto it = Capabilities().find(Service::MEDIA_TRANSPORT);

            if (it == Capabilities().end()) {
                TRACE(FLOW, (_T("Endpoint %02x does not support MEDIA_TRANSPORT category!"), Id()));
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
                TRACE(FLOW, (_T("Endpoint 0x%02x does not support MEDIA_CODEC category!"), Id()));
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

                        _codec.reset(_handler.Codec(codecType, caps.Params()));
                        ASSERT(_codec.get() != nullptr);

                        if (_codec == nullptr) {
                            TRACE(FLOW, (_T("Endpoint 0x%02x supports an unknown audio codec [0x%02x]"),
                                                    Id(), codecType));
                        }
                    }
                    else {
                        TRACE(FLOW, (_T("Endpoint media type is not AUDIO! [0x%02x]"), mediaType));
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
                    TRACE(FLOW, (_T("Endpoint 0x%02x does not support CONTENT_PROTECTION category"), Id()));
                }
                else {
                    const Service& caps = (*it).second;

                    if (caps.Params().size() >= 2) {
                        Bluetooth::DataRecordLE config(caps.Params());

                        a2dp_contentprotection cpType{};
                        config.Pop(cpType);

                        if (cpType == a2dp_contentprotection::NONE) {
                            TRACE(FLOW, (_T("Endpoint 0x%02x uses no copy protection"), Id()));
                        }
                        else {
                            //
                            // TODO Add support for copy protection?
                            ///
                            TRACE(FLOW, (_T("Endpoint 0x%02x supports unknown copy protection! [0x%04x]"),
                                                    Id(), static_cast<uint8_t>(cpType)));
                        }
                    }
                }
            }
        }

    public:
        HANDLER& _handler;
        std::unique_ptr<Bluetooth::A2DP::IAudioCodec> _codec;
        std::unique_ptr<Bluetooth::A2DP::IAudioContentProtection> _contentProtection;
        bool _contentProtectionEnabled;
        uint8_t _intSeid;
    }; // class AudioEndpoint

} // namespace A2DP

} // namespace Plugin

}