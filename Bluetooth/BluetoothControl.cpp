#include "BluetoothControl.h"

namespace WPEFramework {

#define ADAPTER_INDEX 0X00
#define ENABLE_MODE 0x01

namespace Bluetooth {

    /* LMP features mapping */
    struct ConversionTable {
        uint16_t id;
        const TCHAR* text;
    };

    static ConversionTable lmp_features_map[] = {
        /* Byte 0 */
        { LMP_3SLOT | 0x0000, _T("3-slot packets") }, /* Bit 0 */
        { LMP_5SLOT | 0x0000, _T("5-slot packets") }, /* Bit 1 */
        { LMP_ENCRYPT | 0x0000, _T("encryption") }, /* Bit 2 */
        { LMP_SOFFSET | 0x0000, _T("slot offset") }, /* Bit 3 */
        { LMP_TACCURACY | 0x0000, _T("timing accuracy") }, /* Bit 4 */
        { LMP_RSWITCH | 0x0000, _T("role switch") }, /* Bit 5 */
        { LMP_HOLD | 0x0000, _T("hold mode") }, /* Bit 6 */
        { LMP_SNIFF | 0x0000, _T("sniff mode") }, /* Bit 7 */

        /* Byte 1 */
        { LMP_PARK | 0x0100, _T("park state") }, /* Bit 0 */
        { LMP_RSSI | 0x0100, _T("RSSI") }, /* Bit 1 */
        { LMP_QUALITY | 0x0100, _T("channel quality") }, /* Bit 2 */
        { LMP_SCO | 0x0100, _T("SCO link") }, /* Bit 3 */
        { LMP_HV2 | 0x0100, _T("HV2 packets") }, /* Bit 4 */
        { LMP_HV3 | 0x0100, _T("HV3 packets") }, /* Bit 5 */
        { LMP_ULAW | 0x0100, _T("u-law log") }, /* Bit 6 */
        { LMP_ALAW | 0x0100, _T("A-law log") }, /* Bit 7 */

        /* Byte 2 */
        { LMP_CVSD | 0x0200, _T("CVSD") }, /* Bit 0 */
        { LMP_PSCHEME | 0x0200, _T("paging scheme") }, /* Bit 1 */
        { LMP_PCONTROL | 0x0200, _T("power control") }, /* Bit 2 */
        { LMP_TRSP_SCO | 0x0200, _T("transparent SCO") }, /* Bit 3 */
        { LMP_BCAST_ENC | 0x0200, _T("broadcast encrypt") }, /* Bit 7 */

        /* Byte 3 */
        { LMP_EDR_ACL_2M | 0x0300, _T("EDR ACL 2 Mbps") }, /* Bit 1 */
        { LMP_EDR_ACL_3M | 0x0300, _T("EDR ACL 3 Mbps") }, /* Bit 2 */
        { LMP_ENH_ISCAN | 0x0300, _T("enhanced iscan") }, /* Bit 3 */
        { LMP_ILACE_ISCAN | 0x0300, _T("interlaced iscan") }, /* Bit 4 */
        { LMP_ILACE_PSCAN | 0x0300, _T("interlaced pscan") }, /* Bit 5 */
        { LMP_RSSI_INQ | 0x0300, _T("inquiry with RSSI") }, /* Bit 6 */
        { LMP_ESCO | 0x0300, _T("extended SCO") }, /* Bit 7 */

        /* Byte 4 */
        { LMP_EV4 | 0x0400, _T("EV4 packets") }, /* Bit 0 */
        { LMP_EV5 | 0x0400, _T("EV5 packets") }, /* Bit 1 */
        { LMP_AFH_CAP_SLV | 0x0400, _T("AFH cap. slave") }, /* Bit 3 */
        { LMP_AFH_CLS_SLV | 0x0400, _T("AFH class. slave") }, /* Bit 4 */
        { LMP_NO_BREDR | 0x0400, _T("BR/EDR not supp.") }, /* Bit 5 */
        { LMP_LE | 0x0400, _T("LE support") }, /* Bit 6 */
        { LMP_EDR_3SLOT | 0x0400, _T("3-slot EDR ACL") }, /* Bit 7 */

        /* Byte 5 */
        { LMP_EDR_5SLOT | 0x0500, _T("5-slot EDR ACL") }, /* Bit 0 */
        { LMP_SNIFF_SUBR | 0x0500, _T("sniff subrating") }, /* Bit 1 */
        { LMP_PAUSE_ENC | 0x0500, _T("pause encryption") }, /* Bit 2 */
        { LMP_AFH_CAP_MST | 0x0500, _T("AFH cap. master") }, /* Bit 3 */
        { LMP_AFH_CLS_MST | 0x0500, _T("AFH class. master") }, /* Bit 4 */
        { LMP_EDR_ESCO_2M | 0x0500, _T("EDR eSCO 2 Mbps") }, /* Bit 5 */
        { LMP_EDR_ESCO_3M | 0x0500, _T("EDR eSCO 3 Mbps") }, /* Bit 6 */
        { LMP_EDR_3S_ESCO | 0x0500, _T("3-slot EDR eSCO") }, /* Bit 7 */

        /* Byte 6 */
        { LMP_EXT_INQ | 0x0600, _T("extended inquiry") }, /* Bit 0 */
        { LMP_LE_BREDR | 0x0600, _T("LE and BR/EDR") }, /* Bit 1 */
        { LMP_SIMPLE_PAIR | 0x0600, _T("simple pairing") }, /* Bit 3 */
        { LMP_ENCAPS_PDU | 0x0600, _T("encapsulated PDU") }, /* Bit 4 */
        { LMP_ERR_DAT_REP | 0x0600, _T("err. data report") }, /* Bit 5 */
        { LMP_NFLUSH_PKTS | 0x0600, _T("non-flush flag") }, /* Bit 6 */

        /* Byte 7 */
        { LMP_LSTO | 0x0700, _T("LSTO") }, /* Bit 1 */
        { LMP_INQ_TX_PWR | 0x0700, _T("inquiry TX power") }, /* Bit 2 */
        { LMP_EPC | 0x0700, _T("EPC") }, /* Bit 2 */
        { LMP_EXT_FEAT | 0x0700, _T("extended features") }, /* Bit 7 */

        { 0x0000, nullptr }
    };

} // Namespace Bluetooth

namespace Plugin {

