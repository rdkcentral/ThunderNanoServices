/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 RDK Management
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

#include <bitset>

#include <interfaces/IBluetooth.h>
#include <interfaces/IBluetoothAudio.h>
#include <interfaces/json/JBluetoothAudioSink.h>

#include "AudioPlayer.h"

#include "SDPServer.h"

#include "ServiceDiscovery.h"
#include "SignallingChannel.h"
#include "TransportChannel.h"

namespace WPEFramework {

namespace Plugin {

    class BluetoothAudioSink : public PluginHost::IPlugin
                             , public PluginHost::JSONRPC
                             , public Exchange::IBluetoothAudioSink
                             , public Exchange::IBluetoothAudioSink::IControl {
    private:
        class Config : public Core::JSON::Container {
        public:
            class SDPServiceConfig : public Core::JSON::Container {
            public:
                SDPServiceConfig(const SDPServiceConfig&) = delete;
                SDPServiceConfig& operator=(const SDPServiceConfig&) = delete;
                SDPServiceConfig()
                    : Core::JSON::Container()
                    , Enable(true)
                    , Name(_T("A2DP Audio Source"))
                    , Description()
                    , Provider()
                {
                    Add("enable", &Enable);
                    Add("name", &Name);
                    Add("description", &Description);
                    Add("provider", &Provider);
                }
                ~SDPServiceConfig() = default;

            public:
                Core::JSON::Boolean Enable;
                Core::JSON::String Name;
                Core::JSON::String Description;
                Core::JSON::String Provider;
            };

        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Controller(_T("BluetoothControl"))
                , SDPService()
            {
                Add(_T("controller"), &Controller);
                Add(_T("sdpservice"), &SDPService);
                Add(_T("codecs"), &Codecs);
            }
            ~Config() = default;

        public:
            Core::JSON::String Controller;
            SDPServiceConfig SDPService;
            Core::JSON::String Codecs;
        }; // class Config

        class DecoupledJob : private Core::WorkerPool::JobType<DecoupledJob&> {
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
            void Submit(const Job& job, const uint32_t defer = 0)
            {
                _lock.Lock();
                if (_job == nullptr) {
                    _job = job;
                    if (defer == 0) {
                        JobType::Submit();
                    } else {
                        JobType::Schedule(Core::Time::Now().Add(defer));
                    }
                } else {
                    TRACE(Trace::Information, (_T("Job in progress, skipping request")));
                }
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
            friend class Core::ThreadPool::JobType<DecoupledJob&>;
            void Dispatch()
            {
                _lock.Lock();
                Job job = _job;
                _job = nullptr;
                _lock.Unlock();
                job();
            }

        private:
            Core::CriticalSection _lock;
            Job _job;
        }; // class DecoupledJob

    public:
        class ComNotificationSink : public PluginHost::IShell::ICOMLink::INotification {
        public:
            ComNotificationSink() = delete;
            ComNotificationSink(const ComNotificationSink&) = delete;
            ComNotificationSink& operator=(const ComNotificationSink&) = delete;
            ComNotificationSink(BluetoothAudioSink& parent)
                : _parent(parent)
            {
            }
            ~ComNotificationSink() = default;

        public:
            void CleanedUp(const Core::IUnknown* remote VARIABLE_IS_NOT_USED, const uint32_t interfaceId VARIABLE_IS_NOT_USED) override
            {
            }
            void Revoked(const Core::IUnknown* remote, const uint32_t interfaceId) override
            {
                ASSERT(remote != nullptr);
                if (interfaceId == Exchange::ID_BLUETOOTHAUDIOSINK_CALLBACK) {
                    const Exchange::IBluetoothAudioSink::ICallback* result = remote->QueryInterface<const Exchange::IBluetoothAudioSink::ICallback>();
                    ASSERT(result != nullptr);
                    _parent.CallbackRevoked(result);
                    result->Release();
                }
            }

        public:
            BEGIN_INTERFACE_MAP(ComNotificationSink)
            INTERFACE_ENTRY(PluginHost::IShell::ICOMLink::INotification)
            END_INTERFACE_MAP

        private:
            BluetoothAudioSink& _parent;
        }; // class ComNotificationSink

    public:
        class A2DPSink {
        private:
            class ProfileFlow {
            public:
                ~ProfileFlow() = default;
                ProfileFlow() = delete;
                ProfileFlow(const ProfileFlow&) = delete;
                ProfileFlow& operator=(const ProfileFlow&) = delete;
                ProfileFlow(const TCHAR formatter[], ...)
                {
                    va_list ap;
                    va_start(ap, formatter);
                    Trace::Format(_text, formatter, ap);
                    va_end(ap);
                }
                explicit ProfileFlow(const string& text)
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
            }; // class ProfileFlow

            class DeviceCallback : public Exchange::IBluetooth::IDevice::ICallback {
            public:
                DeviceCallback() = delete;
                DeviceCallback(const DeviceCallback&) = delete;
                DeviceCallback& operator=(const DeviceCallback&) = delete;

                DeviceCallback(A2DPSink& parent)
                    : _parent(parent)
                {
                }

                ~DeviceCallback() = default;

            public:
                void Updated() override
                {
                    _parent.OnDeviceUpdated();
                }

                BEGIN_INTERFACE_MAP(DeviceCallback)
                    INTERFACE_ENTRY(Exchange::IBluetooth::IDevice::ICallback)
                END_INTERFACE_MAP

            private:
                A2DPSink& _parent;
            }; // class DeviceCallback

        public:
            static Core::NodeId Designator(const Exchange::IBluetooth::IDevice* device)
            {
                ASSERT(device != nullptr);
                return (Bluetooth::Address(device->LocalId().c_str()).NodeId(static_cast<Bluetooth::Address::type>(device->Type()), 0, 25));
            }
            static Core::NodeId Designator(const Exchange::IBluetooth::IDevice* device, const uint16_t psm)
            {
                ASSERT(device != nullptr);
                return (Bluetooth::Address(device->RemoteId().c_str()).NodeId(static_cast<Bluetooth::Address::type>(device->Type()), 0 /* must be zero */, psm));
            }

        public:
            using AudioServiceData = A2DP::ServiceDiscovery::AudioService::Data;

        public:
            A2DPSink(const A2DPSink&) = delete;
            A2DPSink& operator=(const A2DPSink&) = delete;
            A2DPSink(BluetoothAudioSink* parent, const string& codecSettings, Exchange::IBluetooth::IDevice* device, const uint8_t seid)
                : _parent(*parent)
                , _codecSettings(codecSettings)
                , _device(device)
                , _callback(*this)
                , _lock()
                , _audioService()
                , _discovery(Designator(device), Designator(device, Bluetooth::SDP::ClientSocket::SDP_PSM /* a well-known PSM */))
                , _signalling(seid, std::bind(&A2DPSink::OnSignallingUpdated, this), Designator(device), Designator(device, 0 /* yet unknown PSM */))
                , _transport(1 /* SSRC */, Designator(device), Designator(device, 0 /* yet unknown PSM */))
                , _endpoint(nullptr)
                , _type(Exchange::IBluetoothAudioSink::UNKNOWN)
                , _codecList()
                , _drmList()
                , _player(nullptr)
                , _job()
            {
                ASSERT(parent != nullptr);
                ASSERT(device != nullptr);

                _signalling.State(Exchange::IBluetoothAudioSink::DISCONNECTED);

                _device->AddRef();

                if (_device->IsConnected() == true) {
                    TRACE(Trace::Error, (_T("The device is already connected")));
                }

                if (_device->Callback(&_callback) != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("The device is already in use")));
                }
            }
            A2DPSink(BluetoothAudioSink* parent, const string& codecSettings, Exchange::IBluetooth::IDevice* device, const uint8_t seid, const AudioServiceData& data)
                : A2DPSink(parent, codecSettings, device, seid)
            {
                _audioService = A2DP::ServiceDiscovery::AudioService(data);
                _type = DeviceType(_audioService.Features());
            }
            ~A2DPSink()
            {
                ASSERT(_device != nullptr);

                Close();

                if (_device->Callback(static_cast<Exchange::IBluetooth::IDevice::ICallback*>(nullptr)) != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Could not remove the callback from the device")));
                }

                _device->Release();
            }

        public:
            const string Address() const
            {
                ASSERT(_device != nullptr);
                return (_device->RemoteId());
            }
            Exchange::IBluetoothAudioSink::state State() const
            {
                return (_signalling.State());
            }
            Exchange::IBluetoothAudioSink::devicetype Type() const
            {
                return (_type);
            }
            const std::list<Exchange::IBluetoothAudioSink::audiocodec>& Codecs() const
            {
                return (_codecList);
            }
            const std::list<Exchange::IBluetoothAudioSink::drmscheme>& DRMs() const
            {
                return (_drmList);
            }
            uint32_t Time(uint32_t& timestamp)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                if (_player != nullptr) {
                    result = _player->GetTime(timestamp);
                }

                return (result);
            }
            uint32_t Codec(Exchange::IBluetoothAudioSink::CodecProperties& properties) const
            {
                if (_endpoint != nullptr) {
                    ASSERT(_endpoint->Codec() != nullptr);
                    properties.Codec = _endpoint->Codec()->Type();
                    Exchange::IBluetoothAudioSink::IControl::Format _;
                    _endpoint->Codec()->Configuration(_, properties.Settings);
                    return (Core::ERROR_NONE);
                } else {
                    return (Core::ERROR_ILLEGAL_STATE);
                }
            }
            uint32_t DRM(Exchange::IBluetoothAudioSink::DRMProperties& properties) const
            {
                if (_endpoint != nullptr) {
                    if (_endpoint->ContentProtection() != nullptr) {
                        properties.DRM = _endpoint->ContentProtection()->Type();
                        _endpoint->ContentProtection()->Configuration(properties.Settings);
                    } else {
                        properties.DRM = Exchange::IBluetoothAudioSink::NONE;
                    }
                    return (Core::ERROR_NONE);
                } else {
                    return (Core::ERROR_ILLEGAL_STATE);
                }
            }
            uint32_t Stream(Exchange::IBluetoothAudioSink::StreamProperties& properties) const
            {
                if (_endpoint != nullptr) {
                    ASSERT(_endpoint->Codec() != nullptr);
                    string _;
                    Exchange::IBluetoothAudioSink::IControl::Format format;
                    _endpoint->Codec()->Configuration(format, _);
                    properties.BitRate = _endpoint->Codec()->BitRate();
                    properties.SampleRate = format.SampleRate;
                    properties.Resolution = format.Resolution;
                    properties.Channels = format.Channels;
                    return (Core::ERROR_NONE);
                } else {
                    return (Core::ERROR_ILLEGAL_STATE);
                }
            }

