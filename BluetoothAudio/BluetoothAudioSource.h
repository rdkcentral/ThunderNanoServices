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

#include <bitset>

#include <interfaces/IBluetooth.h>
#include <interfaces/IBluetoothAudio.h>
#include <interfaces/json/JBluetoothAudioSource.h>

#include <bluetooth/audio/bluetooth_audio.h>
#include <bluetooth/audio/codecs/SBC.h>

#include "ServiceDiscovery.h"
#include "AudioEndpoint.h"
#include "DebugTracing.h"
#include "SignallingServer.h"


namespace WPEFramework {

namespace Plugin {

    class BluetoothAudioSource : public PluginHost::JSONRPC
                               , public Exchange::IBluetoothAudio::ISource {
    public:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Connector("/tmp/bluetoothaudiosource")
            {
                Add(_T("connector"), &Connector);

            }
            ~Config() = default;

        public:
            Core::JSON::String Connector;
        }; // class Config

    private:
        class A2DPSource : public Exchange::IBluetoothAudio::ISource::IControl
                         , public A2DP::AudioEndpoint::IHandler {
        private:
            class SendBuffer : public Core::SharedBuffer {
            private:
                static constexpr uint32_t WriteTimeout = 50;

            public:
                SendBuffer() = delete;
                SendBuffer(const SendBuffer&) = delete;
                SendBuffer& operator=(const SendBuffer&) = delete;
                ~SendBuffer() = default;

                SendBuffer(const string& connector, const uint32_t bufferSize)
                    : Core::SharedBuffer(connector.c_str(), 0777, bufferSize, 0)
                    , _bufferSize(bufferSize)
                {
                }

            public:
                uint16_t Write(uint32_t length, const uint8_t data[])
                {
                    ASSERT(IsValid() == true);
                    ASSERT(data != nullptr);

                    uint16_t result = 0;

                    if (RequestProduce(WriteTimeout) == Core::ERROR_NONE) {
                        result = std::min(length, _bufferSize);
                        Size(result);
                        ::memcpy(Buffer(), data, result);
                        Produced();
                    }

                    return (result);
                }

            private:
                uint32_t _bufferSize;
            };

        private:
            class DeviceBroker {
            public:
                class ControllerCallback : public Exchange::IBluetooth::INotification {
                public:
                    ControllerCallback() = delete;
                    ControllerCallback(const ControllerCallback&) = delete;
                    ControllerCallback& operator=(const ControllerCallback&) = delete;
                    ~ControllerCallback() = default;

                    ControllerCallback(DeviceBroker& parent)
                        : _parent(parent)
                    {
                    }

                public:
                    void Update(Exchange::IBluetooth::IDevice* device) override
                    {
                        ASSERT(device != nullptr);

                        _parent.OnControllerUpdated(device);
                    }

                    void Update() override
                    {
                    }

                public:
                    BEGIN_INTERFACE_MAP(ControllerCallback)
                        INTERFACE_ENTRY(Exchange::IBluetooth::INotification)
                    END_INTERFACE_MAP

                private:
                    DeviceBroker& _parent;
                }; // class ControllerCallbacks

            public:
                class Device : public Exchange::IBluetooth::IDevice::ICallback {
                public:
                    Device() = delete;
                    Device(const Device&) = delete;
                    Device& operator=(const Device&) = delete;

                    Device(DeviceBroker& parent, const string address, Exchange::IBluetooth::IDevice* device)
                        : _parent(parent)
                        , _address(std::move(address))
                        , _device(device)
                        , _type(Exchange::IBluetoothAudio::ISource::UNKNOWN)
                    {
                        ASSERT(device != nullptr);
                        _device->AddRef();
                    }
                    ~Device()
                    {
                        _device->Release();
                    }

                public:
                    Exchange::IBluetooth::IDevice* Control() {
                        return (_device);
                    }
                    const Exchange::IBluetooth::IDevice* Control() const {
                        return (_device);
                    }
                    Exchange::IBluetoothAudio::ISource::devicetype Type() const {
                        return (_type);
                    }
                    const string& Address() const {
                        return (_address);
                    }

                private:
                    void Updated() override
                    {
                        _parent.OnDeviceUpdated(this);
                    }

                public:
                    void Type(const uint8_t features)
                    {
                        if (features & ServiceDiscovery::AudioService::features::PLAYER) {
                            _type = Exchange::IBluetoothAudio::ISource::PLAYER;
                        }
                        else if (features & ServiceDiscovery::AudioService::features::MICROPHONE) {
                            _type = Exchange::IBluetoothAudio::ISource::MICROPHONE;
                        }
                        else if (features & ServiceDiscovery::AudioService::features::TUNER) {
                            _type = Exchange::IBluetoothAudio::ISource::TUNER;
                        }
                        else if (features & ServiceDiscovery::AudioService::features::MIXER) {
                            _type = Exchange::IBluetoothAudio::ISource::MIXER;
                        }
                    }

                public:
                    BEGIN_INTERFACE_MAP(Device)
                        INTERFACE_ENTRY(Exchange::IBluetooth::IDevice::ICallback)
                    END_INTERFACE_MAP

                private:
                    DeviceBroker& _parent;
                    string _address;
                    Exchange::IBluetooth::IDevice* _device;
                    Exchange::IBluetoothAudio::ISource::devicetype _type;
                }; // class Device

            public:
                DeviceBroker() = delete;
                DeviceBroker(const DeviceBroker&) = delete;
                DeviceBroker& operator=(DeviceBroker&) = delete;

                DeviceBroker(A2DPSource& parent)
                    : _parent(parent)
                    , _lock()
                    , _controllerCallback(*this)
                    , _devices()
                {
                    Exchange::IBluetooth* ctrl =_parent.Controller();
                    ASSERT(ctrl != nullptr);

                    if (ctrl->Register(&_controllerCallback) != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to register for device updates")));
                    }

                    ctrl->Release();
                }
                ~DeviceBroker()
                {
                    Exchange::IBluetooth* ctrl =_parent.Controller();
                    ASSERT(ctrl != nullptr);

                    ctrl->Unregister(&_controllerCallback);

                    ctrl->Release();

                    for (auto& device : _devices) {
                        device.second->Release();
                    }
                }

            public:
                Device* Find(const string& address)
                {
                    Device* device = nullptr;

                    _lock.Lock();

                    auto it = _devices.find(address);
                    if (it != _devices.end()) {
                        device = (*it).second;
                        ASSERT(device != nullptr);
                        device->AddRef();
                    }

                    _lock.Unlock();

                    return (device);
                }
                void Forget(Device& device)
                {
                    _lock.Lock();

                    _devices.erase(device.Address());

                    device.Control()->Callback(static_cast<Exchange::IBluetooth::IDevice::ICallback*>(nullptr));
                    device.Release();

                    _lock.Unlock();
                }

            private:
                void OnControllerUpdated(Exchange::IBluetooth::IDevice* device)
                {
                    ASSERT(device != nullptr);

                    if (device->Type() == Exchange::IBluetooth::IDevice::ADDRESS_BREDR) {
                        TRACE(SourceFlow, (_T("Now observing device %s"), device->RemoteId().c_str()));

                        _lock.Lock();

                        string address = device->RemoteId();
                        auto it = _devices.emplace(address, Core::Service<Device>::Create<Device>(*this, address, device)).first;

                        if (device->Callback(it->second) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to install device callback!")));
                            it->second->Release();
                            _devices.erase(it);
                        }

                        _lock.Unlock();
                    }
                }
                void OnDeviceUpdated(Device* device)
                {
                    if (device->Control()->IsConnected() == true) {
                        TRACE(SourceFlow, (_T("Observed device %s is connected"), device->Control()->RemoteId().c_str()));

                        _parent.OnDeviceConnected(*device);
                    }
                    else {
                        TRACE(SourceFlow, (_T("Observed device %s is disconnected"), device->Control()->RemoteId().c_str()));
                    }
                }

            private:
                A2DPSource& _parent;
                Core::CriticalSection _lock;
                Core::Sink<ControllerCallback> _controllerCallback;
                std::map<string, Device*> _devices;
            }; // class DeviceBroker

        private:
            class LocalClient : public Exchange::IBluetoothAudio::IStream {
                // Limited control of the source device.
            private:
                static constexpr uint16_t CallTimeoutMs = 5000;

            public:
                LocalClient() = delete;
                LocalClient(const LocalClient&) = delete;
                LocalClient& operator=(const LocalClient&) = delete;
                ~LocalClient() = default;

                LocalClient(A2DPSource& parent)
                    : _parent(parent)
                {
                }

            public:
                Core::hresult Configure(const Exchange::IBluetoothAudio::IStream::Format&) override
                {
                    return (Core::ERROR_NOT_SUPPORTED);
                }
                Core::hresult Acquire(const string&) override
                {
                    return (Core::ERROR_NOT_SUPPORTED);
                }
                Core::hresult Relinquish() override
                {
                    // The client may need to close the streaming session.
                    Core::hresult result = Core::ERROR_ALREADY_RELEASED;

                    ExecuteAsync([this, &result]() {
                        _parent.WithEndpoint([this, &result](A2DP::AudioEndpoint& ep) {
                            result = ep.Abort();
                        });
                    });

                    if ((result != Core::ERROR_NONE) && (result != Core::ERROR_ALREADY_RELEASED)) {
                        TRACE(Trace::Error, (_T("Failed to relinquish endpoint [%d]"), result));
                    }

                    return (result);
                }
                Core::hresult Speed(const int8_t speed) override
                {
                    // The client may need to start/stop the streaming.
                    Core::hresult result = Core::ERROR_ILLEGAL_STATE;

                    ExecuteAsync([this, &result, &speed]() {
                        _parent.WithEndpoint([this, &result, &speed](A2DP::AudioEndpoint& ep) {
                            if (speed == 0) {
                                result = ep.Suspend();
                            }
                            else if (speed == 100) {
                                result = ep.Start();
                            }
                            else {
                                result = Core::ERROR_UNAVAILABLE;
                            }
                        });
                    });

                    if (result != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to set speed %d%% on endpoint [%d]"), speed, result));
                    }

                    return (result);
                }
                Core::hresult Time(uint32_t& positionMs VARIABLE_IS_NOT_USED) const override
                {
                    return (Core::ERROR_NOT_SUPPORTED);
                }
                Core::hresult Delay(uint32_t& delaySamples) const override
                {
                    delaySamples = 0;
                    return (Core::ERROR_NONE);
                }

            public:
                BEGIN_INTERFACE_MAP(LocalClient)
                    INTERFACE_ENTRY(Exchange::IBluetoothAudio::IStream)
                END_INTERFACE_MAP

            private:
                static void ExecuteAsync(const std::function<void()>& fn)
                {
                    Core::Event event(false, true);
                    DecoupledJob job;

                    job.Submit([&fn, &event]() {
                        fn();
                        event.SetEvent();
                    });

                    event.Lock(CallTimeoutMs);
                }

            private:
                A2DPSource& _parent;
            }; // class LocalClient

        public:
            A2DPSource(const A2DPSource&) = delete;
            A2DPSource& operator=(const A2DPSource&) = delete;

            A2DPSource(BluetoothAudioSource& parent)
                : _parent(parent)
                , _lock()
                , _broker(*this)
                , _localClient(Core::Service<LocalClient>::Create<LocalClient>(*this))
                , _sink(nullptr)
                , _state(Exchange::IBluetoothAudio::DISCONNECTED)
                , _connector()
                , _updateJob()
                , _sendBuffer()
                , _discovery()
                , _device(nullptr)
                , _endpoint(nullptr)
            {
            }
            ~A2DPSource() override
            {
                _lock.Lock();

                ASSERT(_localClient != nullptr);
                _localClient->Release();

                if (_sink != nullptr) {
                    _sink->Release();
                    _sink = nullptr;
                }

                ASSERT(_device == nullptr);
                ASSERT(_endpoint == nullptr);

                _lock.Unlock();
            }

        public:
            Exchange::IBluetooth* Controller() {
                return (_parent.Controller());
            }

        private:
            void OnDeviceConnected(DeviceBroker::Device& device)
            {
                std::list<ServiceDiscovery::AudioService> services;

                _discovery.LocalNode(Designator(device.Control()));
                _discovery.RemoteNode(Designator(device.Control(), Bluetooth::SDP::PSM));

                if (_discovery.Connect() == Core::ERROR_NONE) {

                    if (_discovery.Discover({Bluetooth::SDP::ClassID::AudioSource}, services) != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Service discovery failed!")));
                    }

                    _discovery.Disconnect();
                }

                if (services.empty() == false) {
                    ServiceDiscovery::AudioService& service = services.front();

                    if (services.size() > 1) {
                        TRACE(SourceFlow, (_T("More than one audio source available, using the first one!")));
                    }

                    TRACE(SourceFlow, (_T("Audio source service available: A2DP v%d.%d, AVDTP v%d.%d, L2CAP PSM: %i, features: 0b%s"),
                                        (service.ProfileVersion() >> 8), (service.ProfileVersion() & 0xFF),
                                        (service.TransportVersion() >> 8), (service.TransportVersion() & 0xFF),
                                        service.PSM(), std::bitset<8>(service.Features()).to_string().c_str()));

                    device.Type(service.Features());
                }
                else {
                    TRACE(SourceFlow, (_T("Device %s is not an audio source device, stop observing"), device.Address().c_str()));
                    _broker.Forget(device);
                }
            }

        private:
            void Assign(A2DP::AudioEndpoint& endpoint)
            {
                _lock.Lock();

                DeviceBroker::Device* device = _broker.Find(endpoint.DeviceAddress());
                ASSERT(device != nullptr);

                if (device != nullptr) {
                    _device = device;
                }

                ASSERT(_endpoint == nullptr);
                _endpoint = &endpoint;

                _lock.Unlock();

                State(Exchange::IBluetoothAudio::CONNECTED);

            }
            void Unassign()
            {
                _lock.Lock();

                _endpoint = nullptr;

                if (_device != nullptr) {
                    _device->Release();
                    _device = nullptr;
                }

                _lock.Unlock();

                State(Exchange::IBluetoothAudio::CONNECTED);
            }

        private:
            // AudioEndpoint::IHandler overrides
            void SignallingStatus(A2DP::AudioEndpoint& endpoint, const bool running) override
            {
                if (running == true) {
                    TRACE(SourceFlow, (_T("Signalling channel connected")));
                    Assign(endpoint);
                }
                else {
                    TRACE(SourceFlow, (_T("Signalling channel disconnected")));
                }
            }
            void TransportStatus(A2DP::AudioEndpoint& endpoint VARIABLE_IS_NOT_USED, const bool running) override
            {
                if (running == true) {
                    TRACE(SourceFlow, (_T("Transport channel connected")));
                    State(Exchange::IBluetoothAudio::READY);
                }
                else {
                    TRACE(SourceFlow, (_T("Transport channel disconnected")));
                }
            }
            uint32_t OnSetConfiguration(Bluetooth::A2DP::IAudioCodec& codec) override
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                TRACE(SourceFlow, (_T("Signalling channel is connecting")));
                State(Exchange::IBluetoothAudio::CONNECTING);

                _lock.Lock();

                if (_sink == nullptr) {
                    TRACE(Trace::Error, (_T("No receiver audio sink available")));
                }
                else {
                    Bluetooth::A2DP::IAudioCodec::StreamFormat streamFormat;
                    string _;

                    codec.Configuration(streamFormat, _);

                    if ((streamFormat.SampleRate != 0)
                            && ((streamFormat.Channels > 0) && (streamFormat.Channels <= 2))
                            && ((streamFormat.Resolution == 8) || (streamFormat.Resolution == 16))) {

                        Exchange::IBluetoothAudio::IStream::Format format;
                        format.SampleRate = streamFormat.SampleRate;
                        format.FrameRate = streamFormat.FrameRate;
                        format.Channels = streamFormat.Channels;
                        format.Resolution = streamFormat.Resolution;

                        if (_sink->Configure(format) == Core::ERROR_NONE) {
                            TRACE(SourceFlow, (_T("Raw stream format: %d Hz, %d bits, %d channels"), format.SampleRate, format.Resolution, format.Channels));

                            _codec = &codec;

                            _streamProperties.SampleRate = streamFormat.SampleRate;
                            _streamProperties.BitRate = 0; // TODO
                            _streamProperties.Channels = streamFormat.Channels;
                            _streamProperties.Resolution = streamFormat.Resolution;
                            _streamProperties.IsResampled = false;

                            result = Core::ERROR_NONE;
                        }
                        else {
                            TRACE(Trace::Error, (_T("Unsupported configuration")));
                            result = Core::ERROR_NOT_SUPPORTED;
                        }
                    }
                    else {
                        TRACE(Trace::Error, (_T("Invalid configuration requested")));
                        result = Core::ERROR_BAD_REQUEST;
                    }

                    if (result != Core::ERROR_NONE) {
                        State(Exchange::IBluetoothAudio::CONNECTED_BAD);
                    }
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t OnAcquire() override
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_sink == nullptr) {
                    TRACE(Trace::Error, (_T("No receiver audio sink available")));
                }
                else {
                    ASSERT(_sendBuffer == nullptr);

                    // Create the transport medium.
                    _sendBuffer.reset(new (std::nothrow) SendBuffer(_connector, 16*1024));
                    ASSERT(_sendBuffer.get() != nullptr);

                    if (_sendBuffer->IsValid() == true) {

                        // See if the receiver is ready to handle this format.
                        if (_sink->Acquire(_connector) == Core::ERROR_NONE) {
                            // All OK!!

                            TRACE(Trace::Information, (_T("Acquired the receiver audio sink")));

                            result = Core::ERROR_NONE;
                        }
                        else{
                            TRACE(Trace::Error, (_T("Failed to acquire the receiver audio sink")));
                            _sendBuffer.reset();
                            result = Core::ERROR_BAD_REQUEST;
                        }
                    }
                    else {
                        TRACE(Trace::Error, (_T("Failed to open the transport medium")));
                        result = Core::ERROR_UNAVAILABLE;
                    }

                    if (result != Core::ERROR_NONE) {
                        State(Exchange::IBluetoothAudio::CONNECTED_BAD);
                    }
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t OnRelinquish() override
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_sink == nullptr) {
                    // May be this is emergency teardown...
                    TRACE(Trace::Information, (_T("No receiver audio sink available to relinquish")));
                }
                else {
                    result = _sink->Relinquish();

                    if ((result != Core::ERROR_NONE) && (result != Core::ERROR_ALREADY_RELEASED)) {
                        TRACE(Trace::Error, (_T("Failed to relinquish receiver audio sink")));
                    }
                }

                // Even if failed earlier, continue with teardown...
                _sendBuffer.reset();

                result = Core::ERROR_NONE;

                Unassign();

                TRACE(Trace::Information, (_T("Relinquished receiver audio sink%s"), (result != Core::ERROR_NONE? " (with errors)" : "")));

                _lock.Unlock();

                State(Exchange::IBluetoothAudio::DISCONNECTED);

                return (result);
            }
            uint32_t OnBrokenConnection() override
            {
                _cleanupJob.Submit([this]() {
                    OnRelinquish();
                });

                return Core::ERROR_NONE;
            }
            uint32_t OnSetSpeed(const int8_t speed) override
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                TRACE(Trace::Information, (_T("Setting speed %d%% on receiver audio sink"), speed));