    SERVICE_REGISTRATION(BluetoothControl, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<BluetoothControl::DeviceImpl::JSON>> jsonResponseFactoryDevice(1);
    static Core::ProxyPoolType<Web::JSONBodyType<BluetoothControl::Status>> jsonResponseFactoryStatus(1);
    /* static */ string BluetoothControl::_HIDPath;

    const TCHAR* BluetoothControl::DeviceImpl::FeatureIterator::FeatureToText(const uint16_t index) const
    {
        Bluetooth::ConversionTable* pos = Bluetooth::lmp_features_map;

        while ((pos->text != nullptr) && (pos->id != index)) {
            pos++;
        }

        return (pos->text != nullptr ? pos->text : _T("reserved"));
    }

    /* virtual */ const string BluetoothControl::Initialize(PluginHost::IShell* service)
    {
        string result;

        ASSERT(_service == nullptr);
        ASSERT(_driver == nullptr);

        _service = service;
        _skipURL = _service->WebPrefix().length();
        Config config;
        config.FromString(_service->ConfigLine());
        _driver = Bluetooth::Driver::Instance(_service->ConfigLine());
        _HIDPath = config.HIDPath.Value();

        // First see if we can bring up the Driver....
        if (_driver == nullptr) {
            result = _T("Could not load the Bluetooth Driver.");
        } else {
            Bluetooth::HCISocket::Management::OperationalMode command(ADAPTER_INDEX);
            command->val = htobs(ENABLE_MODE);

            _interface = Bluetooth::Driver::Interface(config.Interface.Value());

            if (_interface.IsValid() == false) {
                result = _T("Could not bring up the interface.");
            } else if (_interface.Up() == false) {
                result = "Failed to bring up the Bluetooth interface";
            } else if (_btAddress.Default() == false) {
                result = "Could not get the default Bluetooth address";
            } else if (_administrator.Open(Core::infinite) != Core::ERROR_NONE) {
                result = "Could not open the Bluetooth Administrator channel";
            } else if (_administrator.Exchange(500, command.OpCode(MGMT_OP_SET_POWERED)) != Core::ERROR_NONE) {
                result = "Failed to power on bluetooth adaptor";
            }
            // Enable Bondable on adaptor.
            else if (_administrator.Exchange(500, command.OpCode(MGMT_OP_SET_BONDABLE)) != Core::ERROR_NONE) {
                result = "Failed to enable Bondable";
            }
            // Enable Simple Secure Simple Pairing.
            else if (_administrator.Exchange(500, command.OpCode(MGMT_OP_SET_SSP)) != Core::ERROR_NONE) {
                result = "Failed to enable Simple Secure Simple Pairing";
            }
            // Enable Low Energy
            else if (_administrator.Exchange(500, command.OpCode(MGMT_OP_SET_LE)) != Core::ERROR_NONE) {
                result = "Failed to enable Low Energy";
            }
            // Enable Secure Connections
            else if (_administrator.Exchange(500, command.OpCode(MGMT_OP_SET_SECURE_CONN)) != Core::ERROR_NONE) {
                result = "Failed to enable Secure Connections";
            }
            // Enable Advertising
            else if (_administrator.Exchange(500, command.OpCode(MGMT_OP_SET_ADVERTISING)) != Core::ERROR_NONE) {
                result = "Failed to enable Advertising";
            } else if (_application.Open(_btAddress) != Core::ERROR_NONE) {
                result = "Could not open the Bluetooth Application channel";
            } else if (_application.Advertising(true, 0) != Core::ERROR_NONE) {
                result = "Could not listen to advertisements on the Application channel";
            }
        }

        if ((_driver != nullptr) && (result.empty() == false)) {
            _interface.Down();
            delete _driver;
            _driver = nullptr;
        }
        return result;
    }

    /*virtual*/ void BluetoothControl::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_driver != nullptr);