        public:
            uint32_t Open(const string& connector, const Exchange::IBluetoothAudioSink::IControl::Format& format)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                if (_endpoint != nullptr) {
                    if (_player == nullptr) {
                        TRACE(Trace::Information, (_T("Configuring audio endpoint 0x%02x to: sample rate: %i Hz, resolution: %i bits per sample, channels: %i, frame rate: %i Hz"),
                                                   _endpoint->SEID(), format.SampleRate, format.Resolution, format.Channels, format.FrameRate));

                        if ((result = _endpoint->Configure(format, _codecSettings)) == Core::ERROR_NONE) {
                            if ((result = _endpoint->Open()) == Core::ERROR_NONE) {
                                ASSERT(_endpoint->Codec() != nullptr);
                                if ((result = _transport.Connect(Designator(_device, _audioService.PSM()), _endpoint->Codec())) == Core::ERROR_NONE) {
                                    _player = new AudioPlayer(_transport, connector);
                                    ASSERT(_player != nullptr);

                                    if (_player->IsValid() == true) {
                                        TRACE(Trace::Information, (_T("Successfully created Bluetooth audio playback session")));
                                        result = Core::ERROR_NONE;
                                    } else {
                                        TRACE(Trace::Error, (_T("Failed to create player")));
                                        result = Core::ERROR_OPENING_FAILED;
                                        delete _player;
                                        _player = nullptr;
                                    }
                                } else {
                                    TRACE(Trace::Error, (_T("Failed to connect transport channel")));
                                    _endpoint->Close();
                                }
                            } else {
                                TRACE(Trace::Error, (_T("Failed to open device endpoint")));
                            }
                        } else {
                            TRACE(Trace::Error, (_T("Failed to configure the endpoint")));
                        }
                    } else {
                        TRACE(Trace::Error, (_T("Bluetooth device already in use")));
                        result = Core::ERROR_UNAVAILABLE;
                    }
                } else {
                    TRACE(Trace::Error, (_T("Audio sink not connected")));
                }

