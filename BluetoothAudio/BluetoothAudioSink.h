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

#include <bitset>

#include <interfaces/IBluetooth.h>
#include <interfaces/IBluetoothAudio.h>
#include <interfaces/json/JBluetoothAudioSink.h>

#include <bluetooth/audio/bluetooth_audio.h>
#include <bluetooth/audio/codecs/SBC.h>

#include "AudioEndpoint.h"
#include "ServiceDiscovery.h"
#include "SignallingServer.h"

namespace Thunder {

namespace Plugin {

    class BluetoothAudioSink : public Exchange::IBluetoothAudio::ISink
                             , public Exchange::IBluetoothAudio::IStream
                             , public Exchange::IBluetoothAudio::ISink::IControl
                             , public A2DP::AudioEndpoint::IHandler
                             , public SignallingServer::IObserver {
    public:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , SEID(1)
                , Latency(0)
                , Codecs()
            {
                Add(_T("seid"), &SEID);
                Add(_T("latency"), &Latency);
                Add(_T("codecs"), &Codecs);
            }
            ~Config() = default;

        public:
            Core::JSON::DecUInt8 SEID;
            Core::JSON::DecUInt16 Latency;
            Core::JSON::String Codecs;
        }; // class Config

    private:
        class A2DPSink {
        private:
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

        private:
            // This is the AVDTP transport channel.
            class Transport : public Bluetooth::RTP::ClientSocket {
            private:
                // Payload type should be a value from the dynamic range (96-127).
                // Typically 96 is chosen for A2DP implementations.
                static constexpr uint8_t PAYLOAD_TYPE = 96;

                static constexpr uint16_t OpenTimeout = 500; // ms
                static constexpr uint16_t CloseTimeout = 5000;
                static constexpr uint16_t PacketTimeout = 25;

            public:
                Transport() = delete;
                Transport(const Transport&) = delete;
                Transport& operator=(const Transport&) = delete;

                Transport(A2DPSink& parent, const Core::NodeId& localNode, const Core::NodeId& remoteNode)
                    : Bluetooth::RTP::ClientSocket(localNode, remoteNode)
                    , _parent(parent)
                    , _lock()
                    , _codec(nullptr)
                    , _ssrc(0)
                    , _timestamp(0)
                    , _sequence(0)
                    , _clockRate(0)
                    , _channels(1)
                    , _resolution(16)
                {
                }
                ~Transport()
                {
                    Disconnect();
                }

            private:
                uint32_t Initialize() override
                {
                    return (Core::ERROR_NONE);
                }
                void Operational(const bool upAndRunning) override
                {
                    if (upAndRunning == true) {
                        uint32_t flushable = 1;

                        TRACE(SinkFlow, (_T("Bluetooth A2DP/RTP transport channel is now operational")));

                        if (::setsockopt(Handle(), SOL_BLUETOOTH, BT_FLUSHABLE, &flushable, sizeof(flushable)) < 0) {
                            TRACE(Trace::Error, (_T("Failed to set the RTP socket flushable")));
                        }
                    }
                    else {
                        TRACE(SinkFlow, (_T("Bluetooth A2DP/RTP transport channel is not operational")));
                    }
                }

            public:
                uint32_t Connect(const Core::NodeId& remoteNode, Bluetooth::A2DP::IAudioCodec* codec)
                {
                    RemoteNode(remoteNode);

                    ASSERT(codec != nullptr);
                    _codec = codec;

                    uint32_t result = Open(OpenTimeout);

                    if (result != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to open A2DP/RTP transport socket [%d]"), result));
                        _codec = nullptr;
                    }
                    else {
                        Reset();
                        TRACE(SinkFlow, (_T("Successfully opened A2DP/RTP transport socket")));
                    }

                    return (result);
                }
                uint32_t Disconnect()
                {
                    uint32_t result = Core::ERROR_NONE;

                    if (IsOpen() == true) {
                        result = Close(CloseTimeout);

                        if (result != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to close AVDTP/RTP transport socket [%d]"), result));
                        }
                        else {
                            TRACE(SinkFlow, (_T("Successfully closed AVDTP/RTP transport socket")));
                        }
                    }

                    return (result);
                }

            public:
                uint32_t Timestamp() const
                {
                    return (_timestamp);
                }
                uint32_t ClockRate() const
                {
                    return (_clockRate);
                }
                uint8_t Channels() const
                {
                    return (_channels);
                }
                uint8_t BytesPerSample() const
                {
                    return (_resolution);
                }
                uint16_t MinFrameSize() const
                {
                    ASSERT(_codec != nullptr);

                    return (_codec->RawFrameSize());
                }
                uint16_t PreferredFrameSize() const
                {
                    ASSERT(_codec != nullptr);

                    return ((OutputMTU() / _codec->EncodedFrameSize()) * _codec->RawFrameSize());
                }
                void Reset()
                {
                    ASSERT(_codec != nullptr);

                    ResetOdometer();

                    // Ideally the sequence should start with a random value...
                    _sequence = (Core::Time::Now().Ticks() & 0xFFFF);

                    Bluetooth::A2DP::IAudioCodec::StreamFormat format;
                    string _;

                    _codec->Configuration(format, _);

                    _clockRate = format.SampleRate;
                    _channels = format.Channels;
                    _resolution = (format.Resolution / 8);

                    ASSERT(_clockRate != 0);
                    ASSERT(_resolution != 0);
                    ASSERT((_channels == 1) || (_channels == 2));

                    TRACE(SinkFlow, (_T("Resetting transport: clock rate: %d, channels: %d, resolution: %d bits per sample"), _clockRate, _channels, format.Resolution));
                }
                void ResetOdometer()
                {
                    _timestamp = 0;
                }
                uint32_t Transmit(const uint16_t length /* in bytes! */, const uint8_t data[])
                {
                    ASSERT(_codec != nullptr);

                    uint32_t consumed = 0;

                    Bluetooth::RTP::OutboundMediaPacketType<PAYLOAD_TYPE> packet(OutputMTU(), _ssrc, _sequence, _timestamp);

                    consumed = packet.Ingest(*_codec, data, length);

                    if (consumed > 0) {
                        uint32_t result = Exchange(PacketTimeout, packet);

                        if (result != Core::ERROR_NONE) {
                            // For development only, do not change the printf to TRACE!
                            fprintf(stderr, "BluetoothAudioSink: Failed to send out media packet (%d)\n", result);
                        }

                        // Timestamp clock frequency is the same as the sampling frequency.
                        _timestamp += (consumed / (Channels() * BytesPerSample()));

                        _sequence++;
                    }

                    return (consumed);
                }

