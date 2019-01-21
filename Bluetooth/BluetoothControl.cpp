#include "BluetoothControl.h"

namespace WPEFramework {

#define ADAPTER_INDEX                   0X00
#define ENABLE_MODE                     0x01

namespace Plugin {

    SERVICE_REGISTRATION(BluetoothControl, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<BluetoothControl::DeviceImpl::JSON> > jsonResponseFactoryDevice(1);
    static Core::ProxyPoolType<Web::JSONBodyType<BluetoothControl::Status> > jsonResponseFactoryStatus(1);
    /* static */ string BluetoothControl::_HIDPath;

    /* virtual */ uint32_t BluetoothControl::DeviceImpl::Pair(const string& source) {
        if (_channel == nullptr) {
            Bluetooth::Address from(source.c_str());
            _channel = new HIDSocket(_HIDPath, from, _address);

            if ( (_channel != nullptr) && (_channel->Open(1000) == Core::ERROR_NONE) ) {
                _channel->Security(BT_SECURITY_MEDIUM, 0);
            }
        }
        return (Core::ERROR_NONE);
    }

    /* virtual */ uint32_t BluetoothControl::DeviceImpl::Unpair() {
        if (_channel != nullptr) {
            _channel->Close(Core::infinite);
            delete _channel;
            _channel = nullptr;
        }
        return (Core::ERROR_NONE);
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
        }
        else {
            _interface = Bluetooth::Driver::Interface(config.Interface.Value());
         
            if (_interface.IsValid() == false) {
                result = _T("Could not bring up the interface.");
            }
            else {
                HCISocket channel (*this);

                if (channel.Open(Core::infinite) != Core::ERROR_NONE) {
                    result = _T("Could not open a management Bleutooth connection.");
                }
                else {
                    Bluetooth::ManagementFrame mgmtFrame(ADAPTER_INDEX);
                    struct mgmt_mode modeFlags;
                    modeFlags.val = htobs(ENABLE_MODE);

                    if (channel.Send(mgmtFrame.Set(MGMT_OP_SET_POWERED, modeFlags), 1000) != Core::ERROR_NONE) {
                        result = "Failed to power on bluetooth adaptor";
                    }
                    // Enable Bondable on adaptor.
                    else if (channel.Send(mgmtFrame.Set(MGMT_OP_SET_BONDABLE, modeFlags), 1000) != Core::ERROR_NONE) {
                        result = "Failed to enable Bondable";
                    }
                    // Enable Simple Secure Simple Pairing.
                    else if (channel.Send(mgmtFrame.Set(MGMT_OP_SET_SSP, modeFlags), 1000) != Core::ERROR_NONE) {
                        result = "Failed to enable Simple Secure Simple Pairing";
                    }
                    // Enable Low Energy
                    else if (channel.Send(mgmtFrame.Set(MGMT_OP_SET_LE, modeFlags), 1000) != Core::ERROR_NONE) {
                        result = "Failed to enable Low Energy";
                    }    
                    // Enable Secure Connections
                    else if (channel.Send(mgmtFrame.Set(MGMT_OP_SET_SECURE_CONN, modeFlags), 1000) != Core::ERROR_NONE) {
                        result = "Failed to enable Secure Connections";
                    }
                    else if (_interface.Up() == false) {
                        result = "Failed to bring up the Bluetooth interface";
                    }
                    else if (_btAddress.Default() == false) {
                        result = "Could not get the Bluetooth address";
                    }
                    else {
                        _hciSocket.LocalNode(_btAddress.NodeId());

                        if (_hciSocket.Open(100) != Core::ERROR_NONE) {
                            result = "Could not open the HCI control channel";
                        }
                    }
                }                
            }
        }

        if ( (_driver != nullptr) && (result.empty() == false) ) {
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

        if ((request.Verb == Web::Request::HTTP_PUT) || (request.Verb == Web::Request::HTTP_POST)) {
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

        if (index.IsValid() == false) {
            Core::ProxyType<Web::JSONBodyType<Status> > response(jsonResponseFactoryStatus.Element());

            result->ErrorCode = Web::STATUS_OK;
            result->Message = _T("Current status.");

            response->Scanning = IsScanning();
            std::list<DeviceImpl*>::const_iterator index = _devices.begin();

            while (index != _devices.end()) { 
                response->Devices.Add().Set(*index);
                index++; 
            }

            result->Body(response);
        }
        else {
            if (index.Next() && index.IsValid()) {
                TRACE(Trace::Information, (string(__FUNCTION__)));
                DeviceImpl* device = Find(index.Current().Text());
 
                if (device != nullptr) {
                    Core::ProxyType<Web::JSONBodyType<DeviceImpl::JSON> > response(jsonResponseFactoryDevice.Element());
                    response->Set(device);

                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("device info.");
                    result->Body(response);
                } else {
                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = _T("Unable to display Paired devices.");
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
                TRACE(Trace::Information, (string(__FUNCTION__)));

                if (index.Current() == _T("Scan")) {
                    if (Scan(true) == Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Scan started.");
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Unable to start Scan.");
                    }
                } else if (index.Current() == _T("Pair")) {
                    string destination;
                    if (index.Next() == true) {
                        destination = index.Current().Text();
                    }
                    else if (request.HasBody() == true) {
                        destination = request.Body<const DeviceImpl::JSON>()->Address;
                    }
                    DeviceImpl* device = Find(destination);
                    if (device == nullptr) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = _T("Paired device.");
                    }
                    else if (device->Pair(_btAddress.ToString())) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Paired device.");
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Unable to Pair device.");
                    }
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Unable to Disconnect device.");
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
                    if (Scan(false) == Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Scan stopped.");
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Unable to start Scan.");
                    }
                } else if (index.Current() == _T("Pair")) {
                    string address;
                    if (index.Next() == true) {
                        address = index.Current().Text();
                    }
                    else if (request.HasBody() == true) {
                        address = request.Body<const DeviceImpl::JSON>()->Address;
                    }
                    DeviceImpl* device = Find(address);
                    if (device == nullptr) {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = _T("Unpaired device.");
                    }
                    else if (device->Unpair()) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Unpaired device.");
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Unable to Pair device.");
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
    /* virtual */ bool BluetoothControl::IsScanning() const {
        return (_hciSocket.IsScanning());
    }
    /* virtual */ uint32_t BluetoothControl::Register(IBluetooth::INotification* notification) {
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(std::find(_observers.begin(), _observers.end(), sink) == _observers.end());

        _observers.push_back(notification);
        notification->AddRef();

        // Allright iterate over all devices, so thay get announced by the observer..
        for ( std::list<DeviceImpl*>::iterator index = _devices.begin(), end = _devices.end(); index != end; ++index ) {
            notification->Update(*index);
        }

        _adminLock.Unlock();
    }
    /* virtual */ uint32_t BluetoothControl::Unregister(IBluetooth::INotification* notification) {
        _adminLock.Lock();

        std::list<IBluetooth::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _observers.end());

        if (index != _observers.end()) {
            (*index)->Release();
            _observers.erase(index);
        }

        _adminLock.Unlock();
    }
    /* virtual */ bool BluetoothControl::Scan(const bool enable) {
        if ((_hciSocket.IsScanning() == false) && (enable == true)) {

            TRACE(Trace::Information, ("Start Bluetooth Scan"));

            // Clearing previously discovered devices.
            RemoveDevices ([] (DeviceImpl* device) -> bool { if ((device->IsPaired() == false) && (device->IsConnected() == false)) device->Clear(); return(false); });

            _hciSocket.StartScan(0x10, 0x10);

        } 
        else if ((_hciSocket.IsScanning() == true) && (enable == false)) {

            TRACE(Trace::Information, ("Stop Bluetooth Scan"));

            _hciSocket.StopScan();
        }

        return (_hciSocket.IsScanning() == enable);
    }

    /* virtual */ Exchange::IBluetooth::IDevice* BluetoothControl::Device (const string& address) {
        IBluetooth::IDevice* result = Find(address);
        if (result != nullptr) {
            result->AddRef();
        }
        return (result);
    }

    /* virtual */ Exchange::IBluetooth::IDevice::IIterator* BluetoothControl::Devices () {
        return (Core::Service<DeviceImpl::IteratorImpl>::Create<IBluetooth::IDevice::IIterator>(_devices));
    }

    void BluetoothControl::DiscoveredDevice(const Bluetooth::Address& address, const string& shortName, const string& longName) {

        _adminLock.Lock();

        std::list<DeviceImpl*>::iterator index = _devices.begin();

        while ( (index != _devices.end()) && (*(*index) != address) ) { index++; }

        if (index != _devices.end()) {
            (*index)->Update(shortName,longName);
        }
        else {
            _devices.push_back(Core::Service<DeviceImpl>::Create<DeviceImpl>(address, shortName, longName));
        }

        _adminLock.Unlock();
    }

    void BluetoothControl::RemoveDevices(std::function<bool(DeviceImpl*)> filter) {

        _adminLock.Lock();

        for ( std::list<DeviceImpl*>::iterator index = _devices.begin(), end = _devices.end(); index != end; ++index ) {
            // call the function passed into findMatchingAddresses and see if it matches
            if ( filter ( *index ) == true )
            {
                (*index)->Release();
                index = _devices.erase(index);
            }
        }

        _adminLock.Unlock();
    }
    BluetoothControl::DeviceImpl* BluetoothControl::Find(const string& address) {
        Bluetooth::Address search(address.c_str());
        std::list<DeviceImpl*>::const_iterator index = _devices.begin();

        while ( (index != _devices.end()) && ((*index)->operator==(search) == false) ) { index++; }

        return (index != _devices.end() ? (*index) : nullptr);
    }
}
}
