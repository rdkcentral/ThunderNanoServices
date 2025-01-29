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

#include "BluetoothRemoteControl.h"

namespace Thunder {

ENUM_CONVERSION_BEGIN(Plugin::BluetoothRemoteControl::recorder)

    { Plugin::BluetoothRemoteControl::recorder::OFF,               _TXT("off")               },
    { Plugin::BluetoothRemoteControl::recorder::SINGLE,            _TXT("single")            },
    { Plugin::BluetoothRemoteControl::recorder::SEQUENCED,         _TXT("sequenced")         },
    { Plugin::BluetoothRemoteControl::recorder::SINGLE_PERSIST,    _TXT("single_persist")    },
    { Plugin::BluetoothRemoteControl::recorder::SEQUENCED_PERSIST, _TXT("sequenced_persist") },

ENUM_CONVERSION_END(Plugin::BluetoothRemoteControl::recorder)

namespace Plugin {

    namespace {

        static Metadata<BluetoothRemoteControl> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::BLUETOOTH },
            // Terminations
            { subsystem::NOT_BLUETOOTH },
            // Controls
            {}
        );
    }

    template<typename FROM, typename TO>
    class LUT {
    public:
        struct Entry {
            FROM from;
            TO to;
        };

    public:
        LUT() = delete;
        LUT(const LUT<FROM,TO>&) = delete;
        LUT<FROM,TO>& operator= (const LUT<FROM,TO>&) = delete;

        template<size_t N>
        LUT(const Entry (&input)[N]) : _lut() {
            for (uint8_t index = 0; index < N; index++) {
                _lut.emplace_back(input[index]);
            }
        }

    public:
        bool Lookup(const FROM& from, TO& value) const {
            uint8_t index = 0;
            while ( (index < _lut.size()) && (_lut[index].from != from) ) {
                index++;
            }
            if (index < _lut.size()) {
                value = _lut[index].to;
                return (true);
            }
            return (false);
        }
        bool Lookup(const TO& from, FROM& value) const {
            uint8_t index = 0;
            while ( (index < _lut.size()) && (_lut[index].to != from) ) {
                index++;
            }
            if (index < _lut.size()) {
                value = _lut[index].from;
                return (true);
            }
            return (false);
        }


    private:
        std::vector<Entry> _lut;
    };

    static const LUT<Exchange::IAudioStream::codectype, WAV::Recorder::codec> CodecTable({

        { Exchange::IAudioStream::codectype::PCM,       WAV::Recorder::codec::PCM   },
        { Exchange::IAudioStream::codectype::IMA_ADPCM, WAV::Recorder::codec::ADPCM }

    });

    const string BluetoothRemoteControl::Initialize(PluginHost::IShell* service)
    {
        string sequence;

        ASSERT(_service == nullptr);
        ASSERT(_gattRemote == nullptr);

        _service = service;
        _service->AddRef();
        _configLine = _service->ConfigLine();

        Config config;
        config.FromString(_configLine);

        ASSERT (_service->PersistentPath().empty() == false);

        _controller = config.Controller.Value();
        _record     = config.Recorder.Value();
        _keyMap     = config.KeyMap.Value();

        if ((_record & 0x0F) == 0) {
            sequence = ("voice.wav");
        }
        else {
            sequence = ("voice_%02d_%02d_%02d.wav");
        }

        if ((_record & 0xF0) == 0x20) {
            _recordFile = _service->PersistentPath() + sequence;
        }
        else {
            _recordFile = _service->VolatilePath() + sequence;
        }

        if (config.KeyIngest.Value() == true) {
            _inputHandler = PluginHost::InputHandler::Handler();
            ASSERT(_inputHandler != nullptr);
        }

        if (Core::File(_service->PersistentPath()).IsDirectory() == false) {
            if (Core::Directory(_service->PersistentPath().c_str()).CreatePath() == false) {
                TRACE(Trace::Error, (_T("Failed to create persistent storage folder [%s]"), _service->PersistentPath().c_str()));
            }
        }
        else {
            Exchange::IBluetooth* bluetoothCtl(_service->QueryInterfaceByCallsign<Exchange::IBluetooth>(_controller));

            if (bluetoothCtl != nullptr) {
                Core::Directory storageDir (_service->PersistentPath().c_str(), _T("*.json"));

                while ( (_gattRemote == nullptr) && (storageDir.Next() == true) ) {
                    string filename = Core::File::FileName(storageDir.Name());

                    // See if this is a Bluetooth MAC Address, if so, lets load it..
                    Bluetooth::Address address (filename.c_str());

                    if (address.IsValid() == true)  {
                        Exchange::IBluetooth::IDevice* device = bluetoothCtl->Device(filename, Exchange::IBluetooth::IDevice::ADDRESS_LE_PUBLIC);
                        Core::File fileData(storageDir.Current().c_str());

                        if (device != nullptr) {
                            if (fileData.Open(true) == true) {
                                GATTRemote::Data data;
                                data.IElement::FromFile(fileData);

                                // Seems we have a "real" device, load it..
                                _gattRemote = new GATTRemote(this, device, _configLine, data);
                            }

                            device->Release();
                        }
                    }
                }

                bluetoothCtl->Release();
            }
        }

        Exchange::JAudioStream::Register(*this, this, this);
        Exchange::JBluetoothRemoteControl::Register(*this, this, this);

        return (string());
    }

    void BluetoothRemoteControl::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ASSERT(service != nullptr);
        ASSERT(_service = service);

        Exchange::JAudioStream::Unregister(*this);
        Exchange::JBluetoothRemoteControl::Unregister(*this);

        if (_recorder.IsOpen() == true) {
            _recorder.Close();
        }

        if (_gattRemote != nullptr) {
            delete _gattRemote;
            _gattRemote = nullptr;
        }

        if (_audioHandler != nullptr) {
            _audioHandler->Release();
            _audioHandler = nullptr;
        }

