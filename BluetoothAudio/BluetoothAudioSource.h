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
                         , public Exchange::IBluetoothAudio::IStream
                         , public Bluetooth::AVDTP::Server {
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

        public:
            using AudioEndpoint = A2DP::AudioEndpoint<A2DPSource, SourceFlow>;

            class Signalling : public SignallingServer::IHandler {
            public:
                Signalling(A2DPSource& parent)
                    : _parent(parent)
                    , _lock()
                    , _device(nullptr)
                    , _signalling(nullptr)
                    , _transport(nullptr)
                {
                    SignallingServer::Instance().Register(this);
                }
               ~Signalling() override
                {
                    SignallingServer::Instance().Unregister(this);
                }

            public:
                // IHandler overrides
                bool Accept(Bluetooth::AVDTP::ServerSocket* channel) override
                {
                    bool result = false;

                    ASSERT(channel != nullptr);

                    _lock.Lock();

                    if (_parent.IsBusy() == false) {

                        TRACE(SourceFlow, (_T("Source accepting signalling connection from %s"), channel->RemoteNode().HostAddress().c_str()));

                        Exchange::IBluetooth* btControl = _parent._parent.Controller();
                        ASSERT(btControl != nullptr);

                        if (btControl != nullptr) {

                            ASSERT(_device == nullptr);

                            _device = btControl->Device(channel->RemoteNode().HostAddress(), Exchange::IBluetooth::IDevice::ADDRESS_BREDR);
                            ASSERT(_device != nullptr);

                            btControl->Release();

                            _parent.OnDeviceConnected(_device);
                            _signalling = channel;

                            result = true;
                        }
                    }
                    else if (channel->RemoteNode().HostAddress() == _device->RemoteId()) {

                        ASSERT(_signalling != nullptr);

                        if (_transport == nullptr) {

                            TRACE(SourceFlow, (_T("Source accepting transport connection from %s"), channel->RemoteNode().HostAddress().c_str()));

                            channel->Type(Bluetooth::AVDTP::ServerSocket::channeltype::TRANSPORT);

                            _transport = channel;

                            result = true;
                        }
                        else {
                            TRACE(SourceFlow, (_T("Unsupported connection from %s"), channel->RemoteNode().HostAddress().c_str()));
                        }
                    }

                    _lock.Unlock();

                    return (result);
                }
                void Operational(const bool running, Bluetooth::AVDTP::ServerSocket* channel) override
                {
                    ASSERT(channel != nullptr);

                    if (running == true) {
                        TRACE(SourceFlow, (_T("Signalling channel operational")));
                        //Client::Channel(*channel);
                    }
                    else {
                        TRACE(SourceFlow, (_T("Signalling channel not operational")));

                        _lock.Lock();

                        if (channel == _signalling) {
                            ASSERT(channel->Type() == Bluetooth::AVDTP::ServerSocket::SIGNALLING);

                            _parent.OnSignallingDisconnected();

                            _signalling = nullptr;
                        }
                        else if (channel == _transport) {
                            ASSERT(channel->Type() == Bluetooth::AVDTP::ServerSocket::TRANSPORT);

                            _parent.OnTransportDisconnected();

                            _transport = nullptr;
                        }

                        if ((_signalling == nullptr) && (_transport == nullptr) && (_device != nullptr)) {

                            _parent.OnDeviceDisconnected(_device);

                            _device->Release();
                            _device = nullptr;
                        }

                        _lock.Unlock();
                    }
                }
                void OnSignal(const Bluetooth::AVDTP::Signal& request, const Bluetooth::AVDTP::ServerSocket::ResponseHandler& handler) override
                {
                    // The sink device may also want to close or control the stream!
                    _parent.OnSignal(request, handler);
                }
                void OnPacket(const uint8_t packet[], const uint16_t length) override
                {
                    _parent.DataReceived(packet, length);
                }
                bool IsBusy() const override {
                    return (_parent.IsBusy());
                }

            private:
                A2DPSource& _parent;
                Core::CriticalSection _lock;
                Exchange::IBluetooth::IDevice* _device;
                Bluetooth::AVDTP::ServerSocket* _signalling;
                Bluetooth::AVDTP::ServerSocket* _transport;
            };

        public:
            A2DPSource(const A2DPSource&) = delete;
            A2DPSource& operator=(const A2DPSource&) = delete;

            A2DPSource(BluetoothAudioSource& parent)
                : _parent(parent)
                , _signalling(*this)
                , _lock()
                , _device(nullptr)
                , _endpoints()
                , _counter(0)
                , _sourceEndpoint(nullptr)
                , _sink(nullptr)
                , _state(Exchange::IBluetoothAudio::DISCONNECTED)
                , _connector()
                , _updateJob()
                , _sendBuffer()
            {
            }
            ~A2DPSource() override
            {
            }

        private:
            void OnDeviceConnected(Exchange::IBluetooth::IDevice* device)
            {
                ASSERT(device != nullptr);

                TRACE(SourceFlow, (_T("Device %s connected"), device->RemoteId().c_str()));

                _lock.Lock();

                ASSERT(_device == nullptr);

                _device = device;
                _device->AddRef();

                _lock.Unlock();
            }
            void OnDeviceDisconnected(Exchange::IBluetooth::IDevice* device)
            {
                ASSERT(device != nullptr);

                TRACE(SourceFlow, (_T("Device %s disconnected"), device->RemoteId().c_str()));

                _lock.Lock();

                ASSERT(_device == device);

                _device->Release();
                _device = nullptr;

                _lock.Unlock();
            }
            void OnSignallingDisconnected()
            {
                _disconnectJob.Submit([this]() {
                    _lock.Lock();

                    if (_sourceEndpoint != nullptr) {
                        TRACE(Trace::Information, (_T("Audio source was disconnected abruptly, unlocking endpoint")));
                        _sourceEndpoint->Disconnected();
                    }

                    _lock.Unlock();

                    State(Exchange::IBluetoothAudio::DISCONNECTED);
                });
            }
            void OnTransportDisconnected()
            {
            }

        public:
            void Add(const bool sink, Bluetooth::A2DP::IAudioCodec* codec)
            {
                _lock.Lock();

                const uint8_t id = ++_counter;

                auto it = _endpoints.emplace(std::piecewise_construct,
                                            std::forward_as_tuple(id),
                                            std::forward_as_tuple(*this, id, sink, codec)).first;

                TRACE(Trace::Information, (_T("New audio %s endpoint %d (codec %02x)"), (sink? "sink": "soruce"), (*it).second.Id(), (*it).second.Codec()->Type()));

                _lock.Unlock();
            }
            bool With(const uint8_t id, const std::function<void(AudioEndpoint&)>& inspectCb)
            {
                bool result = false;

                _lock.Lock();

                auto it = _endpoints.find(id);

                if (it != _endpoints.end()) {
                    inspectCb((*it).second);
                    result = true;
                }

                _lock.Unlock();

                return (result);
            }

        public:
            uint32_t Open(AudioEndpoint& endpoint)
            {
                ASSERT(false);
                return (Core::ERROR_UNAVAILABLE);
            }
            uint32_t Close(AudioEndpoint& endpoint)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                Bluetooth::AVDTP::ClientSocket* channel = endpoint.Channel();
                if (channel != nullptr) {
                    Bluetooth::AVDTP::Client client(*channel);
                    result = client.Close(endpoint);
                }

                return (result);
            }
            uint32_t Start(AudioEndpoint& endpoint)
            {
                return (Core::ERROR_UNAVAILABLE);
            }
            uint32_t Suspend(AudioEndpoint& endpoint)
            {
                return (Core::ERROR_UNAVAILABLE);
            }
            Bluetooth::A2DP::IAudioCodec* Codec(uint8_t codec, const Bluetooth::Buffer& params)
            {
                switch (codec) {
                case Bluetooth::A2DP::SBC::CODEC_TYPE:
                    TRACE(SourceFlow, (_T("Endpoint supports LC_SBC codec")));
                    return (new Bluetooth::A2DP::SBC(params));
                default:
                    break;
                }

                return (nullptr);
            }

        public:
            uint32_t OnSetConfiguration(AudioEndpoint& endpoint)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                State(Exchange::IBluetoothAudio::CONNECTING);

                _lock.Lock();

                if (_sink == nullptr) {
                    TRACE(Trace::Error, (_T("No receiver audio sink available")));
                }
                else if (_sourceEndpoint != nullptr) {
                    TRACE(Trace::Error, (_T("Endpoint is already allocated")));
                }
                else {
                    Bluetooth::A2DP::IAudioCodec::StreamFormat streamFormat;
                    string _;

                    endpoint.Codec()->Configuration(streamFormat, _);

                    if ((streamFormat.SampleRate != 0)
                            && ((streamFormat.Channels > 0) && (streamFormat.Channels <= 2))
                            && ((streamFormat.Resolution == 8) || (streamFormat.Resolution == 16))) {

                        Exchange::IBluetoothAudio::IStream::Format format{};
                        format.SampleRate = streamFormat.SampleRate;
                        format.FrameRate = streamFormat.FrameRate;
                        format.Channels = streamFormat.Channels;
                        format.Resolution = streamFormat.Resolution;

                        if (_sink->Configure(format) == Core::ERROR_NONE) {
                            TRACE(SourceFlow, (_T("Raw stream format: %d Hz, %d bits, %d channels"), format.SampleRate, format.Resolution, format.Channels));

                            State(Exchange::IBluetoothAudio::CONNECTED);

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
            uint32_t OnAcquire(AudioEndpoint& endpoint)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_sink == nullptr) {
                    TRACE(Trace::Error, (_T("No receiver audio sink available")));
                }
                else if (_sourceEndpoint != nullptr) {
                    TRACE(Trace::Error, (_T("Endpoint is already allocated")));
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
                            _sourceEndpoint = &endpoint;

                            TRACE(Trace::Information, (_T("Acquired the receiver audio sink (endpoint %d)"), endpoint.Id()));

                            // Notify listneres that the Bluetooth audio source and the client receiver sink are both ready for streaming!
                            State(Exchange::IBluetoothAudio::READY);
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
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t OnRelinquish(AudioEndpoint& endpoint)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_sourceEndpoint == nullptr) {
                    TRACE(Trace::Error, (_T("Endpoint not allocated")));
                }
                else if (_sourceEndpoint->Id() != endpoint.Id()) {
                    TRACE(Trace::Error, (_T("Wrong endpoint")));
                }
                else {
                    if (_sink == nullptr) {
                        // May be this is emergency teardown.
                        TRACE(Trace::Information, (_T("No receiver audio sink available to deallocate")));
                    }
                    else {
                        result = _sink->Relinquish();

                        if (result != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to relinquish receiver audio sink")));
                        }
                    }

                    // Even if failed earlier, continue with teardown...
                    _sendBuffer.reset();

                    _sourceEndpoint = nullptr;
                    result = Core::ERROR_NONE;

                    TRACE(Trace::Information, (_T("Deallocated receiver audio sink%s"), (result != Core::ERROR_NONE? " (with errors)" : "")));

                    State(Exchange::IBluetoothAudio::CONNECTED);
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t OnSetSpeed(AudioEndpoint& endpoint, const int8_t speed)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_sourceEndpoint == nullptr) {
                    TRACE(Trace::Error, (_T("Receiver audio sink not allocated")));
                }
                else if (_sourceEndpoint->Id() != endpoint.Id()) {
                    TRACE(Trace::Error, (_T("Wrong endpoint")));
                }
                else {
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
                }

                _lock.Unlock();

                return (result);
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

                TRACE(SourceFlow, (_T("Sink callbacks were installed")));

                return (result);
            }

        public:
            // IBluetoothAudio::IStream overrides
            Core::hresult Relinquish() override
            {
                Core::hresult result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_sourceEndpoint != nullptr) {

                    TRACE(Trace::Information, (_T("Client receiver is relinquishing the streaming session")));

                    if (_closeTransportCb) {
                        _closeTransportCb();
                    }

                    result = _sourceEndpoint->Close();
                }

                _lock.Unlock();

                return (result);
            }
            Core::hresult Speed(const int8_t speed) override
            {
                Core::hresult result = Core::ERROR_ILLEGAL_STATE;

                return (result);
            }
            Core::hresult Time(uint32_t& positionMs) const override
            {
                Core::hresult result = Core::ERROR_ILLEGAL_STATE;

                return (result);
            }

        public:
            void Config(const Config& config)
            {
                _connector = config.Connector;

            }

        private:
            // Unused IBluetoothAudio::IStream overrides
            Core::hresult Configure(const Exchange::IBluetoothAudio::IStream::Format& VARIABLE_IS_NOT_USED) override
            {
                return (Core::ERROR_NOT_SUPPORTED);
            }
            Core::hresult Acquire(const string& connector VARIABLE_IS_NOT_USED) override
            {
                return (Core::ERROR_NOT_SUPPORTED);
            }
            Core::hresult Delay(uint32_t& delaySamples VARIABLE_IS_NOT_USED) const override
            {
                return (Core::ERROR_NOT_SUPPORTED);
            }

        private:
            bool IsBusy() const
            {
                bool busy = false;

                _lock.Lock();

                if (_sourceEndpoint != nullptr) {
                    busy = (_sourceEndpoint->State() != Bluetooth::AVDTP::StreamEndPoint::IDLE);
                }

                _lock.Unlock();

                return (busy);
            }
            bool WithEndpoint(const uint8_t id, const std::function<void(Bluetooth::AVDTP::StreamEndPoint&)>& inspectCb) override
            {
                bool result = false;

                _lock.Lock();

                auto it = _endpoints.find(id);

                if (it != _endpoints.end()) {
                    inspectCb(it->second);
                    result = true;
                }

                _lock.Unlock();

                return (result);
            }

        public:
            void TransportCloseHook(const std::function<void()>& hook)
            {
                _lock.Lock();
                _closeTransportCb = hook;
                _lock.Unlock();
            }

        public:
            Exchange::IBluetoothAudio::state State() const {
                return (_state);
            }
            string Address() const
            {
                string address;

                _lock.Lock();

                if (_device != nullptr) {
                    address = _device->RemoteId();
                }

                _lock.Unlock();

                return (address);
            }

        public:
            BEGIN_INTERFACE_MAP(A2DPSource)
                INTERFACE_ENTRY(Exchange::IBluetoothAudio::ISource::IControl)
                INTERFACE_ENTRY(Exchange::IBluetoothAudio::IStream)
            END_INTERFACE_MAP

        private:
            void State(const Exchange::IBluetoothAudio::state state)
            {
                _lock.Lock();

                _state = state;

                _lock.Unlock();

                _updateJob.Submit([this, state]() {
                    _parent.StateChanged(state);
                });
            }
            void DataReceived(const uint8_t data[], const uint16_t length)
            {
                ASSERT(_sourceEndpoint != nullptr);
                ASSERT(_sendBuffer != nullptr);

                Bluetooth::RTP::MediaPacket packet(data, length);

                uint8_t stream[32*1024];

                uint16_t produced = sizeof(stream);
                packet.Unpack(*_sourceEndpoint->Codec(), stream, produced);

                //CMD_DUMP("audio stream", stream, produced);

                if (produced != 0) {
                    if (_sendBuffer->Write(produced, stream) != produced) {
                        TRACE(Trace::Error, (_T("Receiver failed to consume stream data")));
                    }
                }
            }

        private:
            BluetoothAudioSource& _parent;
            Signalling _signalling;
            mutable Core::CriticalSection _lock;
            Exchange::IBluetooth::IDevice* _device;
            std::map<uint8_t, AudioEndpoint> _endpoints;
            uint8_t _counter;
            AudioEndpoint* _sourceEndpoint;
            Exchange::IBluetoothAudio::IStream* _sink;
            Exchange::IBluetoothAudio::state _state;
            string _connector;
            DecoupledJob _updateJob;
            DecoupledJob _disconnectJob;
            std::function<void()> _closeTransportCb;
            std::unique_ptr<SendBuffer> _sendBuffer;
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
            , _source(Core::Service<A2DPSource>::Create<A2DPSource>(*this))
        {
        }
        ~BluetoothAudioSource()
        {
            if (_source != nullptr) {
                _source->Release();
                _source = nullptr;
            }
        }

    public:
        void StateChanged(const Exchange::IBluetoothAudio::state state)
        {
            Exchange::IBluetoothAudio::ISource::ICallback* callback = nullptr;

            const Core::EnumerateType<Exchange::IBluetoothAudio::state> value(state);
            TRACE(SourceFlow, (_T("State changed: %s"), value.Data()));

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
            _lock.Lock();

            ASSERT(_source != nullptr);

            if (_source != nullptr) {
                sourceState = _source->State();
            }

            _lock.Unlock();

            return (Core::ERROR_NONE);
        }
        uint32_t Type(Exchange::IBluetoothAudio::ISource::devicetype& type) const override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            return (result);
        }
        uint32_t Device(string& address) const override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_source != nullptr) {
                address = _source->Address();
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Codec(Exchange::IBluetoothAudio::CodecProperties& properties) const override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            return (result);
        }
        uint32_t DRM(Exchange::IBluetoothAudio::DRMProperties& properties) const override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            return (result);
        }
        uint32_t Stream(Exchange::IBluetoothAudio::StreamProperties& properties) const override
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            return (result);
        }

    public:
        Exchange::IBluetooth* Controller() const {
            ASSERT(_service != nullptr);
            return (_service->QueryInterfaceByCallsign<Exchange::IBluetooth>(_controller));
        }

    public:
        void Revoked(const Core::IUnknown* remote, const uint32_t interfaceId VARIABLE_IS_NOT_USED)
        {
            if (remote == _callback) {
                TRACE(Trace::Information, (_T("Remote client died; cleaning up callbaks on behalf of the dead")));

                Callback(nullptr);
                _source->Sink(nullptr);

                _cleanupJob.Submit([this]() {
                    ASSERT(_source != nullptr);
                    _source->Relinquish();
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
