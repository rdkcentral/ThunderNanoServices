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

        class DispatchJob {
        private:
            class Job : public Core::IDispatch {
            public:
                Job(DispatchJob& parent)
                    :_parent(parent)
                {
                }
                ~Job() = default;

            public:
                void Dispatch() override
                {
                    _parent.Trigger();
                }

            private:
                DispatchJob& _parent;
            };

        public:
            using Handler = std::function<void()>;

        public:
            DispatchJob(const DispatchJob&) = delete;
            DispatchJob& operator=(const DispatchJob&) = delete;

            DispatchJob(const Handler& handler)
                : _handler(handler)
                , _job(Core::ProxyType<Job>::Create(*this))
            {
                Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(_job));
            }

        private:
           ~DispatchJob() = default;

            void Trigger()
            {
                _handler();
                delete this;
            }

        private:
            Handler _handler;
            Core::ProxyType<Job> _job;
        }; // class DispatchJob

        class A2DPSink {
        private:
            class A2DPFlow {
            public:
                ~A2DPFlow() = default;
                A2DPFlow() = delete;
                A2DPFlow(const A2DPFlow&) = delete;
                A2DPFlow& operator=(const A2DPFlow&) = delete;
                A2DPFlow(const TCHAR formatter[], ...)
                {
                    va_list ap;
                    va_start(ap, formatter);
                    Trace::Format(_text, formatter, ap);
                    va_end(ap);
                }
                explicit A2DPFlow(const string& text)
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
            }; // class A2DPFlow

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
                    _parent.DeviceUpdated();
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
            A2DPSink(const A2DPSink&) = delete;
            A2DPSink& operator= (const A2DPSink&) = delete;
            A2DPSink(BluetoothAudioSink* parent, Exchange::IBluetooth::IDevice* device, const uint8_t seid)
                : _parent(*parent)
                , _device(device)
                , _callback(this)
                , _lock()
                , _discovery(Designator(device, true), Designator(device, false, Bluetooth::SDPSocket::SDP_PSM /* a well-known PSM */))
            {
                ASSERT(parent != nullptr);
                ASSERT(device != nullptr);

                _device->AddRef();

                if (_device->IsConnected() == true) {
                    TRACE(Trace::Fatal, (_T("The device is already connected")));
                }

                if (_device->Callback(&_callback) != Core::ERROR_NONE) {
                    TRACE(Trace::Fatal, (_T("The device is already in use")));
                }
            }
            ~A2DPSink()
            {
                if (_device->Callback(static_cast<Exchange::IBluetooth::IDevice::ICallback*>(nullptr)) != Core::ERROR_NONE) {
                    TRACE(Trace::Fatal, (_T("Could not remove the callback from the device")));
                }

                _device->Release();
            }

        public:
            void DeviceUpdated()
            {
                if (_device->IsBonded() == true) {
                    _lock.Lock();

                    if (_device->IsConnected() == true) {
                        if (_audioService.Type() == Implementation::ServiceDiscovery::AudioService::UNKNOWN) {
                            TRACE(A2DPFlow, (_T("Unknown device connected, attempt audio sink discovery...")));
                            DiscoverAudioServices();
                        } else if (_audioService.Type() == Implementation::ServiceDiscovery::AudioService::SINK) {
                            TRACE(A2DPFlow, (_T("Audio sink device connected, connect to the sink...")));
                            DiscoverAudioStreamEndpoints(_audioService);
                        } else {
                            // It's not an audio sink device, can't do anything.
                            TRACE(Trace::Information, (_T("Connected device does not feature an audio sink!")));
                        }
                    } else {
                        TRACE(A2DPFlow, (_T("Device disconnected")));
                        _audioService = Implementation::ServiceDiscovery::AudioService();
                        _discovery.Disconnect();
                    }

                    _lock.Unlock();
                }
            }

        private:
            void DiscoverAudioServices()
            {
                if (_discovery.Connect() == Core::ERROR_NONE) {
                    _discovery.Discover([this](const std::list<Implementation::ServiceDiscovery::AudioService>& services) {
                        new DispatchJob([this, &services]() {
                            AudioServices(services);
                        });
                    });
                }
            }
            void AudioServices(const std::list<Implementation::ServiceDiscovery::AudioService>& services)
            {
                _lock.Lock();

                // Audio serivces have been discovered, SDP channel is no longer required.
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

                _lock.Unlock();
            }

        private:
            BluetoothAudioSink& _parent;
            Exchange::IBluetooth::IDevice* _device;
            Core::Sink<DeviceCallback> _callback;
            Core::CriticalSection _lock;
            Implementation::ServiceDiscovery::AudioService _audioService;
            Implementation::ServiceDiscovery _discovery;
        }; // class A2DPSink

    public:
        BluetoothAudioSink(const BluetoothAudioSink&) = delete;
        BluetoothAudioSink& operator= (const BluetoothAudioSink&) = delete;
        BluetoothAudioSink()
            : _lock()
            , _service(nullptr)
            , _sink(nullptr)
            , _controller()

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
        mutable Core::CriticalSection _lock;
        PluginHost::IShell* _service;
        A2DPSink* _sink;
        string _controller;
    }; // class BluetoothAudioSink

} // namespace Plugin

}