            private:
                A2DPSink& _parent;
                Core::CriticalSection _lock;
                Bluetooth::A2DP::IAudioCodec* _codec;
                uint8_t _ssrc;
                uint32_t _timestamp;
                uint16_t _sequence;
                uint16_t _clockRate;
                uint8_t _channels;
                uint8_t _resolution;
            }; // class Transport

        private:
            class AudioPlayer : public Core::Thread {
            private:
                static constexpr uint16_t WRITE_AHEAD_THRESHOLD = 10 /* miliseconds */;

            private:
                class ReceiveBuffer : public Core::SharedBuffer {
                public:
                    ReceiveBuffer() = delete;
                    ReceiveBuffer(const ReceiveBuffer&) = delete;
                    ReceiveBuffer& operator=(const ReceiveBuffer&) = delete;

                    ReceiveBuffer(const string& name)
                        : Core::SharedBuffer(name.c_str())
                    {
                    }

                    ~ReceiveBuffer() = default;

                public:
                    uint32_t Get(const uint32_t length, uint8_t buffer[])
                    {
                        uint32_t result = 0;

                        if (IsValid() == true) {
                            if (RequestConsume(100) == Core::ERROR_NONE) {
                                ASSERT(BytesWritten() <= length);

                                const uint32_t size = std::min(BytesWritten(), length);
                                ::memcpy(buffer, Buffer(), size);
                                result = size;

                                Consumed();
                            }
                        }

                        return (result);
                    }
                }; // class ReceiveBuffer

            public:
                AudioPlayer() = delete;
                AudioPlayer(const AudioPlayer&) = delete;
                AudioPlayer& operator=(const AudioPlayer&) = delete;

            public:
                AudioPlayer(Transport& transport, const string& connector)
                    : _transport(transport)
                    , _startTime(0)
                    , _offset(0)
                    , _available(0)
                    , _minFrameSize(0)
                    , _maxFrameSize(0)
                    , _preferredFrameSize(0)
                    , _receiveBuffer(connector)
                    , _buffer(nullptr)
                    , _readCursor(nullptr)
                    , _bufferSize(0)
                    , _latency(0)
                    , _eos(false)
                {
                    TRACE(SinkFlow, (_T("Using shared buffer '%s'"), connector.c_str()));

                    if (_receiveBuffer.IsValid() == true) {

                        _maxFrameSize = _receiveBuffer.Size();
                        ASSERT(_maxFrameSize != 0);

                        if (transport.IsOpen() == true) {

                            _minFrameSize = _transport.MinFrameSize();
                            _preferredFrameSize = _transport.PreferredFrameSize();

                            ASSERT(_minFrameSize != 0);
                            ASSERT(_preferredFrameSize != 0);
                            ASSERT(_minFrameSize <= _preferredFrameSize);

                            const uint32_t multiplier = (((2 * _preferredFrameSize) / _maxFrameSize) + 1);
                            _bufferSize = (multiplier * _maxFrameSize);
                            _buffer = static_cast<uint8_t*>(::malloc(_bufferSize));

                            ASSERT(_buffer != nullptr);

                            TRACE(SinkFlow, ("Player initialized"));
                        }
                        else {
                            TRACE(SinkFlow, ("Player not initialized, transport channel is closed!"));
                        }
                    }

                    if (_maxFrameSize == 0) {
                        TRACE(Trace::Error, (_T("Shared buffer not available")));
                    }
                }
                ~AudioPlayer()
                {
                    Stop();
                    ::free(_buffer);
                    TRACE(SinkFlow, ("Player closed"));
                }

            public:
                bool IsValid() const
                {
                    return ((_buffer != nullptr) && (_receiveBuffer.IsValid() == true) && (_transport.IsOpen() == true));
                }
                uint32_t Play()
                {
                    uint32_t result = Core::ERROR_NONE;

                    if (IsValid() == true) {
                        _readCursor = _buffer;
                        _available = 0;
                        _startTime = Core::Time::Now().Ticks();
                        _eos = false;
                        Thread::Run();
                    }
                    else {
                        TRACE(Trace::Error, (_T("Transport channel is not open!")));
                        result = Core::ERROR_ILLEGAL_STATE;
                    }

                    return (result);
                }
                uint32_t Stop()
                {
                    uint32_t result = Core::ERROR_NONE;

                    if (IsValid() == true) {
                        _offset += PlayTime();
                        _transport.Reset();
                        _eos = true;
                        Thread::Wait(BLOCKED, Core::infinite);
                    }
                    else {
                        result = Core::ERROR_ILLEGAL_STATE;
                    }

                    return (result);
                }
                uint32_t Time(uint32_t& time /* milliseconds */) const
                {
                    uint32_t result = Core::ERROR_NONE;

                    if (IsValid() == true) {
                        time = (_offset + ((1000ULL * _transport.Timestamp()) / _transport.ClockRate()));
                    }
                    else {
                        result = Core::ERROR_BAD_REQUEST;
                        time = 0;
                    }

                    return (result);
                }
                void Latency(const uint16_t latency /* ms */)
                {
                    ASSERT(latency <= 20000); // makes sure we don't overflow...

                    _latency = ((_transport.ClockRate() * _transport.Channels() * latency) / 1000);

                    TRACE(Trace::Information, (_T("Latency adjustment: +%i ms (%i samples)"), latency, _latency));
                }
                uint32_t Delay(uint32_t& delay /* samples */) const
                {
                    uint32_t result = Core::ERROR_NONE;

                    if (IsValid() == true) {
                        delay = (_latency + (_available) / (_transport.BytesPerSample())); // counting in samples here
                    }
                    else {
                        result = Core::ERROR_BAD_REQUEST;
                    }

                    return (result);
                }

            private:
                uint32_t PlayTime() const
                {
                    return ((1000ULL * _transport.Timestamp()) / _transport.ClockRate());
                }

