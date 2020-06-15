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

#include <bitset>

#include <interfaces/IBluetooth.h>
#include <interfaces/IBluetoothAudio.h>
#include <interfaces/JBluetoothAudioSink.h>

#include "ServiceDiscovery.h"
#include "SignallingChannel.h"
#include "TransportChannel.h"

namespace WPEFramework {

namespace Plugin {

    class BluetoothAudioSink : public PluginHost::IPlugin
                             , public PluginHost::JSONRPC
                             , public Exchange::IBluetoothAudioSink {
    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Controller(_T("BluetoothControl"))
            {
                Add(_T("controller"), &Controller);
            }
            ~Config() = default;

        public:
            Core::JSON::String Controller;
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

                DeviceCallback(A2DPSink* parent)
                    : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
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

            static Core::NodeId Designator(const Exchange::IBluetooth::IDevice* device, const bool local, const uint16_t psm = 0)
            {
                ASSERT(device != nullptr);
                return (Bluetooth::Address((local? device->LocalId() : device->RemoteId()).c_str()).NodeId(static_cast<Bluetooth::Address::type>(device->Type()), 0 /* must be zero */, psm));
            }

        public:
            using UpdatedCb = std::function<void()>;

        public:
            A2DPSink(const A2DPSink&) = delete;
            A2DPSink& operator= (const A2DPSink&) = delete;
            A2DPSink(BluetoothAudioSink* parent, Exchange::IBluetooth::IDevice* device, const uint8_t seid, const UpdatedCb& updatedCb)
                : _parent(*parent)
                , _device(device)
                , _callback(this)
                , _updatedCb(updatedCb)
                , _lock()
                , _job()
                , _discovery(Designator(device, true), Designator(device, false, Bluetooth::SDPSocket::SDP_PSM /* a well-known PSM */))
                , _signalling(seid, std::bind(&A2DPSink::OnSignallingUpdated, this), Designator(device, true), Designator(device, false))
                , _transport(seid, Designator(device, true), Designator(device, false))
                , _endpoint(nullptr)
            {
                ASSERT(parent != nullptr);
                ASSERT(device != nullptr);

                _device->AddRef();

                if (_device->IsConnected() == true) {
                    TRACE(Trace::Error, (_T("The device is already connected")));
                }

                if (_device->Callback(&_callback) != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("The device is already in use")));
                }
            }
            ~A2DPSink()
            {
                if (_device->Callback(static_cast<Exchange::IBluetooth::IDevice::ICallback*>(nullptr)) != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Could not remove the callback from the device")));
                }

                _device->Release();
            }

        public:
            Exchange::IBluetoothAudioSink::status Status() const
            {
                return (_signalling.Status());
            }
            uint32_t Configure(const uint8_t channels, const uint32_t samplingFrequency)
            {
                ASSERT(_endpoint != nullptr);

                uint32_t result = Core::ERROR_NONE;

                bool contentProtection = false;
                A2DP::IAudioCodec::Format format;
                format.samplingFrequency = samplingFrequency;
                format.channels = channels;

                TRACE(Trace::Information, (_T("Configuring audio endpoint 0x%02x to %d Hz and %d channel(s)"),
                                            _endpoint->SEID(), format.samplingFrequency, format.channels));

                _endpoint->Configure(format);
                _endpoint->Configuration(format, contentProtection);
                if ((format.samplingFrequency == samplingFrequency) && (format.channels == channels) && (contentProtection == false)) {
                    TRACE(ProfileFlow, (_T("Configure successful")));
                } else {
                    TRACE(Trace::Error, (_T("Failed to configure audio endpoint")));
                    result = Core::ERROR_BAD_REQUEST;
                }

                return (result);
            }
            uint32_t Open()
            {
                ASSERT(_endpoint != nullptr);
                uint32_t result = Core::ERROR_GENERAL;
                if (_endpoint->Open() == Core::ERROR_NONE) {
                    if (_transport.Connect(Designator(_device, false, _audioService.PSM()), _endpoint->Codec()) == Core::ERROR_NONE) {
                        result = Core::ERROR_NONE;
                    }
                }
                return (result);
            }
            uint32_t Close()
            {
                ASSERT(_endpoint != nullptr);
                uint32_t result = Core::ERROR_GENERAL;
                if (_endpoint->Close() == Core::ERROR_NONE) {
                    if (_transport.Disconnect() == Core::ERROR_NONE) {
                        result = Core::ERROR_NONE;
                    }
                }
                return (result);
            }
            uint32_t Start()
            {
                ASSERT(_endpoint != nullptr);
                return (_endpoint->Start());
            }
            uint32_t Stop()
            {
                ASSERT(_endpoint != nullptr);
                return (_endpoint->Stop());
            }
            uint32_t Transmit(const uint32_t length, const uint8_t data[], uint32_t& transmitted)
            {
                uint32_t result = Core::ERROR_NONE;
                if (_transport.IsOpen() == true) {
                    transmitted = _transport.Transmit(length, data);
                } else {
                    TRACE(Trace::Error, (_T("Transport channel is not opened!")));
                    result = Core::ERROR_BAD_REQUEST;
                    transmitted = 0;
                }
                return (result);
            }

