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

    static const LUT<Exchange::IVoiceProducer::IProfile::codec, WAV::Recorder::codec> CodecTable({

        { Exchange::IVoiceProducer::IProfile::codec::PCM,   WAV::Recorder::codec::PCM   },
        { Exchange::IVoiceProducer::IProfile::codec::ADPCM, WAV::Recorder::codec::ADPCM }

    });

    const string BluetoothRemoteControl::Initialize(PluginHost::IShell* service)
    {
        string sequence;

        ASSERT(_service == nullptr);
        ASSERT(_gattRemote == nullptr);

        _service = service;
        _service->AddRef();
        _skipURL = _service->WebPrefix().length();
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

        return (string());
    }

    void BluetoothRemoteControl::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            if (_recorder.IsOpen() == true) {
                _recorder.Close();
            }

            if (_gattRemote != nullptr) {
                delete _gattRemote;
                _gattRemote = nullptr;
            }

            if (_voiceHandler != nullptr) {
                _voiceHandler->Release();
                _voiceHandler = nullptr;
            }

            if (_buffer != nullptr) {
                delete _buffer;
                _buffer = nullptr;
            }

            _service->Release();
            _service = nullptr;

        }
    }

    string BluetoothRemoteControl::Information() const
    {
        return { };
    }

    void BluetoothRemoteControl::Inbound(Thunder::Web::Request& /* request */)
    {
        // Not needed
    }

    Core::ProxyType<Web::Response> BluetoothRemoteControl::Process(const Thunder::Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());
        TRACE(Trace::Information, (_T("Received BluetoothRemoteControl request")));

        Core::ProxyType<Web::Response> response;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, (request.Path.length() - _skipURL)), false, '/');

        index.Next();
        index.Next();

        if (request.Verb == Web::Request::HTTP_GET) {
            response = GetMethod(index);
        } else if (request.Verb == Web::Request::HTTP_PUT) {
            response = PutMethod(index, request);
        } else if (request.Verb == Web::Request::HTTP_POST) {
            response = PostMethod(index, request);
        } else if (request.Verb == Web::Request::HTTP_DELETE) {
            response = DeleteMethod(index, request);
        }

        return (response);
    }

    Core::ProxyType<Web::Response> BluetoothRemoteControl::GetMethod(Core::TextSegmentIterator& /* index */)
    {
        Core::ProxyType<Web::Response> response(PluginHost::IFactories::Instance().Response());
        response->ErrorCode = Web::STATUS_BAD_REQUEST;
        response->Message = _T("Unsupported GET request.");

        return (response);
    }

    Core::ProxyType<Web::Response> BluetoothRemoteControl::PutMethod(Core::TextSegmentIterator& index, const Web::Request& /* request */)
    {
        Core::ProxyType<Web::Response> response(PluginHost::IFactories::Instance().Response());
        response->ErrorCode = Web::STATUS_BAD_REQUEST;
        response->Message = _T("Unsupported PUT request");

        // Nothing here yet
        if ((_gattRemote != nullptr) && (index.IsValid() == true)) {
            if ((index.Current() == _T("Voice")) && (index.Next() != false)) {
                if (index.Current() == _T("Off")) {
                    uint32_t result = _gattRemote->VoiceOutput(false);
                    if (result == Core::ERROR_NONE) {
                        response->ErrorCode = Web::STATUS_OK;
                        response->Message = _T("Voice disabled.");
                    }
                    else {
                        response->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                        response->Message = _T("Voice could not be disabled. Error: ") + Core::NumberType<uint32_t>(result).Text();
                    }
                } else if (index.Current() == _T("On")) {
                    uint32_t result = _gattRemote->VoiceOutput(true);
                    if (result == Core::ERROR_NONE) {
                        response->ErrorCode = Web::STATUS_OK;
                        response->Message = _T("Voice enabled.");
                    }
                    else {
                        response->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                        response->Message = _T("Voice could not be enabled. Error: ") + Core::NumberType<uint32_t>(result).Text();
                    }
                }
            }
        }

        return (response);
    }

    Core::ProxyType<Web::Response> BluetoothRemoteControl::PostMethod(Core::TextSegmentIterator& index, const Web::Request& /* request */)
    {
        Core::ProxyType<Web::Response> response(PluginHost::IFactories::Instance().Response());
        response->ErrorCode = Web::STATUS_BAD_REQUEST;
        response->Message = _T("Unsupported POST request");

        if (index.IsValid() == true) {
            if ((index.Current() == "Assign") && (index.Next() != false)) {
                uint32_t result = Assign(index.Current().Text());
                if (result == Core::ERROR_NONE) {
                    response->ErrorCode = Web::STATUS_OK;
                    response->Message = _T("OK");
                } else if (result == Core::ERROR_ALREADY_CONNECTED) {
                    response->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                    response->Message = _T("A remote is already assigned");
                } else if (result == Core::ERROR_UNKNOWN_KEY) {
                    response->ErrorCode = Web::STATUS_NOT_FOUND;
                    response->Message = _T("The requested Bluetooth device is unknown");
                } else if (result == Core::ERROR_UNAVAILABLE) {
                    response->ErrorCode = Web::STATUS_SERVICE_UNAVAILABLE;
                    response->Message = _T("Bluetooth is not available");
                } else {
                    response->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                    response->Message = _T("Failed to assign the remote control");
                }
            }
        }

        return (response);
    }

    Core::ProxyType<Web::Response> BluetoothRemoteControl::DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& /* request */)
    {
        Core::ProxyType<Web::Response> response(PluginHost::IFactories::Instance().Response());
        response->ErrorCode = Web::STATUS_BAD_REQUEST;
        response->Message = _T("Unsupported DELETE request");

        if (index.IsValid() == true) {
            if ((index.Current() == "Assign") && (index.Next() == false)) {
                uint32_t result = Revoke();
                if (result == Core::ERROR_NONE) {
                    response->ErrorCode = Web::STATUS_OK;
                    response->Message = _T("OK");
                } else if (result == Core::ERROR_ALREADY_RELEASED) {
                    response->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                    response->Message = _T("A remote is not assigned");
                } else {
                    response->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                    response->Message = _T("Failed to revoke the remote control");
                }
            }
        }

        return (response);
    }

    uint32_t BluetoothRemoteControl::Assign(const string& address)
    {
        uint32_t result = Core::ERROR_ALREADY_CONNECTED;

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

        return (result);
    }

    uint32_t BluetoothRemoteControl::Revoke()
    {
        uint32_t result = Core::ERROR_ALREADY_RELEASED;

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

        return (result);
    }

    void BluetoothRemoteControl::Connected(const string& name)
    {
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
    }

    void BluetoothRemoteControl::Operational(const GATTRemote::Data& settings)
    {
        ASSERT (_gattRemote != nullptr);

        _gattRemote->Decoder(_configLine);

        if (_buffer != nullptr) {
            delete _buffer;
            _buffer = nullptr;
        }

        Config config;
        config.FromString(_configLine);

        Exchange::IVoiceProducer::IProfile* profile = _gattRemote->SelectedProfile();
        ASSERT(profile != nullptr);

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

        profile->Release();

        // Store the settings, if not already done..
        Core::File settingsFile(_service->PersistentPath() + _gattRemote->Address() + _T(".json"));
        if ( (settingsFile.Exists() == false) && (settingsFile.Create() == true) ) {
            settings.IElement::ToFile(settingsFile);
            settingsFile.Close();
        }
    }

    void BluetoothRemoteControl::VoiceData(Exchange::IVoiceProducer::IProfile* profile)
    {
        if (profile != nullptr) {

            string codecText(_T("<<unknown>>"));
            Core::EnumerateType<Exchange::IVoiceProducer::IProfile::codec> codec(profile->Codec());

            if (codec.Data() != nullptr) {
                codecText = string(codec.Data());
            }

            _adminLock.Lock();

            _sequence = 0;

            if (_buffer != nullptr) {
                _buffer->Reset();
            }

            if (_voiceHandler != nullptr) {
                _voiceHandler->Start(profile);
            }
            else {
                event_audiotransmission(codecText);
            }

            _adminLock.Unlock();

            if (_recorder.IsOpen() == true) {
                _recorder.Close();
            }

            if (_record != recorder::OFF) {

                WAV::Recorder::codec wavCodec;
                TCHAR fileName[256];
                Core::Time now (Core::Time::Now());
                ::snprintf(fileName, sizeof(fileName), _recordFile.c_str(), now.Hours(), now.Minutes(), now.Seconds());


                if (CodecTable.Lookup(profile->Codec(), wavCodec) == true) {
                    _recorder.Open(string(fileName), wavCodec, profile->Channels(), profile->SampleRate(), profile->Resolution());

                    if (_recorder.IsOpen() == true) {
                        TRACE(Trace::Information, (_T("Recorder started on: %s"), fileName));
                    }
                }
            }

            TRACE(Trace::Information, (_T("Audio transmission: %s"), codecText.c_str()));
        }
        else {

            _adminLock.Lock();

            if ((_buffer != nullptr) && (_sequence != 0)) {
                // Push out whatever is left (but only if a complete first chunk went out)
                SendOut(_buffer->Used());
            }

            if (_voiceHandler != nullptr) {
                _voiceHandler->Stop();
            }
            else {
                event_audiotransmission(string());
            }

            _adminLock.Unlock();

            if (_recorder.IsOpen() == true) {
                _recorder.Close();
            }

            TRACE(Trace::Information, (_T("Audio transmission: end")));
        }
    }

    void BluetoothRemoteControl::VoiceData(const uint32_t seq, const uint16_t length, const uint8_t dataBuffer[])
    {
        _adminLock.Lock();

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

        if (_recorder.IsOpen() == true) {
            _recorder.Write(length, dataBuffer);
        }

        _adminLock.Unlock();
    }

    void BluetoothRemoteControl::SendOut(const uint32_t seq, const uint32_t length, const uint8_t dataBuffer[])
    {
        ASSERT(length != 0);
        ASSERT(dataBuffer != nullptr);

        if (_voiceHandler != nullptr) {
            _voiceHandler->Data(seq, dataBuffer, length);
        }
        else {
            string frame;
            Core::ToString(dataBuffer, length, true, frame);
            event_audioframe(seq, length, frame);
        }
    }

    void BluetoothRemoteControl::SendOut(const uint32_t length)
    {
        if (length != 0) {
            uint8_t* buffer = static_cast<uint8_t*>(ALLOCA(length));
            const uint32_t read = _buffer->Pop(length, buffer);

            if (read > 0) {
                SendOut(_sequence, read, buffer);
                _sequence++;
            }
            else if (read < length) {
                ASSERT(!"not enouqh data");
            }
        }
    }

    void BluetoothRemoteControl::KeyEvent(const bool pressed, const uint32_t keyCode)
    {
        _adminLock.Lock();
        if (_inputHandler != nullptr) {
            uint32_t result = _inputHandler->KeyEvent(pressed, keyCode, _name);
            if (result == Core::ERROR_NONE) {
                TRACE(Trace::Information, ("key send: %d (%s)", keyCode, pressed ? "pressed": "released"));
            } else {
                TRACE(Trace::Information, ("Unknown key send: %d (%s)", keyCode, pressed ? "pressed": "released"));
            }
        }

        _adminLock.Unlock();
    }

    void BluetoothRemoteControl::BatteryLevel(const uint8_t level)
    {
        printf ("Battery level!!!!! %d\n", level);
        _batteryLevel = level;
        event_batterylevelchange(level);
    }

} // namespace Plugin

}
