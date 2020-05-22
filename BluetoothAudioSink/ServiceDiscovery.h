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

namespace WPEFramework {

namespace Implementation {

    class ServiceDiscovery : public Bluetooth::SDPSocket {
    private:
        static constexpr uint16_t DefaultMTU = 1024;
        static constexpr uint16_t OpenTimeout = 2000; // ms
        static constexpr uint16_t CloseTimeout = 5000;
        static constexpr uint16_t DiscoverTimeout = 5000;

    private:
        class SDPFlow {
        public:
            ~SDPFlow() = default;
            SDPFlow() = delete;
            SDPFlow(const SDPFlow&) = delete;
            SDPFlow& operator=(const SDPFlow&) = delete;
            SDPFlow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            explicit SDPFlow(const string& text)
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
        }; // class SDPFlow

    public:
        using ClassID = Bluetooth::SDPProfile::ClassID;

        class AudioService {
        private:
            enum attributeid : uint16_t {
                SupportedFeatures = 0x0311
            };

        public:
            enum type {
                UNKNOWN = 0,
                SOURCE  = 1,
                SINK    = 2,
                NEITHER = 3
            };

            enum features : uint16_t {
                NONE        = 0,
                HEADPHONE   = (1 << 1),
                SPEAKER     = (1 << 2),
                RECORDER    = (1 << 3),
                AMPLIFIER   = (1 << 4),
                PLAYER      = (1 << 5),
                MICROPHONE  = (1 << 6),
                TUNER       = (1 << 7),
                MIXER       = (1 << 8)
            };

        public:
            AudioService()
                : _l2capPsm(0)
                , _avdtpVersion(0)
                , _a2dpVersion(0)
                , _features(NONE)
                , _type(UNKNOWN)
            {
            }
            AudioService(const Bluetooth::SDPProfile::Service& service)
                : AudioService()
            {
                using SDPProfile = Bluetooth::SDPProfile;

                _type = NEITHER;

                const SDPProfile::ProfileDescriptor* a2dp = service.Profile(ClassID::AdvancedAudioDistribution);
                ASSERT(a2dp != nullptr);
                if (a2dp != nullptr) {
                    _a2dpVersion = a2dp->Version();
                    ASSERT(_a2dpVersion != 0);

                    const SDPProfile::ProtocolDescriptor* l2cap = service.Protocol(ClassID::L2CAP);
                    ASSERT(l2cap != nullptr);
                    if (l2cap != nullptr) {
                        SDPSocket::Payload params(l2cap->Parameters());
                        params.Pop(SDPSocket::use_descriptor, _l2capPsm);
                        ASSERT(_l2capPsm != 0);

                        const SDPProfile::ProtocolDescriptor* avdtp = service.Protocol(ClassID::AVDTP);
                        ASSERT(avdtp != nullptr);
                        if (avdtp != nullptr) {
                            SDPSocket::Payload params(avdtp->Parameters());
                            params.Pop(SDPSocket::use_descriptor, _avdtpVersion);
                            ASSERT(_avdtpVersion != 0);

                            // It's a A2DP service using L2CAP and AVDTP protocols; finally confirm its class ID
                            if (service.IsClassSupported(ClassID::AudioSink)) {
                                _type = SINK;
                            } else if (service.IsClassSupported(ClassID::AudioSource)) {
                                _type = SOURCE;
                            }

                            // This one is optional...
                            const SDPProfile::Service::AttributeDescriptor* supportedFeatures = service.Attribute(SupportedFeatures);
                            if (supportedFeatures != nullptr) {
                                SDPSocket::Payload value(supportedFeatures->Value());
                                value.Pop(SDPSocket::use_descriptor, _features);
                                if (service.IsClassSupported(ClassID::AudioSource)) {
                                    _features = static_cast<features>((static_cast<uint8_t>(_features) << 4));
                                }
                            }
                        }
                    }
                }
            }
            ~AudioService() = default;

        public:
            type Type() const
            {
                return (_type);
            }
            uint16_t PSM() const
            {
                return (_l2capPsm);
            }
            uint16_t TransportVersion() const
            {
                return (_avdtpVersion);
            }
            uint16_t ProfileVersion() const
            {
                return (_a2dpVersion);
            }
            features Features() const
            {
                return (_features);
            }

        private:
            Bluetooth::UUID _class;
            uint16_t _l2capPsm;
            uint16_t _avdtpVersion;
            uint16_t _a2dpVersion;
            features _features;
            type _type;
        }; // class AudioService

    public:
        using DiscoveryCompleteCb = std::function<void(const std::list<AudioService>&)>;

        ServiceDiscovery() = delete;
        ServiceDiscovery(const ServiceDiscovery&) = delete;
        ServiceDiscovery& operator=(const ServiceDiscovery&) = delete;

        ServiceDiscovery(const Core::NodeId& localNode, const Core::NodeId& remoteNode, const uint16_t mtu = DefaultMTU)
            : Bluetooth::SDPSocket(localNode, remoteNode, mtu)
            , _lock()
            , _profile(ClassID::AdvancedAudioDistribution)
        {
        }
        ~ServiceDiscovery() = default;

    private:
        bool Initialize() override
        {
            _lock.Lock();
            _audioServices.clear();
            _lock.Unlock();
            return (true);
        }

        void Operational() override
        {
            TRACE(SDPFlow, (_T("Bluetooth SDP connection is operational")));
        }

    public:
        uint32_t Connect()
        {
            uint32_t result = Open(OpenTimeout);
            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to open SDP socket [%d]"), result));
            } else {
                TRACE(SDPFlow, (_T("Successfully opened SDP socket")));
            }

            return (result);
        }
        uint32_t Disconnect()
        {
            uint32_t result = Core::ERROR_NONE;

            if (IsOpen() == true) {
                result = Close(CloseTimeout);
                if (result != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Failed to close SDP socket [%d]"), result));
                } else {
                    TRACE(SDPFlow, (_T("Successfully closed SDP socket")));
                }
            }

            return (result);
        }
        void Discover(const DiscoveryCompleteCb discoveryComplete)
        {
            _discoveryComplete = discoveryComplete;
            if (SDPSocket::IsOpen() == true) {
                _profile.Discover(DiscoverTimeout, *this, std::list<Bluetooth::UUID>{ ClassID::AudioSink }, [&](const uint32_t result) {
                    if (result == Core::ERROR_NONE) {
                        TRACE(SDPFlow, (_T("Service discovery complete")));

                        _lock.Lock();

                        DumpProfile();

                        if (_profile.Services().empty() == false) {
                            for (auto const& service : _profile.Services()) {
                                if (service.IsClassSupported(ClassID::AudioSink) == true) {
                                    _audioServices.emplace_back(service);
                                }
                            }

                            _discoveryComplete(_audioServices);
                        } else {
                            TRACE(Trace::Information, (_T("Not an A2DP audio sink device!")));
                        }

                        _lock.Unlock();
                    } else {
                        TRACE(Trace::Error, (_T("SDP service discovery failed [%d]"), result));
                    }
                });
            }
        }

    private:
        void DumpProfile() const;

    private:
        Core::CriticalSection _lock;
        Bluetooth::SDPProfile _profile;
        std::list<AudioService> _audioServices;
        DiscoveryCompleteCb _discoveryComplete;
    }; // class ServiceDiscovery

} // namespace Implementation

}
