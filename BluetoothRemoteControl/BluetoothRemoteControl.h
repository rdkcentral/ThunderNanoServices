/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "Administrator.h"
#include "WAVRecorder.h"
#include "HID.h"

#include <interfaces/IBluetooth.h>
#include <interfaces/IKeyHandler.h>
#include <interfaces/IBluetoothRemoteControl.h>
#include <interfaces/json/JAudioStream.h>
#include <interfaces/json/JBluetoothRemoteControl.h>

#define USE_VOICE_API

#ifdef USE_VOICE_API
#include <interfaces/IVoiceHandler.h>
#endif

namespace Thunder {

namespace Plugin {

    template<typename SIZE>
    class RingBufferType {
    public:
        using size = SIZE;

    public:
        RingBufferType(const SIZE capacity)
            : _buffer(nullptr)
            , _bufferEnd(nullptr)
            , _head(nullptr)
            , _tail(nullptr)
            , _capacity(capacity)
            , _used(0)
        {
            _buffer = static_cast<uint8_t*>(::malloc(capacity));
            _bufferEnd = (_buffer + capacity);
            ASSERT(_buffer != nullptr);

            Reset();
        }
        ~RingBufferType()
        {
            ::free(_buffer);
        }

        RingBufferType(const RingBufferType&) = delete;
        RingBufferType(RingBufferType&&) = delete;
        RingBufferType& operator=(const RingBufferType&) = delete;
        RingBufferType& operator=(RingBufferType&&) = delete;

    public:
        SIZE Used() const {
            return (_used);
        }
        SIZE Free() const {
            return (_capacity - _used);
        }
        SIZE Capacity() const {
            return (_capacity);
        }

    public:
        SIZE Reset()
        {
            _head = _buffer;
            _tail = _head;
            _used = 0;
            return (_capacity);
        }
        void Resize(const SIZE newCapacity)
        {
            if (newCapacity != _capacity) {
                uint8_t* newBuffer = static_cast<uint8_t*>(::malloc(newCapacity));
                ASSERT(newBuffer != nullptr);

                SIZE used = std::min(Used(), newCapacity);
                used = Pop(used, newBuffer);

                ::free(_buffer);

                _buffer = newBuffer;
                _bufferEnd = (_buffer + newCapacity);
                _head = (_buffer + used);
                _tail = _buffer;
                _used = used;
                _capacity = newCapacity;
            }
        }

    public:
        SIZE Push(const SIZE length, const uint8_t data[])
        {
            ASSERT((data != nullptr) || (length == 0));

            const uint8_t* const end = ((_tail <= _head) ? _bufferEnd : _tail);
            SIZE count = (_capacity == _used? 0 : std::min(length, static_cast<SIZE>(end - _head)));

            if (count != 0) {
                ::memcpy(_head, data, count);
                _head += count;

                if (end == _bufferEnd) {
                    const SIZE wrap = (length - count);
                    if (wrap != 0) {
                        const SIZE count2 = std::min(wrap, static_cast<SIZE>(_tail - _buffer));
                        if (count2 != 0) {
                            ::memcpy(_buffer, (data + count), count2);
                            _head = (_buffer + count2);
                            count += count2;
                        }
                    }
                }

                if (_head == _bufferEnd) {
                    _head = _buffer;
                }

                _used += count;
            }

            return (count);
        }
        SIZE Pop(const SIZE length, uint8_t data[])
        {
            ASSERT((data != nullptr) || (length == 0));

            const uint8_t* const end = (_head <= _tail ? _bufferEnd : _head);
            SIZE count = (_used == 0? 0 : std::min(length, static_cast<SIZE>(end - _tail)));

            if (count != 0) {
                ::memcpy(data, _tail, count);
                _tail += count;

                if (end == _bufferEnd) {
                    const SIZE wrap = (length - count);
                    if (wrap != 0) {
                        const SIZE count2 = std::min(wrap, static_cast<SIZE>(_head - _buffer));
                        if (count2 != 0) {
                            ::memcpy(data + count, _buffer, count2);
                            _tail = (_buffer + count2);
                            count += count2;
                        }
                    }
                }

                if (_tail == _bufferEnd) {
                    _tail = _buffer;
                }

                _used -= count;
            }

            return (count);
        }

    private:
        uint8_t* _buffer;
        uint8_t* _bufferEnd;
        uint8_t* _head;
        uint8_t* _tail;
        SIZE _capacity;
        SIZE _used;
    };


