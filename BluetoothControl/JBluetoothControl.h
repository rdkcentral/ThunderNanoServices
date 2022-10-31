
// Generated automatically from 'BluetoothControl.json'. DO NOT EDIT.

#pragma once

#if _IMPLEMENTATION_STUB
// sample implementation class
class JSONRPCImplementation {
public:
    uint32_t SetDiscoverable(const Core::JSON::EnumType<JsonData::BluetoothControl::scantype>& type,
             const Core::JSON::EnumType<JsonData::BluetoothControl::scanmode>& mode, const Core::JSON::Boolean& connectable, const Core::JSON::DecUInt16& duration) { return (Core::ERROR_NONE); }
    uint32_t StopDiscoverable(const Core::JSON::EnumType<JsonData::BluetoothControl::scantype>& type) { return (Core::ERROR_NONE); }
    uint32_t Scan(const Core::JSON::EnumType<JsonData::BluetoothControl::scantype>& type,
             const Core::JSON::EnumType<JsonData::BluetoothControl::scanmode>& mode, const Core::JSON::DecUInt16& timeout, const Core::JSON::DecUInt16& duration) { return (Core::ERROR_NONE); }
    uint32_t StopScanning(const Core::JSON::EnumType<JsonData::BluetoothControl::scantype>& type) { return (Core::ERROR_NONE); }
    uint32_t Connect(const Core::JSON::String& address, const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type) { return (Core::ERROR_NONE); }
    uint32_t Disconnect(const Core::JSON::String& address, const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type) { return (Core::ERROR_NONE); }
    uint32_t Pair(const Core::JSON::String& address, const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type,
             const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::pairingcapabilities>& capabilities, const Core::JSON::DecUInt16& timeout) { return (Core::ERROR_NONE); }
    uint32_t Unpair(const Core::JSON::String& address, const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type) { return (Core::ERROR_NONE); }
    uint32_t AbortPairing(const Core::JSON::String& address,
             const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type) { return (Core::ERROR_NONE); }
    uint32_t ProvidePINCode(const Core::JSON::String& address, const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type,
             const Core::JSON::String& secret) { return (Core::ERROR_NONE); }
    uint32_t ProvidePasskey(const Core::JSON::String& address, const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type,
             const Core::JSON::DecUInt32& secret) { return (Core::ERROR_NONE); }
    uint32_t ConfirmPasskey(const Core::JSON::String& address, const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type,
             const Core::JSON::Boolean& iscorrect) { return (Core::ERROR_NONE); }
    uint32_t Forget(const Core::JSON::String& address, const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type) { return (Core::ERROR_NONE); }
    uint32_t GetDeviceList(Core::JSON::ArrayType<JsonData::BluetoothControl::ConnectParamsInfo>& result) { return (Core::ERROR_NONE); }
    uint32_t GetDeviceInfo(Core::JSON::String& address, Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type, Core::JSON::String& name,
             Core::JSON::DecUInt32& class_, Core::JSON::DecUInt32& appearance, Core::JSON::ArrayType<Core::JSON::String>& services, Core::JSON::Boolean& connected,
             Core::JSON::Boolean& paired) { return (Core::ERROR_NONE); }
    uint32_t Adapters(Core::JSON::ArrayType<Core::JSON::DecUInt16>& result) const { return (Core::ERROR_NONE); }
    uint32_t Adapter(const string& index_, Core::JSON::DecUInt16& id, Core::JSON::String& address, Core::JSON::String& interface,
             Core::JSON::EnumType<JsonData::BluetoothControl::AdapterData::adaptertype>& type, Core::JSON::DecUInt8& version, Core::JSON::DecUInt16& manufacturer,
             Core::JSON::DecUInt32& class_, Core::JSON::String& name, Core::JSON::String& shortname) const { return (Core::ERROR_NONE); }
    uint32_t Devices(Core::JSON::ArrayType<Core::JSON::String>& result) const { return (Core::ERROR_NONE); }
    uint32_t Device(const string& index_, Core::JSON::String& address, Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type,
             Core::JSON::String& name, Core::JSON::DecUInt32& class_, Core::JSON::DecUInt32& appearance, Core::JSON::ArrayType<Core::JSON::String>& services,
             Core::JSON::Boolean& connected, Core::JSON::Boolean& paired) const { return (Core::ERROR_NONE); }
    void OnDiscoverableStartedEventRegistration(const string& client, const JBluetoothControl::JSONRPC::Status status) { }
    void OnScanStartedEventRegistration(const string& client, const JBluetoothControl::JSONRPC::Status status) { }
    void OnDeviceStateChangedEventRegistration(const string& client, const JBluetoothControl::JSONRPC::Status status) { }
    template<typename IMPLEMENTATION> static bool DeviceStateChangedEventSendIfHook(IMPLEMENTATION& _implementation, const string& designator,
             const string& id) { return (false); }
}; // class JSONRPCImplementation
#endif // _IMPLEMENTATION_STUB

