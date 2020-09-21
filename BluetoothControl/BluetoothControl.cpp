/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "BluetoothControl.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(BluetoothControl, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<BluetoothControl::DeviceImpl::Data>> jsonResponseFactoryDevice(1);
    static Core::ProxyPoolType<Web::JSONBodyType<BluetoothControl::Status>> jsonResponseFactoryStatus(1);

    /* virtual */ const string BluetoothControl::Initialize(PluginHost::IShell* service)
    {
        string result;

        ASSERT(_service == nullptr);

        _service = service;
        _skipURL = _service->WebPrefix().length();
        _config.FromString(_service->ConfigLine());
        const char* driverMessage = ::construct_bluetooth_driver(_service->ConfigLine().c_str());

        // First see if we can bring up the Driver....
        if (driverMessage != nullptr) {
            result = Core::ToString(driverMessage);
        }
        else {
            Bluetooth::ManagementSocket& administrator = _application.Control();
            Bluetooth::ManagementSocket::Devices(_adapters);
            administrator.DeviceId(_config.Interface.Value());
            uint32_t deviceClass(_config.Class.Value());

            _persistentStoragePath = _service->PersistentPath() + "Devices/";
            Data controllerData;
            LoadController(_service->PersistentPath(), controllerData);

            if ((_config.PersistMAC.Value() == true) &&
                (controllerData.MAC.Value().empty() == false) &&
                (administrator.PublicAddress(Bluetooth::Address(controllerData.MAC.Value().c_str())) != Core::ERROR_NONE)) {
                result = "Could not set the persistent MAC address for the bluetooth interface.";
            }
            else if (Bluetooth::ManagementSocket::Up(_config.Interface.Value()) == false) {
                result = "Could not activate bluetooth interface";
            }
            else if (administrator.Power(false) != Core::ERROR_NONE) {
                result = "Failed to power down the bluetooth interface";
            }
            else if (administrator.SimplePairing(true) != Core::ERROR_NONE) {
                result = "Failed to enable simple pairing on thetrue bluetooth interface";
            }
            else if (administrator.SecureLink(true) != Core::ERROR_NONE) {
                result = "Failed to enable secure links on the bluetooth interface";
            }
            else if (administrator.Connectable(true) != Core::ERROR_NONE) {
                result = "Failed to enable Connectable on the bluetooth interface";
            }
            else if (administrator.Bondable(true) != Core::ERROR_NONE) {
                result = "Failed to enable bonding on the bluetooth interface";
            }
            else if (administrator.LowEnergy(true) != Core::ERROR_NONE) {
                result = "Failed to enable low energy on the bluetooth interface";
            }
            else if (administrator.Privacy(0, nullptr) != Core::ERROR_NONE) {
                result = "Failed to disable LE privacy on the bluetooth interface";
            }
            else if (administrator.SecureConnection(true) != Core::ERROR_NONE) {
                result = "Failed to enable secure connections on the bluetooth interface";
            }
            else if (administrator.Name(_T("Thunder"), _config.Name.Value()) != Core::ERROR_NONE) {
                result = "Failed to set device name";
            }
            else if ((deviceClass != 0)
                    && (administrator.DeviceClass(((deviceClass >> 8) & 0xFF), (deviceClass & 0xFF) >> 2) != Core::ERROR_NONE)) {
                result = "Failed to set class of device";
            }
            else if (LoadDevices(_persistentStoragePath, administrator) != Core::ERROR_NONE) {
                result = "Failed to load the stored devices";
            }
            else if (administrator.Power(true) != Core::ERROR_NONE) {
                result = "Failed to power up the bluetooth interface";
            }
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

                if (controllerData.MAC.Value().empty() == true) {
                    controllerData.MAC = info.Address().ToString();
                    SaveController(_service->PersistentPath(), controllerData);
                }

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

                // Bluetooth is ready!
                PluginHost::ISubSystem* subSystems(_service->SubSystems());
                ASSERT(subSystems != nullptr);
                if (subSystems != nullptr) {
                    subSystems->Set(PluginHost::ISubSystem::BLUETOOTH, nullptr);
                    subSystems->Release();
                }
            }
        }

        return result;
    }

    /*virtual*/ void BluetoothControl::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        PluginHost::ISubSystem* subSystems(_service->SubSystems());
        ASSERT(subSystems != nullptr);
        if (subSystems != nullptr) {
            subSystems->Set(PluginHost::ISubSystem::NOT_BLUETOOTH, nullptr);
            subSystems->Release();
        }

        // Deinitialize what we initialized..
        _service = nullptr;

        // We bring the interface up, so we should bring it down as well..
        _application.Close();
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
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
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
                    Core::ProxyType<Web::JSONBodyType<DeviceImpl::Data>> response(jsonResponseFactoryDevice.Element());
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
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
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
                        destination = request.Body<const DeviceImpl::Data>()->RemoteId.Value();
                    }
                    DeviceImpl* device = Find(Bluetooth::Address(destination.c_str()));
                    if (device == nullptr) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = _T("Device not found.");
                    } else if (pair == true) {
                        uint32_t res = device->Pair(IBluetooth::DISPLAY_YES_NO, 20);
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
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST request.");

        return (result);
    }

    Core::ProxyType<Web::Response> BluetoothControl::DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
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
                        address = request.Body<const DeviceImpl::Data>()->RemoteId.Value();
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
                        if (device->Disconnect() == Core::ERROR_NONE) {
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
            RemoveDevices([](DeviceImpl* device) -> bool { if ((device->IsBonded() == false) && (device->IsConnected() == false)) device->Clear(); return(false); });

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
        _adminLock.Lock();

        IBluetooth::IDevice* result = Find(Bluetooth::Address(address.c_str()));
        if (result != nullptr) {
            result->AddRef();
        }

        _adminLock.Unlock();

        return (result);
    }
    /* virtual */ Exchange::IBluetooth::IDevice::IIterator* BluetoothControl::Devices()
    {
        return (Core::Service<DeviceImpl::IteratorImpl>::Create<IBluetooth::IDevice::IIterator>(_devices));
    }
    BluetoothControl::DeviceImpl* BluetoothControl::Discovered(const bool lowEnergy, const Bluetooth::Address& address)
    {
        _adminLock.Lock();

        DeviceImpl* impl = Find(address, lowEnergy);

        if (impl == nullptr) {
            if (lowEnergy == true) {
                impl = Core::Service<DeviceLowEnergy>::Create<DeviceImpl>(this, _btInterface, address);
            } else {
                impl = Core::Service<DeviceRegular>::Create<DeviceImpl>(this, _btInterface, address);
            }

            ASSERT(impl != nullptr);
            _devices.push_back(impl);
        }

        _adminLock.Unlock();

        return (impl);
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
        _adminLock.Lock();

        DeviceImpl* entry = Find(device);

        if (entry != nullptr) {
            entry->Capabilities(capability, authentication, oob_data);
        }
        else {
            TRACE(Trace::Information, (_T("Could not set the capabilities for device %s"), device.ToString()));
        }

        _adminLock.Unlock();
    }
    BluetoothControl::DeviceImpl* BluetoothControl::Find(const Bluetooth::Address& search) const
    {
        std::list<DeviceImpl*>::const_iterator index = _devices.begin();

        while ((index != _devices.end()) && ((*index)->operator==(search) == false)) {
            index++;
        }

        return (index != _devices.end() ? (*index) : nullptr);
    }
    BluetoothControl::DeviceImpl* BluetoothControl::Find(const Bluetooth::Address& search, bool lowEnergy) const
    {
        std::list<DeviceImpl*>::const_iterator index = _devices.begin();

        while ((index != _devices.end()) && ((*index)->operator==(std::make_pair(search, lowEnergy)) == false)) {
            index++;
        }

        return (index != _devices.end() ? (*index) : nullptr);
    }
    template<typename DEVICE=BluetoothControl::DeviceImpl>
    DEVICE* BluetoothControl::Find(const uint16_t handle) const
    {
        std::list<DeviceImpl*>::const_iterator index = _devices.begin();

        while ((index != _devices.end()) && ((*index)->ConnectionId() != handle)) {
            index++;
        }

        return (index != _devices.end() ? (*index) : nullptr);
    }

    uint32_t BluetoothControl::LoadDevices(const string& devicePath, Bluetooth::ManagementSocket& administrator)
    {
        uint32_t result = Core::ERROR_NONE;

        if (Core::File(devicePath, true).Exists() == false) {
            if (Core::Directory(devicePath.c_str()).CreatePath() == false) {
                SYSLOG(Logging::Startup, (_T("Failed to create persistent storage folder '%s'\n"), devicePath.c_str()));
            }
        }
        else {
            Bluetooth::LinkKeys lks;
            Bluetooth::LongTermKeys ltks;
            Bluetooth::IdentityKeys irks;

            Core::Directory dir(devicePath.c_str(), _T("*.json"));
            while (dir.Next() == true) {
                if (LoadDevice(dir.Current(), lks, ltks, irks) != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Failed to load device %s"), dir.Name().c_str()));
                } else {
                    SYSLOG(Logging::Startup, (_T("Loaded device: %s"), Core::File::FileName(dir.Name()).c_str()));
                }
            }
            TRACE(Trace::Information, (_T("Loaded %i previously bonded device(s): %i LKs, %i LTKs, %i IRKs"),
                                         _devices.size(), lks.Entries(), ltks.Entries(), irks.Entries()));


            if (administrator.LinkKey(lks) != Core::ERROR_NONE) {
                result = Core::ERROR_UNAVAILABLE;
            }
            else if (administrator.LongTermKey(ltks) != Core::ERROR_NONE) {
                result = Core::ERROR_UNAVAILABLE;
            }
            else if (administrator.IdentityKey(irks) != Core::ERROR_NONE) {
                result = Core::ERROR_UNAVAILABLE;
            }
        }
        return (result);
    }

    void BluetoothControl::LoadController(const string& pathName, Data& data) const
    {
        Core::File file(pathName + _T("Controller.json"), true);
        if (file.Open(true) == true) {
            data.IElement::FromFile(file);
            file.Close();
        }
    }

    void BluetoothControl::SaveController(const string& pathName, const Data& data)
    {
        Core::File file(pathName + _T("Controller.json"), true);
        if (file.Create() == true) {
            data.IElement::ToFile(file);
            file.Close();
        }
    }

    uint32_t BluetoothControl::LoadDevice(const string& fileName,
                                          Bluetooth::LinkKeys& linkKeysList,
                                          Bluetooth::LongTermKeys& longTermKeysList,
                                          Bluetooth::IdentityKeys& identityKeysList)
    {
        uint32_t result = Core::ERROR_OPENING_FAILED;

        Core::File file(fileName, true);

        if (file.Open(true) == true) {
            Bluetooth::Address address(file.FileName().c_str());
            if (address.IsValid() == true) {
                result = Core::ERROR_READ_ERROR;
                DeviceImpl::Config config;

                if (config.IElement::FromFile(file) == true) {
                    result = Core::ERROR_INVALID_DESIGNATOR;

                    if (config.Type.IsSet() == true) {

                        DeviceImpl* device;
                        if (config.Type.Value() == Bluetooth::Address::BREDR_ADDRESS) {

                            // Classic Bluetooth device
                            device = Core::Service<DeviceRegular>::Create<DeviceImpl>(this, _btInterface, address, &config);
                            if (device != nullptr) {
                                device->SecurityKey(linkKeysList);
                            }
                        } else {
                            // Bluetooth Low Energy device
                            device = Core::Service<DeviceLowEnergy>::Create<DeviceImpl>(this, _btInterface, address, &config);
                            if (device != nullptr) {
                                device->SecurityKey(longTermKeysList);
                                const Bluetooth::IdentityKey& identity = device->IdentityKey();
                                if (identity.IsValid() == true) {
                                    identityKeysList.Add(identity);
                                }
                            }
                        }

                        if (device != nullptr) {

                            _devices.push_back(device);

                            result = Core::ERROR_NONE;
                        }
                    }
                }
            }
            file.Close();
        }

        return (result);
    }

    uint32_t BluetoothControl::SaveDevice(const DeviceImpl* device) const
    {
        uint32_t result = Core::ERROR_OPENING_FAILED;
        Bluetooth::Address address (device->Address());

        if (address.IsValid() == true) {

            Core::File file(_persistentStoragePath + device->RemoteId() + _T(".json"));

            if (file.Create() == true) {

                Bluetooth::LinkKeys lks;
                Bluetooth::LongTermKeys ltks;
                DeviceImpl::Config config;

                // Get the security information
                device->SecurityKey(lks);
                device->SecurityKey(ltks);
                const Bluetooth::IdentityKey& idKey = device->IdentityKey();

                config.Type = device->AddressType();

                if (device->Name().empty() == false) {
                    config.Name = device->Name();
                }
                if (device->Class() != 0) {
                    config.Class = device->Class();
                }
                if (lks.Entries() > 0) {
                    config.Set(config.LinkKeys, lks);
                }
                if (ltks.Entries() > 0) {
                    config.Set(config.LongTermKeys, ltks);
                }
                if (idKey.IsValid() == true) {
                    config.IdentityKey = idKey.ToString();
                }
                result = (config.IElement::ToFile(file) == true? Core::ERROR_NONE : Core::ERROR_WRITE_ERROR);
                file.Close();
            }
        }

        return (result);
    }

    uint32_t BluetoothControl::ForgetDevice(const DeviceImpl* device)
    {
        uint32_t result = Core::ERROR_OPENING_FAILED;

        Bluetooth::Address address (device->Address());

        if (address.IsValid() == true) {

            Core::File file(_persistentStoragePath + device->RemoteId() + _T(".json"));

            result = (file.Destroy() == true? Core::ERROR_NONE : Core::ERROR_WRITE_ERROR);
        }
        return (result);
    }

} // namespace Plugin

}