                if (_sink->Speed(speed) == Core::ERROR_NONE) {
                    if (speed == 100) {
                        // Notify listneres that the Bluetooth audio source is starting to stream data to the client receiver sink!
                        State(Exchange::IBluetoothAudio::STREAMING);
                        result = Core::ERROR_NONE;
                    }
                    else if (speed == 0) {
                        State(Exchange::IBluetoothAudio::READY);
                        result = Core::ERROR_NONE;
                    } else {
                        result = Core::ERROR_NOT_SUPPORTED;
                    }
                }
                else {
                    TRACE(Trace::Error, (_T("Failed to set speed on receiver audio sink")));
                    result = Core::ERROR_UNAVAILABLE;
                }

                _lock.Unlock();

                return (result);
            }
            void OnPacketReceived(const uint8_t data[], const uint16_t length) override
            {
                ASSERT(_sendBuffer != nullptr);
                ASSERT(_codec != nullptr);

                Bluetooth::RTP::MediaPacket packet(data, length);

                uint8_t stream[32*1024];

                uint16_t produced = sizeof(stream);
                packet.Unpack(*_codec, stream, produced);

                // CMD_DUMP("audio stream", stream, produced);

                if (produced != 0) {
                    if (_sendBuffer->Write(produced, stream) != produced) {
                        TRACE(Trace::Error, (_T("Receiver failed to consume stream data")));
                    }
                }
            }