                uint32_t Worker() override
                {
                    uint32_t delay = 0;
                    uint32_t transmitted = 0;

                    if ((_available < _minFrameSize) && (_eos != true)) {
                        // Have to replenish the local buffer...
                        // TODO: optimization opportunity
                        ::memmove(_buffer, _readCursor, _available);

                        // Make sure we have at least the encoder preferred frame size available.
                        while (_available < _preferredFrameSize) {
                            ASSERT(_available + _maxFrameSize <= _bufferSize);

                            const uint16_t bytesRead = _receiveBuffer.Get(_maxFrameSize, (_buffer + _available));
                            ASSERT(bytesRead <= _maxFrameSize);

                            _available += bytesRead;

                            if (bytesRead == 0) {
                                // The audio source has problem providing more data at the moment...
                                // We don't have the preferred data available, let's try to play out whatever there is left in the buffer anyway.
                                if (_available < _minFrameSize) {
                                    // All data was used and no new is currently available, apparently the source has stalled.
                                    _offset += PlayTime();
                                    _startTime = Core::Time::Now().Ticks();
                                    _transport.ResetOdometer();

                                    // Give them a break...
                                    ::SleepMs(10);
                                }
                                break;
                            }
                        }

                        _readCursor = _buffer;
                    }

                    if (_transport.IsOpen() == true) {
                        const uint32_t playTime = PlayTime();
                        const uint32_t elapsedTime = ((Core::Time::Now().Ticks() - _startTime) / 1000);

                        if ((playTime > WRITE_AHEAD_THRESHOLD) && (playTime - WRITE_AHEAD_THRESHOLD > elapsedTime)) {
                            // We're writing ahead of time, let's wait a bit, so the device's buffer does not overflow.
                            ::SleepMs(playTime - elapsedTime);
                        }

                        if (_available >= _minFrameSize) {
                            transmitted = _transport.Transmit(_available, _readCursor);

                            #ifdef __DEBUG__
                                // For development only, do not change the printf to TRACE!
                                fprintf(stderr, "streaming; time %3i.%03i / %3i.%03i sec; delta %3i ms, frame %i bytes  \r",
                                    (playTime / 1000), (playTime % 1000), (elapsedTime / 1000), (elapsedTime % 1000), (playTime - elapsedTime), transmitted);
                            #endif

                            _readCursor += transmitted;
                            _available -= transmitted;
                        }
                    }
                    else {
                        TRACE(Trace::Error, (_T("Bluetooth transport link failure - terminating audio stream")));
                        _eos = true;
                    }

                    if ((transmitted == 0) && (_eos == true)) {
                        // Played out everything in the buffer and end-of-stream was signalled.
                        Thread::Block();
                        delay = Core::infinite;
                    }

                    return (delay);
                }

            private:
                Transport& _transport;
                uint64_t _startTime;
                uint32_t _offset;
                uint32_t _available;
                uint32_t _minFrameSize;
                uint32_t _maxFrameSize;
                uint32_t _preferredFrameSize;
                ReceiveBuffer _receiveBuffer;
                uint8_t* _buffer;
                uint8_t* _readCursor;
                uint32_t _bufferSize;
                uint32_t _latency; // in samples
                bool _eos;
            }; // class Transport

            // This is the AVDTP signalling channel.
            class Signalling {
            private:
                static constexpr uint16_t OpenTimeoutMs = 5000;
                static constexpr uint16_t CloseTimeoutMs = 5000;

            public:
                Signalling() = delete;
                Signalling(const Signalling&) = delete;
                Signalling& operator=(const Signalling&) = delete;

                Signalling(A2DPSink& parent, Exchange::IBluetooth::IDevice* device)
                    : _lock()
                    , _parent(parent)
                    , _device(device)
                    , _channel()
                    , _remote(nullptr)
                    , _psm(0)
                {
                    ASSERT(_device != nullptr);

                    _device->AddRef();
                }
                ~Signalling()
                {
                    Disconnect();

                    _device->Release();
                }

            public:
                uint8_t PSM() const
                {
                    // TODO: This should be extractble from the channel node.
                    ASSERT(_psm != 0);
                    return (_psm);
                }

            public:
                void Discover(std::list<A2DP::AudioEndpointData>& endpoints)
                {
                    // Look for AUDIO/SINK endpoints only!

                    _remote.Discover([this, &endpoints](A2DP::AudioEndpointData&& sep) {

                        #ifdef __DEBUG__
                            TRACE(SinkFlow, (_T("%s"), sep.AsString().c_str()));

                            if (sep.Capabilities().empty() == false) {
                                TRACE(SinkFlow, (_T("Capabilities:")));

                                for (auto const& caps : sep.Capabilities()) {
                                    TRACE(SinkFlow, (_T(" - %s"), caps.second.AsString().c_str()));
                                }
                            }
                        #endif // __DEBUG__

                        if ((sep.MediaType() == Bluetooth::AVDTP::StreamEndPoint::AUDIO)
                            && (sep.Type() == Bluetooth::AVDTP::StreamEndPoint::SINK)) {

                            endpoints.emplace_back(std::move(sep));
                        }
                    });

                    TRACE(SinkFlow, (_T("Stream endpoint discovery complete, found %d audio sink endpoint(s)"), endpoints.size()));
                }

            public:
                void Claim(const Core::ProxyType<SignallingServer::PeerConnection>& connection)
                {
                    // Reuse the existing AVDTP connection initiated by the sink device.

                    ASSERT(connection.IsValid() == true);

                    _lock.Lock();

                    if (_channel.IsValid() == false) {
                        _channel = connection;
                    }

                    _lock.Unlock();
                }

            public:
                uint32_t Connect(const uint8_t psm)
                {
                    uint32_t result = Core::ERROR_UNAVAILABLE;

                    _lock.Lock();

                    if (_channel.IsValid() == false) {
                        // Seems we are initiating the connection.
                        _channel = SignallingServer::Instance().Create(Designator(_device), Designator(_device, psm));
                    }

                    _lock.Unlock();

                    // Ensure the connection is open (establish it or reuse existing).

                    if (_channel.IsValid() == false) {
                        TRACE(Trace::Error, (_T("Failed to open signalling channel!")));
                    }
                    else {
                        if (_channel->IsOpen() == false) {

                            TRACE(SinkFlow, (_T("Opening A2DP/AVDTP signalling channel...")));
                            result = _channel->Open(OpenTimeoutMs);

                            if (result != Core::ERROR_NONE) {
                                TRACE(Trace::Error, (_T("Failed to open A2DP/AVDTP signalling socket [%d]"), result));
                            }
                        }

                        if (_channel->IsOpen() == true) {
                            result = Core::ERROR_NONE;
                            _remote.Channel(&(*_channel));
                        }

                        _psm = psm;
                    }

                    return (result);
                }
                uint32_t Disconnect()
                {
                    uint32_t result = Core::ERROR_NONE;

                    if (_channel.IsValid() == true) {
                        if (_channel->IsOpen() == true) {

                            TRACE(SinkFlow, (_T("Closing A2DP/AVDTP signalling channel...")));
                            result = _channel->Close(CloseTimeoutMs);

                            if (result != Core::ERROR_NONE) {
                                TRACE(Trace::Error, (_T("Failed to close A2DP/AVDTP signalling socket [%d]"), result));
                            }
                        }

                        _channel.Release();
                    }

                    return (result);
                }