    class BluetoothRemoteControl : public PluginHost::IPlugin
                                 , public PluginHost::JSONRPCSupportsEventStatus
#ifdef USE_VOICE_API
                                 , public Exchange::IVoiceProducer
#endif
                                 , public Exchange::IAudioStream
                                 , public Exchange::JAudioStream::IHandler
                                 , public Exchange::IBluetoothRemoteControl
                                 , public Exchange::JBluetoothRemoteControl::IHandler {
    public:
        enum recorder {
            OFF               = 0x00,
            SINGLE            = 0x10,
            SEQUENCED         = 0x11,
            SINGLE_PERSIST    = 0x20,
            SEQUENCED_PERSIST = 0x21
        };

   private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Controller(_T("BluetoothControl"))
                , KeyMap()
                , KeyIngest(true)
                , Recorder(OFF)
            {
                Add(_T("controller"), &Controller);
                Add(_T("keymap"), &KeyMap);
                Add(_T("keyingest"), &KeyIngest);
                Add(_T("recorder"), &Recorder);
                Add(_T("audiobuffersize"), &AudioBufferSize);
                Add(_T("firstaudiochunksize"), &FirstAudioChunkSize);
                Add(_T("audiochunksize"), &AudioChunkSize);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Controller;
            Core::JSON::String KeyMap;
            Core::JSON::Boolean KeyIngest;
            Core::JSON::EnumType<recorder> Recorder;
            Core::JSON::DecUInt32 AudioBufferSize;
            Core::JSON::DecUInt32 FirstAudioChunkSize;
            Core::JSON::DecUInt32 AudioChunkSize;
        };

        class GATTRemote : public Bluetooth::GATTSocket {
        private:
            static constexpr uint16_t HID_UUID         = 0x1812;

            class Flow {
            public:
                Flow() = delete;
                Flow(const Flow&) = delete;
                Flow& operator=(const Flow&) = delete;
                Flow(const TCHAR formatter[], ...)
                {
                    va_list ap;
                    va_start(ap, formatter);
                    Core::Format(_text, formatter, ap);
                    va_end(ap);
                }
                explicit Flow(const string& text)
                    : _text(Core::ToString(text))
                {
                }
                explicit Flow(const uint16_t handle, const uint8_t length, const uint8_t buffer[])
                {
                    static const char hexArray[] = "0123456789ABCDEF";

                    Core::Format(_text, _T("Received: [0x%04X], data: [0x%02X]:["), handle, length);
                    std::string textData;
                    for (uint8_t index = 0; index < length; index++) {
                        if (index != 0) {
                          textData += ':';
                        }
                        textData += hexArray[ ((buffer[index] >> 4) & 0x0F) ];
                        textData += hexArray[ (buffer[index] & 0x0F) ];
                    }
                    _text += textData + ']';
                }
                ~Flow() = default;

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
            };

            class Sink : public Exchange::IBluetooth::IDevice::ICallback {
            public:
                Sink() = delete;
                Sink(const Sink&) = delete;
                Sink& operator=(const Sink&) = delete;
                Sink(GATTRemote* parent) : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }
                ~Sink() override
                {
                }

            public:
                void Updated() override
                {
                    _parent.Updated();
                }

                BEGIN_INTERFACE_MAP(Sink)
                    INTERFACE_ENTRY(Exchange::IBluetooth::IDevice::ICallback)
                END_INTERFACE_MAP

            private:
                GATTRemote& _parent;
            };

            class Decoupling : public Core::Thread {
            private:
                class Slot {
                public:
                    Slot() = delete;
                    Slot& operator= (const Slot&) = delete;
                    Slot(const uint16_t handle, const uint8_t length, const uint8_t data[])
                        : _handle(handle)
                        , _data(reinterpret_cast<const char*>(data), length) {
                    }
                    Slot(const Slot& copy)
                        : _handle(copy._handle)
                        , _data(copy._data) {
                    }
                    ~Slot() {
                    }
                public:
                    uint16_t Handle() const {
                        return (_handle);
                    }
                    uint8_t Length() const {
                        return (static_cast<uint8_t>(_data.length()));
                    }
                    const uint8_t* Data() const {
                        return (reinterpret_cast<const uint8_t*>(_data.c_str()));
                    }

                private:
                    uint16_t _handle;
                    std::string _data;
                };

            public:
                Decoupling(const Decoupling&) = delete;
                Decoupling& operator=(const Decoupling&) = delete;
                Decoupling(GATTRemote* parent)
                    : _parent(*parent)
                    , _adminLock()
                    , _queue()
                {
                    ASSERT(parent != nullptr);
                }
                ~Decoupling() override
                {
                }

            public:
                void Submit(const uint16_t handle, const uint8_t length, const uint8_t buffer[])
                {
                    ASSERT (length > 0);

                    _adminLock.Lock();
                    _queue.emplace_back(Slot(handle, length, buffer));
                    _adminLock.Unlock();

                    Run();
                }
                uint32_t Worker() override
                {
                    Block();

                    while (_queue.size() > 0) {

                        _adminLock.Lock();
                        Slot entry (_queue.front());
                        _queue.pop_front();
                        _adminLock.Unlock();
                        _parent.Message(entry.Handle(), entry.Length(), entry.Data());
                    }

                    return (Core::infinite);
                }

            private:
                GATTRemote& _parent;
                Core::CriticalSection _adminLock;
                std::list<Slot> _queue;
            };

#ifdef USE_VOICE_API
            class AudioProfile : public Exchange::IVoiceProducer::IProfile {
            public:
                AudioProfile(const AudioProfile&) = delete;
                AudioProfile& operator= (const AudioProfile&) = delete;
                AudioProfile(const Exchange::IVoiceProducer::IProfile::codec codec, const uint8_t channels, const uint32_t sampleRate, const uint8_t resolution)
                    : _codec(codec)
                    , _channels(channels)
                    , _sampleRate(sampleRate)
                    , _resolution(resolution) {
                }
                ~AudioProfile() override = default;

            public:
                Exchange::IVoiceProducer::IProfile::codec Codec() const override {
                    return (_codec);
                }
                uint8_t Channels() const override {
                    return (_channels);
                }
                uint32_t SampleRate() const override {
                    return (_sampleRate);
                }
                uint8_t Resolution() const override {
                    return (_resolution);
                }

                BEGIN_INTERFACE_MAP(AudioProfile)
                    INTERFACE_ENTRY(Exchange::IVoiceProducer::IProfile)
                END_INTERFACE_MAP

            private:
                codec _codec;
                uint8_t _channels;
                uint32_t _sampleRate;
                uint8_t _resolution;
            };
#endif

            class Config : public Core::JSON::Container {
            public:
                class Profile : public Core::JSON::Container {
                public:
                    Profile(const Profile&) = delete;
                    Profile& operator=(const Profile&) = delete;
                    Profile()
                        : Core::JSON::Container()
                        , Codec(Exchange::IAudioStream::codectype::PCM)
                        , SampleRate(16000)
                        , Channels(1)
                        , Resolution(16)
                    {
                        Add(_T("codec"), &Codec);
                        Add(_T("samplerate"), &SampleRate);
                        Add(_T("channels"), &Channels);
                        Add(_T("resolution"), &Resolution);
                        Add(_T("configuration"), &Configuration);
                    }
                    ~Profile() override
                    {
                    }

                public:
                    Core::JSON::EnumType<Exchange::IAudioStream::codectype> Codec;
                    Core::JSON::DecUInt32 SampleRate;
                    Core::JSON::DecUInt8 Channels;
                    Core::JSON::DecUInt8 Resolution;
                    Core::JSON::String Configuration;
                };

            public:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;
                Config()
                    : Core::JSON::Container()
                    , AudioProfile()
                    , ServiceUUID()
                    , CommandUUID()
                    , DataUUID()
                {
                    Add(_T("audioprofile"), &AudioProfile);
                    Add(_T("serviceuuid"), &ServiceUUID);
                    Add(_T("commanduuid"), &CommandUUID);
                    Add(_T("datauuid"), &DataUUID);
                }
                ~Config()
                {
                }

            public:
                Profile AudioProfile;
                Core::JSON::String ServiceUUID;
                Core::JSON::String CommandUUID;
                Core::JSON::String DataUUID;
            };

            class Profile : public Bluetooth::GATTProfile {
            public:
                Profile() = delete;
                Profile(const Profile&) = delete;
                Profile& operator= (const Profile&) = delete;

                Profile(const Config& config)
                    : Bluetooth::GATTProfile(true) {
                    VoiceService = Bluetooth::UUID(config.ServiceUUID.Value());
                    VoiceCommandChar = Bluetooth::UUID(config.CommandUUID.Value());
                    VoiceDataChar = Bluetooth::UUID(config.DataUUID.Value());
                }
                ~Profile() {
                }

            public:
                void Find(const Bluetooth::UUID& serviceUuid, const Bluetooth::UUID& charUuid, std::list<const Bluetooth::GATTProfile::Service::Characteristic*>& characteristics) const
                {
                    const Bluetooth::GATTProfile::Service* service = (*this)[serviceUuid];
                    if (service != nullptr) {
                        auto it = service->Characteristics();
                        while (it.Next() == true) {
                            if (it.Current() == charUuid) {
                                characteristics.push_back(&it.Current());
                            }
                        }
                    }
                }
                uint16_t FindHandle(const Bluetooth::GATTProfile::Service::Characteristic& characteristic, const Bluetooth::UUID& descUuid) const
                {
                    uint16_t handle = 0;
                    const Bluetooth::GATTProfile::Service::Characteristic::Descriptor* descriptor = characteristic[descUuid];
                    if (descriptor != nullptr) {
                        handle = descriptor->Handle();
                    }
                    return (handle);
                }
                uint16_t FindHandle(const Bluetooth::UUID& serviceUuid, const Bluetooth::UUID& charUuid) const
                {
                    const Bluetooth::GATTProfile::Service::Characteristic* characteristic = FindCharacteristic(serviceUuid, charUuid);
                    return (characteristic == nullptr ? 0 : characteristic->Handle());
                }
                uint16_t FindHandle(const Bluetooth::UUID& serviceUuid, const Bluetooth::UUID& charUuid, const Bluetooth::UUID& descUuid) const
                {
                    uint16_t handle = 0;
                    const Bluetooth::GATTProfile::Service::Characteristic* characteristic = FindCharacteristic(serviceUuid, charUuid);
                    if (characteristic != nullptr) {
                        const Bluetooth::GATTProfile::Service::Characteristic::Descriptor* descriptor= (*characteristic)[descUuid];
                        if (descriptor != nullptr) {
                            handle = descriptor->Handle();
                        }
                    }
                    return (handle);
                }

            private:
                const Bluetooth::GATTProfile::Service::Characteristic* FindCharacteristic(const Bluetooth::UUID& serviceUuid, const Bluetooth::UUID& charUuid) const
                {
                    const Bluetooth::GATTProfile::Service::Characteristic* result = nullptr;
                    const Bluetooth::GATTProfile::Service* service = (*this)[serviceUuid];
                    if (service != nullptr) {
                        result = (*service)[charUuid];
                    }
                    return (result);
                }

            public:
                Bluetooth::UUID VoiceService;
                Bluetooth::UUID VoiceCommandChar;
                Bluetooth::UUID VoiceDataChar;
            };

            static Core::NodeId Designator(const uint8_t type, const string& address)
            {
                return (Bluetooth::Address(address.c_str()).NodeId(static_cast<Bluetooth::Address::type>(type), GATTSocket::LE_ATT_CID, 0));
            }

        public:
            class Data : public Core::JSON::Container {
            public:
                Data(const Data&) = delete;
                Data& operator=(const Data&) = delete;
                Data()
                    : Core::JSON::Container()
                    , Name()
                    , Address()
                    , ModelNumber()
                    , SerialNumber()
                    , FirmwareRevision()
                    , ManufacturerName()
                    , SoftwareRevision()
                    , SoftwareRevisionHandle(0)
                    , KeysDataHandle(0)
                    , KeysDataHandles()
                    , BatteryLevelHandle(0)
                    , VoiceCommandHandle(0)
                    , VoiceDataHandle(0)
                {
                    Add(_T("name"), &Name);
                    Add(_T("address"), &Address);
                    Add(_T("model"), &ModelNumber);
                    Add(_T("serial"), &SerialNumber);
                    Add(_T("firmware"), &FirmwareRevision);
                    Add(_T("manufacturer"), &ManufacturerName);
                    Add(_T("software"), &SoftwareRevision);
                    Add(_T("version"), &SoftwareRevisionHandle);
                    Add(_T("key"), &KeysDataHandle); // obsolete
                    Add(_T("keys"), &KeysDataHandles);
                    Add(_T("battery"), &BatteryLevelHandle);
                    Add(_T("command"), &VoiceCommandHandle);
                    Add(_T("voice"), &VoiceDataHandle);
                }
               ~Data()
                {
                }

            public:
               Core::JSON::String Name;
               Core::JSON::String Address;
               Core::JSON::String ModelNumber;
               Core::JSON::String SerialNumber;
               Core::JSON::String FirmwareRevision;
               Core::JSON::String ManufacturerName;
               Core::JSON::String SoftwareRevision;

               Core::JSON::DecUInt16 SoftwareRevisionHandle;
               Core::JSON::DecUInt16 KeysDataHandle;
               Core::JSON::ArrayType<Core::JSON::DecUInt16> KeysDataHandles;
               Core::JSON::DecUInt16 BatteryLevelHandle;
               Core::JSON::DecUInt16 VoiceCommandHandle;
               Core::JSON::DecUInt16 VoiceDataHandle;
            };

        public:
            GATTRemote() = delete;
            GATTRemote(const GATTRemote&) = delete;
            GATTRemote& operator=(const GATTRemote&) = delete;

            GATTRemote(BluetoothRemoteControl* parent, Exchange::IBluetooth::IDevice* device, const string& configuration)
                : Bluetooth::GATTSocket(Designator(device->Type(), device->LocalId()), Designator(device->Type(), device->RemoteId()), 255)
                , _parent(parent)
                , _adminLock()
                , _profile(nullptr)
                , _command()
                , _device(device)
                , _decoupling(this)
                , _sink(this)
                , _name()
                , _address()
                , _modelNumber()
                , _serialNumber()
                , _manufacturerName()
                , _firmwareRevision()
                , _softwareRevision()
                , _softwareRevisionHandle(0)
                , _keysDataHandles()
                , _batteryLevelHandle(~0)
                , _voiceDataHandle(~0)
                , _voiceCommandHandle(~0)
                , _hidReportCharacteristics()
                , _hidReportCharacteristicsIterator(_hidReportCharacteristics.cbegin())
                , _hid()
                , _hidInputReports()
                , _audioProfile()
                , _decoder(nullptr)
            {
                Config config;
                config.FromString(configuration);

                _profile = new Profile(config);

                Constructor(config);
            }

            GATTRemote(BluetoothRemoteControl* parent, Exchange::IBluetooth::IDevice* device, const string& configuration, const Data& data)
                : Bluetooth::GATTSocket(Designator(device->Type(), device->LocalId()), Designator(device->Type(), device->RemoteId()), 255)
                , _parent(parent)
                , _adminLock()
                , _profile(nullptr)
                , _command()
                , _device(device)
                , _decoupling(this)
                , _sink(this)
                , _name()
                , _address()
                , _modelNumber(data.ModelNumber.Value())
                , _serialNumber(data.SerialNumber.Value())
                , _manufacturerName(data.ManufacturerName.Value())
                , _firmwareRevision(data.FirmwareRevision.Value())
                , _softwareRevision(data.SoftwareRevision.Value())
                , _softwareRevisionHandle(data.SoftwareRevisionHandle.Value())
                , _keysDataHandles()
                , _batteryLevelHandle(data.BatteryLevelHandle.Value())
                , _voiceDataHandle(data.VoiceDataHandle.Value())
                , _voiceCommandHandle(data.VoiceCommandHandle.Value())
                , _hidReportCharacteristics()
                , _hidReportCharacteristicsIterator(_hidReportCharacteristics.cbegin())
                , _hid()
                , _hidInputReports()
                , _audioProfile()
                , _decoder(nullptr)
                , _voiceEnabled(false)
            {
                bool discover = (data.ManufacturerName.IsSet() == false) || (data.ManufacturerName.Value().empty() == true);

                if (data.KeysDataHandle.IsSet() == true) {
                    if (data.KeysDataHandle.Value() != static_cast<uint16_t>(~0)) {
                        _keysDataHandles.push_back(data.KeysDataHandle.Value());
                    }
                    else {
                        discover = false;
                    }
                } else {
                    auto index = data.KeysDataHandles.Elements();
                    while (index.Next() == true) {
                        if (index.Current().Value() != static_cast<uint16_t>(~0)) {
                            _keysDataHandles.push_back(index.Current().Value());
                        }
                        else {
                            discover = false;
                        }
                    }
                }


                Config config;
                config.FromString(configuration);

                if (discover == true) {
                    _profile = new Profile(config);
                }

                Constructor(config);
            }

            ~GATTRemote() override
            {
                if (_device != nullptr) {
                    if (_device->Callback(static_cast<Exchange::IBluetooth::IDevice::ICallback*>(nullptr)) != Core::ERROR_NONE) {
                        TRACE(Trace::Fatal, (_T("Could not remove the callback from the device")));
                    }
                }

                if (GATTSocket::IsOpen() == true) {
                    GATTSocket::Close(Core::infinite);
                }

                if (_device != nullptr) {
                    _device->Disconnect();
                    _device->Release();
                    _device = nullptr;
                }

                if (_decoder != nullptr) {
                    delete _decoder;
                }

                if (_profile != nullptr) {
                    delete _profile;
                }

                _parent->Inoperational();
            }

        public:
            const string& Name() const
            {
                return(_name);
            }
            inline const string& Address() const
            {
                return(_address);
            }
            inline const string& ModelNumber() const
            {
                return (_modelNumber);
            }
            inline const string& SerialNumber() const
            {
                return (_serialNumber);
            }
            inline const string& FirmwareRevision() const
            {
                return (_firmwareRevision);
            }
            inline const string& SoftwareRevision() const
            {
                return (_softwareRevision);
            }
            inline const string& ManufacturerName() const
            {
                return (_manufacturerName);
            }
            inline const Core::OptionalType<Exchange::IAudioStream::audioprofile>& SelectedProfile() const {
                return (_audioProfile);
            }
#ifdef USE_VOICE_API
            inline AudioProfile* SelectedAudioProfile() const {
                ASSERT(_audioProfile.IsSet() == true);
                return (Core::ServiceType<AudioProfile>::Create<AudioProfile>(_audioProfile.Value().codec,
                    _audioProfile.Value().channels, _audioProfile.Value().sampleRate, _audioProfile.Value().resolution));
            }
#endif
            inline bool VoiceOutput() const {
                return (_voiceEnabled);
            }
            inline uint32_t VoiceOutput(const bool enabled) {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                if (_voiceCommandHandle != 0) {
                    GATTSocket::Command cmd;
                    uint8_t val = htobs(enabled ? 0x01 : 0x00);
                    cmd.Write(_voiceCommandHandle, sizeof(val), reinterpret_cast<const uint8_t*>(&val));
                    if ( ((result = Execute(CommunicationTimeOut, cmd)) == Core::ERROR_NONE) && (cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) ) {
                        TRACE(Flow, (_T("Voice Control (handle 0x%04X) enabled: %s"), _voiceCommandHandle, enabled ? _T("true") : _T("false")));
                        result = Core::ERROR_NONE;
                        _voiceEnabled = enabled;

                        // TODO: Validate, this functionality might alse be triggered by a niootification send from the change on this handle.
                        if (_decoder != nullptr) {
                            // If we start, reset.
                            if (enabled == false) {
                                // We are done, signal that the button to speak has been released!
                                _parent->VoiceData();
                            }
                            else {
                                // Looks like the TPress-to-talk button is pressed...
                                _decoder->Reset();
                                _startFrame = true;
                            }
                        }
                    }
                    else {
                        uint32_t outcome = result;
                        result = (result == Core::ERROR_NONE ? (cmd.Error() == Core::ERROR_NONE ? (uint32_t)Core::ERROR_PRIVILIGED_REQUEST : cmd.Error()) : result);
                        TRACE(Trace::Error, (_T("Failed to enable voice (handle 0x%04X), error: %d [%d]"), _voiceCommandHandle, outcome, result == Core::ERROR_PRIVILIGED_REQUEST ? cmd.Result().Error() : 0));
                    }
                }
                return (result);
            }
            void DecoderFromConfigLine(const string& configLine)
            {
                if (SelectedProfile().IsSet() == false) {
                    Config config;
                    config.FromString(configLine);
                    SetDecoder(config.AudioProfile);
                }
            }
#ifdef USE_VOICE_API
            void Decoder(const string& profile)
            {
                Config::Profile config;
                config.FromString(profile);
                SetDecoder(config);
            }
#endif
            void Decoder(const Exchange::IAudioStream::audioprofile& profile)
            {
                Config::Profile config;
                config.SampleRate = profile.sampleRate;
                config.Resolution = profile.resolution;
                config.Channels = profile.channels;
                config.Codec = profile.codec;
                config.Configuration = profile.codecParams;
                SetDecoder(config);
            }

        private:
            void SetDecoder(const Config::Profile& config) {

                if (_decoder != nullptr) {
                    delete _decoder;
                }

                Core::EnumerateType<Exchange::IAudioStream::codectype> enumValue(config.Codec.Value());

                _decoder = Decoders::IDecoder::Instance(_manufacturerName.c_str(), enumValue.Value(), config.Configuration.Value());
                if (_decoder == nullptr) {
                    TRACE(Trace::Error, (_T("Failed to create a decoder for %s type: [%s]"), _manufacturerName.c_str(), enumValue.Data()));
                }
                else {
                    TRACE(Flow, (_T("Created a decoder for %s type: [%s]"), _manufacturerName.c_str(), enumValue.Data()));
                    _audioProfile = { config.Codec.Value(), {}, config.Channels.Value(), config.Resolution.Value(), config.SampleRate.Value(), {} };

                    if (config.Configuration.IsSet() == true) {
                        _audioProfile.Value().codecParams = config.Configuration.Value();
                    }

                    if (config.Codec.Value() == Exchange::IAudioStream::IMA_ADPCM) {
                        // Fixed 1:4 compression
                        _audioProfile.Value().bitRate = ((config.Channels.Value() * config.Resolution.Value() * config.SampleRate.Value()) / 4);
                    }
                }
            }
            void Discover()
            {
                _profile->Discover(CommunicationTimeOut * 20, *this, [&](const uint32_t result) {
                    if (result == Core::ERROR_NONE) {
                        DumpProfile();

                        if ((*_profile)[Bluetooth::UUID(HID_UUID)] == nullptr) {
                            TRACE(Flow, (_T("The given bluetooth device does not support a HID service!!")));
                        }
                        else {
                            TRACE(Flow, (_T("Reading the remaining information")));
                            ReadModelNumber();
                        }
                    }
                    else {
                        TRACE(Flow, (_T("The given bluetooth device could not be read for services!!")));
                    }
                });
            }
            void Notification(const uint16_t handle, const uint8_t dataFrame[], const uint16_t length) override
            {
                // Decouple the notifications from the communciator thread, they will pop-up and need to be handled
                // by the Message method!
                _decoupling.Submit(handle, static_cast<uint8_t>(length), dataFrame);
            }
            void Message(const uint16_t handle, const uint8_t length, const uint8_t buffer[])
            {
                TRACE(Flow, (handle, length, buffer));

                _adminLock.Lock();

                if ( (handle == _voiceDataHandle) && (_decoder != nullptr) ) {
                    uint8_t decoded[1024];
                    uint16_t sendLength = _decoder->Decode(length, buffer, sizeof(decoded), decoded);
                    if (sendLength > 0) {
                        ASSERT (sendLength <= sizeof(decoded));
                        if (_startFrame == true) {
                            _startFrame = false;
                            _parent->VoiceData(_audioProfile);
                        }
                        _parent->VoiceData(_decoder->Frames(), sendLength, decoded);
                    }
                }
                else if ( (std::any_of(_keysDataHandles.cbegin(), _keysDataHandles.cend(), [handle](const uint16_t reportHandle) { return (reportHandle == handle); }))
                           && (length >= 1) && (length <= 8) ) {

                    uint32_t scancode = 0;
                    // TODO: official spec allows for 6 keys to be pressed simultaneously. We only
                    //       allow 2... Might require fixing if we want to support this.
                    uint8_t endMarker = std::min(length, static_cast<uint8_t>(4));

                    for (uint8_t i = 2; i < endMarker; i++) {
                        scancode |= (buffer[i] << (8 * (i - 2)));
                    }

                    // Move the modifiers (actually only the first byte) to the higehr 16 bits)
                    scancode |= (buffer[0] << 16) | (buffer[1] << 16);

                    bool pressed = (scancode != 0);

                    if (pressed == true) {
                        _currentKey = scancode;
                    }

                    _parent->KeyEvent(pressed, _currentKey);
                }
                else if ( (handle == _voiceCommandHandle) && (_decoder != nullptr) ) {

                    // If we start, reset.
                    if (buffer[0] == 0) {
                        // We are done, signal that the button to speak has been released!
                        _parent->VoiceData();
                    }
                    else {
                        // Looks like the TPress-to-talk button is pressed...
                        _decoder->Reset();
                        _startFrame = true;
                    }
                }
                else if ( (handle == _batteryLevelHandle) && (length >= 1) ) {
                    _parent->UpdateBatteryLevel(buffer[0]);
                }

                _adminLock.Unlock();
            }
            void Constructor(const Config& config)
            {
                ASSERT(_parent != nullptr);
                ASSERT(_device != nullptr);

                _device->AddRef();
                // _decoupling.AddRef();
                _name = _device->Name();
                _address = _device->RemoteId();

                if (_device->Callback(&_sink) != Core::ERROR_NONE) {
                    TRACE(Trace::Fatal, (_T("The device is already in use. Only 1 callback allowed")));
                }

                if ((_profile == nullptr) && (_voiceCommandHandle != static_cast<uint8_t>(~0))) {
                    SetDecoder(config.AudioProfile);
                }

                if (_device->IsConnected() == true) {
                    uint32_t result = GATTSocket::Open(5000);
                    if (result != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to open GATT socket [%s] [%i]"), _device->RemoteId().c_str(), result));
                        GATTSocket::Close(Core::infinite);
                    } else {
                        if (_profile == nullptr) {
                            TRACE(Flow, (_T("The HoG device is ready for operation")));
                        } else {
                            TRACE(Flow, (_T("The device is not configured. Read the device configuration.")));
                            Discover();
                        }
                    }
                } else {
                    TRACE(Flow, (_T("The device is not configured. Connect to it, so we can read the config.")));
                }

                // Try to connect or mark for autoconnection if already connected.
                _device->Connect();
            }
            uint32_t Initialize() override
            {
                return (Security(BT_SECURITY_MEDIUM) ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
            }
            void Operational() override
            {
                _parent->Connected(_name);

                if (_profile == nullptr) {
                    TRACE(Flow, (_T("The received MTU: %d, no need for discovery, we know it all"), MTU()));

                    // No need to do service discovery if device knows the Handles to use. If so, DeviceDiscovery has
                    // already been done and the only thing we need to do is get the starting values :-)
                    ReadSoftwareRevision();
                }
                else {
                    TRACE(Flow, (_T("The received MTU: %d, we have no clue yet, start discovery"), MTU()));
                    Discover();
               }
            }
            void ReadModelNumber()
            {
                ASSERT (_profile != nullptr);

                uint16_t modelNumberHandle = _profile->FindHandle(Bluetooth::GATTProfile::Service::DeviceInformation, Bluetooth::GATTProfile::Service::Characteristic::ModelNumberString);
                if (modelNumberHandle != 0) {
                    _command.Read(modelNumberHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _modelNumber = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                        } else {
                            TRACE(Flow, (_T("Failed to read model name")));
                        }

                        ReadSerialNumber();
                    });
                } else {
                    TRACE(Flow, (_T("ModelNumberString characteristic not available")));
                    ReadSerialNumber();
                }
            }
            void ReadSerialNumber()
            {
                ASSERT (_profile != nullptr);

                uint16_t serialNumberHandle = _profile->FindHandle(Bluetooth::GATTProfile::Service::DeviceInformation, Bluetooth::GATTProfile::Service::Characteristic::SerialNumberString);
                if (serialNumberHandle != 0) {
                    _command.Read(serialNumberHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            Core::ToHexString(cmd.Result().Data(), cmd.Result().Length(), _serialNumber);
                        } else {
                            TRACE(Flow, (_T("Failed to read serial number")));
                        }

                        ReadFirmwareRevision();
                    });
                } else {
                    TRACE(Flow, (_T("SerialNumberString characteristic not available")));
                    ReadFirmwareRevision();
                }
            }
            void ReadFirmwareRevision()
            {
                ASSERT (_profile != nullptr);

                uint16_t firmwareRevisionHandle = _profile->FindHandle(Bluetooth::GATTProfile::Service::DeviceInformation, Bluetooth::GATTProfile::Service::Characteristic::FirmwareRevisionString);
                if (firmwareRevisionHandle != 0) {
                    _command.Read(firmwareRevisionHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _firmwareRevision = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                        } else {
                            TRACE(Flow, (_T("Failed to read firmware revision")));
                        }

                        ReadManufacturerName();
                    });
                } else {
                    TRACE(Flow, (_T("FirmwareRevisionString characteristic not available")));
                    ReadManufacturerName();
                }
            }
            void ReadManufacturerName()
            {
                ASSERT (_profile != nullptr);

                uint16_t manufacturerNameHandle = _profile->FindHandle(Bluetooth::GATTProfile::Service::DeviceInformation, Bluetooth::GATTProfile::Service::Characteristic::ManufacturerNameString);
                if (manufacturerNameHandle != 0) {
                    _command.Read(manufacturerNameHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _manufacturerName = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                        } else {
                            TRACE(Flow, (_T("Failed to read manufacturer name")));
                        }
                        ReadHIDReportMap();
                    });
                } else {
                    TRACE(Flow, (_T("ManufacturerNameString characteristic not available")));
                    ReadHIDReportMap();
                }
            }
            void ReadHIDReportMap()
            {
                uint16_t hidReportMapHandle = _profile->FindHandle(Bluetooth::GATTProfile::Service::HumanInterfaceDevice, Bluetooth::GATTProfile::Service::Characteristic::ReportMap);
                if (hidReportMapHandle != 0) {
                    _command.Read(hidReportMapHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            if (_hid.Deserialize(cmd.Result().Length(), cmd.Result().Data()) == true) {
                                // Look up input reports in the HID map...
                                for (auto& collection : _hid.ReportMap().Collections()) {
                                    if ((collection.Usage() == USB::HID::MakeUsage(USB::HID::usagepage::CONSUMER, USB::HID::consumerusage::CONSUMER_CONTROL))
                                        || (collection.Usage() == USB::HID::MakeUsage(USB::HID::usagepage::GENERIC_DESKTOP, USB::HID::desktopusage::KEYPAD))) {
                                        for (auto& report : collection.Reports()) {
                                            for (auto& element : report.Elements()) {
                                                if (element.Type() == USB::HID::Report::Element::INPUT) {
                                                    _hidInputReports.push_back(&report);
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    else if (collection.Usage() == USB::HID::MakeUsage(USB::HID::usagepage::VENDOR_SPECIFIC, 0x0001)) {
                                        for (auto& report : collection.Reports()) {
                                            for (auto& element : report.Elements()) {
                                                if (element.Type() == USB::HID::Report::Element::INPUT) {
                                                    _hidInputReports.push_back(&report);
					                                _voiceDataHandle = report.ID();
                                                }
                                                else if (element.Type() == USB::HID::Report::Element::OUTPUT) {
                                                    _hidInputReports.push_back(&report);
                                                }
                                            }
    	                				}
                                    }
                                    else {
                                    	TRACE(Flow, (_T("HID Report Map [0x%08X] not recognized"), collection.Usage()));
	                			    }
                                }
                            }

                            if (_hidInputReports.empty() == false) {
                                _profile->Find(Bluetooth::GATTProfile::Service::HumanInterfaceDevice, Bluetooth::GATTProfile::Service::Characteristic::Report, _hidReportCharacteristics);
                                _hidReportCharacteristicsIterator = _hidReportCharacteristics.cbegin();
                                ReadHIDReportReferences();
                            } else {
                                TRACE(Flow, (_T("No input reports in report map or failed to parse the report map")));
                                EnableEvents(); // skip HID parsing altogether...
                            }
                        } else {
                            TRACE(Flow, (_T("Failed to read HID ReportMap name")));
                            EnableEvents();
                        }
                    });
                } else {
                    TRACE(Flow, (_T("HID ReportMap characteristic not available")));
                    EnableEvents();
                }
            }
            void ReadHIDReportReferences()
            {
                if (_hidReportCharacteristicsIterator != _hidReportCharacteristics.cend()) {
                    uint16_t referenceHandle = _profile->FindHandle(*(*_hidReportCharacteristicsIterator), Bluetooth::GATTProfile::Service::Characteristic::Descriptor::ReportReference);
                    if (referenceHandle != 0) {
                        _command.Read(referenceHandle);
                        Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                            if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                                uint8_t reportId = cmd.Result().Data()[0];
                                if (std::any_of(_hidInputReports.cbegin(), _hidInputReports.cend(), [reportId](const USB::HID::Report* report) { return (report->ID() == reportId); }) == false) {
                                    // This characteristic's reference ID is not on the list of HID input reports, remove it
                                    _hidReportCharacteristicsIterator = _hidReportCharacteristics.erase(_hidReportCharacteristicsIterator);
                                }
                                else {
                                    if (_voiceDataHandle == reportId) {
                                        TRACE(Flow, (_T("Updating the Voice DataHandler from [%d] to [0x%04X]"), reportId, (*_hidReportCharacteristicsIterator)->Handle()));
                                        _voiceDataHandle = (*_hidReportCharacteristicsIterator)->Handle();
                                    }
                                    _hidReportCharacteristicsIterator++;
                                }
                                ReadHIDReportReferences();
                            } else {
                                TRACE(Flow, (_T("Failed to read ReportReference descriptor")));
                                _hidReportCharacteristics.erase(_hidReportCharacteristicsIterator, _hidReportCharacteristics.cend());
                                _hidReportCharacteristicsIterator = _hidReportCharacteristics.cbegin();
                                EnableEvents();
                            }
                        });
                    } else {
                        // No ReferenceHandle descriptor for this Report characteristic, remove it as well
                        _hidReportCharacteristicsIterator = _hidReportCharacteristics.erase(_hidReportCharacteristicsIterator);
                        ReadHIDReportReferences();
                    }
                } else {
                    _hidReportCharacteristicsIterator = _hidReportCharacteristics.cbegin();
                     _keysDataHandles.clear();
                    EnableEvents();
                }
            }
            void EnableEvents()
            {
                uint16_t notificationHandle;
                ASSERT (_profile != nullptr);

                // Set all interesting handles...
                if (_hidReportCharacteristicsIterator != _hidReportCharacteristics.cend()) {
                    notificationHandle = _profile->FindHandle(*(*_hidReportCharacteristicsIterator), Bluetooth::GATTProfile::Service::Characteristic::Descriptor::ClientCharacteristicConfiguration);

                    if (notificationHandle == 0) {
                        // This is the control for the voice Data..
                        _voiceCommandHandle = (*_hidReportCharacteristicsIterator)->Handle();
                        _hidReportCharacteristicsIterator++;
                        TRACE(Flow, (_T("Selecting handle: [0x%04X] for voice control"), _voiceCommandHandle));
                        EnableEvents();
                        _voiceEnabled = true;
                    }
                    else {
                        _keysDataHandles.push_back((*_hidReportCharacteristicsIterator)->Handle());
                        _hidReportCharacteristicsIterator++;
                        EnableEvents(_T("Key handling"), notificationHandle);
                    }
                }
                else if (_batteryLevelHandle == static_cast<uint16_t>(~0)) {
                    notificationHandle = _profile->FindHandle(Bluetooth::GATTProfile::Service::BatteryService, Bluetooth::GATTProfile::Service::Characteristic::BatteryLevel, Bluetooth::GATTProfile::Service::Characteristic::Descriptor::ClientCharacteristicConfiguration);
                    _batteryLevelHandle = _profile->FindHandle(Bluetooth::GATTProfile::Service::BatteryService, Bluetooth::GATTProfile::Service::Characteristic::BatteryLevel);
                    EnableEvents(_T("Battery handling"), notificationHandle);
                }
                else if (_voiceCommandHandle == static_cast<uint16_t>(~0)) {
                    notificationHandle = _profile->FindHandle(_profile->VoiceService, _profile->VoiceCommandChar, Bluetooth::GATTProfile::Service::Characteristic::Characteristic::Descriptor::ClientCharacteristicConfiguration);
                    _voiceCommandHandle = _profile->FindHandle(_profile->VoiceService, _profile->VoiceCommandChar);
                    EnableEvents(_T("Voice Command handling"), notificationHandle);
                    _voiceEnabled = true;
                }
                else if (_voiceDataHandle == static_cast<uint16_t>(~0)) {
                    notificationHandle = _profile->FindHandle(_profile->VoiceService, _profile->VoiceDataChar, Bluetooth::GATTProfile::Service::Characteristic::Descriptor::ClientCharacteristicConfiguration);
                    _voiceDataHandle = _profile->FindHandle(_profile->VoiceService, _profile->VoiceDataChar);
                    EnableEvents(_T("Voice Data handling"), notificationHandle);
                }
                else {
                    _softwareRevisionHandle = _profile->FindHandle(Bluetooth::GATTProfile::Service::DeviceInformation, Bluetooth::GATTProfile::Service::Characteristic::SoftwareRevisionString);

                    // We are done with the profile, all is read, all is set, we hope :-) Time
                    // to register what we did.
                    ReadSoftwareRevision();
                }
            }
            void EnableEvents(const TCHAR* message, const uint16_t handle)
            {
                if (handle == 0) {
                    TRACE(Trace::Error, (_T("There is no notification for %s"), message));
                    EnableEvents();
                } else {
                    TRACE(Flow, (_T("Enabling notification for %s with handle 0x%02x"), message, handle));
                    uint16_t val = htobs(1);
                    _command.Write(handle, sizeof(val), reinterpret_cast<const uint8_t*>(&val));
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((_command.Error() != Core::ERROR_NONE) || (_command.Result().Error() != 0) ) {
                            TRACE(Trace::Error, (_T("Failed to enable notifications (event handle 0x%04X), error: %d"),
                                                cmd.Result().Handle(), _command.Result().Error()));
                        }
                        EnableEvents();
                   });
                }
            }
            void ReadSoftwareRevision()
            {
                if (_softwareRevisionHandle != 0) {
                    _command.Read(_softwareRevisionHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _softwareRevision = string(reinterpret_cast<const char*>(cmd.Result().Data()), cmd.Result().Length());
                        } else {
                            TRACE(Flow, (_T("Failed to read software revision")));
                        }

                        ReadBatteryLevel();
                    });
                } else {
                    TRACE(Flow, (_T("SoftwareRevisionString characteristic not available")));
                    ReadBatteryLevel();
                }
            }
            void ReadBatteryLevel()
            {
                if (_batteryLevelHandle != 0) {
                    _command.Read(_batteryLevelHandle);
                    Execute(CommunicationTimeOut, _command, [&](const GATTSocket::Command& cmd) {
                        if ((cmd.Error() == Core::ERROR_NONE) && (cmd.Result().Error() == 0) && (cmd.Result().Length() >= 1)) {
                            _parent->UpdateBatteryLevel(cmd.Result().Data()[0]);
                        } else {
                            TRACE(Flow, (_T("Failed to retrieve battery level")));
                        }
                        Completed();
                   });
                } else {
                    TRACE(Flow, (_T("BatteryLevel characteristic not available")));
                    Completed();
                }
            }
            void Completed()
            {
                Data info;

                info.ModelNumber = _modelNumber;
                info.SerialNumber = _serialNumber;
                info.FirmwareRevision = _firmwareRevision;
                info.SoftwareRevision = _softwareRevision;
                info.ManufacturerName = _manufacturerName;
                info.SoftwareRevisionHandle = _softwareRevisionHandle;
                info.BatteryLevelHandle = _batteryLevelHandle;
                info.VoiceCommandHandle = _voiceCommandHandle;
                info.VoiceDataHandle = _voiceDataHandle;

                info.KeysDataHandles.Clear();
                for (auto& handle : _keysDataHandles) {
                    Core::JSON::DecUInt16 jHandle;
                    jHandle = handle;
                    info.KeysDataHandles.Add(jHandle);
                }

                // Don't perform service recovery on subsequent connections.
                if (_profile != nullptr) {
                    delete _profile;
                    _profile = nullptr;
                }

                TRACE(Flow, (_T("All required information gathered. Report it to the parent")));
                _parent->Operational(info);
            }
            void Updated()
            {
                _adminLock.Lock();
                if (_device != nullptr) {
                    if (_device->IsConnected() == true) {
                        if ((IsOpen() == false) && (GATTSocket::IsConnecting() == false)) {
                            TRACE(Trace::Information, (_T("Connecting GATT socket [%s]"), _device->RemoteId().c_str()));
                            uint32_t result = GATTSocket::Open(5000);
                            if (result != Core::ERROR_NONE) {
                                TRACE(Trace::Error, (_T("Failed to open GATT socket [%s] [%i]"), _device->RemoteId().c_str(), result));
                                GATTSocket::Close(Core::infinite);
                            }
                        }
                    } else if (_device->IsValid() == true) {
                        if (IsOpen() == true) {
                            TRACE(Trace::Information, (_T("Disconnecting GATT socket [%s]"), _device->RemoteId().c_str()));
                            uint32_t result = GATTSocket::Close(Core::infinite);
                            if (result != Core::ERROR_NONE) {
                                TRACE(Trace::Error, (_T("Failed to close GATT socket [%s]"), _device->RemoteId().c_str()));
                            }
                            _parent->Inoperational();
                        }
                    } else {
                        TRACE(Flow, (_T("Releasing device")));
                        if (_device->Callback(static_cast<Exchange::IBluetooth::IDevice::ICallback*>(nullptr)) != Core::ERROR_NONE) {
                            TRACE(Trace::Information, (_T("Could not unlink the Callback sink. [%s]"), _device->RemoteId().c_str()));
                        }
                        _device->Release();
                        _device = nullptr;
                    }
                }
                _adminLock.Unlock();
            }
            void DumpProfile() const
            {
                Bluetooth::GATTProfile::Iterator serviceIdx = _profile->Services();
                while (serviceIdx.Next() == true) {
                    const Bluetooth::GATTProfile::Service& service = serviceIdx.Current();
                    TRACE(Flow, (_T("[0x%04X] Service: [0x%04X]         %s"), service.Handle(), service.Max(), service.Name().c_str()));

                    Bluetooth::GATTProfile::Service::Iterator characteristicIdx = service.Characteristics();
                    while (characteristicIdx.Next() == true) {
                        const Bluetooth::GATTProfile::Service::Characteristic& characteristic(characteristicIdx.Current());
                        const Bluetooth::UUID& type(characteristic.Type());

                        TRACE(Flow, (_T("[0x%04X]    Characteristic [0x%02X]: %s [%d]"), characteristic.Handle(), characteristic.Rights(), characteristic.Name().c_str(), characteristic.Error()));
                        TRACE(Flow, (_T("[0x%04X]                             %s"), (type.HasShort() ? type.Short() : 0x0000), characteristic.ToString().c_str()));

                        Bluetooth::GATTProfile::Service::Characteristic::Iterator descriptorIdx = characteristic.Descriptors();
                        while (descriptorIdx.Next() == true) {
                            const Bluetooth::GATTProfile::Service::Characteristic::Descriptor& descriptor(descriptorIdx.Current());
                            TRACE(Flow, (_T("[0x%04X]       Descriptor:         %s"), descriptor.Handle(), descriptor.Name().c_str()));
                        }
                    }
                }
            }

        private:
            BluetoothRemoteControl* _parent;
            Core::CriticalSection _adminLock;
            Profile* _profile;
            GATTSocket::Command _command;
            Exchange::IBluetooth::IDevice* _device;
            Decoupling _decoupling;
            Core::SinkType<Sink> _sink;

            // This is the Name, as depicted by the Bluetooth device (generic IF)
            string _name;
            string _address;

            // The next section of information is coming from the config that
            // was stored for this remote or it is read from the profile if
            // this is the first registration
            string _modelNumber;
            string _serialNumber;
            string _manufacturerName;
            string _firmwareRevision;

            // If we are already registered, no need to read the profile as all
            // is coming from the RemoteControl persistent storage file.
            // The only thing that might have changed, and what should be read on
            // each new creation is the software revision, that might have been
            // upgraded using Android phone upgrades..
            string _softwareRevision;

            // During the initial creation/subscribe of this RemoteControl, the
            // handles that report keys, battery level and voice (if applicable)
            // are registered. Remember these to trigger the right handler if
            // new data comes in.
            uint16_t _softwareRevisionHandle;
            std::list<uint16_t>_keysDataHandles;
            uint16_t _batteryLevelHandle;
            uint16_t _voiceDataHandle;
            uint16_t _voiceCommandHandle;

            std::list<const Bluetooth::GATTProfile::Service::Characteristic*> _hidReportCharacteristics;
            std::list<const Bluetooth::GATTProfile::Service::Characteristic*>::const_iterator _hidReportCharacteristicsIterator;

            USB::HID _hid;
            std::list<const USB::HID::Report*> _hidInputReports;

            // What profile will this system be working with ???
            Core::OptionalType<Exchange::IAudioStream::audioprofile> _audioProfile;
            Decoders::IDecoder* _decoder;
            bool _startFrame;
            uint32_t _currentKey;

            bool _voiceEnabled;
        };

    public:
        BluetoothRemoteControl(const BluetoothRemoteControl&) = delete;
        BluetoothRemoteControl& operator= (const BluetoothRemoteControl&) = delete;
        BluetoothRemoteControl()
            : _adminLock()
            , _service()
            , _gattRemote(nullptr)
            , _name()
            , _controller()
            , _keyMap()
            , _configLine()
            , _recordFile()
            , _batteryLevel(~0)