#ifdef USE_VOICE_API
        if (_voiceHandler != nullptr) {
            _voiceHandler->Release();
            _voiceHandler = nullptr;
        }
#endif

        if (_buffer != nullptr) {
            delete _buffer;
            _buffer = nullptr;
        }

        _service->Release();
        _service = nullptr;
    }

    string BluetoothRemoteControl::Information() const
    {
        return { };
    }

    uint32_t BluetoothRemoteControl::Assign(const string& address)
    {
        uint32_t result = Core::ERROR_ALREADY_CONNECTED;

        _adminLock.Lock();

        if (_gattRemote == nullptr) {
            ASSERT(_service != nullptr);
            Exchange::IBluetooth* bluetoothCtl(_service->QueryInterfaceByCallsign<Exchange::IBluetooth>(_controller));
            if (bluetoothCtl != nullptr) {
                Exchange::IBluetooth::IDevice* device = bluetoothCtl->Device(address, Exchange::IBluetooth::IDevice::ADDRESS_LE_PUBLIC);
                if (device != nullptr) {
                    _gattRemote = new GATTRemote(this, device, _configLine);
                    if (_gattRemote != nullptr) {
                        result = Core::ERROR_NONE;
                    } else {
                        TRACE(Trace::Error, (_T("Failed to create BLE GATT remote control")));
                        result = Core::ERROR_GENERAL;
                    }
                } else {
                    TRACE(Trace::Error, (_T("Device [%s] is not known"), address.c_str()));
                    result = Core::ERROR_UNKNOWN_KEY;
                }

                bluetoothCtl->Release();
            } else {
                TRACE(Trace::Error, (_T("Bluetooth is not available")));
                result = Core::ERROR_UNAVAILABLE;
            }
        } else {
            TRACE(Trace::Error, (_T("A remote is already assigned, revoke first")));
        }

        _adminLock.Unlock();

        return (result);
    }

    uint32_t BluetoothRemoteControl::Revoke()
    {
        uint32_t result = Core::ERROR_ALREADY_RELEASED;

        _adminLock.Lock();

        if (_gattRemote != nullptr) {
            Core::File file(_service->PersistentPath() + _gattRemote->Address() + _T(".json"));
            if (file.Destroy() == true) {
                TRACE(Trace::Information, (_T("BLE GATT remote control unit [%s] removed from persistent storage"), _gattRemote->Address().c_str()));
            }

            delete _gattRemote;
            _gattRemote = nullptr;
            result = Core::ERROR_NONE;
        } else {
            TRACE(Trace::Error, (_T("A remote has not been assigned")));
        }

        _adminLock.Unlock();

        return (result);
    }

    void BluetoothRemoteControl::Connected(const string& name)
    {
        _adminLock.Lock();

        _name = name;

        if (_inputHandler != nullptr) {
            // Load the keyMap for this remote
            string keyMapFile(_service->DataPath() + (_keyMap.empty() ? name : _keyMap) + _T(".json"));
            TRACE(Trace::Information, (_T("Loading keymap file [%s] for remote [%s]"), keyMapFile.c_str(), _name.c_str()));

            if (Core::File(keyMapFile).Exists() == true) {
                PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(name));

                Core::File mappingFile(keyMapFile);
                Core::JSON::ArrayType<PluginHost::VirtualInput::KeyMap::KeyMapEntry> mappingTable;

                if (mappingFile.Open(true) == true) {
                    Core::OptionalType<Core::JSON::Error> error;
                    mappingTable.IElement::FromFile(mappingFile, error);
                    if (error.IsSet() == true) {
                        TRACE(Trace::Error, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                    } else {
                        if (map.Import(mappingTable) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to load keymap file [%s]"), keyMapFile.c_str()));
                        }
                    }
                    mappingFile.Close();
                }

            } else {
                TRACE(Trace::Information, (_T("Keymap file [%s] not available"), keyMapFile.c_str()));
            }
        }

        _adminLock.Unlock();
    }

    void BluetoothRemoteControl::Inoperational()
    {
        _adminLock.Lock();

        if (_transmission == true) {
            VoiceData();
        }

        _batteryLevel = ~0;
        _name.clear();

        delete _buffer;
        _buffer = nullptr;

        _adminLock.Unlock();

        Exchange::JAudioStream::Event::StateChanged(*this, Exchange::IAudioStream::UNAVAILABLE, {});
    }

    void BluetoothRemoteControl::Operational(const GATTRemote::Data& settings)
    {
        _adminLock.Lock();

        ASSERT (_gattRemote != nullptr);

        if (settings.VoiceCommandHandle != 0) {
            _gattRemote->DecoderFromConfigLine(_configLine);

            if (_buffer != nullptr) {
                delete _buffer;
                _buffer = nullptr;
            }

            _transmission = false;

            const auto profile = _gattRemote->SelectedProfile();

            if (profile.IsSet() == true) {

                Config config;
                config.FromString(_configLine);
                const uint16_t maxChunkDuration = std::max(config.AudioChunkSize.Value(), config.FirstAudioChunkSize.Value());

                if (maxChunkDuration != 0) {

                    // If buffer size is not specified, then set it based on the configured chunk sizes
                    const uint32_t maxDuration = std::max(config.AudioBufferSize.Value(), (maxChunkDuration + config.AudioChunkSize.Value()));
                    const uint32_t bufferSize = TimeToBytes(maxDuration, profile);
                    _firstAudioChunkSize = TimeToBytes(config.FirstAudioChunkSize.Value(), profile);
                    _audioChunkSize = TimeToBytes(config.AudioChunkSize.Value(), profile);

                    // Some sanity...
                    ASSERT(bufferSize <= (512 * 1024));
                    ASSERT(_firstAudioChunkSize <= (512 * 1024));
                    ASSERT(_audioChunkSize <= (512 * 1024));

                    _buffer = new RingBufferType<uint32_t>(bufferSize);
                    ASSERT(_buffer != nullptr);

                    TRACE(Trace::Information, (_T("Audio buffer size is %d bytes (%d ms)"), bufferSize, maxDuration));

                    if (_firstAudioChunkSize != 0) {
                        TRACE(Trace::Information, (_T("First audio chunk size is %d bytes (%d ms)"), _firstAudioChunkSize, config.FirstAudioChunkSize.Value()));
                    }
                    else {
                        TRACE(Trace::Information, (_T("Audio will not be pre-buffered")));
                    }

                    if (_audioChunkSize != 0) {
                        TRACE(Trace::Information, (_T("Audio chunk size is %d bytes (%d ms)"), _audioChunkSize, config.AudioChunkSize.Value()));
                    }
                    else {
                        TRACE(Trace::Information, (_T("Audio chunks will not be buffered")));
                    }
                }
                else {
                    TRACE(Trace::Information, (_T("Audio buffering is disabled")));
                }
            }
        }

        // Store the settings, if not already done..
        Core::File settingsFile(_service->PersistentPath() + _gattRemote->Address() + _T(".json"));
        if ( (settingsFile.Exists() == false) && (settingsFile.Create() == true) ) {
            settings.IElement::ToFile(settingsFile);
            settingsFile.Close();
        }

        _adminLock.Unlock();

        Exchange::JAudioStream::Event::StateChanged(*this, Exchange::IAudioStream::IDLE, {});
    }

    void BluetoothRemoteControl::VoiceData(const Exchange::IAudioStream::audioprofile& profile)
    {
        _adminLock.Lock();

        if (_transmission == true) {
            // This should not happen, but let's be prepared
            TRACE(Trace::Error, (_T("Missing end of transmission event from the unit")));
            VoiceData();
        }

        ASSERT(_gattRemote != nullptr);
        ASSERT(_gattRemote->SelectedProfile().IsSet() == true);

        if (_audioHandler != nullptr) {
            for (auto& observer : _streamObservers) {
                observer->StateChanged(Exchange::IAudioStream::STARTED, _gattRemote->SelectedProfile());
            }
        }
#ifdef USE_VOICE_API
        else if (_voiceHandler != nullptr) {
            _voiceHandler->Start(_gattRemote->SelectedAudioProfile());
        }
#endif
        else {
            Exchange::JAudioStream::Event::StateChanged(*this, Exchange::IAudioStream::STARTED, _gattRemote->SelectedProfile());
        }

        _sequence = 0;
        _transmission = true;

        _adminLock.Unlock();

        if (_recorder.IsOpen() == true) {
            _recorder.Close();
        }

        if (_record != recorder::OFF) {

            WAV::Recorder::codec wavCodec;
            TCHAR fileName[256];
            Core::Time now (Core::Time::Now());
            ::snprintf(fileName, sizeof(fileName), _recordFile.c_str(), now.Hours(), now.Minutes(), now.Seconds());

            if (CodecTable.Lookup(profile.codec, wavCodec) == true) {
                _recorder.Open(string(fileName), wavCodec, profile.channels, profile.sampleRate, profile.resolution);

                if (_recorder.IsOpen() == true) {
                    TRACE(Trace::Information, (_T("Recorder started on: %s"), fileName));
                }
            }
        }

        TRACE(Trace::Information, (_T("Audio transmission start")));
    }

    void BluetoothRemoteControl::VoiceData()
    {
        _adminLock.Lock();

        if (_transmission == true) {

            ASSERT(_buffer != nullptr);

            if (_buffer != nullptr) {
                if (_sequence != 0) {
                    // Push out whatever is left (but only if a complete first chunk went out)
                    SendOut(_buffer->Used());
                }

                _buffer->Reset();
            }

            _transmission = false;

            if (_audioHandler != nullptr) {
                for (auto& observer : _streamObservers) {
                    observer->StateChanged(Exchange::IAudioStream::IDLE, {});
                }
            }
#ifdef USE_VOICE_API
            else if (_voiceHandler != nullptr) {
                _voiceHandler->Stop();
            }
#endif
            else {
                Exchange::JAudioStream::Event::StateChanged(*this, Exchange::IAudioStream::IDLE, {});
            }

            _adminLock.Unlock();

            if (_recorder.IsOpen() == true) {
                _recorder.Close();
            }

            TRACE(Trace::Information, (_T("Audio transmission end")));
        }
        else {
            _adminLock.Unlock();

            TRACE(Trace::Error, (_T("Missing start of transmission event from the unit")));
        }
    }

    void BluetoothRemoteControl::VoiceData(const uint32_t seq, const uint16_t length, const uint8_t dataBuffer[])
    {
        _adminLock.Lock();

        if (_transmission == true) {

            const uint32_t chunkSize = ((_firstAudioChunkSize != 0) && (_sequence == 0)? _firstAudioChunkSize : _audioChunkSize);

            if (chunkSize != 0) {
                ASSERT(_buffer != nullptr);

                if (_buffer->Free() < length) {
                    const uint32_t newSize = std::max((_buffer->Capacity() * 2), (_buffer->Capacity() + length));
                    TRACE(Trace::Warning, (_T("Ring buffer size is too small, resizing from %d to %d bytes!"), _buffer->Capacity(), newSize));
                    _buffer->Resize(newSize);
                }

                const uint32_t written = _buffer->Push(length, dataBuffer);
                DEBUG_VARIABLE(written);
                ASSERT(written == length);

                while (_buffer->Used() >= chunkSize) {
                    SendOut(chunkSize);
                }
            }
            else if (length > 0) {
                SendOut(seq, length, dataBuffer);
            }

            _adminLock.Unlock();

            if (_recorder.IsOpen() == true) {
                _recorder.Write(length, dataBuffer);
            }
        }
        else {
            _adminLock.Unlock();
        }
    }

    void BluetoothRemoteControl::SendOut(const uint32_t seq, const uint32_t length, const uint8_t dataBuffer[])
    {
        ASSERT(length != 0);
        ASSERT(dataBuffer != nullptr);

        if (_audioHandler != nullptr) {
            _audioHandler->Data(seq, {}, length, dataBuffer);
        }
#ifdef USE_VOICE_API
        else if (_voiceHandler != nullptr) {
            _voiceHandler->Data(seq, dataBuffer, length);
        }
#endif
        else {
            string frame;
            Core::ToString(dataBuffer, length, true, frame);
            Exchange::JAudioStream::Event::Data(*this, seq, {}, length, frame);
        }
    }

    void BluetoothRemoteControl::SendOut(const uint32_t length)
    {
        if (length != 0) {
            uint8_t* const buffer = static_cast<uint8_t*>(ALLOCA(length));
            ASSERT(buffer != nullptr);

            const uint32_t read = _buffer->Pop(length, buffer);

            if (read > 0) {
                SendOut(_sequence, read, buffer);
                _sequence++;
            }
            else if (read < length) {
                ASSERT(!"not enough data");
            }
        }
    }

    void BluetoothRemoteControl::KeyEvent(const bool pressed, const uint32_t keyCode)
    {
        _adminLock.Lock();

        if (_inputHandler != nullptr) {
            uint32_t result = _inputHandler->KeyEvent(pressed, keyCode, _name);
            if (result == Core::ERROR_NONE) {
                TRACE(Trace::Information, ("Key send: %d (%s)", keyCode, pressed ? _T("pressed") : _T("released")));
            } else {
                TRACE(Trace::Information, ("Unknown key send: %d (%s)", keyCode, pressed ? _T("pressed") : _T("released")));
            }
        }

        _adminLock.Unlock();
    }

    void BluetoothRemoteControl::UpdateBatteryLevel(const uint8_t level)
    {
        printf("Battery level!!!!! %d\n", level);

        _adminLock.Lock();

        if (_batteryLevel != level) {

            _batteryLevel = level;

            for (auto const& observer : _observers) {
                observer->BatteryLevelChange(level);
            }

            Exchange::JBluetoothRemoteControl::Event::BatteryLevelChange(*this, level);

            TRACE(Trace::Information, (_T("Battery level: %d"), level));
        }

        _adminLock.Unlock();
    }

} // namespace Plugin

}
