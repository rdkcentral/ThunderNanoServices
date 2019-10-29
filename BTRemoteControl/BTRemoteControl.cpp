#include "BTRemoteControl.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(BTRemoteControl, 1, 0);

    const string BTRemoteControl::Initialize(PluginHost::IShell* service)
    {
        string result;

        ASSERT(_service == nullptr);
        ASSERT(_gattRemote == nullptr);

        _service = service;
        _service->AddRef();
        _skipURL = _service->WebPrefix().length();

        _config.FromString(_service->ConfigLine());

        string persistentStoragePath = _service->PersistentPath();
        if (persistentStoragePath.empty() == false) {
            if (Core::Directory(persistentStoragePath.c_str()).CreatePath() == false) {
                TRACE(Trace::Error, (_T("Failed to create persistent storage folder [%s]"), persistentStoragePath.c_str()));
            } else {
                _settingsPath = persistentStoragePath + "Settings.json";

                Settings settings;
                if (LoadSettings(settings) == Core::ERROR_NONE) {
                    if ((settings.Name.IsSet() == true) && (settings.Address.IsSet() == true)) {
                        if (Assign(settings.Address.Value()) != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Failed to read settings")));
                        }
                    }
                }
            }
        }

        return result;
    }

    void BTRemoteControl::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        if (_gattRemote != nullptr) {
            _gattRemote->Release();
            _gattRemote = nullptr;
        }

        _service->Release();
        _service = nullptr;
    }

    string BTRemoteControl::Information() const
    {
        return { };
    }

    void BTRemoteControl::Inbound(WPEFramework::Web::Request& request)
    {
        // Not needed
    }

    Core::ProxyType<Web::Response> BTRemoteControl::Process(const WPEFramework::Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());
        TRACE(Trace::Information, (_T("Received BTRemoteControl request")));

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

    Core::ProxyType<Web::Response> BTRemoteControl::GetMethod(Core::TextSegmentIterator& index)
    {
        Core::ProxyType<Web::Response> response(PluginHost::Factories::Instance().Response());
        response->ErrorCode = Web::STATUS_BAD_REQUEST;
        response->Message = _T("Unsupported GET request.");

        if (index.IsValid() == true) {
            if (index.Current() == "Properties") {
                uint32_t result = Properties();
                if (result == Core::ERROR_NONE) {
                    response->ErrorCode = Web::STATUS_OK;
                    response->Message = _T("OK");
                } else {
                    response->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                    response->Message = _T("Failed to retrieve remote control properties");
                }
            }  else if (index.Current() == "BatteryLevel") {
                uint16_t level = ~0;
                uint32_t result = BatteryLevel(level);
                if (result == Core::ERROR_NONE) {
                    response->ErrorCode = Web::STATUS_OK;
                    response->Message = _T("OK");
                } else {
                    response->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                    response->Message = _T("Failed to retrieve remote control battery level");
                }
            }
        }

        return (response);
    }

    Core::ProxyType<Web::Response> BTRemoteControl::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> response(PluginHost::Factories::Instance().Response());
        response->ErrorCode = Web::STATUS_BAD_REQUEST;
        response->Message = _T("Unsupported PUT request");

        // Nothing here yet

        return (response);
    }

    Core::ProxyType<Web::Response> BTRemoteControl::PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
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

    Core::ProxyType<Web::Response> BTRemoteControl::DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request)
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

    uint32_t BTRemoteControl::Assign(const string& address)
    {
        uint32_t result = Core::ERROR_ALREADY_CONNECTED;

        if (_gattRemote == nullptr) {
            ASSERT(_service != nullptr);
            Exchange::IBluetooth* bluetoothCtl(_service->QueryInterfaceByCallsign<Exchange::IBluetooth>(_config.BtCtlCallsign.Value()));
            if (bluetoothCtl != nullptr) {
                Exchange::IBluetooth::IDevice* device = bluetoothCtl->Device(address);
                if (device != nullptr) {
                    if ((_config.MTU.IsSet() == true) && (_config.MTU.Value() != 0)) {
                        _gattRemote = Core::Service<GATTRemote>::Create<GATTRemote>(this, device, _config.MTU.Value());
                    } else {
                        _gattRemote = Core::Service<GATTRemote>::Create<GATTRemote>(this, device);
                    }
                    if (_gattRemote != nullptr) {
                        if ((_config.Audio.IsSet() == true) && (_config.Audio.ServiceUUID.IsSet() == true) \
                                && (_config.Audio.CommandUUID.IsSet() == true) && (_config.Audio.DataUUID.IsSet() == true)) {
                            if (_gattRemote->SetAudioUuids(_config.Audio.ServiceUUID.Value(), _config.Audio.CommandUUID.Value(), _config.Audio.DataUUID.Value()) == false) {
                                TRACE(Trace::Error, (_T("Failed to set audio UUIDs")));
                            }
                        }
                        _gattRemote->Callback(this);
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

    uint32_t BTRemoteControl::Revoke()
    {
        uint32_t result = Core::ERROR_ALREADY_RELEASED;

        if (_gattRemote != nullptr) {
            _gattRemote->Release();
            _gattRemote = nullptr;
            result = Core::ERROR_NONE;
        } else {
            TRACE(Trace::Error, (_T("A remote has not been assigned")));
        }

        return (result);
    }

    uint32_t BTRemoteControl::Properties()
    {
        uint32_t result = Core::ERROR_ALREADY_RELEASED;

        if (_gattRemote != nullptr) {
            // todo
            result = Core::ERROR_NONE;
        } else {
            TRACE(Trace::Error, (_T("A remote has not been assigned")));
        }

        return (result);
    }

    uint32_t BTRemoteControl::BatteryLevel(uint16_t& level)
    {
        uint32_t result = Core::ERROR_ALREADY_RELEASED;

        if (_gattRemote != nullptr) {
            // todo
            result = Core::ERROR_NONE;
        } else {
            TRACE(Trace::Error, (_T("A remote has not been assigned")));
        }

        return (result);
    }

    uint32_t BTRemoteControl::LoadKeymap(GATTRemote& remote, const string& name)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        string path(_service->DataPath() + name + "-remote.json");
        if (Core::File(path).Exists() == true) {
            TRACE(Trace::Information, (_T("Loading keymap file [%s] for remote [%s]"), path.c_str(), remote.Name()));
            PluginHost::VirtualInput::KeyMap& map(_inputHandler->Table(remote.Name()));

            result = map.Load(path);
            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to load keymap file [%s]"), path.c_str()));
            }
        }

        return (result);
    }

    uint32_t BTRemoteControl::LoadSettings(Settings& settings) const
    {
        uint32_t result = Core::ERROR_OPENING_FAILED;

        if (_settingsPath.empty() == false) {
            Core::File file(_settingsPath);

            if (file.Open() == true) {
                result = (settings.FromFile(file) == true? Core::ERROR_NONE : Core::ERROR_READ_ERROR);
                file.Close();
            }
        }

        return (result);
    }

    uint32_t BTRemoteControl::SaveSettings(const Settings& settings) const
    {
        uint32_t result = Core::ERROR_OPENING_FAILED;

        if (_settingsPath.empty() == false) {
            Core::File file(_settingsPath);

            if (file.Create() == true) {
                result = (settings.ToFile(file) == true? Core::ERROR_NONE : Core::ERROR_WRITE_ERROR);
                file.Close();
            }
        }

        return (result);
    }

    void BTRemoteControl::RemoteCreated(GATTRemote& remote)
    {
        TRACE(Trace::Information, (_T("BLE GATT remote control unit [%s] created"), remote.Name()));

        if (LoadKeymap(remote, remote.Name()) != Core::ERROR_NONE) {
            if ((_config.DefaultKeymap.IsSet() == false) || (LoadKeymap(remote, _config.DefaultKeymap.Value()) != Core::ERROR_NONE)) {
                TRACE(Trace::Information, (_T("No keymap available for remote [%s]"), remote.Name()));
            }
        }

        if (SaveSettings(Settings(remote.Name(), remote.Address())) != Core::ERROR_NONE) {
            TRACE(Trace::Error, (_T("Failed to save settings")));
        }
    }

    void BTRemoteControl::RemoteDestroyed(GATTRemote& remote)
    {
        TRACE(Trace::Information, (_T("BLE GATT remote control unit [%s] destroyed"), remote.Name()));

        _inputHandler->ClearTable(remote.Name());
        if (_config.DefaultKeymap.IsSet() == true) {
            _inputHandler->ClearTable(_config.DefaultKeymap.Value());
        }
    }

} // namespace Plugin

}
