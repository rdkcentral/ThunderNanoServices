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

#include "Module.h"
#include "AudioEndpoint.h"
#include "AudioCodec.h"
#include "AudioCodecSBC.h"

namespace WPEFramework {

namespace A2DP {

    void AudioEndpoint::ParseCapabilities(const Bluetooth::AVDTPProfile::StreamEndPoint& sep)
    {
        using ServiceCapabilities = Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities;

        enum a2dp_contentprotection : uint16_t {
            NONE    = 0x0000,
            DTC     = 0x0001,
            SCMS_T  = 0x0002
        };

        enum a2dp_mediatype : uint8_t {
            AUDIO           = IAudioCodec::MEDIA_TYPE
        };

        enum a2dp_audiocodec : uint8_t {
            SBC             = AudioCodecSBC::CODEC_TYPE, // mandatory support
            MPEG2_AUDIO     = 0x01,
            MPEG4_AAC       = 0x02,
            ATRAC_FAMILY    = 0x03,
            NON_A2DP        = 0xFF
        };

        // Need media transport capability
        auto mtIt = sep.Capabilities().find(ServiceCapabilities::MEDIA_TRANSPORT);
        if (mtIt != sep.Capabilities().end()) {

            // Need media codec capability
            auto mcIt = sep.Capabilities().find(ServiceCapabilities::MEDIA_CODEC);
            if (mcIt != sep.Capabilities().end()) {
                const ServiceCapabilities& caps = (*mcIt).second;
                if (caps.Data().size() >= 5) {
                    Bluetooth::DataRecord config(caps.Data());
                    a2dp_mediatype mediaType{};
                    config.Pop(mediaType);

                    if (mediaType == a2dp_mediatype::AUDIO) {
                        a2dp_audiocodec codecType{};
                        config.Pop(codecType);

                        if (codecType == a2dp_audiocodec::SBC) {
                            _codec = new AudioCodecSBC(caps.Data());
                            TRACE(Trace::Information, (_T("Endpoint SEID 0x%02x uses LC-SBC codec! [0x%02x]"), SEID()));
                        } else {
                            TRACE(Trace::Information, (_T("Endpoint SEID 0x%02x uses an unsupported audio codec! [0x%02x]"),
                                                       SEID(), static_cast<uint8_t>(codecType)));
                        }
                    } else {
                        TRACE(Trace::Information, (_T("Endpoint media type is not AUDIO! [0x%02x]"), static_cast<uint8_t>(mediaType)));
                    }
                }
            }

            // Optional content protection capability
            if (_codec != nullptr) {
                auto cpIt = sep.Capabilities().find(ServiceCapabilities::MEDIA_CODEC);
                if (cpIt != sep.Capabilities().end()) {
                    const ServiceCapabilities& caps = (*cpIt).second;
                    if (caps.Data().size() >= 2) {
                        Bluetooth::DataRecordLE caps(caps.Data());
                        a2dp_contentprotection cpType{};
                        caps.Pop(cpType);

                        // TODO...
                    }
                }
            }
        } else {
            TRACE(Trace::Information, (_T("Endpoint does not support media transport")));
        }
    }

    uint32_t AudioEndpoint::Configure(const string& format, const bool enableCP)
    {
        using ServiceCapabilities = Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities;

        Bluetooth::AVDTPSocket::SEPConfiguration configuration;

        // Configure Media Transport (always empty)
        configuration.emplace(ServiceCapabilities::MEDIA_TRANSPORT, Bluetooth::Buffer());

        // Configure Content Protection if enabled and supported
        _cpEnabled = enableCP;
        if ((_cp != nullptr) && (_cpEnabled == true)) {
            Bluetooth::Buffer cpConfig;
            _cp->SerializeConfiguration(cpConfig);
            configuration.emplace(ServiceCapabilities::CONTENT_PROTECTION, cpConfig);
        }

        // Configure Media Codec
        if (_codec != nullptr) {
            if (_codec->Configure(format) == Core::ERROR_NONE) {
                Bluetooth::Buffer mcConfig;
                _codec->SerializeConfiguration(mcConfig);
                configuration.emplace(ServiceCapabilities::MEDIA_CODEC, mcConfig);
            }
        }

        return (CmdSetConfiguration(configuration));
    }

} // namespace A2DP

}
