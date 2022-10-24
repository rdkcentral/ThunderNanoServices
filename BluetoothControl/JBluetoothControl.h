
// Partially generated from 'BluetoothControl.json'

// TODO: Add copyright disclaimer here...

#pragma once

#include "Module.h"
#include <interfaces/json/JsonData_BluetoothControl.h>
#include <interfaces/json/JBluetoothControl.h> // version

namespace WPEFramework {

namespace Exchange {

    namespace JBluetoothControl {

        using JSONRPC = PluginHost::JSONRPCSupportsEventStatus;

        using scantype = JsonData::BluetoothControl::scantype;
        using scanmode = JsonData::BluetoothControl::scanmode;
        using devicestate = JsonData::BluetoothControl::DevicestatechangeParamsData::devicestate;
        using disconnectreason = JsonData::BluetoothControl::DevicestatechangeParamsData::disconnectreason;

        template<typename IMPLEMENTATION>
        static void Register(JSONRPC& _module, IMPLEMENTATION* _implementation)
        {
            ASSERT(_implementation != nullptr);

            _module.RegisterVersion(_T("JBluetoothControl"), Version::Major, Version::Minor, Version::Patch);

            // Register methods and properties...

            // Method: setdiscoverable - Starts advertising (or inquiry scanning), making the local interface visible by nearby Bluetooth devices
            _module.Register<JsonData::BluetoothControl::SetdiscoverableParamsData, void>(_T("setdiscoverable"),
                [_implementation](const JsonData::BluetoothControl::SetdiscoverableParamsData& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const JsonData::BluetoothControl::scantype type{Params.Type.Value()};
                    const JsonData::BluetoothControl::scanmode mode{Params.Mode.Value()};
                    const bool connectable{Params.Connectable.Value()};
                    const uint16_t duration{Params.Duration.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: stopdiscoverable - Stops advertising (or inquiry scanning) operation
            _module.Register<JsonData::BluetoothControl::StopdiscoverableParamsInfo, void>(_T("stopdiscoverable"),
                [_implementation](const JsonData::BluetoothControl::StopdiscoverableParamsInfo& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const JsonData::BluetoothControl::scantype type{Params.Type.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: scan - Starts active discovery (or inquiry) of nearby Bluetooth devices
            _module.Register<JsonData::BluetoothControl::ScanParamsData, void>(_T("scan"),
                [_implementation](const JsonData::BluetoothControl::ScanParamsData& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const JsonData::BluetoothControl::scantype type{Params.Type.Value()};
                    const JsonData::BluetoothControl::scanmode mode{Params.Mode.Value()};
                    const uint16_t timeout{Params.Timeout.Value()};
                    const uint16_t duration{Params.Duration.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: stopscanning - Stops discovery (or inquiry) operation
            _module.Register<JsonData::BluetoothControl::StopdiscoverableParamsInfo, void>(_T("stopscanning"),
                [_implementation](const JsonData::BluetoothControl::StopdiscoverableParamsInfo& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const JsonData::BluetoothControl::scantype type{Params.Type.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: connect - Connects to a Bluetooth device
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, void>(_T("connect"),
                [_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const string address{Params.Address.Value()};
                    const Exchange::IBluetooth::IDevice::type type{Params.Type.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: disconnect - Disconnects from a connected Bluetooth device
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, void>(_T("disconnect"),
                [_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const string address{Params.Address.Value()};
                    const Exchange::IBluetooth::IDevice::type type{Params.Type.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: pair - Pairs a Bluetooth device
            _module.Register<JsonData::BluetoothControl::PairParamsData, void>(_T("pair"),
                [_implementation](const JsonData::BluetoothControl::PairParamsData& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const string address{Params.Address.Value()};
                    const Exchange::IBluetooth::IDevice::pairingcapabilities capabilities{Params.Capabilities.Value()};
                    const Exchange::IBluetooth::IDevice::type type{Params.Type.Value()};
                    const uint16_t timeout{Params.Timeout.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: unpair - Unpairs a paired Bluetooth device
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, void>(_T("unpair"),
                [_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const string address{Params.Address.Value()};
                    const Exchange::IBluetooth::IDevice::type type{Params.Type.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: abortpairing - Aborts pairing operation
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, void>(_T("abortpairing"),
                [_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const string address{Params.Address.Value()};
                    const Exchange::IBluetooth::IDevice::type type{Params.Type.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: providepincode - Provides a PIN-code for authentication during a legacy pairing process
            _module.Register<JsonData::BluetoothControl::ProvidepincodeParamsData, void>(_T("providepincode"),
                [_implementation](const JsonData::BluetoothControl::ProvidepincodeParamsData& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const string address{Params.Address.Value()};
                    const Exchange::IBluetooth::IDevice::type type{Params.Type.Value()};
                    const string secret{Params.Secret.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: providepasskey - Provides a passkey for authentication during a pairing process
            _module.Register<JsonData::BluetoothControl::ProvidepasskeyParamsData, void>(_T("providepasskey"),
                [_implementation](const JsonData::BluetoothControl::ProvidepasskeyParamsData& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const string address{Params.Address.Value()};
                    const Exchange::IBluetooth::IDevice::type type{Params.Type.Value()};
                    const uint32_t secret{Params.Secret.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: confirmpasskey - Confirms a passkey for authentication during a pairing process
            _module.Register<JsonData::BluetoothControl::ConfirmpasskeyParamsData, void>(_T("confirmpasskey"),
                [_implementation](const JsonData::BluetoothControl::ConfirmpasskeyParamsData& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const string address{Params.Address.Value()};
                    const Exchange::IBluetooth::IDevice::type type{Params.Type.Value()};
                    const bool iscorrect{Params.Iscorrect.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: forget - Forgets a known Bluetooth device
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, void>(_T("forget"),
                [_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& Params) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    const string address{Params.Address.Value()};
                    const Exchange::IBluetooth::IDevice::type type{Params.Type.Value()};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    return (errorCode);
                });

            // Method: getdevicelist - Retrieves a list of known remote Bluetooth devices
            _module.Register<void, Core::JSON::ArrayType<JsonData::BluetoothControl::ConnectParamsInfo>>(_T("getdevicelist"),
                [_implementation](Core::JSON::ArrayType<JsonData::BluetoothControl::ConnectParamsInfo>& Result) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    std::list<JsonData::BluetoothControl::ConnectParamsInfo> result{};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    if (errorCode == Core::ERROR_NONE) {
                        for (const JsonData::BluetoothControl::ConnectParamsInfo& resultElement : result) {
                            // TODO: Fill in the array...
                            Result.Add(resultElement);
                        }
                    }

                    return (errorCode);
                });

            // Method: getdeviceinfo - Retrieves detailed information about a known Bluetooth device
            _module.Register<JsonData::BluetoothControl::ConnectParamsInfo, JsonData::BluetoothControl::DeviceData>(_T("getdeviceinfo"),
                [_implementation](const JsonData::BluetoothControl::ConnectParamsInfo& Params, JsonData::BluetoothControl::DeviceData& Result) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    string address{Params.Address.Value()};
                    Exchange::IBluetooth::IDevice::type type{Params.Type.Value()};
                    string name{};
                    uint32_t class_{};
                    uint32_t appearance{};
                    std::list<string> services{};
                    bool connected{};
                    bool paired{};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    if (errorCode == Core::ERROR_NONE) {
                        Result.Address = address;
                        Result.Type = type;
                        Result.Name = name;
                        Result.Class = class_;
                        Result.Appearance = appearance;
                        for (const string& servicesElement : services) {
                            // TODO: Fill in the array...
                            Core::JSON::String& servicesItem(Result.Services.Add());
                            servicesItem = servicesElement;
                        }
                        Result.Connected = connected;
                        Result.Paired = paired;
                    }

                    return (errorCode);
                });

            // Property: adapters - List of local Bluetooth adapters (r/o)
            _module.Register<void, Core::JSON::ArrayType<Core::JSON::DecUInt16>>(_T("adapters"),
                [_implementation](Core::JSON::ArrayType<Core::JSON::DecUInt16>& Result) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    // read-only property get
                    std::list<uint16_t> result{};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    if (errorCode == Core::ERROR_NONE) {
                        for (const uint16_t& resultElement : result) {
                            // TODO: Fill in the array...
                            Core::JSON::DecUInt16& resultItem(Result.Add());
                            resultItem = resultElement;
                        }
                    }

                    return (errorCode);
                });

            // Indexed Property: adapter - Local Bluetooth adapter information (r/o)
            _module.Register<void, JsonData::BluetoothControl::AdapterData, std::function<uint32_t(const std::string&, JsonData::BluetoothControl::AdapterData&)>>(_T("adapter"),
                [_implementation](const string& Index, JsonData::BluetoothControl::AdapterData& Result) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    // read-only property get
                    uint16_t id{};
                    string address{};
                    string interface{};
                    JsonData::BluetoothControl::AdapterData::adaptertype type{};
                    uint8_t version{};
                    uint16_t manufacturer{};
                    uint32_t class_{};
                    string name{};
                    string shortname{};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(Index, ...);

                    if (errorCode == Core::ERROR_NONE) {
                        Result.Id = id;
                        Result.Interface = interface;
                        Result.Address = address;
                        Result.Type = type;
                        Result.Version = version;
                        Result.Manufacturer = manufacturer;
                        Result.Class = class_;
                        Result.Name = name;
                        Result.Shortname = shortname;
                    }

                    return (errorCode);
                });

            // Property: devices - List of known remote Bluetooth devices (DEPRECATED) (r/o)
            _module.Register<void, Core::JSON::ArrayType<Core::JSON::String>>(_T("devices"),
                [_implementation](Core::JSON::ArrayType<Core::JSON::String>& Result) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    // read-only property get
                    std::list<string> result{};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(...);

                    if (errorCode == Core::ERROR_NONE) {
                        for (const string& resultElement : result) {
                            // TODO: Fill in the array...
                            Core::JSON::String& resultItem(Result.Add());
                            resultItem = resultElement;
                        }
                    }

                    return (errorCode);
                });

            // Indexed Property: device - Remote Bluetooth device information (DEPRECATED) (r/o)
            _module.Register<void, JsonData::BluetoothControl::DeviceData, std::function<uint32_t(const std::string&, JsonData::BluetoothControl::DeviceData&)>>(_T("device"),
                [_implementation](const string& Index, JsonData::BluetoothControl::DeviceData& Result) -> uint32_t {
                    uint32_t errorCode = Core::ERROR_NONE;
                    // read-only property get
                    string address{};
                    Exchange::IBluetooth::IDevice::type type{};
                    string name{};
                    uint32_t class_{};
                    uint32_t appearance{};
                    std::list<string> services{};
                    bool connected{};
                    bool paired{};

                    // TODO: Call implementation here...
                    // errorCode = _implementation->api(Index, ...);

                    if (errorCode == Core::ERROR_NONE) {
                        Result.Address = address;
                        Result.Type = type;
                        Result.Name = name;
                        Result.Class = class_;
                        Result.Appearance = appearance;
                        for (const string& servicesElement : services) {
                            // TODO: Fill in the array...
                            Core::JSON::String& servicesItem(Result.Services.Add());
                            servicesItem = servicesElement;
                        }
                        Result.Connected = connected;
                        Result.Paired = paired;
                    }

                    return (errorCode);
                });

            // Register event status listeners...

            _module.RegisterEventStatusListener(_T("discoverablestarted"),
                [_implementation](const string& Client, const JSONRPC::Status Status) {
                    const string id = Client.substr(0, Client.find('.'));

                    if (Status == JSONRPC::Status::registered) {
                        // TODO...
                    } else if (Status == JSONRPC::Status::unregistered) {
                        // TODO...
                    }
                });

            _module.RegisterEventStatusListener(_T("scanstarted"),
                [_implementation](const string& Client, const JSONRPC::Status Status) {
                    const string id = Client.substr(0, Client.find('.'));

                    if (Status == JSONRPC::Status::registered) {
                        // TODO...
                    } else if (Status == JSONRPC::Status::unregistered) {
                        // TODO...
                    }
                });

            _module.RegisterEventStatusListener(_T("devicestatechange"),
                [_implementation](const string& Client, const JSONRPC::Status Status) {
                    const string id = Client.substr(0, Client.find('.'));

                    if (Status == JSONRPC::Status::registered) {
                        // TODO...
                    } else if (Status == JSONRPC::Status::unregistered) {
                        // TODO...
                    }
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

            // Event: discoverablestarted - Notifies of entering the discoverable state
            static void DiscoverableStarted(const JSONRPC& _module, const JsonData::BluetoothControl::scantype& Type, const JsonData::BluetoothControl::scanmode& Mode)
            {
                JsonData::BluetoothControl::DiscoverablestartedParamsInfo _params;
                _params.Type = Type;
                _params.Mode = Mode;

                _module.Notify(_T("discoverablestarted"), _params);
            }

            // Event: discoverablecomplete - Notifies of leaving the discoverable state
            static void DiscoverableComplete(const JSONRPC& _module, const JsonData::BluetoothControl::scantype& Type)
            {
                JsonData::BluetoothControl::StopdiscoverableParamsInfo _params;
                _params.Type = Type;

                _module.Notify(_T("discoverablecomplete"), _params);
            }

            // Event: scanstarted - Notifies of scan start
            static void ScanStarted(const JSONRPC& _module, const JsonData::BluetoothControl::scantype& Type, const JsonData::BluetoothControl::scanmode& Mode)
            {
                JsonData::BluetoothControl::DiscoverablestartedParamsInfo _params;
                _params.Type = Type;
                _params.Mode = Mode;

                _module.Notify(_T("scanstarted"), _params);
            }

            // Event: scancomplete - Notifies of scan completion
            static void ScanComplete(const JSONRPC& _module, const JsonData::BluetoothControl::scantype& Type)
            {
                JsonData::BluetoothControl::StopdiscoverableParamsInfo _params;
                _params.Type = Type;

                _module.Notify(_T("scancomplete"), _params);
            }

            // Event: devicestatechange - Notifies of device state changes
            static void DeviceStateChanged(const JSONRPC& _module, const string& id, const string& Address, const Exchange::IBluetooth::IDevice::type& Type, 
                const JsonData::BluetoothControl::DevicestatechangeParamsData::devicestate& State, 
                const JsonData::BluetoothControl::DevicestatechangeParamsData::disconnectreason& Disconnectreason)
            {
                JsonData::BluetoothControl::DevicestatechangeParamsData _params;
                _params.Address = Address;
                _params.Type = Type;
                _params.State = State;

                if (_params.State == JsonData::BluetoothControl::DevicestatechangeParamsData::devicestate::DISCONNECTED) {
                    _params.Disconnectreason = Disconnectreason;
                }

                _module.Notify(_T("devicestatechange"), _params, [&id](const string& designator) -> bool {
                    const string designatorId = designator.substr(0, designator.find('.'));
                    return ((id == designatorId) || (designatorId.find(':') == string::npos));
                });
            }

            // Event: pincoderequest - Notifies of a PIN code request
            static void PINCodeRequest(const JSONRPC& _module, const string& Address, const Exchange::IBluetooth::IDevice::type& Type)
            {
                JsonData::BluetoothControl::ConnectParamsInfo _params;
                _params.Address = Address;
                _params.Type = Type;

                _module.Notify(_T("pincoderequest"), _params);
            }

            // Event: passkeyrequest - Notifies of a passkey request
            static void PasskeyRequest(const JSONRPC& _module, const string& Address, const Exchange::IBluetooth::IDevice::type& Type)
            {
                JsonData::BluetoothControl::ConnectParamsInfo _params;
                _params.Address = Address;
                _params.Type = Type;

                _module.Notify(_T("passkeyrequest"), _params);
            }

            // Event: passkeyconfirmrequest - Notifies of a passkey confirmation request
            static void PasskeyConfirmRequest(const JSONRPC& _module, const string& Address, const Exchange::IBluetooth::IDevice::type& Type, const uint32_t& Secret)
            {
                JsonData::BluetoothControl::PasskeyconfirmrequestParamsData _params;
                _params.Address = Address;
                _params.Type = Type;
                _params.Secret = Secret;

                _module.Notify(_T("passkeyconfirmrequest"), _params);
            }

        } // namespace Event

    } // namespace JBluetoothControl

} // namespace Exchange

}