#ifdef USE_VOICE_API
            , _voiceHandler(nullptr)
#endif
            , _audioHandler(nullptr)
            , _inputHandler(nullptr)
            , _record(recorder::OFF)
            , _recorder()
            , _buffer(nullptr)
            , _sequence(0)
            , _firstAudioChunkSize(0)
            , _audioChunkSize(0)
            , _observers()
            , _streamObservers()
            , _transmission(false)
        {
        }
        ~BluetoothRemoteControl() override
        {
        }

    public:

#ifdef USE_VOICE_API
        // Echange::IVoiceProducer overrides (all DEPRECATED)
        Core::hresult Callback(Exchange::IVoiceHandler* callback) override
        {
            Core::hresult result = Core::ERROR_NONE;

            _adminLock.Lock();

            if (_audioHandler != nullptr) {
                result = Core::ERROR_UNAVAILABLE; // someone already installed IAudioStream::ICallback callback
            }
            else {
                if (_voiceHandler != nullptr) {
                    _voiceHandler->Release();
                }

                if (callback != nullptr) {
                    callback->AddRef();

                    // TODO SIGNAL STARTED
                }

                _voiceHandler = callback;
            }

            _adminLock.Unlock();

            return (result);
        }
        string Name() const override
        {
            string name;
            Name(name);
            return (name);
        }
        virtual uint32_t Error() const override
        {
            return (_gattRemote != nullptr ? Core::ERROR_NONE : Core::ERROR_ILLEGAL_STATE);
        }
        virtual string MetaData() const override
        {
            string result;

            _adminLock.Lock();

            if (_gattRemote != nullptr) {
                GATTRemote::Data info;

                info.Name = _gattRemote->Name();
                info.Address = _gattRemote->Address();
                info.ModelNumber = _gattRemote->ModelNumber();
                info.SerialNumber = _gattRemote->SerialNumber();
                info.FirmwareRevision = _gattRemote->FirmwareRevision();
                info.SoftwareRevision = _gattRemote->SoftwareRevision();
                info.ManufacturerName = _gattRemote->ManufacturerName();
                info.BatteryLevelHandle = _batteryLevel;

                info.ToString(result);
            }

            _adminLock.Unlock();

            return (result);
        }
        virtual void Configure(const string& settings) override
        {
            _adminLock.Lock();

            if (_gattRemote != nullptr) {
                _gattRemote->Decoder(settings);
            }

            _adminLock.Unlock();
        }