            public:
                void Assign(A2DP::AudioEndpoint* endpoint)
                {
                    ASSERT(endpoint != nullptr);

                    _lock.Lock();

                    ASSERT(_channel.IsValid() == true);

                    // Ensure the connection is not closed by the inactivity monitor.
                    _channel.AddRef();
                    _channel->Latch(true);

                    endpoint->ClientChannel(&(*_channel));

                    _lock.Unlock();
                }
                void Unassign(A2DP::AudioEndpoint* endpoint)
                {
                    _lock.Lock();

                    if (_channel.IsValid() == true) {
                        _channel->Latch(false);
                        _channel.Release();
                    }

                    if (endpoint != nullptr) {
                        endpoint->ClientChannel(nullptr);
                    }

                    _lock.Unlock();
                }

            private:
                Core::CriticalSection _lock;
                A2DPSink& _parent;
                Exchange::IBluetooth::IDevice* _device;
                Core::ProxyType<SignallingServer::PeerConnection> _channel;
                Bluetooth::AVDTP::Client _remote;
                uint8_t _psm;
            }; // class Signalling

        public:
            using AudioServiceData = ServiceDiscovery::AudioService::Data;

        public:
            A2DPSink(const A2DPSink&) = delete;
            A2DPSink& operator=(const A2DPSink&) = delete;
            A2DPSink(BluetoothAudioSink* parent, const string& codecSettings, Exchange::IBluetooth::IDevice* device)
                : _lock()
                , _parent(*parent)
                , _callback(*this)
                , _discoveryJob()
                , _updateJob()
                , _cleanupJob()
                , _codecSettings(codecSettings)
                , _discovery(Designator(device), Designator(device, Bluetooth::SDP::PSM /* a well-known PSM */))
                , _signalling(*this, device)
                , _transport(*this, Designator(device), Designator(device, 0 /* yet unknown PSM */))
                , _device(device)
                , _endpoint(nullptr)
                , _state(Exchange::IBluetoothAudio::UNASSIGNED)
                , _type(Exchange::IBluetoothAudio::ISink::UNKNOWN)
                , _codecList()
                , _drmList()
                , _latency(0)
                , _player()
            {
                ASSERT(parent != nullptr);
                ASSERT(device != nullptr);

                State(Exchange::IBluetoothAudio::DISCONNECTED);

                _device->AddRef();

                // TODO: This is a workaround, if the source compoonent is already watching the device.
                // Since we know this is the user-assigned device, we take priority.
                _device->Callback(static_cast<Exchange::IBluetooth::IDevice::ICallback*>(nullptr));

                // Get notified when the assign device connects/disconnects.
                if (_device->Callback(&_callback) != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("The device is already in use")));
                }

                if (_device->IsConnected() == true) {
                    TRACE(Trace::Error, (_T("The device is already connected")));
                    OnDeviceUpdated();
                }
            }
            A2DPSink(BluetoothAudioSink* parent, const string& codecSettings, Exchange::IBluetooth::IDevice* device, 
                        const AudioServiceData& data VARIABLE_IS_NOT_USED)
                : A2DPSink(parent, codecSettings, device)
            {
                // TODO: Use cached data to speed up startup?
                ASSERT(_device != nullptr);

                _device->Connect(); // == enable autoconnect
            }
            ~A2DPSink()
            {
                ASSERT(_device != nullptr);

                Relinquish();

                if (_device->Callback(static_cast<Exchange::IBluetooth::IDevice::ICallback*>(nullptr)) != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Could not remove the callback from the device")));
                }

                _device->Release();

                _parent.OnStateChanged(Exchange::IBluetoothAudio::DISCONNECTED);
            }

        public:
            bool IsReadable() const
            {
               return((_state == Exchange::IBluetoothAudio::CONNECTED)
                    || (_state == Exchange::IBluetoothAudio::CONNECTED_BAD)
                    || (_state == Exchange::IBluetoothAudio::CONNECTED_RESTRICTED)
                    || (_state == Exchange::IBluetoothAudio::READY)
                    || (_state == Exchange::IBluetoothAudio::STREAMING));
            }

        public:
            const string Address() const
            {
                ASSERT(_device != nullptr);
                return (_device->RemoteId());
            }
            Exchange::IBluetoothAudio::state State() const
            {
                return (_state);
            }
            Exchange::IBluetoothAudio::ISink::devicetype Type() const
            {
                return (_type);
            }
            const std::list<Exchange::IBluetoothAudio::audiocodec>& Codecs() const
            {
                return (_codecList);
            }
            const std::list<Exchange::IBluetoothAudio::drmscheme>& DRMs() const
            {
                return (_drmList);
            }
            uint32_t Latency() const
            {
                return (_latency);
            }