        // Deinitialize what we initialized..
        _service = nullptr;

        if (_driver != nullptr) {
            // We bring the interface up, so we should bring it down as well..
            _interface.Down();
            delete _driver;
            _driver = nullptr;
        }
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
            TRACE(Trace::Information, (string(__FUNCTION__)));
            DeviceImpl* device = Find(index.Current().Text());

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

        return result;
    }

    Core::ProxyType<Web::Response> BluetoothControl::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported PUT request.");

        if (index.IsValid() == true) {
            if (index.Next()) {
                TRACE(Trace::Information, (string(__FUNCTION__)));

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
                        destination = request.Body<const DeviceImpl::JSON>()->Address.Value();
                    }
                    DeviceImpl* device = Find(destination);
                    if (device == nullptr) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = _T("Device not found.");
                    } else if (pair == true) {
                        if (device->Pair() == Core::ERROR_NONE) {
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
                TRACE(Trace::Information, (string(__FUNCTION__)));

                if (index.Current() == _T("Remote")) {
                    string address;
                    if (index.Next() == true) {
                        address = index.Current().Text();
                    } else if (request.HasBody() == true) {
                        address = request.Body<const DeviceImpl::JSON>()->Address.Value();
                    }
                    DeviceImpl* device = Find(address);
                    if (device == nullptr) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = _T("Unknown device.");
                    } else {
                        _gattRemotes.emplace_back(device->Locator(), _HIDPath);
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Unpaired device.");
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
                TRACE(Trace::Information, (string(__FUNCTION__)));

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
                        address = request.Body<const DeviceImpl::JSON>()->Address.Value();
                    }
                    DeviceImpl* device = Find(address);
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

            TRACE(Trace::Information, ("Start Bluetooth Scan"));

            // Clearing previously discovered devices.
            RemoveDevices([](DeviceImpl* device) -> bool { if ((device->IsPaired() == false) && (device->IsConnected() == false)) device->Clear(); return(false); });

            bool lowEnergy = true;
            bool limited = false;
            bool passive = false;
            uint16_t duration = 10;
            uint8_t flags = 0;
            uint32_t type = 0x338B9E;

            if (lowEnergy == true) {
                _application.Scan(duration, limited, passive);
            } else {
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
        IBluetooth::IDevice* result = Find(address);
        if (result != nullptr) {
            result->AddRef();
        }
        return (result);
    }

    /* virtual */ Exchange::IBluetooth::IDevice::IIterator* BluetoothControl::Devices()
    {
        return (Core::Service<DeviceImpl::IteratorImpl>::Create<IBluetooth::IDevice::IIterator>(_devices));
    }

    void BluetoothControl::DiscoveredDevice(const bool lowEnergy, const Bluetooth::Address& address, const string& name)
    {

        _adminLock.Lock();

        std::list<DeviceImpl*>::iterator index = _devices.begin();

        while ((index != _devices.end()) && (*(*index) != address)) {
            index++;
        }

        if (index != _devices.end()) {
            (*index)->Discovered();
        } else if (lowEnergy == true) {
            TRACE(Trace::Information, ("Added LowEnergy Bluetooth device: %s, name: %s", address.ToString().c_str(), name.c_str()));
            _devices.push_back(Core::Service<DeviceLowEnergy>::Create<DeviceImpl>(&_administrator, &_application, address, name));
        } else {
            TRACE(Trace::Information, ("Added Regular Bluetooth device: %s, name: %s", address.ToString().c_str(), name.c_str()));
            _devices.push_back(Core::Service<DeviceRegular>::Create<DeviceImpl>(&_administrator, &_application, address, name));
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
    BluetoothControl::DeviceImpl* BluetoothControl::Find(const string& address)
    {
        Bluetooth::Address search(address.c_str());
        std::list<DeviceImpl*>::const_iterator index = _devices.begin();

        while ((index != _devices.end()) && ((*index)->operator==(search) == false)) {
            index++;
        }

        return (index != _devices.end() ? (*index) : nullptr);
    }
    void BluetoothControl::Notification(const uint8_t subEvent, const uint16_t length, const uint8_t* dataFrame)
    {
        _adminLock.Lock();
        std::list<DeviceImpl*>::iterator index = _devices.begin();
        while (index != _devices.end()) {
            (*index)->Notification(subEvent, length, dataFrame);
            index++;
        }
        _adminLock.Unlock();
    }
}
}