#include "Module.h"
#include "JsonData_BluetoothControl.h"

namespace WPEFramework {

namespace Exchange {

    namespace JBluetoothControl {
        namespace Version {

            constexpr uint8_t Major = 1;
            constexpr uint8_t Minor = 0;
            constexpr uint8_t Patch = 0;

        } // namespace Version

        using JSONRPC = PluginHost::JSONRPCSupportsEventStatus;

        template<typename IMPLEMENTATION>
        static void Register(JSONRPC& _module, IMPLEMENTATION& _implementation)
        {
            _module.RegisterVersion(_T("JBluetoothControl"), Version::Major, Version::Minor, Version::Patch);

            // Register methods and properties...

            // Method: setdiscoverable - Starts advertising (or inquiry scanning), making the local interface visible by nearby Bluetooth devices
            _module.Register<JsonData::BluetoothControl::SetdiscoverableParamsData, void>(_T("setdiscoverable"),
                [&_implementation](const JsonData::BluetoothControl::SetdiscoverableParamsData& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.SetDiscoverable(params.Type, params.Mode, params.Connectable, params.Duration);
                    return (_errorCode);
                });

            // Method: stopdiscoverable - Stops advertising (or inquiry scanning) operation
            _module.Register<JsonData::BluetoothControl::StopdiscoverableParamsInfo, void>(_T("stopdiscoverable"),
                [&_implementation](const JsonData::BluetoothControl::StopdiscoverableParamsInfo& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.StopDiscoverable(params.Type);
                    return (_errorCode);
                });

            // Method: scan - Starts active discovery (or inquiry) of nearby Bluetooth devices
            _module.Register<JsonData::BluetoothControl::ScanParamsData, void>(_T("scan"),
                [&_implementation](const JsonData::BluetoothControl::ScanParamsData& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.Scan(params.Type, params.Mode, params.Timeout, params.Duration);
                    return (_errorCode);
                });

            // Method: stopscanning - Stops discovery (or inquiry) operation
            _module.Register<JsonData::BluetoothControl::StopdiscoverableParamsInfo, void>(_T("stopscanning"),
                [&_implementation](const JsonData::BluetoothControl::StopdiscoverableParamsInfo& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.StopScanning(params.Type);
                    return (_errorCode);
                });

            // Method: connect - Connects to a Bluetooth device
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, void>(_T("connect"),
                [&_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.Connect(params.Address, params.Type);
                    return (_errorCode);
                });

            // Method: disconnect - Disconnects from a connected Bluetooth device
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, void>(_T("disconnect"),
                [&_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.Disconnect(params.Address, params.Type);
                    return (_errorCode);
                });

            // Method: pair - Pairs a Bluetooth device
            _module.Register<JsonData::BluetoothControl::PairParamsData, void>(_T("pair"),
                [&_implementation](const JsonData::BluetoothControl::PairParamsData& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.Pair(params.Address, params.Type, params.Capabilities, params.Timeout);
                    return (_errorCode);
                });

            // Method: unpair - Unpairs a paired Bluetooth device
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, void>(_T("unpair"),
                [&_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.Unpair(params.Address, params.Type);
                    return (_errorCode);
                });

            // Method: abortpairing - Aborts pairing operation
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, void>(_T("abortpairing"),
                [&_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.AbortPairing(params.Address, params.Type);
                    return (_errorCode);
                });

            // Method: providepincode - Provides a PIN-code for authentication during a legacy pairing process
            _module.Register<JsonData::BluetoothControl::ProvidepincodeParamsData, void>(_T("providepincode"),
                [&_implementation](const JsonData::BluetoothControl::ProvidepincodeParamsData& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.ProvidePINCode(params.Address, params.Type, params.Secret);
                    return (_errorCode);
                });

            // Method: providepasskey - Provides a passkey for authentication during a pairing process
            _module.Register<JsonData::BluetoothControl::ProvidepasskeyParamsData, void>(_T("providepasskey"),
                [&_implementation](const JsonData::BluetoothControl::ProvidepasskeyParamsData& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.ProvidePasskey(params.Address, params.Type, params.Secret);
                    return (_errorCode);
                });

            // Method: confirmpasskey - Confirms a passkey for authentication during a pairing process
            _module.Register<JsonData::BluetoothControl::ConfirmpasskeyParamsData, void>(_T("confirmpasskey"),
                [&_implementation](const JsonData::BluetoothControl::ConfirmpasskeyParamsData& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.ConfirmPasskey(params.Address, params.Type, params.Iscorrect);
                    return (_errorCode);
                });

            // Method: forget - Forgets a known Bluetooth device
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, void>(_T("forget"),
                [&_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& params) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.Forget(params.Address, params.Type);
                    return (_errorCode);
                });

