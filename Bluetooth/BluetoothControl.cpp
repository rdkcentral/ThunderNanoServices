#include "BluetoothControl.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(BluetoothControl, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<BluetoothControl::DeviceImpl::JSON>> jsonResponseFactoryDevice(1);
    static Core::ProxyPoolType<Web::JSONBodyType<BluetoothControl::Status>> jsonResponseFactoryStatus(1);

    /* static */ BluetoothControl::ControlSocket BluetoothControl::_application;

    /* virtual */ const string BluetoothControl::Initialize(PluginHost::IShell* service)
    {
        string result;

        ASSERT(_service == nullptr);
        ASSERT(_gattRemote == nullptr);

        _service = service;
        _skipURL = _service->WebPrefix().length();
        _config.FromString(_service->ConfigLine());
        const char* driverMessage = ::construct_bluetooth_driver(_service->ConfigLine().c_str());
        bool slaving = (_config.External.Value() == true);

        if (slaving == TRUE) {
            SYSLOG(Logging::Startup, (_T("Bluetooth stack working in EXTERNAL mode!!!")));
        }

        // First see if we can bring up the Driver....
        if (driverMessage != nullptr) {
            result = Core::ToString(driverMessage);
        }
        else {
            Bluetooth::ManagementSocket& administrator = _application.Control();
            administrator.DeviceId(_config.Interface.Value());

            _persistentStoragePath = _service->PersistentPath();
            if (_persistentStoragePath.empty() == false) {
                if (Core::Directory(_persistentStoragePath.c_str()).CreatePath() == false) {
                    _persistentStoragePath.clear();
                    TRACE(Trace::Error, (_T("Failed to create persistent storage folder '%s'\n"), _persistentStoragePath.c_str()));
                } else {
                    _linkKeysFile = (_persistentStoragePath + "ShortTermKeys.json");
                    _longTermKeysFile = (_persistentStoragePath + "LongTermKeys.json");
                    _identityKeysFile = (_persistentStoragePath + "IdentityResolvingKeys.json");
                    _signatureKeysFile = (_persistentStoragePath + "ConnectionSignatureResolvingKeys.json");

                    LoadEncryptionKeys();
                }
            }

            if (Bluetooth::ManagementSocket::Up(_config.Interface.Value()) == false) {
                result = "Could not activate bluetooth interface.";
            }
            else if ((slaving == false) && (administrator.Power(false) != Core::ERROR_NONE)) {
                result = "Failed to power down the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.SimplePairing(true) != Core::ERROR_NONE)) {
                result = "Failed to enable simple pairing on the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.SecureLink(true) != Core::ERROR_NONE)) {
                result = "Failed to enable secure links on the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.Bondable(true) != Core::ERROR_NONE)) {
                result = "Failed to enable bonding on the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.LowEnergy(true) != Core::ERROR_NONE)) {
                result = "Failed to enable low energy on the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.Privacy(0, nullptr) != Core::ERROR_NONE)) {
                result = "Failed to disable LE privacy on the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.SecureConnection(true) != Core::ERROR_NONE)) {
                result = "Failed to enable secure connections on the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.Name(_T("Thunder"), _config.Name.Value()) != Core::ERROR_NONE)) {
                result = "Failed to upload identity keys to the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.LinkKey(_linkKeys) != Core::ERROR_NONE)) {
              result = "Failed to upload link keys to the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.LongTermKey(_longTermKeys) != Core::ERROR_NONE)) {
                result = "Failed to upload long term keys to the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.IdentityKey(_identityKeys) != Core::ERROR_NONE)) {
                result = "Failed to upload identity keys to the bluetooth interface";
            }
            //else if ((slaving == false) && (administrator.Notifications(true) != Core::ERROR_NONE)) {
            //    result = "Failed to enable the management notifications on the bluetooth interface";
            //}
            else if ((slaving == false) && (administrator.Power(true) != Core::ERROR_NONE)) {
                result = "Failed to power up the bluetooth interface";
            }
            //else if ((slaving == false) && (administrator.Discovering(true, true, true) != Core::ERROR_NONE)) {
            //    result = "Failed to change device discovery on bluetooth interface";
            //}
            else if (_application.Open(*this) != Core::ERROR_NONE) {
                result = "Could not open the bluetooth application channel";
            }

            if (result.empty() == false) {
                Bluetooth::ManagementSocket::Down(administrator.DeviceId());
                _application.Close();
                ::destruct_bluetooth_driver();
            }
            else {
                Bluetooth::ManagementSocket::Info info(administrator.Settings());
                Bluetooth::ManagementSocket::Info::Properties actuals(info.Actuals());
                Bluetooth::ManagementSocket::Info::Properties supported(info.Supported());

                SYSLOG(Logging::Startup, (_T("        Name:              %s"), info.ShortName().c_str()));
                SYSLOG(Logging::Startup, (_T("        Version:           %d"), info.Version()));
                SYSLOG(Logging::Startup, (_T("        Address:           %s"), info.Address().ToString().c_str()));
                SYSLOG(Logging::Startup, (_T("        DeviceClass:       0x%06X"), info.DeviceClass()));
                SYSLOG(Logging::Startup, (_T("%s Power:             %s"), supported.IsPowered() ? _T("[true] ") : _T("[false]"), actuals.IsPowered() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s Connectable:       %s"), supported.IsConnectable() ? _T("[true] ") : _T("[false]"), actuals.IsConnectable() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s FastConnectable:   %s"), supported.IsFastConnectable() ? _T("[true] ") : _T("[false]"), actuals.IsFastConnectable() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s Discoverable:      %s"), supported.IsDiscoverable() ? _T("[true] ") : _T("[false]"), actuals.IsDiscoverable() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s Bondable:          %s"), supported.IsBondable() ? _T("[true] ") : _T("[false]"), actuals.IsBondable() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s LinkSecurity:      %s"), supported.HasLinkLevelSecurity() ? _T("[true] ") : _T("[false]"), actuals.HasLinkLevelSecurity() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s SimplePairing:     %s"), supported.HasSecureSimplePairing() ? _T("[true] ") : _T("[false]"), actuals.HasSecureSimplePairing() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s BasicEnhancedRate: %s"), supported.HasBasicEnhancedRate() ? _T("[true] ") : _T("[false]"), actuals.HasBasicEnhancedRate() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s HighSpeed:         %s"), supported.HasHighSpeed() ? _T("[true] ") : _T("[false]"), actuals.HasHighSpeed() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s LowEnergy:         %s"), supported.HasLowEnergy() ? _T("[true] ") : _T("[false]"), actuals.HasLowEnergy() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s Advertising:       %s"), supported.IsAdvertising() ? _T("[true] ") : _T("[false]"), actuals.IsAdvertising() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s SecureConnection:  %s"), supported.HasSecureConnections() ? _T("[true] ") : _T("[false]"), actuals.HasSecureConnections() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s DebugKeys:         %s"), supported.HasDebugKeys() ? _T("[true] ") : _T("[false]"), actuals.HasDebugKeys() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s Privacy:           %s"), supported.HasPrivacy() ? _T("[true] ") : _T("[false]"), actuals.HasPrivacy() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s Configuration:     %s"), supported.HasConfiguration() ? _T("[true] ") : _T("[false]"), actuals.HasConfiguration() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s StaticAddress:     %s"), supported.HasStaticAddress() ? _T("[true] ") : _T("[false]"), actuals.HasStaticAddress() ? _T("on") : _T("off")));
            }
        }

        return result;
    }

    /*virtual*/ void BluetoothControl::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        if (_gattRemote != nullptr) {
            _gattRemote->Release();
            _gattRemote = nullptr;
        }

        // Deinitialize what we initialized..
        _service = nullptr;

        // We bring the interface up, so we should bring it down as well..
        if (_config.External.Value() != true) {
            _application.Close();
        }

        ::destruct_bluetooth_driver();
    }

    /* virtual */ string BluetoothControl::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }

    /* virtual */ void BluetoothControl::Inbound(WPEFramework::Web::Request& request)
    {
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length() - _skipURL)), false, '/');
        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        if ((request.Verb == Web::Request::HTTP_PUT) || (request.Verb == Web::Request::HTTP_POST) || (request.Verb == Web::Request::HTTP_DELETE)) {
            if ((index.IsValid() == true) && (index.Next() && index.IsValid())) {
                if ((index.Remainder() == _T("Pair")) || (index.Remainder() == _T("Connect")) || (index.Remainder() == _T("Disconnect")))
                    request.Body(jsonResponseFactoryDevice.Element());
            }
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> BluetoothControl::Process(const WPEFramework::Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (string(_T("Received BluetoothControl request"))));

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        if (request.Verb == Web::Request::HTTP_GET) {

            result = GetMethod(index);
        } else if (request.Verb == Web::Request::HTTP_PUT) {

            result = PutMethod(index, request);
        } else if (request.Verb == Web::Request::HTTP_POST) {

            result = PostMethod(index, request);
        } else if (request.Verb == Web::Request::HTTP_DELETE) {

            result = DeleteMethod(index, request);
        }

        return result;
    }

    Core::ProxyType<Web::Response> BluetoothControl::GetMethod(Core::TextSegmentIterator& index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");

        if (index.Next() == false) {

            Core::ProxyType<Web::JSONBodyType<Status>> response(jsonResponseFactoryStatus.Element());

            result->ErrorCode = Web::STATUS_OK;
            result->Message = _T("Current status.");

            response->Scanning = IsScanning();
            std::list<DeviceImpl*>::const_iterator loop = _devices.begin();

            while (loop != _devices.end()) {
                response->Devices.Add().Set(*loop);
                loop++;
            }

            result->Body(response);
        } else {
            if (index.Current().Text() == _T("Properties")) {
                Core::ProxyType<Web::JSONBodyType<Status>> response(jsonResponseFactoryStatus.Element());
                Bluetooth::ManagementSocket& administrator = _application.Control();
                Bluetooth::ManagementSocket::Info info(administrator.Settings());
                Bluetooth::ManagementSocket::Info::Properties actuals(info.Actuals());
                Bluetooth::ManagementSocket::Info::Properties supported(info.Supported());

                response->Name = info.ShortName();
                response->Version = info.Version();
                response->Address = info.Address().ToString();
                response->DeviceClass = info.DeviceClass();
                response->AddProperty(_T("power"), supported.IsPowered(), actuals.IsPowered());
                response->AddProperty(_T("connectable"), supported.IsConnectable(), actuals.IsConnectable());
                response->AddProperty(_T("fast_connectable"), supported.IsFastConnectable(), actuals.IsFastConnectable());
                response->AddProperty(_T("discoverable"), supported.IsDiscoverable(), actuals.IsDiscoverable());
                response->AddProperty(_T("bondable"), supported.IsBondable(), actuals.IsBondable());
                response->AddProperty(_T("link_level_security"), supported.HasLinkLevelSecurity(), actuals.HasLinkLevelSecurity());
                response->AddProperty(_T("secure_simple_pairing"), supported.HasSecureSimplePairing(), actuals.HasSecureSimplePairing());
                response->AddProperty(_T("basic_enhanced_rate"), supported.HasBasicEnhancedRate(), actuals.HasBasicEnhancedRate());
                response->AddProperty(_T("high_speed"), supported.HasHighSpeed(), actuals.HasHighSpeed());
                response->AddProperty(_T("low_energy"), supported.HasLowEnergy(), actuals.HasLowEnergy());
                response->AddProperty(_T("advertising"), supported.IsAdvertising(), actuals.IsAdvertising());
                response->AddProperty(_T("secure_connection"), supported.HasSecureConnections(), actuals.HasSecureConnections());
                response->AddProperty(_T("debug_keys"), supported.HasDebugKeys(), actuals.HasDebugKeys());
                response->AddProperty(_T("privacy"), supported.HasPrivacy(), actuals.HasPrivacy());
                response->AddProperty(_T("configuration"), supported.HasConfiguration(), actuals.HasConfiguration());
                response->AddProperty(_T("static_address"), supported.HasStaticAddress(), actuals.HasStaticAddress());

                result->ErrorCode = Web::STATUS_OK;
                result->Message = _T("Current status.");
                result->Body(response);
            }
            else {
                DeviceImpl* device = Find(Bluetooth::Address(index.Current().Text().c_str()));

                if (device != nullptr) {
                    Core::ProxyType<Web::JSONBodyType<DeviceImpl::JSON>> response(jsonResponseFactoryDevice.Element());
                    response->Set(device);

                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("device info.");
                    result->Body(response);
                } else {
                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = _T("Unable to display device. Device not found");
                }
            }
        }

        return result;
    }

    Core::ProxyType<Web::Response> BluetoothControl::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported PUT request.");

        if (index.IsValid() == true) {
            if (index.Next()) {

                if (index.Current() == _T("Scan")) {
                    Core::URL::KeyValue options(request.Query.Value());

                    bool lowEnergy = options.Boolean(_T("LowEnergy"), true);
                    bool limited = options.Boolean(_T("Limited"), false);
                    bool passive = options.Boolean(_T("Passive"), false);
                    uint16_t duration = options.Number<uint16_t>(_T("ScanTime"), 10);
                    uint8_t flags = 0;
                    uint32_t type = 0x338B9E;

                    if (lowEnergy == true) {
                        _application.Scan(duration, limited, passive);
                    } else {
                        _application.Scan(duration, type, flags);
                    }
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("Scan started.");
                } else if ((index.Current() == _T("Pair")) || (index.Current() == _T("Connect"))) {
                    bool pair = (index.Current() == _T("Pair"));
                    string destination;
                    if (index.Next() == true) {
                        destination = index.Current().Text();
                    } else if (request.HasBody() == true) {
                        destination = request.Body<const DeviceImpl::JSON>()->RemoteId.Value();
                    }
                    DeviceImpl* device = Find(Bluetooth::Address(destination.c_str()));
                    if (device == nullptr) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = _T("Device not found.");
                    } else if (pair == true) {
                        uint32_t res = device->Pair(IBluetooth::IDevice::DISPLAY_ONLY);
                        if (res == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Paired device.");
                        } else if (res == Core::ERROR_ALREADY_CONNECTED) {
                            result->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                            result->Message = _T("Already paired.");
                        } else {
                            result->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                            result->Message = _T("Unable to Pair device.");
                        }
                    } else if (device->Connect() == Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Connected device.");
                    } else {
                        result->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                        result->Message = _T("Unable to connect to device.");
                    }
                } else if (index.Current() == _T("Create")) {
                    if (index.Next() == true) {
                        Bluetooth::Address address(index.Current().Text().c_str());
                        string name(index.Next() == true? index.Current().Text().c_str() : "Unknown");
                        _devices.push_back(Core::Service<DeviceLowEnergy>::Create<DeviceImpl>(_btInterface, address, name, true));
                        _gattRemote = Core::Service<GATTRemote>::Create<GATTRemote>(this, _devices.back());
                        if (_gattRemote != nullptr) {
                            _gattRemote->Callback(this);
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Created device.");
                        } else {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Failed to create device.");
                        }
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Unable to process PUT request.");
                    }
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Unable to process PUT request.");
                }
            }
        }

        return result;
    }

    Core::ProxyType<Web::Response> BluetoothControl::PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST request.");

        if (index.IsValid() == true) {
            if (index.Next()) {
                if (index.Current() == _T("Remote")) {
                    string address;
                    if (index.Next() == true) {
                        address = index.Current().Text();
                    } else if (request.HasBody() == true) {
                        address = request.Body<const DeviceImpl::JSON>()->RemoteId.Value();
                    }
                    DeviceImpl* device = Find(Bluetooth::Address(address.c_str()));
                    if (device == nullptr) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = _T("Unknown device.");
                    } else {
                        if (_gattRemote == nullptr) {
                            _gattRemote = Core::Service<GATTRemote>::Create<GATTRemote>(this, device);
                            if (_gattRemote != nullptr) {
                                _gattRemote->Callback(this);
                                result->ErrorCode = Web::STATUS_OK;
                                result->Message = _T("OK.");
                            } else {
                                result->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                                result->Message = _T("Failed to assign remote");
                            }
                        } else {
                            result->Message = _T("Remote already assigned");
                        }
                    }
                }
            }
        }

        return result;
    }

    Core::ProxyType<Web::Response> BluetoothControl::DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported DELETE request.");

        if (index.IsValid() == true) {
            if (index.Next()) {

                if (index.Current() == _T("Scan")) {
                    _application.Abort();
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("Scan stopped.");
                } else if ((index.Current() == _T("Pair")) || (index.Current() == _T("Connect"))) {
                    bool pair = (index.Current() == _T("Pair"));
                    string address;
                    if (index.Next() == true) {
                        address = index.Current().Text();
                    } else if (request.HasBody() == true) {
                        address = request.Body<const DeviceImpl::JSON>()->RemoteId.Value();
                    }
                    DeviceImpl* device = Find(Bluetooth::Address(address.c_str()));
                    if (device == nullptr) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = _T("Unknown device.");
                    } else if (pair == true) {
                        uint32_t res = device->Unpair();
                        if (res == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Unpaired device.");
                        } else if (res == Core::ERROR_ALREADY_RELEASED) {
                            result->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                            result->Message = _T("Device is not paired.");
                        } else {
                            result->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                            result->Message = _T("Unable to Unpair device.");
                        }
                    } else {
                        uint16_t reason = 0;
                        if (index.Next() == true) {
                            reason = Core::NumberType<uint16_t>(index.Current()).Value();
                        } else if (request.HasBody() == true) {
                            reason = request.Body<const DeviceImpl::JSON>()->Reason;
                        }

                        if (device->Disconnect(reason) == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Disconnected device.");
                        } else {
                            result->ErrorCode = Web::STATUS_UNPROCESSABLE_ENTITY;
                            result->Message = _T("Unable to Disconnect device.");
                        }
                    }
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Unable to Disconnect device.");
                }
            }
        }

        return result;
    }

    //  IBluetooth methods
    // -------------------------------------------------------------------------------------------------------
    /* virtual */ bool BluetoothControl::IsScanning() const
    {
        return (_application.IsScanning());
    }
    /* virtual */ uint32_t BluetoothControl::Register(IBluetooth::INotification* notification)
    {
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());

        _observers.push_back(notification);
        notification->AddRef();

        // Allright iterate over all devices, so thay get announced by the observer..
        for (std::list<DeviceImpl*>::iterator index = _devices.begin(), end = _devices.end(); index != end; ++index) {
            notification->Update(*index);
        }

        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }
    /* virtual */ uint32_t BluetoothControl::Unregister(IBluetooth::INotification* notification)
    {
        _adminLock.Lock();

        std::list<IBluetooth::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _observers.end());

        if (index != _observers.end()) {
            (*index)->Release();
            _observers.erase(index);
        }

        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }
    /* virtual */ bool BluetoothControl::Scan(const bool enable)
    {
        if ((_application.IsScanning() == false) && (enable == true)) {


            // Clearing previously discovered devices.
            RemoveDevices([](DeviceImpl* device) -> bool { if ((device->IsPaired() == false) && (device->IsConnected() == false)) device->Clear(); return(false); });

            bool lowEnergy = true;
            bool limited = false;
            bool passive = false;
            uint16_t duration = 10;
            uint8_t flags = 0;
            uint32_t type = 0x338B9E;

            if (lowEnergy == true) {
                TRACE(Trace::Information, ("Start Low-Energy Bluetooth Scan"));
                _application.Scan(duration, limited, passive);
            } else {
                TRACE(Trace::Information, ("Start Regular Bluetooth Scan"));
                _application.Scan(duration, type, flags);
            }
        } else if ((_application.IsScanning() == true) && (enable == false)) {

            TRACE(Trace::Information, ("Stop Bluetooth Scan"));

            _application.Abort();
        }

        return (_application.IsScanning() == enable);
    }

    /* virtual */ Exchange::IBluetooth::IDevice* BluetoothControl::Device(const string& address)
    {
        IBluetooth::IDevice* result = Find(Bluetooth::Address(address.c_str()));
        if (result != nullptr) {
            result->AddRef();
        }
        return (result);
    }

    /* virtual */ Exchange::IBluetooth::IDevice::IIterator* BluetoothControl::Devices()
    {
        return (Core::Service<DeviceImpl::IteratorImpl>::Create<IBluetooth::IDevice::IIterator>(_devices));
    }
    void BluetoothControl::Discovered(const bool lowEnergy, const Bluetooth::Address& address, const string& name)
    {
        _adminLock.Lock();

        std::list<DeviceImpl*>::iterator index = _devices.begin();

        while ((index != _devices.end()) && (*(*index) != address)) {
            index++;
        }

        if (index == _devices.end()) {
            if (lowEnergy == true) {
                TRACE(Trace::Information, ("Added LowEnergy Bluetooth device: %s, name: %s", address.ToString().c_str(), name.c_str()));
                _devices.push_back(Core::Service<DeviceLowEnergy>::Create<DeviceImpl>(_btInterface, address, name));
            } else {
                TRACE(Trace::Information, ("Added Regular Bluetooth device: %s, name: %s", address.ToString().c_str(), name.c_str()));
                _devices.push_back(Core::Service<DeviceRegular>::Create<DeviceImpl>(_btInterface, address, name));
            }
        }

        _adminLock.Unlock();
    }

    void BluetoothControl::RemoveDevices(std::function<bool(DeviceImpl*)> filter)
    {
        _adminLock.Lock();

        for (std::list<DeviceImpl*>::iterator index = _devices.begin(), end = _devices.end(); index != end; ++index) {
            // call the function passed into findMatchingAddresses and see if it matches
            if (filter(*index) == true) {
                (*index)->Release();
                index = _devices.erase(index);
            }
        }

        _adminLock.Unlock();
    }
    void BluetoothControl::Capabilities(const Bluetooth::Address& device, const uint8_t capability, const uint8_t authentication, const uint8_t oob_data)
    {
        DeviceImpl* entry = Find(device);

        if (entry != nullptr) {
            entry->Capabilities(capability, authentication, oob_data);
        }
        else {
            TRACE(Trace::Information, (_T("Could not set the capabilities for device %s"), device.ToString()));
        }
    }
    BluetoothControl::DeviceImpl* BluetoothControl::Find(const Bluetooth::Address& search)
    {
        std::list<DeviceImpl*>::const_iterator index = _devices.begin();

        while ((index != _devices.end()) && ((*index)->operator==(search) == false)) {
            index++;
        }

        return (index != _devices.end() ? (*index) : nullptr);
    }
    BluetoothControl::DeviceImpl* BluetoothControl::Find(const uint16_t handle) {
        std::list<DeviceImpl*>::const_iterator index = _devices.begin();

        while ((index != _devices.end()) && ((*index)->ConnectionId() != handle)) {
            index++;
        }

        return (index != _devices.end() ? (*index) : nullptr);
    }

    template<typename KEYLISTTYPE>
    class EncryptionKeyList {
    public:
        EncryptionKeyList() = delete;
        EncryptionKeyList(const EncryptionKeyList& ) = delete;
        EncryptionKeyList operator= (const EncryptionKeyList) = delete;

        EncryptionKeyList(KEYLISTTYPE& list)
            : _array()
            , _list(list)
        {
            for (auto it = list.Elements().cbegin(); it != list.Elements().cend(); it++) {
                string hexKey;
                Core::JSON::String jsonKey;
                Core::ToHexString((*it).Data(), (*it).Length(), hexKey);
                jsonKey = hexKey;
                _array.Add(jsonKey);
            }
        }

        uint32_t Load(Core::File& file)
        {
            uint32_t result = Core::ERROR_NONE;

            if (file.Open() == true) {
                if (_array.FromFile(file) == false) {
                    TRACE(Trace::Error, (_T("Failed to read file %s"), file.Name().c_str()));
                    result = Core::ERROR_READ_ERROR;
                } else {
                    _list.Clear();
                    auto index = _array.Elements();
                    while (index.Next() == true) {
                        uint8_t keyBin[_list.Length()];
                        Core::FromHexString(index.Current().Value(), keyBin, _list.Length());
                        typename KEYLISTTYPE::type key(keyBin, _list.Length());
                        if (key.IsValid() == true) {
                            _list.Add(key);
                        } else {
                            TRACE(Trace::Error, (_T("Invalid key in file %s"), file.Name().c_str()));
                        }
                    }
                }

                file.Close();
            }

            return result;
        }

        uint32_t Save(Core::File& file) const
        {
            uint32_t result = Core::ERROR_NONE;

            if (_array.Length() > 0) {
                if (file.Create() == true) {
                    if (_array.ToFile(file) == false) {
                        TRACE(Trace::Error, (_T("Failed to write file %s"), file.Name().c_str()));
                        result = Core::ERROR_WRITE_ERROR;
                    }

                    file.Close();
                } else {
                    TRACE(Trace::Error, (_T("Failed to create file %s"), file.Name().c_str()));
                    result = Core::ERROR_OPENING_FAILED;
                }
            }

            return result;
        }

    private:
        Core::JSON::ArrayType<Core::JSON::String> _array;
        KEYLISTTYPE& _list;
    };

    void BluetoothControl::LoadEncryptionKeys()
    {
        if (_persistentStoragePath.empty() == false) {

            _adminLock.Lock();

            EncryptionKeyList<Bluetooth::LinkKeys> stks(_linkKeys);
            stks.Load(_linkKeysFile);

            EncryptionKeyList<Bluetooth::LongTermKeys> ltks(_longTermKeys);
            ltks.Load(_longTermKeysFile);

            EncryptionKeyList<Bluetooth::IdentityKeys> irks(_identityKeys);
            irks.Load(_identityKeysFile);

            EncryptionKeyList<Bluetooth::SignatureKeys> csrks(_signatureKeys);
            csrks.Load(_signatureKeysFile);

            TRACE(Trace::Information, (_T("Loaded %i STKs, %i LTKs, %i IRKs, %i CSRKs"),
                        _linkKeys.Entries(), _longTermKeys.Entries(), _identityKeys.Entries(), _signatureKeys.Entries()));

            _adminLock.Unlock();
        }
    }

    uint32_t BluetoothControl::SaveEdrEncryptionKeys()
    {
        uint32_t result = Core::ERROR_NONE;

        if (_persistentStoragePath.empty() == false) {

            _adminLock.Lock();

            TRACE(Trace::Information, (_T("Saving EDR keys: %i STKs"), _linkKeys.Entries()));

            if (_linkKeys.Entries() > 0) {
                EncryptionKeyList<Bluetooth::LinkKeys> keys(_linkKeys);
                result = keys.Save(_linkKeysFile);
            }

            _adminLock.Unlock();
        }

        return result;
    }

    uint32_t BluetoothControl::SaveLeEncryptionKeys()
    {
        uint32_t result = Core::ERROR_NONE;

        if (_persistentStoragePath.empty() == false) {

            _adminLock.Lock();

            TRACE(Trace::Information, (_T("Saving LE keys: %i LTKs, %i IRKs, %i CSRKs"), _longTermKeys.Entries(), _identityKeys.Entries(), _signatureKeys.Entries()));

            if (_identityKeys.Entries() > 0) {
                EncryptionKeyList<Bluetooth::IdentityKeys> keys(_identityKeys);
                keys.Save(_identityKeysFile);
            }

            if (_signatureKeys.Entries() > 0) {
                EncryptionKeyList<Bluetooth::SignatureKeys> keys(_signatureKeys);
                keys.Save(_signatureKeysFile);
            }

            if (_longTermKeys.Entries() > 0) {
                EncryptionKeyList<Bluetooth::LongTermKeys> keys(_longTermKeys);
                result = keys.Save(_longTermKeysFile);
            }

            _adminLock.Unlock();
        }

        return result;
    }

    void BluetoothControl::PurgeDeviceKeys(const Bluetooth::Address& device)
    {
        _adminLock.Lock();

        TRACE(Trace::Information, (_T("Purging encryption keys of device [%s]"), device.ToString().c_str()));

        if (_linkKeys.Purge(device) > 0) {
            SaveEdrEncryptionKeys();
        }

        if ((_longTermKeys.Purge(device) + _signatureKeys.Purge(device) + _identityKeys.Purge(device)) > 0) {
            SaveLeEncryptionKeys();
        }

        _adminLock.Unlock();
    }

} // namespace Plugin

}
