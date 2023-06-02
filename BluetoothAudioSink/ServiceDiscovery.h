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
#include "Tracing.h"

namespace WPEFramework {

namespace A2DP {

    class ServiceDiscovery : public Bluetooth::SDP::ClientSocket {
    private:
        static constexpr uint16_t OpenTimeout = 500; // ms
        static constexpr uint16_t CloseTimeout = 5000;
        static constexpr uint16_t DiscoverTimeout = 5000;

    public:
        class AudioService {
        public:
            enum type : uint8_t {
                UNKNOWN,
                SINK
            };

            enum features : uint16_t {
                NONE        = 0,
                HEADPHONE   = (1 << 1),
                SPEAKER     = (1 << 2),
                RECORDER    = (1 << 3),
                AMPLIFIER   = (1 << 4)
            };

        public:
            class Data : public Core::JSON::Container {
            public:
                Data(const Data&) = delete;
                Data& operator=(const Data&) = delete;

                Data()
                    : Core::JSON::Container()
                    , PSM(0)
                    , A2DPVersion(0)
                    , AVDTPVersion(0)
                    , Features(0)
                {
                    Add(_T("psm"), &PSM);
                    Add(_T("a2dp"), &A2DPVersion);
                    Add(_T("avdtp"), &AVDTPVersion);
                    Add(_T("features"), &Features);
                }

                ~Data() = default;

            public:
                Core::JSON::DecUInt16 PSM;
                Core::JSON::HexUInt16 A2DPVersion;
                Core::JSON::HexUInt16 AVDTPVersion;
                Core::JSON::DecUInt16 Features;
            };

        public:
            class FeaturesDescriptor : public Bluetooth::SDP::Service::Data::Element<uint16_t> {
            public:
                using Element::Element;

            public:
                features Features() const
                {
                    return (static_cast<features>(Value()));
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

                                // This one is optional...
                                const auto* supportedFeaturesData = service.Attribute(Bluetooth::SDP::Service::AttributeDescriptor::a2dp::SupportedFeatures);
                                if (supportedFeaturesData != nullptr) {
                                    _features = FeaturesDescriptor(*supportedFeaturesData).Features();
                                }
                            }
                        }
                    }
                }
            }

            AudioService(const Data& data)
                : AudioService()
            {
                _type = SINK;
                _l2capPsm = data.PSM.Value();
                _a2dpVersion = data.A2DPVersion.Value();
                _avdtpVersion = data.AVDTPVersion.Value();
                _features = static_cast<features>(data.Features.Value());
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
            void Settings(Data& data) const
            {
                data.PSM = _l2capPsm;
                data.A2DPVersion = _a2dpVersion;
                data.AVDTPVersion = _avdtpVersion;
                data.Features = _avdtpVersion;
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
        ~ServiceDiscovery()
        {
            Disconnect();
        }

    private:
        uint32_t Initialize() override
        {
            _lock.Lock();
            _audioServices.clear();
            _lock.Unlock();
            return (Core::ERROR_NONE);
        }

        void Operational(const bool upAndRunning) override
        {
            TRACE(DiscoveryFlow, (_T("Bluetooth A2DP/SDP connection is now %s"), upAndRunning? "open" : "closed"));
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
                        Dump<DiscoveryFlow>(_profile);

                        if (_profile.Services().empty() == false) {
                            for (auto const& service : _profile.Services()) {
                                if (service.HasClassID(Bluetooth::SDP::ClassID::AudioSink) == true) {
                                    _audioServices.emplace_back(service);
                                }
                            }
                        } else {
                            TRACE(Trace::Information, (_T("Not an A2DP audio sink device!")));
                        }

                        _discoveryComplete(_audioServices);

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