        public:
            // IBluetoothAudio::IControl overrides
            uint32_t Sink(Exchange::IBluetoothAudio::IStream* sink) override
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                _lock.Lock();

                if (sink == nullptr) {
                    if (_sink != nullptr) {
                        _sink->Release();
                        _sink = nullptr;
                        result = Core::ERROR_NONE;
                    }
                }
                else if (_sink == nullptr) {
                    _sink = sink;
                    _sink->AddRef();
                    result = Core::ERROR_NONE;
                }

                _lock.Unlock();

                if (sink != nullptr) {
                    TRACE(SourceFlow, (_T("Sink callbacks were installed")));
                }
                else {
                    TRACE(SourceFlow, (_T("Sink callbacks were uninstalled")));

                    if (_device != nullptr) {
                        TRACE(SourceFlow, (_T("Sink disconnected in busy state, clean up!")));

                        ASSERT(_localClient != nullptr);
                        _localClient->Relinquish();
                    }
                }

                return (result);
            }

        public:
            void Config(const Config& config)
            {
                _connector = config.Connector;
            }

        private:
            bool WithEndpoint(const std::function<void(A2DP::AudioEndpoint&)>& cb)
            {
                bool result = false;

                A2DP::AudioEndpoint* ep = nullptr;

                _lock.Lock();

                if (_endpoint != nullptr) {
                    ep = _endpoint;
                    result = true;
                }

                _lock.Unlock();

                // The source endpoint is a static resource, hence the pointer will be always valid.
                if (ep != nullptr) {
                    cb(*ep);
                }

                return (result);
            }

        public:
            Exchange::IBluetoothAudio::state State() const {
                return (_state);
            }
            uint32_t Device(string& address) const
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_device != nullptr) {
                    address = _device->Address();
                    result = Core::ERROR_NONE;
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t Type(Exchange::IBluetoothAudio::ISource::devicetype& type) const
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_device != nullptr) {
                    type  = _device->Type();
                    result = Core::ERROR_NONE;
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t StreamProperties(Exchange::IBluetoothAudio::StreamProperties& props) const
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_device != nullptr) {
                    props = _streamProperties;
                    result = Core::ERROR_NONE;
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t CodecProperties(Exchange::IBluetoothAudio::CodecProperties& props) const
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_device != nullptr) {
                    if (_codec->Type() == Bluetooth::A2DP::IAudioCodec::codectype::LC_SBC) {
                        props.Codec = Exchange::IBluetoothAudio::LC_SBC;
                    }

                    result = Core::ERROR_NONE;
                }

                _lock.Unlock();

                return (result);
            }

        public:
            BEGIN_INTERFACE_MAP(A2DPSource)
                INTERFACE_ENTRY(Exchange::IBluetoothAudio::ISource::IControl)
                INTERFACE_AGGREGATE(Exchange::IBluetoothAudio::IStream, _localClient)
            END_INTERFACE_MAP

        private:
            void State(const Exchange::IBluetoothAudio::state state)
            {
                _state = state;

                _updateJob.Submit([this, state]() {
                    _parent.StateChanged(state);
                });
            }

        private:
            BluetoothAudioSource& _parent;
            mutable Core::CriticalSection _lock;
            DeviceBroker _broker;
            LocalClient* _localClient;
            Exchange::IBluetoothAudio::IStream* _sink;
            std::atomic<Exchange::IBluetoothAudio::state> _state;
            string _connector;
            DecoupledJob _updateJob;
            DecoupledJob _cleanupJob;
            Bluetooth::A2DP::IAudioCodec* _codec;
            std::unique_ptr<SendBuffer> _sendBuffer;
            ServiceDiscovery _discovery;
            DeviceBroker::Device* _device;
            A2DP::AudioEndpoint* _endpoint;
            Exchange::IBluetoothAudio::StreamProperties _streamProperties;
        }; //class A2DPSource

    public:
        BluetoothAudioSource(const BluetoothAudioSource&) = delete;
        BluetoothAudioSource& operator=(const BluetoothAudioSource&) = delete;

        BluetoothAudioSource()
            : _lock()
            , _cleanupJob()
            , _controller()
            , _service(nullptr)
            , _callback(nullptr)
            , _source(nullptr)
        {
        }
        ~BluetoothAudioSource()
        {
            if (_source != nullptr) {
                _source->Release();
            }

            if (_callback != nullptr) {
                _callback->Release();
            }

            ASSERT(_service == nullptr);
        }

    public:
        void StateChanged(const Exchange::IBluetoothAudio::state state)
        {
            Exchange::IBluetoothAudio::ISource::ICallback* callback = nullptr;

            const Core::EnumerateType<Exchange::IBluetoothAudio::state> value(state);
            TRACE(Trace::Information, (_T("Bluetooth audio source state changed: %s"), value.Data()));

            _lock.Lock();

            if (_callback != nullptr) {
                callback = _callback;
                _callback->AddRef();
            }

            _lock.Unlock();

            if (callback != nullptr) {
                callback->StateChanged(state);
                callback->Release();
            }

            Exchange::BluetoothAudio::JSource::Event::StateChanged(*this, state);
        }

    public:
        // IPlugin overrides
        string Initialize(PluginHost::IShell* service, const string& controller, const Config& config);
        void Deinitialize(PluginHost::IShell* service);

    public:
        // IBluetoothAudioSource overrides
        uint32_t Callback(Exchange::IBluetoothAudio::ISource::ICallback* callback) override
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
        uint32_t State(Exchange::IBluetoothAudio::state& sourceState) const override
        {
            ASSERT(_source != nullptr);
            sourceState = _source->State();
            return (Core::ERROR_NONE);
        }
        uint32_t Type(Exchange::IBluetoothAudio::ISource::devicetype& type) const override
        {
            ASSERT(_source != nullptr);
            return (_source->Type(type));

        }
        uint32_t Device(string& address) const override
        {
            ASSERT(_source != nullptr);
            return (_source->Device(address));
        }
        uint32_t Codec(Exchange::IBluetoothAudio::CodecProperties& properties) const override
        {
            return (_source->CodecProperties(properties));
            return (Core::ERROR_NONE);
        }
        uint32_t DRM(Exchange::IBluetoothAudio::DRMProperties& properties VARIABLE_IS_NOT_USED) const override
        {
            // DRM not supported
            return (Core::ERROR_NONE);
        }
        uint32_t Stream(Exchange::IBluetoothAudio::StreamProperties& properties) const override
        {
            ASSERT(_source != nullptr);
            return (_source->StreamProperties(properties));
        }

    public:
        Exchange::IBluetooth* Controller() const {
            ASSERT(_service != nullptr);
            return (_service->QueryInterfaceByCallsign<Exchange::IBluetooth>(_controller));
        }
        operator A2DP::AudioEndpoint::IHandler&() {
            return (*_source);
        }

    public:
        void Revoked(const Core::IUnknown* remote, const uint32_t interfaceId VARIABLE_IS_NOT_USED)
        {
            if (remote == _callback) {
                TRACE(Trace::Information, (_T("Remote client died; cleaning up callbaks on behalf of the dead")));

                Callback(nullptr);

                _cleanupJob.Submit([this]() {
                    ASSERT(_source != nullptr);
                    _source->Sink(nullptr);
                });
            }
        }

    public:
        BEGIN_INTERFACE_MAP(BluetoothAudioSource)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IBluetoothAudio::ISource)
            INTERFACE_AGGREGATE(Exchange::IBluetoothAudio::ISource::IControl, _source)
            INTERFACE_AGGREGATE(Exchange::IBluetoothAudio::IStream, _source)
        END_INTERFACE_MAP

    private:
        mutable Core::CriticalSection _lock;
        DecoupledJob _cleanupJob;
        string _controller;
        PluginHost::IShell* _service;
        Exchange::IBluetoothAudio::ISource::ICallback* _callback;
        A2DPSource* _source;
    }; // class BluetoothAudioSource

} // namespace Plugin

}