                return (result);
            }
            uint32_t Start()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                if ((_endpoint != nullptr) && (_player != nullptr)) {
                    result = _endpoint->Start();
                    if (result == Core::ERROR_NONE) {
                        result = _player->Play();
                        if (result != Core::ERROR_NONE) {
                            _endpoint->Stop();
                        } else {
                            TRACE(Trace::Information, (_T("Starting Bluetooth audio playback")));
                        }
                    }
                }

                return (result);
            }
            uint32_t Stop()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                if ((_endpoint != nullptr) && (_player != nullptr)) {
                    result = _player->Stop();
                    const uint32_t stopResult = _endpoint->Stop();
                    result = (result != Core::ERROR_NONE? result : stopResult);
                    TRACE(Trace::Information, (_T("Stopped Bluetooth audio playback")));
                }

                return (result);
            }
            uint32_t Close()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                if ((_endpoint != nullptr) && (_player != nullptr)) {
                    result = Core::ERROR_ASYNC_FAILED;

                    Stop();

                    delete _player;
                    _player = nullptr;

                    if (_endpoint->Close() == Core::ERROR_NONE) {
                        if (_transport.Disconnect() == Core::ERROR_NONE) {
                            result = Core::ERROR_NONE;
                            TRACE(Trace::Information, (_T("Closed Bluetooth audio session")));
                        }
                    }
                }

                return (result);
            }

        private:
            void OnDeviceUpdated()
            {
                ASSERT(_device != nullptr);
                if (_device->IsBonded() == true) {
                    if (_device->IsConnected() == true) {
                        if (_audioService.Type() == A2DP::ServiceDiscovery::AudioService::UNKNOWN) {
                            TRACE(ProfileFlow, (_T("Unknown device connected, attempt audio sink discovery...")));
                            DiscoverAudioServices();
                        } else if (_audioService.Type() == A2DP::ServiceDiscovery::AudioService::SINK) {
                            TRACE(ProfileFlow, (_T("Audio sink device connected, connect to the sink...")));
                            DiscoverAudioStreamEndpoints(_audioService);
                        } else {
                            // It's not an audio sink device, can't do anything.
                            TRACE(Trace::Information, (_T("Connected device does not feature an audio sink!")));
                            _signalling.State(Exchange::IBluetoothAudioSink::CONNECTED_BAD_DEVICE);
                        }
                    } else {
                        TRACE(ProfileFlow, (_T("Device disconnected")));
                        _audioService = A2DP::ServiceDiscovery::AudioService();
                        Close();
                        _signalling.Disconnect();
                        _discovery.Disconnect();
                        _signalling.State(Exchange::IBluetoothAudioSink::DISCONNECTED);
                    }
                }
            }
            void OnSignallingUpdated()
            {
                if ((State() == Exchange::IBluetoothAudioSink::CONNECTED)) {
                    TRACE(ProfileFlow, (_T("Audio sink connected")));
                } else if (State() == Exchange::IBluetoothAudioSink::READY) {
                    TRACE(ProfileFlow, (_T("Audio sink ready")));
                } else if (State() == Exchange::IBluetoothAudioSink::STREAMING) {
                    TRACE(ProfileFlow, (_T("Audio sink streaming")));
                } else if (State() == Exchange::IBluetoothAudioSink::DISCONNECTED) {
                    TRACE(ProfileFlow, (_T("Audio sink disconnected")));
                } else if (State() == Exchange::IBluetoothAudioSink::CONNECTED_BAD_DEVICE) {
                    TRACE(ProfileFlow, (_T("Bluetooth device connected but is not an audio sink!")));
                } else if (State() == Exchange::IBluetoothAudioSink::CONNECTED_RESTRICTED) {
                    TRACE(ProfileFlow, (_T("Audio sink connected but blocked by content protection criteria!")));
                }

                _job.Submit([this]() {
                    _parent.Updated();
                });
            }

        private:
            void DiscoverAudioServices()
            {
                // Let's see if the device features audio sink services...
                if (_discovery.Connect() == Core::ERROR_NONE) {
                    _discovery.Discover([this](const std::list<A2DP::ServiceDiscovery::AudioService>& services) {
                        _job.Submit([this, &services]() {
                            OnAudioServicesDiscovered(services);
                        });
                    });
                }
            }
            void OnAudioServicesDiscovered(const std::list<A2DP::ServiceDiscovery::AudioService>& services)
            {
                // Audio services have been discovered, SDP channel is no longer required.
                _discovery.Disconnect();

                if (services.empty() == false) {

                    // Disregard possibility of multiple sink services for now.
                    _audioService = services.front();
                    if (services.size() > 1) {
                        TRACE(Trace::Information, (_T("More than one audio sink available, using the first one!")));
                    }

                    TRACE(Trace::Information, (_T("Audio sink service available! A2DP v%d.%d, AVDTP v%d.%d, L2CAP PSM: %i, features: 0b%s"),
                                            (_audioService.ProfileVersion() >> 8), (_audioService.ProfileVersion() & 0xFF),
                                            (_audioService.TransportVersion() >> 8), (_audioService.TransportVersion() & 0xFF),
                                            _audioService.PSM(), std::bitset<8>(_audioService.Features()).to_string().c_str()));

                    // Now open AVDTP signalling channel...
                    TRACE(ProfileFlow, (_T("Audio sink device discovered, connect to the sink...")));

                    _type = DeviceType(_audioService.Features());

                    DiscoverAudioStreamEndpoints(_audioService);
                } else {
                    _signalling.State(Exchange::IBluetoothAudioSink::CONNECTED_BAD_DEVICE);
                }
            }

            Exchange::IBluetoothAudioSink::devicetype DeviceType(const A2DP::ServiceDiscovery::AudioService::features features) const
            {
                Exchange::IBluetoothAudioSink::devicetype type;
                if (features & A2DP::ServiceDiscovery::AudioService::features::HEADPHONE) {
                    type = Exchange::IBluetoothAudioSink::HEADPHONE;
                } else if (features & A2DP::ServiceDiscovery::AudioService::features::SPEAKER) {
                    type = Exchange::IBluetoothAudioSink::SPEAKER;
                } else if (features & A2DP::ServiceDiscovery::AudioService::features::RECORDER) {
                    type = Exchange::IBluetoothAudioSink::RECORDER;
                } else if (features & A2DP::ServiceDiscovery::AudioService::features::AMPLIFIER) {
                    type = Exchange::IBluetoothAudioSink::AMPLIFIER;
                }
                return (type);
            }

        private:
            void DiscoverAudioStreamEndpoints(A2DP::ServiceDiscovery::AudioService& service)
            {
                // Let's see what endpoints does the sink have...
                if (_signalling.Connect(Designator(_device, service.PSM())) == Core::ERROR_NONE) {
                    _signalling.Discover([this](std::list<A2DP::AudioEndpoint>& endpoints) {
                        _job.Submit([this, &endpoints]() {
                            OnAudioStreamEndpointsDiscovered(endpoints);
                        });
                    });
                }
            }
            void OnAudioStreamEndpointsDiscovered(std::list<A2DP::AudioEndpoint>& endpoints)
            {
                // Endpoints discovery is complete, but the AVDTP signalling channel remains open!

                if (endpoints.empty() == false) {

                    A2DP::AudioEndpoint* sbcEndpoint = nullptr;

                    for (auto& ep : endpoints) {
                        if (ep.Codec() != nullptr) {
                            _codecList.push_back(ep.Codec()->Type());

                            switch (ep.Codec()->Type()) {
                            case Exchange::IBluetoothAudioSink::LC_SBC:
                                sbcEndpoint = &ep;
                                break;
                            default:
                                break;
                            }
                        }

                        if (ep.ContentProtection() != nullptr) {
                            _drmList.push_back(ep.ContentProtection()->Type());
                        }
                    }

                    if (sbcEndpoint != nullptr) {
                        // For now always choose SBC endpoint.
                        _endpoint = sbcEndpoint;
                    } else {
                        // This should not be possible.
                        TRACE(Trace::Error, (_T("No supported codec available on the connected audio sink!")));
                        ASSERT(false && "No SBC codec?");
                    }
                }

                if (_endpoint != nullptr) {
                    AudioServiceData settings;
                    _audioService.Settings(settings);
                    _signalling.State(Exchange::IBluetoothAudioSink::CONNECTED);
                    _parent.Operational(settings);
                } else {
                    _signalling.State(Exchange::IBluetoothAudioSink::CONNECTED_BAD_DEVICE);
                }
           }

        private:
            BluetoothAudioSink& _parent;
            const string& _codecSettings;
            Exchange::IBluetooth::IDevice* _device;
            Core::Sink<DeviceCallback> _callback;
            Core::CriticalSection _lock;
            A2DP::ServiceDiscovery::AudioService _audioService;
            A2DP::ServiceDiscovery _discovery;
            A2DP::SignallingChannel _signalling;
            A2DP::TransportChannel _transport;
            A2DP::AudioEndpoint* _endpoint;
            Exchange::IBluetoothAudioSink::devicetype _type;
            std::list<Exchange::IBluetoothAudioSink::audiocodec> _codecList;
            std::list<Exchange::IBluetoothAudioSink::drmscheme> _drmList;
            AudioPlayer* _player;
            DecoupledJob _job;
        }; // class A2DPSink

    public:
        BluetoothAudioSink(const BluetoothAudioSink&) = delete;
        BluetoothAudioSink& operator=(const BluetoothAudioSink&) = delete;
        BluetoothAudioSink()
            : _lock()
            , _service(nullptr)
            , _comNotificationSink(*this)
            , _controller()
            , _sdpServer()
            , _sink(nullptr)
            , _sinkSEID(1)
            , _callback(nullptr)
            , _codecSettings()
        {
        }
        ~BluetoothAudioSink() = default;

    public:
        // IPlugin overrides
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override { return {}; }

    public:
        // IBluetoothAudioSink overrides
        uint32_t Callback(Exchange::IBluetoothAudioSink::ICallback* callback) override;

        uint32_t Assign(const string& device) override;

        uint32_t Revoke() override;

        uint32_t State(Exchange::IBluetoothAudioSink::state& sinkState) const override
        {
            _lock.Lock();

            if (_sink != nullptr) {
                sinkState = _sink->State();
            } else {
                sinkState = Exchange::IBluetoothAudioSink::UNASSIGNED;
            }

            _lock.Unlock();

            return (Core::ERROR_NONE);
        }

        uint32_t Type(Exchange::IBluetoothAudioSink::devicetype& type) const override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if (_sink != nullptr) {
                type = _sink->Type();
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t SupportedCodecs(IAudioCodecIterator*& codecs) const override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if ((_sink != nullptr) && ((_sink->State() == Exchange::IBluetoothAudioSink::CONNECTED)
                    || (_sink->State() == Exchange::IBluetoothAudioSink::READY) || (_sink->State() == Exchange::IBluetoothAudioSink::STREAMING))) {
                using Implementation = RPC::IteratorType<Exchange::IBluetoothAudioSink::IAudioCodecIterator>;
                codecs = Core::Service<Implementation>::Create<Exchange::IBluetoothAudioSink::IAudioCodecIterator>(_sink->Codecs());
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t SupportedDRMs(IDRMSchemeIterator*& drms) const override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if ((_sink != nullptr) && ((_sink->State() == Exchange::IBluetoothAudioSink::CONNECTED)
                    || (_sink->State() == Exchange::IBluetoothAudioSink::READY) || (_sink->State() == Exchange::IBluetoothAudioSink::STREAMING))) {
                using Implementation = RPC::IteratorType<Exchange::IBluetoothAudioSink::IDRMSchemeIterator>;
                drms = Core::Service<Implementation>::Create<Exchange::IBluetoothAudioSink::IDRMSchemeIterator>(_sink->DRMs());
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Codec(Exchange::IBluetoothAudioSink::CodecProperties& info) const override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->Codec(info);
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t DRM(Exchange::IBluetoothAudioSink::DRMProperties& info) const override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->DRM(info);
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Stream(Exchange::IBluetoothAudioSink::StreamProperties& info) const override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->Stream(info);
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }

        // IControl overrides
        uint32_t Acquire(const string& connector, const Exchange::IBluetoothAudioSink::IControl::Format& format) override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->Open(connector, format);
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Relinquish() override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->Close();
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Speed(const int8_t speed) override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if (_sink != nullptr) {
                if (speed == 0) {
                    result = _sink->Stop();
                } else if (speed == 100) {
                    result = _sink->Start();
                } else {
                    result = Core::ERROR_NOT_SUPPORTED;
                }
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Time(uint32_t& position) override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if (_sink != nullptr) {
                _sink->Time(position);
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }

    public:
        Exchange::IBluetooth* Controller() const
        {
            return (_service->QueryInterfaceByCallsign<Exchange::IBluetooth>(_controller));
        }

    public:
        BEGIN_INTERFACE_MAP(BluetoothAudioSink)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IBluetoothAudioSink)
            INTERFACE_ENTRY(Exchange::IBluetoothAudioSink::IControl)
        END_INTERFACE_MAP

    private:
        void Updated()
        {
            Exchange::IBluetoothAudioSink::ICallback* callback = nullptr;

            _lock.Lock();

            if (_callback != nullptr) {
                callback = _callback;
                _callback->AddRef();
            }

            _lock.Unlock();

            if (callback != nullptr) {
                callback->Updated();
                callback->Release();
            }
        }

        void CallbackRevoked(const Exchange::IBluetoothAudioSink::ICallback* remote VARIABLE_IS_NOT_USED)
        {
            TRACE(Trace::Information, (_T("Remote is dead, revoking the callback")));
            Callback(nullptr);
        }

        void Operational(const A2DPSink::AudioServiceData& data);

    private:
        mutable Core::CriticalSection _lock;
        PluginHost::IShell* _service;
        Core::Sink<ComNotificationSink> _comNotificationSink;
        string _controller;
        SDP::ServiceDiscoveryServer _sdpServer;
        A2DPSink *_sink;
        uint32_t _sinkSEID;
        Exchange::IBluetoothAudioSink::ICallback* _callback;
        string _codecSettings;
    }; // class BluetoothAudioSink

} // namespace Plugin

}