        public:
            uint32_t Time(uint32_t& timestamp) const
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_player != nullptr) {
                    result = _player->Time(timestamp);
                }
                else {
                    TRACE(Trace::Error, (_T("Player not configured")));
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t Latency(const int16_t latency /* ms */)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                const int32_t hostLatency = _parent.ControllerLatency();

                if ((latency >= -hostLatency) && (latency <= 10000)) {

                    _lock.Lock();

                    _latency = latency;

                    if (_player != nullptr) {
                        _player->Latency(hostLatency + _latency);
                        result = Core::ERROR_NONE;
                    }
                    else {
                        TRACE(Trace::Error, (_T("Player not configured")));
                    }

                    _lock.Unlock();
                }
                else {
                    TRACE(Trace::Error, (_T("Invalid latency setting")));
                    result = Core::ERROR_BAD_REQUEST;
                }

                return (result);
            }
            uint32_t Delay(uint32_t& delay /* samples */) const
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_player != nullptr) {
                    result = _player->Delay(delay);
                }
                else {
                    TRACE(Trace::Error, (_T("Player not configured")));
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t Codec(Exchange::IBluetoothAudio::CodecProperties& properties) const
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_endpoint != nullptr) {
                    ASSERT(_endpoint->Codec() != nullptr);

                    switch (_endpoint->Codec()->Type()) {
                    case Bluetooth::A2DP::SBC::CODEC_TYPE:
                        properties.Codec = Exchange::IBluetoothAudio::LC_SBC;
                        break;
                    default:
                        ASSERT(!"Invalid codec");
                        break;
                    }

                    Bluetooth::A2DP::IAudioCodec::StreamFormat _;

                    _endpoint->Codec()->Configuration(_, properties.Settings);

                    result = Core::ERROR_NONE;
                }
                else {
                    TRACE(Trace::Error, (_T("Audio sink device not connected")));
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t DRM(Exchange::IBluetoothAudio::DRMProperties& properties) const
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_endpoint != nullptr) {
                    if (_endpoint->ContentProtection() != nullptr) {
                        switch (_endpoint->ContentProtection()->Type()) {
                        case Bluetooth::A2DP::IAudioContentProtection::SCMS_T:
                            properties.DRM = Exchange::IBluetoothAudio::SCMS_T;
                            break;
                        }

                        _endpoint->ContentProtection()->Configuration(properties.Settings);

                        result = Core::ERROR_NONE;
                    }
                    else {
                        TRACE(Trace::Error, (_T("Copy protection not available")));
                        result = Core::ERROR_UNAVAILABLE;
                    }
                }
                else {
                    TRACE(Trace::Error, (_T("Audio sink device not connected")));
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t Stream(Exchange::IBluetoothAudio::StreamProperties& properties) const
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_endpoint != nullptr) {
                    ASSERT(_endpoint->Codec() != nullptr);

                    string _;
                    Bluetooth::A2DP::IAudioCodec::StreamFormat format;
                    _endpoint->Codec()->Configuration(format, _);

                    properties.BitRate = _endpoint->Codec()->BitRate();
                    properties.SampleRate = format.SampleRate;
                    properties.Resolution = format.Resolution;
                    properties.Channels = format.Channels;

                    result = Core::ERROR_NONE;
                }
                else {
                    TRACE(Trace::Error, (_T("Audio sink device not connected")));
                }

                _lock.Unlock();

                return (result);
            }

        public:
            uint32_t SetConfiguration(const Exchange::IBluetoothAudio::IStream::Format& format)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_endpoint == nullptr) {
                    TRACE(Trace::Error, (_T("Audio sink device not connected")));
                }
                else if (_player != nullptr) {
                    TRACE(Trace::Error, (_T("Bluetooth sink device already in use")));
                    result = Core::ERROR_UNAVAILABLE;
                }
                else {
                    TRACE(SinkFlow, (_T("Configuring audio endpoint 0x%02x to: sample rate: %d Hz, resolution: %d bits per sample, channels: %d, frame rate: %d.%02d Hz"), 
                            _endpoint->Id(), format.SampleRate, format.Resolution, format.Channels, (format.FrameRate / 100), (format.FrameRate % 100)));

                    Bluetooth::A2DP::IAudioCodec::StreamFormat streamFormat;
                    streamFormat.SampleRate = format.SampleRate;
                    streamFormat.FrameRate = format.FrameRate;
                    streamFormat.Resolution = format.Resolution;
                    streamFormat.Channels = format.Channels;

                    if ((result = _endpoint->Configure(streamFormat, _codecSettings)) != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to configure sink device endpoint")));
                    }
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t OnSetConfiguration(A2DP::AudioEndpoint& ep VARIABLE_IS_NOT_USED, Bluetooth::AVDTP::Socket* channel VARIABLE_IS_NOT_USED)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_endpoint == nullptr) {
                    TRACE(Trace::Error, (_T("Audio sink device not connected")));
                    // This also blocks the incomming session establishment attempt, so we can take over.
                }
                else if (_player != nullptr) {
                    TRACE(Trace::Error, (_T("Bluetooth sink device already in use")));
                    result = Core::ERROR_UNAVAILABLE;
                }
                else {
                    result = Core::ERROR_NONE;
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t Acquire(const string& connector)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_endpoint == nullptr) {
                    TRACE(Trace::Error, (_T("Audio sink device not connected")));
                }
                else if (_player != nullptr) {
                    TRACE(Trace::Error, (_T("Bluetooth sink device already in use")));
                    result = Core::ERROR_UNAVAILABLE;
                }
                else {
                    _connector = connector;

                    if ((result = _endpoint->Open()) != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to open sink device endpoint")));
                    }
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t OnAcquire()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (_endpoint == nullptr) {
                    TRACE(Trace::Error, (_T("Audio sink device not connected")));
                }
                else {
                    ASSERT(_endpoint->Codec() != nullptr);

                    // Once the session is open, yet another L2CAP connection is created for data transport.

                    if (_transport.Connect(Designator(_device, _signalling.PSM()), _endpoint->Codec()) != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to open transport channel")));
                        result = Core::ERROR_OPENING_FAILED;
                    }
                    else {
                        _player.reset(new (std::nothrow) AudioPlayer(_transport, _connector));
                        ASSERT(_player.get() != nullptr);

                        if (_player->IsValid() == true) {
                            TRACE(SinkFlow, (_T("Successfully created Bluetooth audio playback session")));

                            _player->Latency(_parent.ControllerLatency() + _latency);

                            State(Exchange::IBluetoothAudio::READY);
                            result = Core::ERROR_NONE;
                        }
                        else {
                            TRACE(Trace::Error, (_T("Failed to create audio player")));
                            _player.reset();
                            _transport.Disconnect();

                            result = Core::ERROR_OPENING_FAILED;
                        }
                    }
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t SetSpeed(const int8_t speed)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if ((_endpoint != nullptr) && (_player != nullptr)) {
                    if (speed == 100) {
                        if (_state == Exchange::IBluetoothAudio::READY) {

                            result = _endpoint->Start();
                            if (result != Core::ERROR_NONE) {
                                TRACE(Trace::Error, (_T("Failed to start endpoint!")));
                            }
                        }
                        else if (_state == Exchange::IBluetoothAudio::STREAMING) {
                            // Allow 100% to 100% setting.
                            result = Core::ERROR_NONE;
                        }
                    }
                    else if (speed == 0) {
                        if (_state == Exchange::IBluetoothAudio::STREAMING) {

                            result = _endpoint->Suspend();
                            if (result != Core::ERROR_NONE) {
                                TRACE(Trace::Error, (_T("Failed to suspend endpoint!")));
                            }
                        }
                        else if (_state == Exchange::IBluetoothAudio::READY) {
                            // Allow 0% to 0% setting.
                            result = Core::ERROR_NONE;
                        }
                    }
                    else {
                        TRACE(Trace::Error, (_T("Unsupported speed setting")));
                        result = Core::ERROR_NOT_SUPPORTED;
                    }
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t OnSetSpeed(const int8_t speed)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if (speed == 100) {
                    result = _player->Play();

                    if (result == Core::ERROR_NONE) {
                        TRACE(SinkFlow, (_T("Starting Bluetooth audio playback")));
                        State(Exchange::IBluetoothAudio::STREAMING);
                    }
                }
                else if (speed == 0) {
                    result = _player->Stop();

                    if (result == Core::ERROR_NONE) {
                        TRACE(SinkFlow, (_T("Stopped Bluetooth audio playback")));
                        State(Exchange::IBluetoothAudio::READY);
                    }
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t Relinquish()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if ((_endpoint != nullptr) && (_player != nullptr)) {

                    result = _endpoint->Close();

                    if (result != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to close sink device endpoint")));
                    }
                }

                _lock.Unlock();

                return (result);
            }
            uint32_t OnRelinquish()
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;

                _lock.Lock();

                if ((_endpoint != nullptr) && (_player != nullptr)) {
                    _player.reset();

                    TRACE(SinkFlow, (_T("Closed Bluetooth audio session")));

                    _transport.Disconnect();

                    if (_device->IsConnected() == true) {
                        State(Exchange::IBluetoothAudio::CONNECTED);
                    }

                    result = Core::ERROR_NONE;
                }

                _lock.Unlock();

                return (result);
            }

        private:
            void State(const Exchange::IBluetoothAudio::state state)
            {
                bool updated = false;

                _lock.Lock();

                if (_state != state) {
                    _state = state;
                    updated = true;
                }

                _lock.Unlock();

                if (updated == true) {
                    Core::EnumerateType<Exchange::IBluetoothAudio::state> value(_state);
                    TRACE(Trace::Information, (_T("Bluetooth audio sink state: %s"), (value.IsSet() ? value.Data() : "(undefined)")));

                    _updateJob.Submit([this, state]() {
                        _parent.OnStateChanged(state);
                    });
                }
            }

        private:
            // Callbacks from IBluetooth
            void OnDeviceUpdated()
            {
                ASSERT(_device != nullptr);

                _lock.Lock();

                if (_device->IsPaired() == true) {
                    if (_device->IsConnected() == true) {
                        OnDeviceConnected();
                    }
                    else {
                        OnDeviceDisconnected();
                    }
                }

                _lock.Unlock();
            }
            void OnDeviceConnected()
            {
                _lock.Lock();

                if (_state == Exchange::IBluetoothAudio::DISCONNECTED) {

                    State(Exchange::IBluetoothAudio::CONNECTING);

                    TRACE(Trace::Information, (_T("Bluetooth audio sink device connecting...")));

                    _discoveryJob.Submit([this]() {
                        DiscoverAudioServices();
                    });
                }

                _lock.Unlock();
            }
            void OnDeviceDisconnected()
            {
                TRACE(Trace::Information, (_T("Bluetooth audio sink device disconnected")));

                _lock.Lock();

                // Ensure everything is cleaned up, in case the remote device didn't do it.
                _player.reset();

                _transport.Disconnect();
                _signalling.Disconnect();
                _discovery.Disconnect();
                _signalling.Unassign(_endpoint);

                _endpoint = nullptr;

                _lock.Unlock();

                State(Exchange::IBluetoothAudio::DISCONNECTED);
            }

        public:
            // Callbacks from the SignallingSever
            void SignallingConnected(const Core::ProxyType<SignallingServer::PeerConnection>& connection)
            {
                _signalling.Claim(connection);
            }
            void SignallingDisconnected()
            {
                if (_endpoint != nullptr) {
                    A2DP::AudioEndpoint* endpoint = _endpoint;

                    _cleanupJob.Submit([this, endpoint]() {
                        endpoint->Disconnected();
                    });
                }
            }

        private:
            void DiscoverAudioServices()
            {
                ServiceDiscovery::AudioService* service = nullptr;
                uint8_t sbcEndpoint = ~0;

                ASSERT(_endpoint == nullptr);

                // Let's see if the device features audio sink services...
                if (_discovery.Connect() == Core::ERROR_NONE) {

                    std::list<ServiceDiscovery::AudioService> audioServices;

                    if (_discovery.Discover({ Bluetooth::SDP::ClassID::AudioSink }, audioServices) == Core::ERROR_NONE) {

                        // Service discovery connection no longer needed!
                        _discovery.Disconnect();

                        if (audioServices.empty() == false) {
                            // Disregard possibility of multiple sink services for now.
                            service = &audioServices.front();

                            if (audioServices.size() > 1) {
                                TRACE(SinkFlow, (_T("More than one audio sink available, using the first one!")));
                            }

                            TRACE(SinkFlow, (_T("Audio sink service available: A2DP v%d.%d, AVDTP v%d.%d, L2CAP PSM: %i, features: 0b%s"),
                                    (service->ProfileVersion() >> 8), (service->ProfileVersion() & 0xFF), (service->TransportVersion() >> 8),
                                    (service->TransportVersion() & 0xFF), service->PSM(), std::bitset<8>(service->Features()).to_string().c_str()));

                            DiscoverAudioStreamEndpoints(service->PSM(), sbcEndpoint);
                        }
                    }
                    else {
                        _discovery.Disconnect();

                        TRACE(Trace::Error, (_T("Connected device does not host a valid audio sink service!")));
                    }
                }

                _lock.Lock();

                if (service != nullptr) {
                    if (sbcEndpoint != static_cast<uint8_t>(~0)) {

                        // This really looks like an audio sink device, so let's use it!
                        _type = DeviceType(service->Features());

                        // Match our endpoint with theirs.
                        _endpoint = Admin().Endpoint(Bluetooth::A2DP::IAudioCodec::LC_SBC);
                        ASSERT(_endpoint != nullptr);
                        _endpoint->RemoteId(sbcEndpoint);

                        _signalling.Assign(_endpoint);

                        // Maybe this is a new device, remember it.
                        AudioServiceData settings;
                        service->Settings(settings);
                        _parent.Operational(settings);
                    }
                }

                if (_endpoint == nullptr) {
                    State(Exchange::IBluetoothAudio::CONNECTED_BAD);
                }
                else {
                    State(Exchange::IBluetoothAudio::CONNECTED);
                }

                _lock.Unlock();
            }

        private:
            void DiscoverAudioStreamEndpoints(const uint8_t psm, uint8_t& sbcEndpoint)
            {
                sbcEndpoint = ~0;

                // Let's see what endpoints does the sink have...
                if (_signalling.Connect(psm) == Core::ERROR_NONE) {

                    std::list<A2DP::AudioEndpointData> audioEndpoints;

                    _signalling.Discover(audioEndpoints);

                    // Endpoints discovery is complete, but the AVDTP signalling channel remains open!

                    if (audioEndpoints.empty() == false) {

                        // This is a shortcut, the stack currently only supports SBC codec,
                        // so we look for SBC endpoint on the remote device.

                        for (auto& ep : audioEndpoints) {
                            if (ep.Codec() != nullptr) {
                                switch (ep.Codec()->Type()) {
                                case Bluetooth::A2DP::IAudioCodec::LC_SBC:
                                    sbcEndpoint = ep.Id();
                                    _codecList.push_back(Exchange::IBluetoothAudio::LC_SBC);
                                    break;
                                default:
                                    ASSERT(!"Invalid codec");
                                    break;
                                }
                            }

                            if (ep.ContentProtection() != nullptr) {
                                switch (ep.ContentProtection()->Type()) {
                                case Bluetooth::A2DP::IAudioContentProtection::SCMS_T:
                                    _drmList.push_back(Exchange::IBluetoothAudio::SCMS_T);
                                    break;
                                default:
                                    ASSERT(!"Invalid content protection");
                                    break;
                                }
                            }
                        }

                        if (sbcEndpoint == static_cast<uint8_t>(~0)) {
                            // This should not be possible. :)
                            TRACE(Trace::Error, (_T("No supported codec available on the connected audio sink!")));
                        }
                    }
                    else {
                        TRACE(SinkFlow, (_T("No audio sink endpoints found")));
                    }
                }
            }

        private:
            BluetoothAudioSink& Admin()
            {
                return (_parent);
            }

        private:
            static Exchange::IBluetoothAudio::ISink::devicetype DeviceType(const ServiceDiscovery::AudioService::features features)
            {
                Exchange::IBluetoothAudio::ISink::devicetype type = Exchange::IBluetoothAudio::ISink::UNKNOWN;

                if (features & ServiceDiscovery::AudioService::features::HEADPHONE) {
                    type = Exchange::IBluetoothAudio::ISink::HEADPHONE;
                }
                else if (features & ServiceDiscovery::AudioService::features::SPEAKER) {
                    type = Exchange::IBluetoothAudio::ISink::SPEAKER;
                }
                else if (features & ServiceDiscovery::AudioService::features::RECORDER) {
                    type = Exchange::IBluetoothAudio::ISink::RECORDER;
                }
                else if (features & ServiceDiscovery::AudioService::features::AMPLIFIER) {
                    type = Exchange::IBluetoothAudio::ISink::AMPLIFIER;
                }

                return (type);
            }

        private:
            mutable Core::CriticalSection _lock;
            BluetoothAudioSink& _parent;
            Core::SinkType<DeviceCallback> _callback;
            DecoupledJob _discoveryJob;
            DecoupledJob _updateJob;
            DecoupledJob _cleanupJob;
            const string& _codecSettings;

            // Connection channels
            ServiceDiscovery _discovery;
            Signalling _signalling;
            Transport _transport;

            // Properties of the connected device
            Exchange::IBluetooth::IDevice* _device;
            A2DP::AudioEndpoint* _endpoint;

            // Status of the connected device
            Exchange::IBluetoothAudio::state _state;
            Exchange::IBluetoothAudio::ISink::devicetype _type;
            std::list<Exchange::IBluetoothAudio::audiocodec> _codecList;
            std::list<Exchange::IBluetoothAudio::drmscheme> _drmList;
            uint32_t _latency; // device latency

            std::unique_ptr<AudioPlayer> _player;
            string _connector;
        }; // class A2DPSink

    public:
        BluetoothAudioSink(const BluetoothAudioSink&) = delete;
        BluetoothAudioSink& operator=(const BluetoothAudioSink&) = delete;

        BluetoothAudioSink(PluginHost::JSONRPC* module)
            : _lock()
            , _jsonRpcModule(module)
            , _service(nullptr)
            , _cleanupJob()
            , _controller()
            , _controllerLatency(0)
            , _codecSettings()
            , _callback(nullptr)
            , _source(nullptr)
            , _sink()
        {
        }
        ~BluetoothAudioSink() override
        {
            _lock.Lock();

            if (_callback != nullptr) {
                _callback->Release();
            }

            if (_source != nullptr) {
                _source->Release();
            }

            if (_service != nullptr) {
                _service->Release();
            }

            _lock.Unlock();
        }

    public:
        string Initialize(PluginHost::IShell* service, const string& controller, const Config& config);
        void Deinitialize(PluginHost::IShell* service);

    public:
        // IBluetoothAudio:ISink overrides
        Core::hresult Assign(const string& device) override;
        Core::hresult Revoke() override;

        Core::hresult Callback(Exchange::IBluetoothAudio::ISink::ICallback* callback) override
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
        Core::hresult Latency(const int16_t latency) override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->Latency(latency);
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult Latency(int16_t& latency) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sink != nullptr) {
                latency = _sink->Latency();
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult State(Exchange::IBluetoothAudio::state& sinkState) const override
        {
            _lock.Lock();

            if (_sink != nullptr) {
                sinkState = _sink->State();
            }
            else {
                sinkState = Exchange::IBluetoothAudio::UNASSIGNED;
            }

            _lock.Unlock();

            return (Core::ERROR_NONE);
        }
        Core::hresult Device(string& address) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sink != nullptr) {
                address = _sink->Address();
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult Type(Exchange::IBluetoothAudio::ISink::devicetype& type) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sink != nullptr) {
                type = _sink->Type();
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult SupportedCodecs(Exchange::IBluetoothAudio::audiocodec& codecs) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if ((_sink != nullptr) && (_sink->IsReadable())) {

                using T = std::underlying_type<Exchange::IBluetoothAudio::audiocodec>::type;
                T intCodecs = 0;

                for (auto c : _sink->Codecs()) {
                    intCodecs |= c;
                }

                codecs = static_cast<Exchange::IBluetoothAudio::audiocodec>(intCodecs);
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult SupportedDRMs(Exchange::IBluetoothAudio::drmscheme& drms) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if ((_sink != nullptr) && (_sink->IsReadable())) {

                using T = std::underlying_type<Exchange::IBluetoothAudio::drmscheme>::type;
                T intDrms = 0;

                for (auto c : _sink->DRMs()) {
                    intDrms |= c;
                }

                drms = static_cast<Exchange::IBluetoothAudio::drmscheme>(intDrms);
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult Codec(Exchange::IBluetoothAudio::CodecProperties& info) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if ((_sink != nullptr) && (_sink->IsReadable())) {
                result = _sink->Codec(info);
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult DRM(Exchange::IBluetoothAudio::DRMProperties& info) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if ((_sink != nullptr) && (_sink->IsReadable())) {
                result = _sink->DRM(info);
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult Stream(Exchange::IBluetoothAudio::StreamProperties& info) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if ((_sink != nullptr) && (_sink->IsReadable())) {
                result = _sink->Stream(info);
            }

            _lock.Unlock();

            return (result);
        }

    public:
        // IStream overrides
        Core::hresult Configure(const Exchange::IBluetoothAudio::IStream::Format& format) override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->SetConfiguration(format);
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult Acquire(const string& connector) override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->Acquire(connector);
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult Relinquish() override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->Relinquish();
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult Speed(const int8_t speed) override
        {
            Core::hresult result = Core::ERROR_NONE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->SetSpeed(speed);
            }
            else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult Time(uint32_t& position) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->Time(position);
            }

            _lock.Unlock();

            return (result);
        }
        Core::hresult Delay(uint32_t& delay) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->Delay(delay);
            }

            _lock.Unlock();

            return (result);
        }

    public:
        // ISink::IControl overrides
        Core::hresult Source(Exchange::IBluetoothAudio::IStream* source) override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            _lock.Lock();

            if (source == nullptr) {
                if (_source != nullptr) {
                    _source->Release();
                    _source = nullptr;
                    result = Core::ERROR_NONE;
                }
            }
            else if (_source == nullptr) {
                _source = source;
                _source->AddRef();
                result = Core::ERROR_NONE;
            }

            _lock.Unlock();

            return (result);
        }

    public:
        A2DP::AudioEndpoint* Endpoint(const Bluetooth::A2DP::IAudioCodec::codectype codec)
        {
            A2DP::AudioEndpoint* ep = nullptr;

            _lock.Lock();

            auto it = _endpoints.find(codec);

            if (it != _endpoints.end()) {
                ep = SignallingServer::Instance().Endpoint((*it).second);
            }

            _lock.Unlock();

            return (ep);
        }

    public:
        Exchange::IBluetooth* Controller()
        {
            ASSERT(_service != nullptr);
            return (_service->QueryInterfaceByCallsign<Exchange::IBluetooth>(_controller));
        }
        uint16_t ControllerLatency() const
        {
            return (_controllerLatency);
        }

    public:
        void Revoked(const Core::IUnknown* remote, const uint32_t interfaceId VARIABLE_IS_NOT_USED)
        {
            // The COM-RPC link has been disconnected abruptly, clean up!

            if (remote == _callback) {
                TRACE(SinkFlow, (_T("Bluetooth audio sink remote client died; cleaning up the playback session on behalf of the dead")));

                Callback(nullptr);
                Source(nullptr);

                _cleanupJob.Submit([this]() {
                    Relinquish();
                });
            }
        }

    public:
        BEGIN_INTERFACE_MAP(BluetoothAudioSink)
        INTERFACE_ENTRY(Exchange::IBluetoothAudio::ISink)
        INTERFACE_ENTRY(Exchange::IBluetoothAudio::ISink::IControl)
        INTERFACE_ENTRY(Exchange::IBluetoothAudio::IStream)
        END_INTERFACE_MAP

    private:
        void OnStateChanged(const Exchange::IBluetoothAudio::state state)
        {
            Exchange::IBluetoothAudio::ISink::ICallback* callback = nullptr;

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

            ASSERT(_jsonRpcModule != nullptr);
            Exchange::BluetoothAudio::JSink::Event::StateChanged(*_jsonRpcModule, state);
        }

    private:
        // AudioEndpoint::IHandler overrides
        uint32_t OnSetConfiguration(A2DP::AudioEndpoint& ep, Bluetooth::AVDTP::Socket* channel) override
        {
            ASSERT(_sink != nullptr);
            return (_sink->OnSetConfiguration(ep, channel));
        }
        uint32_t OnAcquire() override
        {
            ASSERT(_sink != nullptr);
            return (_sink->OnAcquire());
        }
        uint32_t OnRelinquish() override
        {
            uint32_t result = Core::ERROR_NONE;

            Exchange::IBluetoothAudio::IStream* source = _source;

            _lock.Lock();

            if (_source != nullptr) {
                result = source->Relinquish();
            }

            ASSERT(_sink != nullptr);

            if (result == Core::ERROR_NONE) {
                result = _sink->OnRelinquish();
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t OnSetSpeed(const int8_t speed) override
        {
            uint32_t result = Core::ERROR_NONE;

            _lock.Lock();

            if (_source != nullptr) {
                result = _source->Speed(speed);
            }

            ASSERT(_sink != nullptr);

            if (result == Core::ERROR_NONE) {
                result = _sink->OnSetSpeed(speed);
            }

            _lock.Unlock();

            return (result);
        }

    private:
        // SignallingServer::IObserver overrides
        void Operational(const Core::ProxyType<SignallingServer::PeerConnection>& channel, const bool isRunning) override
        {
            _lock.Lock();

            if (_sink != nullptr) {
                if (_sink->Address() == channel->RemoteNode().HostAddress() && (channel->Type() == Bluetooth::AVDTP::Socket::SIGNALLING)) {
                    if (isRunning == true) {
                        TRACE(SinkFlow, (_T("Sink device %s signalling channel is open"), _sink->Address().c_str()));
                        _sink->SignallingConnected(channel);
                    }
                    else {
                        TRACE(SinkFlow, (_T("Sink device %s signalling channel is closing"), _sink->Address().c_str()));
                        _sink->SignallingDisconnected();
                    }
                }
            }

            _lock.Unlock();
        }

    private:
        void Operational(const A2DPSink::AudioServiceData& data)
        {
            _lock.Lock();

            ASSERT(_sink != nullptr);

            // Store the settings, if not already done..
            Core::File settingsFile(_service->PersistentPath() + _sink->Address() + _T(".json"));

            if ((settingsFile.Exists() == false) && (settingsFile.Create() == true)) {
                TRACE(Trace::Information, (_T("Storing Bluetooth audio sink information [%s]"), _sink->Address().c_str()));
                data.IElement::ToFile(settingsFile);
                settingsFile.Close();
            }

            _lock.Unlock();
        }

    private:
        mutable Core::CriticalSection _lock;
        PluginHost::JSONRPC* _jsonRpcModule;
        PluginHost::IShell* _service;
        DecoupledJob _cleanupJob;

        // Cached configuration
        string _controller;
        uint16_t _controllerLatency;
        string _codecSettings;
        std::map<Bluetooth::A2DP::IAudioCodec::codectype, uint8_t> _endpoints;

        Exchange::IBluetoothAudio::ISink::ICallback* _callback;
        Exchange::IBluetoothAudio::IStream* _source;

        std::unique_ptr<A2DPSink> _sink;
    }; // class BluetoothAudioSink

} // namespace Plugin

}