            // Method: getdevicelist - Retrieves a list of known remote Bluetooth devices
            _module.Register<void, Core::JSON::ArrayType<JsonData::BluetoothControl::ConnectParamsInfo>>(_T("getdevicelist"),
                [&_implementation](Core::JSON::ArrayType<JsonData::BluetoothControl::ConnectParamsInfo>& result) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    _errorCode = _implementation.GetDeviceList(result);
                    return (_errorCode);
                });

            // Method: getdeviceinfo - Retrieves detailed information about a known Bluetooth device
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, JsonData::BluetoothControl::DeviceData>(_T("getdeviceinfo"),
                [&_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& params, JsonData::BluetoothControl::DeviceData& result) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    result.Address = params.Address;
                    result.Type = params.Type;
                    _errorCode = _implementation.GetDeviceInfo(result.Address, result.Type, result.Name, result.Class, result.Appearance, result.Services,
                             result.Connected, result.Paired);
                    return (_errorCode);
                });

            // Property: adapters - List of local Bluetooth adapters (r/o)
            _module.Register<void, Core::JSON::ArrayType<Core::JSON::DecUInt16>>(_T("adapters"),
                [&_implementation](Core::JSON::ArrayType<Core::JSON::DecUInt16>& result) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    // read-only property get
                    _errorCode = _implementation.Adapters(result);
                    return (_errorCode);
                });

            // Indexed Property: adapter - Local Bluetooth adapter information (r/o)
            _module.Register<void, JsonData::BluetoothControl::AdapterData, std::function<uint32_t(const std::string&,
                     JsonData::BluetoothControl::AdapterData&)>>(_T("adapter"),
                [&_implementation](const string& index_, JsonData::BluetoothControl::AdapterData& result) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    // read-only property get
                    _errorCode = _implementation.Adapter(index_, result.Id, result.Address, result.Interface, result.Type, result.Version, result.Manufacturer,
                             result.Class, result.Name, result.Shortname);
                    return (_errorCode);
                });

            // Property: devices - List of known remote Bluetooth devices (DEPRECATED) (r/o)
            _module.Register<void, Core::JSON::ArrayType<Core::JSON::String>>(_T("devices"),
                [&_implementation](Core::JSON::ArrayType<Core::JSON::String>& result) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    // read-only property get
                    _errorCode = _implementation.Devices(result);
                    return (_errorCode);
                });

            // Indexed Property: device - Remote Bluetooth device information (DEPRECATED) (r/o)
            _module.Register<void, JsonData::BluetoothControl::DeviceData, std::function<uint32_t(const std::string&,
                     JsonData::BluetoothControl::DeviceData&)>>(_T("device"),
                [&_implementation](const string& index_, JsonData::BluetoothControl::DeviceData& result) -> uint32_t {
                    uint32_t _errorCode = Core::ERROR_NONE;
                    // read-only property get
                    _errorCode = _implementation.Device(index_, result.Address, result.Type, result.Name, result.Class, result.Appearance, result.Services,
                             result.Connected, result.Paired);
                    return (_errorCode);
                });

            // Register event status listeners...

            _module.RegisterEventStatusListener(_T("discoverablestarted"),
                [&_implementation](const string& client, const JSONRPC::Status status) {
                    const string id = client.substr(0, client.find('.'));
                    _implementation.OnDiscoverableStartedEventRegistration(id, status);
                });

            _module.RegisterEventStatusListener(_T("scanstarted"),
                [&_implementation](const string& client, const JSONRPC::Status status) {
                    const string id = client.substr(0, client.find('.'));
                    _implementation.OnScanStartedEventRegistration(id, status);
                });

            _module.RegisterEventStatusListener(_T("devicestatechange"),
                [&_implementation](const string& client, const JSONRPC::Status status) {
                    const string id = client.substr(0, client.find('.'));
                    _implementation.OnDeviceStateChangedEventRegistration(id, status);
                });

        }

        static void Unregister(JSONRPC& _module)
        {
            // Unregister methods and properties...
            _module.Unregister(_T("setdiscoverable"));
            _module.Unregister(_T("stopdiscoverable"));
            _module.Unregister(_T("scan"));
            _module.Unregister(_T("stopscanning"));
            _module.Unregister(_T("connect"));
            _module.Unregister(_T("disconnect"));
            _module.Unregister(_T("pair"));
            _module.Unregister(_T("unpair"));
            _module.Unregister(_T("abortpairing"));
            _module.Unregister(_T("providepincode"));
            _module.Unregister(_T("providepasskey"));
            _module.Unregister(_T("confirmpasskey"));
            _module.Unregister(_T("forget"));
            _module.Unregister(_T("getdevicelist"));
            _module.Unregister(_T("getdeviceinfo"));
            _module.Unregister(_T("adapters"));
            _module.Unregister(_T("adapter"));
            _module.Unregister(_T("devices"));
            _module.Unregister(_T("device"));

            // Unregister event status listeners...
            _module.UnregisterEventStatusListener(_T("discoverablestarted"));
            _module.UnregisterEventStatusListener(_T("scanstarted"));
            _module.UnregisterEventStatusListener(_T("devicestatechange"));
        }

        namespace Event {

            PUSH_WARNING(DISABLE_WARNING_UNUSED_FUNCTIONS)

            // Event: discoverablestarted - Notifies of entering the discoverable state
            static void DiscoverableStarted(const JSONRPC& _module, const Core::JSON::EnumType<JsonData::BluetoothControl::scantype>& type,
                     const Core::JSON::EnumType<JsonData::BluetoothControl::scanmode>& mode, const Core::JSON::Boolean& connectable,
                     const std::function<bool(const string&)>& _sendifFunction = nullptr)
            {
                JsonData::BluetoothControl::DiscoverablestartedParamsData _params;
                _params.Type = type;
                _params.Mode = mode;
                _params.Connectable = connectable;

                _module.Notify(_T("discoverablestarted"), _params, _sendifFunction);
            }

            // Event: discoverablecomplete - Notifies of leaving the discoverable state
            static void DiscoverableComplete(const JSONRPC& _module, const Core::JSON::EnumType<JsonData::BluetoothControl::scantype>& type,
                     const std::function<bool(const string&)>& _sendifFunction = nullptr)
            {
                JsonData::BluetoothControl::StopdiscoverableParamsInfo _params;
                _params.Type = type;

                _module.Notify(_T("discoverablecomplete"), _params, _sendifFunction);
            }

            // Event: scanstarted - Notifies of scan start
            static void ScanStarted(const JSONRPC& _module, const Core::JSON::EnumType<JsonData::BluetoothControl::scantype>& type,
                     const Core::JSON::EnumType<JsonData::BluetoothControl::scanmode>& mode, const std::function<bool(const string&)>& _sendifFunction = nullptr)
            {
                JsonData::BluetoothControl::ScanstartedParamsData _params;
                _params.Type = type;
                _params.Mode = mode;

                _module.Notify(_T("scanstarted"), _params, _sendifFunction);
            }

            // Event: scancomplete - Notifies of scan completion
            static void ScanComplete(const JSONRPC& _module, const Core::JSON::EnumType<JsonData::BluetoothControl::scantype>& type,
                     const std::function<bool(const string&)>& _sendifFunction = nullptr)
            {
                JsonData::BluetoothControl::StopdiscoverableParamsInfo _params;
                _params.Type = type;

                _module.Notify(_T("scancomplete"), _params, _sendifFunction);
            }

            // Event: devicestatechange - Notifies of device state changes
            static void DeviceStateChanged(const JSONRPC& _module, const string& _id, const Core::JSON::String& address,
                     const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type, const Core::JSON::EnumType<JsonData::BluetoothControl::DevicestatechangeParamsData::devicestate>& state,
                     const Core::JSON::EnumType<JsonData::BluetoothControl::DevicestatechangeParamsData::disconnectreason>& disconnectreason,
                     const std::function<bool(const string&)>& _sendifFunction = nullptr)
            {
                JsonData::BluetoothControl::DevicestatechangeParamsData _params;
                _params.Address = address;
                _params.Type = type;
                _params.State = state;
                _params.Disconnectreason = disconnectreason;

                if (_sendifFunction == nullptr) {
                    _module.Notify(_T("devicestatechange"), _params, [&_id](const string& _designator) -> bool {
                        const string _designatorId = _designator.substr(0, _designator.find('.'));
                        return (_id == _designatorId);
                    });
                } else {
                    _module.Notify(_T("devicestatechange"), _params, _sendifFunction);
                }
            }

            // Event: pincoderequest - Notifies of a PIN code request
            static void PINCodeRequest(const JSONRPC& _module, const Core::JSON::String& address,
                     const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type, const std::function<bool(const string&)>& _sendifFunction = nullptr)
            {
                JsonData::BluetoothControl::ConnectParamsInfo _params;
                _params.Address = address;
                _params.Type = type;

                _module.Notify(_T("pincoderequest"), _params, _sendifFunction);
            }

            // Event: passkeyrequest - Notifies of a passkey request
            static void PasskeyRequest(const JSONRPC& _module, const Core::JSON::String& address,
                     const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type, const std::function<bool(const string&)>& _sendifFunction = nullptr)
            {
                JsonData::BluetoothControl::ConnectParamsInfo _params;
                _params.Address = address;
                _params.Type = type;

                _module.Notify(_T("passkeyrequest"), _params, _sendifFunction);
            }

            // Event: passkeyconfirmrequest - Notifies of a passkey confirmation request
            static void PasskeyConfirmRequest(const JSONRPC& _module, const Core::JSON::String& address,
                     const Core::JSON::EnumType<Exchange::IBluetooth::IDevice::type>& type, const Core::JSON::DecUInt32& secret, const std::function<bool(const string&)>& _sendifFunction = nullptr)
            {
                JsonData::BluetoothControl::PasskeyconfirmrequestParamsData _params;
                _params.Address = address;
                _params.Type = type;
                _params.Secret = secret;

                _module.Notify(_T("passkeyconfirmrequest"), _params, _sendifFunction);
            }

            POP_WARNING()

        } // namespace Event

    } // namespace JBluetoothControl

} // namespace Exchange

}

