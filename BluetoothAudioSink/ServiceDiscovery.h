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

namespace A2DP {

    class ServiceDiscovery : public Bluetooth::SDP::ClientSocket {
    private:
        static constexpr uint16_t OpenTimeout = 2000; // ms
        static constexpr uint16_t CloseTimeout = 5000;
        static constexpr uint16_t DiscoverTimeout = 5000;

    private:
        class DiscoveryFlow {
        public:
            ~DiscoveryFlow() = default;
            DiscoveryFlow() = delete;
            DiscoveryFlow(const DiscoveryFlow&) = delete;
            DiscoveryFlow& operator=(const DiscoveryFlow&) = delete;
            DiscoveryFlow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            explicit DiscoveryFlow(const string& text)
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
        }; // class DiscoveryFlow

    public:
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
                OTHER   = 3
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
            class FeaturesDescriptor : public Bluetooth::SDP::Service::Data::Element<uint16_t> {
            public:
                using Element::Element;

            public:
                features Features(const type devType) const
                {
                    if (devType == SINK) {
                        return (static_cast<features>(Value()));
                    } else {
                        return (static_cast<features>((static_cast<uint8_t>(Value()) << 4)));
                    }
                }
            }; // class FeaturesDescriptor

        public:
            AudioService()
                : _l2capPsm(0)
                , _avdtpVersion(0)
                , _a2dpVersion(0)
                , _features(NONE)
                , _type(UNKNOWN)
            {
            }

            AudioService(const AudioService&) = default;
            AudioService& operator=(const AudioService&) = default;

            AudioService(const Bluetooth::SDP::Service& service)
                : AudioService()
            {
                namespace SDP = Bluetooth::SDP;

                _type = OTHER;

                const auto* a2dpData = service.Profile(SDP::ClassID::AdvancedAudioDistribution);
                if (a2dpData != nullptr) {
                    _a2dpVersion = a2dpData->Version();

                    const auto* l2capData = service.Protocol(SDP::ClassID::L2CAP);
                    if (l2capData != nullptr) {
                        _l2capPsm = SDP::Service::Protocol::L2CAP(*l2capData).PSM();

                        const auto* avdtpData = service.Protocol(SDP::ClassID::AVDTP);
                        if (avdtpData != nullptr) {
                            _avdtpVersion = SDP::Service::Protocol::AVDTP(*avdtpData).Version();

                            if (service.HasClassID(SDP::ClassID::AudioSink)) {
                                _type = SINK;
                            } else if (service.HasClassID(SDP::ClassID::AudioSource)) {
                                _type = SOURCE;
                            }

                            // This one is optional...
                            const auto* supportedFeaturesData = service.Attribute(attributeid::SupportedFeatures);
                            if (supportedFeaturesData != nullptr) {
                                _features = FeaturesDescriptor(*supportedFeaturesData).Features(_type);
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

        ServiceDiscovery(const Core::NodeId& localNode, const Core::NodeId& remoteNode)
            : Bluetooth::SDP::ClientSocket(localNode, remoteNode)
            , _lock()
            , _profile()
        {
        }
        ~ServiceDiscovery() = default;

    private:
        uint32_t Initialize() override
        {
            _lock.Lock();
            _audioServices.clear();
            _lock.Unlock();
            return (Core::ERROR_NONE);
        }

        void Operational() override
        {
            TRACE(DiscoveryFlow, (_T("Bluetooth A2DP/SDP connection is operational")));
        }

    public:
        uint32_t Connect()
        {
            uint32_t result = Open(OpenTimeout);
            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to open A2DP/SDP socket [%d]"), result));
            } else {
                TRACE(DiscoveryFlow, (_T("Successfully opened A2DP/SDP socket")));
            }

            return (result);
        }
        uint32_t Disconnect()
        {
            uint32_t result = Core::ERROR_NONE;

            if (IsOpen() == true) {
                result = Close(CloseTimeout);
                if (result != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Failed to close A2DP/SDP socket [%d]"), result));
                } else {
                    TRACE(DiscoveryFlow, (_T("Successfully closed A2DP/SDP socket")));
                }
            }

            return (result);
        }
        void Discover(const DiscoveryCompleteCb discoveryComplete)
        {
            _discoveryComplete = discoveryComplete;
            if (IsOpen() == true) {
                _profile.Discover(DiscoverTimeout, *this, std::list<Bluetooth::UUID>{ Bluetooth::SDP::ClassID::AudioSink }, [&](const uint32_t result) {
                    if (result == Core::ERROR_NONE) {
                        TRACE(DiscoveryFlow, (_T("Service discovery complete")));

                        _lock.Lock();

                        TRACE(DiscoveryFlow, (_T("Discovered %d service(s)"), _profile.Services().size()));
                        SDP::Dump<DiscoveryFlow>(_profile);

                        if (_profile.Services().empty() == false) {
                            for (auto const& service : _profile.Services()) {
                                if (service.HasClassID(Bluetooth::SDP::ClassID::AudioSink) == true) {
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
        Core::CriticalSection _lock;
        Bluetooth::SDP::Profile _profile;
        std::list<AudioService> _audioServices;
        DiscoveryCompleteCb _discoveryComplete;
    }; // class ServiceDiscovery

} // namespace A2DP



}