#endif // USE_VOICE_API

        // Echange::IAudioStream overrides
        Core::hresult Register(Exchange::IAudioStream::INotification* const notification) override
        {
            Core::hresult result = Core::ERROR_ALREADY_CONNECTED;

            ASSERT(notification != nullptr);

            _adminLock.Lock();

            auto it = std::find(_streamObservers.begin(), _streamObservers.end(), notification);

            ASSERT(it == _streamObservers.end());

            if (it == _streamObservers.end()) {
                notification->AddRef();
                _streamObservers.push_back(notification);

                if ((_gattRemote != nullptr) && (_gattRemote->SelectedProfile().IsSet() == true)) {
                    if (_transmission == true) {
                        for (auto& observer : _streamObservers) {
                            observer->StateChanged(Exchange::IAudioStream::STARTED, _gattRemote->SelectedProfile());
                        }
                    }
                    else {
                        for (auto& observer : _streamObservers) {
                            observer->StateChanged(Exchange::IAudioStream::IDLE, {});
                        }
                    }
                }

                result = Core::ERROR_NONE;
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult Unregister(const Exchange::IAudioStream::INotification* const notification) override
        {
            Core::hresult result = Core::ERROR_ALREADY_RELEASED;

            ASSERT(notification != nullptr);

            _adminLock.Lock();

            auto it = std::find(_streamObservers.cbegin(), _streamObservers.cend(), notification);
            ASSERT(it != _streamObservers.cend());

            if (it != _streamObservers.cend()) {
                (*it)->Release();
                _streamObservers.erase(it);

                result = Core::ERROR_NONE;
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult Callback(Exchange::IAudioStream::ICallback* const callback) override
        {
            Core::hresult result = Core::ERROR_NONE;

            _adminLock.Lock();

#ifdef USE_VOICE_API
            if (_voiceHandler != nullptr) {
                result = Core::ERROR_UNAVAILABLE; // someone already installed a IVoiceHandler callback
            }
            else
#endif

            {
                if (_audioHandler != nullptr) {
                    _audioHandler->Release();
                }

                if (callback != nullptr) {
                    callback->AddRef();
                }

                _audioHandler = callback;
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult Name(string& name) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _adminLock.Lock();

            if (_gattRemote != nullptr) {
                name = _gattRemote->Name();
                result = Core::ERROR_NONE;
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult Profile(const audioprofile& profile) override
        {
            Core::hresult result = Core::ERROR_BAD_REQUEST;

            _adminLock.Lock();

            if (_gattRemote != nullptr) {
                auto const& p = _gattRemote->SelectedProfile();

                // Only allow to toggle codec (PCM or IMA-ADPCM)
                if (p.IsSet() == true) {
                    if ((profile.sampleRate == p.Value().sampleRate) && (profile.resolution == p.Value().resolution) && (profile.channels == p.Value().channels)) {
                        if ((profile.codec == Exchange::IAudioStream::codectype::PCM) || (profile.codec == Exchange::IAudioStream::codectype::IMA_ADPCM)) {

                            _gattRemote->Decoder(profile);
                            result = (_transmission == true? Core::ERROR_INPROGRESS : Core::ERROR_NONE);
                        }
                    }
                }
            }
            else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult Profile(audioprofile& profile) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _adminLock.Lock();

            if (_gattRemote != nullptr) {
                auto const& p = _gattRemote->SelectedProfile();

                if (p.IsSet() == true) {
                    profile = p.Value();
                    result = Core::ERROR_NONE;
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult State(Exchange::IAudioStream::streamstate& state) const override
        {
            _adminLock.Lock();

            if ((_gattRemote == nullptr) || (_gattRemote->SelectedProfile().IsSet() == false)) {
                state = Exchange::IAudioStream::streamstate::UNAVAILABLE;
            }
            else if (_transmission == false) {
                state = Exchange::IAudioStream::streamstate::IDLE;
            }
            else {
                state = Exchange::IAudioStream::streamstate::STARTED;
            }

            _adminLock.Unlock();

            return (Core::ERROR_NONE);
        }
        Core::hresult Capabilities(Exchange::IAudioStream::codectype& caps) const override
        {
            caps = static_cast<Exchange::IAudioStream::codectype>(static_cast<uint16_t>(Exchange::IAudioStream::codectype::PCM) | static_cast<uint16_t>(Exchange::IAudioStream::IMA_ADPCM));
            return (Core::ERROR_NONE);
        }
        Core::hresult Time(uint32_t& time VARIABLE_IS_NOT_USED) const override
        {
            return (Core::ERROR_NOT_SUPPORTED);
        }
        Core::hresult Speed(const uint8_t speed VARIABLE_IS_NOT_USED) override
        {
            // This is live stream, no trickplays allowed :)
            return (Core::ERROR_NOT_SUPPORTED);
        }
        Core::hresult Speed(uint8_t& speed) const override
        {
            Core::hresult result = Core::ERROR_NONE;

            _adminLock.Lock();

            if ((_gattRemote == nullptr) || (_gattRemote->SelectedProfile().IsSet() == false)) {
                result = Core::ERROR_ILLEGAL_STATE;
            }
            else {
                if (_transmission == true) {
                    speed = 100;
                }
                else {
                    speed = 0;
                }
            }

            _adminLock.Unlock();

            return (result);
        }

        // Exchange::IBluetoothRemoteControl overrides
        uint32_t Assign(const string& address) override;
        uint32_t Revoke() override;

        Core::hresult Register(Exchange::IBluetoothRemoteControl::INotification* const notification) override
        {
            Core::hresult result = Core::ERROR_ALREADY_CONNECTED;

            ASSERT(notification != nullptr);

            _adminLock.Lock();

            auto it = std::find(_observers.begin(), _observers.end(), notification);

            ASSERT(it == _observers.end());

            if (it == _observers.end()) {
                notification->AddRef();
                _observers.push_back(notification);

                if ((_gattRemote != nullptr) && (_batteryLevel != static_cast<uint8_t>(~0))) {
                    notification->BatteryLevelChange(_batteryLevel);
                }

                result = Core::ERROR_NONE;
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult Unregister(const Exchange::IBluetoothRemoteControl::INotification* const notification) override
        {
            Core::hresult result = Core::ERROR_ALREADY_RELEASED;

            ASSERT(notification != nullptr);

            _adminLock.Lock();

            auto it = std::find(_observers.cbegin(), _observers.cend(), notification);
            ASSERT(it != _observers.cend());

            if (it != _observers.cend()) {
                (*it)->Release();
                _observers.erase(it);

                result = Core::ERROR_NONE;
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult BatteryLevel(uint8_t& level) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _adminLock.Lock();

            if (_gattRemote != nullptr) {
                if (_batteryLevel != static_cast<uint8_t>(~0)) {
                    level = _batteryLevel;
                    result = Core::ERROR_NONE;
                }
                else {
                    result = Core::ERROR_UNAVAILABLE;
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult Metadata(Exchange::IBluetoothRemoteControl::unitmetadata& info) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _adminLock.Lock();

            if (_gattRemote != nullptr) {
                info.name = _gattRemote->Name();
                info.model = _gattRemote->ModelNumber();
                info.serial = _gattRemote->SerialNumber();
                info.firmware = _gattRemote->FirmwareRevision();
                info.software = _gattRemote->SoftwareRevision();
                info.manufacturer = _gattRemote->ManufacturerName();

                result = Core::ERROR_NONE;
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult Device(string& address) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            _adminLock.Lock();

            if (_gattRemote != nullptr) {
                address = _gattRemote->Address();
                result = Core::ERROR_NONE;
            }

            _adminLock.Unlock();

            return (result);
        }
        Core::hresult VoiceControl(bool& value) const override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            if (_gattRemote != nullptr) {

                auto const& profile = _gattRemote->SelectedProfile();

                if (profile.IsSet() == true) {
                    value = _gattRemote->VoiceOutput();
                    result = Core::ERROR_NONE;
                }
                else {
                    result = Core::ERROR_NOT_SUPPORTED;
                }
            }

            return (result);
        }
        Core::hresult VoiceControl(const bool value) override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            if (_gattRemote != nullptr) {
                auto const& profile = _gattRemote->SelectedProfile();

                if (profile.IsSet() == true) {
                    result = _gattRemote->VoiceOutput(value);
                }
                else {
                    result = Core::ERROR_NOT_SUPPORTED;
                }
            }

            return (result);
        }

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;


        BEGIN_INTERFACE_MAP(BluetoothRemoteControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
#ifdef USE_VOICE_API
            INTERFACE_ENTRY(Exchange::IVoiceProducer)
#endif
            INTERFACE_ENTRY(Exchange::IAudioStream)
            INTERFACE_ENTRY(Exchange::IBluetoothRemoteControl)
        END_INTERFACE_MAP

    private:
        // Exchange::JBluetoothRemoteControl::IHander overrides
        void OnBatteryLevelChangeEventRegistration(const string& client, const JSONRPCSupportsEventStatus::Status status) override
        {
            if (status == PluginHost::JSONRPCSupportsEventStatus::Status::registered) {

                _adminLock.Lock();

                const uint8_t level = (_gattRemote == nullptr? ~0 : _batteryLevel);

                _adminLock.Unlock();

                if (level != static_cast<uint8_t>(~0)) {
                    Exchange::JBluetoothRemoteControl::Event::BatteryLevelChange(*this, level,
                        [&client](const string& designator) {
                            // Only send to the client that has just registered...
                            return (designator.substr(0, designator.find('.')) == client);
                        });
                }
            }
        }

        // Exchange::JAudioStream::IHander overrides
        void OnStateChangedEventRegistration(const string& client, const JSONRPCSupportsEventStatus::Status status) override
        {
            if (status == JSONRPCSupportsEventStatus::Status::registered) {

                _adminLock.Lock();

                _adminLock.Unlock();

                if (_gattRemote != nullptr) {
                    if (_transmission == true) {
                        Exchange::JAudioStream::Event::StateChanged(*this, Exchange::IAudioStream::STARTED, _gattRemote->SelectedProfile(),
                            [&client](const string& designator) {
                                // Only send to the client that has just registered...
                                return (designator.substr(0, designator.find('.')) == client);
                            });
                    }
                    else {
                        Exchange::JAudioStream::Event::StateChanged(*this, Exchange::IAudioStream::IDLE, {},
                            [&client](const string& designator) {
                                // Only send to the client that has just registered...
                                return (designator.substr(0, designator.find('.')) == client);
                            });
                    }
                }
            }
        }

    private:
        // Bluetooth Remote Control implementation
        void Connected(const string& name);
        void Operational(const GATTRemote::Data& settings);
        void SendOut(const uint32_t length);
        void SendOut(const uint32_t seq, const uint32_t length, const uint8_t dataBuffer[]);
        void Inoperational();
        void VoiceData();
        void VoiceData(const Exchange::IAudioStream::audioprofile& profile);
        void VoiceData(const uint32_t seq, const uint16_t length, const uint8_t dataBuffer[]);
        void KeyEvent(const bool pressed, const uint32_t keyCode);
        void UpdateBatteryLevel(const uint8_t level);

    private:
        static uint32_t TimeToBytes(const uint32_t time, const uint32_t rate, const uint8_t channels, const uint8_t res)
        {
            return ((time * (rate / 10) * channels * (res / 8)) / 100);
        }
        static uint32_t TimeToBytes(const uint32_t time, const Exchange::IAudioStream::audioprofile& profile)
        {
            return (TimeToBytes(time, profile.sampleRate, profile.channels, profile.resolution));
        }

    private:
        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        GATTRemote* _gattRemote;
        string _name;
        string _controller;
        string _keyMap;
        string _configLine;
        string _recordFile;
        uint8_t _batteryLevel;
#ifdef USE_VOICE_API
        Exchange::IVoiceHandler* _voiceHandler; /* deprecated */
#endif
        Exchange::IAudioStream::ICallback* _audioHandler;
        PluginHost::VirtualInput* _inputHandler;
        recorder _record;
        WAV::Recorder _recorder;
        RingBufferType<uint32_t>* _buffer;
        uint16_t _sequence;
        uint32_t _firstAudioChunkSize;
        uint32_t _audioChunkSize;
        std::vector<Exchange::IBluetoothRemoteControl::INotification*> _observers;
        std::vector<Exchange::IAudioStream::INotification*> _streamObservers;
        bool _transmission;

    }; // class BluetoothRemoteControl

} //namespace Plugin

}
