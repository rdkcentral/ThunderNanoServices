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
        ASSERT(_administrator.IsOpen() == false);

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
            Bluetooth::LinkKeys linkKeys;
            Bluetooth::LongTermKeys longTermKeys;
            Bluetooth::IdentityKeys identityKeys;

            Bluetooth::ManagementSocket& administrator = _application.Control();
            administrator.DeviceId(_config.Interface.Value());

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
            else if ((slaving == false) && (administrator.Discovering(false, false, false) != Core::ERROR_NONE)) {
                result = "Failed to stop device discovery on bluetooth interface";
            }
            else if ((slaving == false) && (administrator.SecureConnection(true) != Core::ERROR_NONE)) {
                result = "Failed to enable secure connections on the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.Name(_T("Thunder"), _config.Name.Value()) != Core::ERROR_NONE)) {
                result = "Failed to upload identity keys to the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.LinkKey(linkKeys) != Core::ERROR_NONE)) {
                result = "Failed to upload link keys to the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.LongTermKey(longTermKeys) != Core::ERROR_NONE)) {
                result = "Failed to upload long term keys to the bluetooth interface";
            }
            else if ((slaving == false) && (administrator.IdentityKey(identityKeys) != Core::ERROR_NONE)) {
                result = "Failed to upload identity keys to the bluetooth interface";
            }
            //else if ((slaving == false) && (administrator.Notifications(true) != Core::ERROR_NONE)) {
            //    result = "Failed to enable the management notifications on the bluetooth interface";
            //}
            else if ((slaving == false) && (administrator.Power(true) != Core::ERROR_NONE)) {
                result = "Failed to power up the bluetooth interface";
            }
            else if (_application.Open(*this) != Core::ERROR_NONE) {
                result = "Could not open the bluetooth application channel";
            } 
            else if ((slaving == false) && (_application.ReadStoredLinkKeys(Bluetooth::Address(administrator.DeviceId()), true, linkKeys) != Core::ERROR_NONE)) {
                result = "Could not read the stored keys for the configured interface";
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
                SYSLOG(Logging::Startup, (_T("%s Discovery:         %s"), supported.HasDiscovery() ? _T("[true] ") : _T("[false]"), actuals.HasDiscovery() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s Pairing:           %s"), supported.HasPairing() ? _T("[true] ") : _T("[false]"), actuals.HasPairing() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s LinkSecurity:      %s"), supported.HasLinkLevelSecurity() ? _T("[true] ") : _T("[false]"), actuals.HasLinkLevelSecurity() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s SimplePairing:     %s"), supported.HasSecureSimplePairing() ? _T("[true] ") : _T("[false]"), actuals.HasSecureSimplePairing() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s BasicEnhancedRate: %s"), supported.HasBasicEnhancedRate() ? _T("[true] ") : _T("[false]"), actuals.HasBasicEnhancedRate() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s HighSpeed:         %s"), supported.HasHighSpeed() ? _T("[true] ") : _T("[false]"), actuals.HasHighSpeed() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s LowEnergy:         %s"), supported.HasLowEnergy() ? _T("[true] ") : _T("[false]"), actuals.HasLowEnergy() ? _T("on") : _T("off")));
                SYSLOG(Logging::Startup, (_T("%s Advertising:       %s"), supported.HasAdvertising() ? _T("[true] ") : _T("[false]"), actuals.HasAdvertising() ? _T("on") : _T("off")));
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
                response->AddProperty(_T("discovery"), supported.HasDiscovery(), actuals.HasDiscovery());
                response->AddProperty(_T("pairing"), supported.HasPairing(), actuals.HasPairing());
                response->AddProperty(_T("link_level_security"), supported.HasLinkLevelSecurity(), actuals.HasLinkLevelSecurity());
                response->AddProperty(_T("secure_simple_pairing"), supported.HasSecureSimplePairing(), actuals.HasSecureSimplePairing());
                response->AddProperty(_T("basic_enhanced_rate"), supported.HasBasicEnhancedRate(), actuals.HasBasicEnhancedRate());
                response->AddProperty(_T("high_speed"), supported.HasHighSpeed(), actuals.HasHighSpeed());
                response->AddProperty(_T("low_energy"), supported.HasLowEnergy(), actuals.HasLowEnergy());
                response->AddProperty(_T("advertising"), supported.HasAdvertising(), actuals.HasAdvertising());
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
                        if (destination.empty()) {
                            result->ErrorCode = _gattRemotes.front().Pair();
                            result->Message = _T("Paring the first remote.");
                        }
                        else {
                            result->ErrorCode = Web::STATUS_NOT_FOUND;
                            result->Message = _T("Device not found.");
                        }
                    } else if (pair == true) {
                        if (device->Pair(IBluetooth::IDevice::DISPLAY_ONLY) == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Paired device.");
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
                        _gattRemotes.emplace_back(device);
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("OK.");
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
                        if (device->Unpair() == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Unpaired device.");
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


}
}
