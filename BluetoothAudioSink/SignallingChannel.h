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
#include "AudioEndpoint.h"

#include <interfaces/IBluetoothAudio.h>

namespace WPEFramework {

namespace A2DP {

    class SignallingChannel : public Bluetooth::AVDTPSocket {
    private:
        static constexpr uint16_t DefaultMTU = 672;
        static constexpr uint16_t OpenTimeout = 2000; // ms
        static constexpr uint16_t CloseTimeout = 5000;
        static constexpr uint16_t DiscoverTimeout = 2000;

    public:
        using DiscoveryCompleteCb = std::function<void(std::list<AudioEndpoint>&)>;
        using UpdatedCb = std::function<void()>;

        SignallingChannel() = delete;
        SignallingChannel(const SignallingChannel&) = delete;
        SignallingChannel& operator=(const SignallingChannel&) = delete;

        SignallingChannel(const uint8_t seid, const Core::NodeId& localNode, const Core::NodeId& remoteNode)
            : Bluetooth::AVDTPSocket(localNode, remoteNode)
            , _seid(seid)
            , _lock()
            , _profile()
            , _audioEndpoints()
            , _discoveryComplete()
        {
        }
        ~SignallingChannel() override
        {
            Disconnect();
        }

    private:
        uint32_t Initialize() override
        {
            return (Core::ERROR_NONE);
        }
        void Operational() override
        {
            TRACE(SignallingFlow, (_T("Bluetooth A2DP/AVDTP signalling channel is operational")));
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

                            _audioEndpoints.emplace_back(*this, sep, SEID());
                        }
                    }
                }

                if (_audioEndpoints.empty() == true) {
                    TRACE(SignallingFlow, (_T("No audio sink stream endpoints available")));
                }

                _discoveryComplete(_audioEndpoints);
            });
        }

    public:
        uint32_t Connect(const Core::NodeId& remoteNode)
        {
            RemoteNode(remoteNode);

            uint32_t result = Open(OpenTimeout);
            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to open A2DP/AVDTP signalling socket [%d]"), result));
            } else {
                TRACE(SignallingFlow, (_T("Successfully opened A2DP/AVDTP signalling socket")));
            }

            return (result);
        }
        uint32_t Disconnect()
        {
            uint32_t result = Core::ERROR_NONE;

            if (IsOpen() == true) {
                result = Close(CloseTimeout);
                if (result != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Failed to close A2DP/AVDTP signalling socket [%d]"), result));
                } else {
                    TRACE(SignallingFlow, (_T("Successfully closed A2DP/AVDTP signalling socket")));
                }
            }

             return (result);
        }

    private:
        void DumpProfile() const
        {
            TRACE(SignallingFlow, (_T("Discovered %d stream endpoints(s)"), _profile.StreamEndPoints().size()));

            uint16_t cnt = 1;
            for (auto const& sep : _profile.StreamEndPoints()) {
                TRACE(SignallingFlow, (_T("Stream endpoint #%i"), cnt++));
                TRACE(SignallingFlow, (_T("  SEID: 0x%02x"), sep.SEID()));
                TRACE(SignallingFlow, (_T("  Service Type: %s"), (sep.ServiceType() == Bluetooth::AVDTPProfile::StreamEndPoint::SINK? "Sink" : "Source")));
                TRACE(SignallingFlow, (_T("  Media Type: %s"), (sep.MediaType() == Bluetooth::AVDTPProfile::StreamEndPoint::AUDIO? "Audio"
                                                            : (sep.MediaType() == Bluetooth::AVDTPProfile::StreamEndPoint::VIDEO? "Video" : "Multimedia"))));

                if (sep.Capabilities().empty() == false) {
                    TRACE(SignallingFlow, (_T("  Capabilities:")));
                    for (auto const& caps : sep.Capabilities()) {
                        TRACE(SignallingFlow, (_T("    - %02x '%s', parameters[%d]: %s"), caps.first, caps.second.Name().c_str(),
                                            caps.second.Data().size(), Bluetooth::DataRecord(caps.second.Data()).ToString().c_str()));
                    }
                }
            }
        }

    private:
        uint8_t _seid;
        Core::CriticalSection _lock;
        Bluetooth::AVDTPProfile _profile;
        std::list<AudioEndpoint> _audioEndpoints;
        DiscoveryCompleteCb _discoveryComplete;
    }; // class SignallingChannel

} // namespace A2DP

}