        public:
            void OnDeviceUpdated()
            {
                if (_device->IsBonded() == true) {
                    _lock.Lock();

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
                        }
                    } else {
                        TRACE(ProfileFlow, (_T("Device disconnected")));
                        _audioService = A2DP::ServiceDiscovery::AudioService();
                        _transport.Disconnect();
                        _signalling.Disconnect();
                        _discovery.Disconnect();
                    }

                    _lock.Unlock();
                }
            }
            void OnSignallingUpdated()
            {
                if ((Status() == Exchange::IBluetoothAudioSink::IDLE)) {
                    TRACE(ProfileFlow, (_T("Audio endpoint connected/idle")));
                } else if (Status() == Exchange::IBluetoothAudioSink::CONFIGURED) {
                    TRACE(ProfileFlow, (_T("Audio endpoint configured")));
                } else if (Status() == Exchange::IBluetoothAudioSink::OPEN) {
                    TRACE(ProfileFlow, (_T("Audio endpoint open")));
                } else if (Status() == Exchange::IBluetoothAudioSink::STREAMING) {
                    TRACE(ProfileFlow, (_T("Audio endpoint streaming")));
                }
                _updatedCb();
            }

        private:
            void DiscoverAudioServices()
            {
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
                _lock.Lock();

                // Audio services have been discovered, SDP channel is no longer required.
                _discovery.Disconnect();

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
                DiscoverAudioStreamEndpoints(_audioService);

                _lock.Unlock();
            }

        private:
            void DiscoverAudioStreamEndpoints(A2DP::ServiceDiscovery::AudioService& service)
            {
                if (_signalling.Connect(Designator(_device, false, service.PSM())) == Core::ERROR_NONE) {
                    _signalling.Discover([this](std::list<A2DP::AudioEndpoint>& endpoints) {
                        _job.Submit([this, &endpoints]() {
                            OnAudioStreamEndpointsDiscovered(endpoints);
                        });
                    });
                }
            }
            void OnAudioStreamEndpointsDiscovered(std::list<A2DP::AudioEndpoint>& endpoints)
            {
                _lock.Lock();

                // Endpoints have been discovered, but the AVDTP signalling channel remains open!
                for (auto& ep : endpoints) {
                    // TODO: If there are any, it will be the SBC codec, so pick up the first one anyway.
                    //       If more codecs become supported this will have to be revisited!
                    if (ep.Codec() != nullptr) {
                        _endpoint = &ep;
                        break;
                    }
                }

                _lock.Unlock();
            }

        private:
            BluetoothAudioSink& _parent;
            Exchange::IBluetooth::IDevice* _device;
            Core::Sink<DeviceCallback> _callback;
            UpdatedCb _updatedCb;
            Core::CriticalSection _lock;
            DecoupledJob _job;
            A2DP::ServiceDiscovery::AudioService _audioService;
            A2DP::ServiceDiscovery _discovery;
            A2DP::SignallingChannel _signalling;
            A2DP::TransportChannel _transport;
            A2DP::AudioEndpoint* _endpoint;
        }; // class A2DPSink

    public:
        BluetoothAudioSink(const BluetoothAudioSink&) = delete;
        BluetoothAudioSink& operator= (const BluetoothAudioSink&) = delete;
        BluetoothAudioSink()
            : _lock()
            , _service(nullptr)
            , _sink(nullptr)
            , _controller()
            , _callback(nullptr)

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
        uint32_t Callback(Exchange::IBluetoothAudioSink::ICallback* callback)
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            _lock.Lock();

            if (callback == nullptr) {
                if (_callback != nullptr) {
                    _callback->Release();
                    _callback = nullptr;
                    result = Core::ERROR_NONE;
                }
            }
            else if (_callback == nullptr) {
                _callback = callback;
                _callback->AddRef();
                result = Core::ERROR_NONE;
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Assign(const string& device) override;
        uint32_t Revoke(const string& device) override;
        uint32_t Status(Exchange::IBluetoothAudioSink::status& sinkStatus) const
        {
            if (_sink) {
                sinkStatus =_sink->Status();
            } else {
                sinkStatus = Exchange::IBluetoothAudioSink::UNASSIGNED;
            }
            return (Core::ERROR_NONE);
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

    private:
        mutable Core::CriticalSection _lock;
        PluginHost::IShell* _service;
        A2DPSink* _sink;
        string _controller;
        Exchange::IBluetoothAudioSink::ICallback* _callback;
    }; // class BluetoothAudioSink

} // namespace Plugin

}
