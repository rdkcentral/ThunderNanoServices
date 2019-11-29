#include "BluetoothRemoteControl.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(BluetoothRemoteControl, 1, 0);

    const string BluetoothRemoteControl::Initialize(PluginHost::IShell* service)
    {
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

        Core::Directory storageDir (_service->PersistentPath().c_str(), _T("*.json"));

        if (storageDir.IsDirectory() == false) {
            if (storageDir.CreatePath() == false) {
                TRACE(Trace::Error, (_T("Failed to create persistent storage folder [%s]"), storageDir.Name().c_str()));
            } 
        }
        else {
            Exchange::IBluetooth* bluetoothCtl(_service->QueryInterfaceByCallsign<Exchange::IBluetooth>(_controller));

            if (bluetoothCtl != nullptr) {
 
                while ( (_gattRemote == nullptr) && (storageDir.Next() == false) ) {
                    string filename = Core::File::FileName(storageDir.Name());

                    // See if this is a Bluetooth MAC Address, if so, lets load it..
                    Bluetooth::Address address (filename.c_str());

                    if (address.IsValid() == true)  {
                        Exchange::IBluetooth::IDevice* device = bluetoothCtl->Device(filename);
                        Core::File fileData(storageDir.Current().c_str(), true);

                        if ( (device != nullptr) && (fileData.Open(true) == true) ){
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

        if (config.KeyIngest.Value() == true) {
            _inputHandler = PluginHost::InputHandler::Handler();
            ASSERT(_inputHandler != nullptr);
        }

        return (string());
    }

    void BluetoothRemoteControl::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        
        if (_gattRemote != nullptr) {
            delete _gattRemote;
            _gattRemote = nullptr;
        }
        if (_keyHandler != nullptr) {
            _keyHandler->Release();
            _keyHandler = nullptr;
        }
        if (_voiceHandler != nullptr) {
            _voiceHandler->Release();
            _voiceHandler = nullptr;
        }
        _service->Release();
        _service = nullptr;
    }

    string BluetoothRemoteControl::Information() const
    {
        return { };
    }

    void BluetoothRemoteControl::Inbound(WPEFramework::Web::Request& request)
    {
        // Not needed
    }

    Core::ProxyType<Web::Response> BluetoothRemoteControl::Process(const WPEFramework::Web::Request& request)
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

    Core::ProxyType<Web::Response> BluetoothRemoteControl::GetMethod(Core::TextSegmentIterator& index)
    {
        Core::ProxyType<Web::Response> response(PluginHost::Factories::Instance().Response());
        response->ErrorCode = Web::STATUS_BAD_REQUEST;
        response->Message = _T("Unsupported GET request.");

        return (response);
    }

    Core::ProxyType<Web::Response> BluetoothRemoteControl::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> response(PluginHost::Factories::Instance().Response());
        response->ErrorCode = Web::STATUS_BAD_REQUEST;
        response->Message = _T("Unsupported PUT request");

        // Nothing here yet

        return (response);
    }

    Core::ProxyType<Web::Response> BluetoothRemoteControl::PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> response(PluginHost::Factories::Instance().Response());
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

    Core::ProxyType<Web::Response> BluetoothRemoteControl::DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> response(PluginHost::Factories::Instance().Response());
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
                Exchange::IBluetooth::IDevice* device = bluetoothCtl->Device(address);
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

    void BluetoothRemoteControl::Operational(const GATTRemote::Data& settings)
    {
        ASSERT (_gattRemote != nullptr);

        // Store the settings, if not already done..
        Core::File settingsFile(_service->PersistentPath() + _gattRemote->Address() + _T(".json"));
        if ( (settingsFile.Exists() == false) && (settingsFile.Create() == true) ) {
            settings.IElement::ToFile(settingsFile);
            settingsFile.Close();
        }

        if (_inputHandler != nullptr) {
            // Load the keyMap for this remote
            string keyMapFile(_service->DataPath() + _gattRemote->Name() + _T(".json"));
            if (Core::File(keyMapFile).Exists() == true) {
                TRACE(Trace::Information, (_T("Loading keymap file [%s] for remote [%s]"), keyMapFile.c_str(), _gattRemote->Name().c_str()));
                PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(_gattRemote->Name()));

                uint32_t result = map.Load(keyMapFile);
                if (result != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Failed to load keymap file [%s]"), keyMapFile.c_str()));
                }
            }
        }
    }

    void BluetoothRemoteControl::VoiceData(Exchange::IVoiceProducer::IProfile* profile)
    {
        _adminLock.Lock();
        if (_voiceHandler != nullptr) {
        }
        _adminLock.Unlock();
    }

    void BluetoothRemoteControl::VoiceData(const uint16_t length, const uint8_t dataBuffer[])
    {
        _adminLock.Lock();
        if (_voiceHandler != nullptr) {
        }
        _adminLock.Unlock();
    }

    void BluetoothRemoteControl::KeyEvent(const bool pressed, const uint16_t keyCode)
    {
        _adminLock.Lock();
        if (_keyHandler != nullptr) {
        }
        _adminLock.Unlock();
    }

    void BluetoothRemoteControl::BatteryLevel(const uint8_t level)
    {
        _batteryLevel = level;
    }

} // namespace Plugin

}
