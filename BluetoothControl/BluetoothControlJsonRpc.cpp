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

#include "Module.h"
#include "BluetoothControl.h"
#include <interfaces/json/JsonData_BluetoothControl.h>


namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::BluetoothControl;

    // Registration
    //

    void BluetoothControl::RegisterAll()
    {
        JSONRPC::Register<ScanParamsData,void>(_T("scan"), &BluetoothControl::endpoint_scan, this);
        JSONRPC::Register<ConnectParamsInfo,void>(_T("connect"), &BluetoothControl::endpoint_connect, this);
        JSONRPC::Register<ConnectParamsInfo,void>(_T("disconnect"), &BluetoothControl::endpoint_disconnect, this);
        JSONRPC::Register<PairParamsData,void>(_T("pair"), &BluetoothControl::endpoint_pair, this);
        JSONRPC::Register<ConnectParamsInfo,void>(_T("unpair"), &BluetoothControl::endpoint_unpair, this);
        JSONRPC::Register<ConnectParamsInfo,void>(_T("abortpairing"), &BluetoothControl::endpoint_abortpairing, this);
        JSONRPC::Register<PincodeParamsData,void>(_T("pincode"), &BluetoothControl::endpoint_pincode, this);
        JSONRPC::Register<PasskeyParamsInfo,void>(_T("passkey"), &BluetoothControl::endpoint_passkey, this);
        JSONRPC::Register<ConfirmpasskeyParamsData,void>(_T("confirmpasskey"), &BluetoothControl::endpoint_confirmpasskey, this);
        JSONRPC::Property<Core::JSON::ArrayType<Core::JSON::DecUInt16>>(_T("adapters"), &BluetoothControl::get_adapters, nullptr, this);
        JSONRPC::Property<AdapterData>(_T("adapter"), &BluetoothControl::get_adapter, nullptr, this);
        JSONRPC::Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("devices"), &BluetoothControl::get_devices, nullptr, this);
        JSONRPC::Property<JsonData::BluetoothControl::DeviceData>(_T("device"), &BluetoothControl::get_device, nullptr, this);
    }

    void BluetoothControl::UnregisterAll()
    {
        JSONRPC::Unregister(_T("confirmpasskey"));
        JSONRPC::Unregister(_T("passkey"));
        JSONRPC::Unregister(_T("pincode"));
        JSONRPC::Unregister(_T("abortpairing"));
        JSONRPC::Unregister(_T("unpair"));
        JSONRPC::Unregister(_T("pair"));
        JSONRPC::Unregister(_T("disconnect"));
        JSONRPC::Unregister(_T("connect"));
        JSONRPC::Unregister(_T("scan"));
        JSONRPC::Unregister(_T("device"));
        JSONRPC::Unregister(_T("devices"));
        JSONRPC::Unregister(_T("adapter"));
        JSONRPC::Unregister(_T("adapters"));
    }

    // API implementation
    //

    // Method: scan - Starts scanning for Bluetooth devices
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: Failed to scan
    //  - ERROR_INPROGRES: Scan already in progress
    uint32_t BluetoothControl::endpoint_scan(const ScanParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if (_application.IsScanning() == false) {
            const DevicetypeType& type = params.Type.Value();
            const uint32_t& timeout = params.Timeout.Value();

            if (type == DevicetypeType::LOWENERGY) {
                _application.Scan((timeout == 0? 10 : timeout), false, false);
            } else if (type == DevicetypeType::CLASSIC) {
                uint32_t type = 0x338B9E;
                uint8_t flags = 0;
                _application.Scan((timeout == 0? 10 : timeout), type, flags);
            } else {
                result = Core::ERROR_BAD_REQUEST;
            }
        } else {
            result = Core::ERROR_INPROGRESS;
        }

        return (result);
    }

    // Method: connect - Connects to a Bluetooth device
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown device
    //  - ERROR_ALREADY_CONNECTED: Device already connected
    //  - ERROR_GENERAL: Failed to connect the device
    uint32_t BluetoothControl::endpoint_connect(const ConnectParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& address = params.Address.Value();

        DeviceImpl* device = Find(Bluetooth::Address(address.c_str()));
        if (device != nullptr) {
            result = device->Connect();
        }

        return (result);
    }

    // Method: disconnect - Disconnects from a Bluetooth device
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown device
    //  - ERROR_ALREADY_RELEASED: Device not connected
    uint32_t BluetoothControl::endpoint_disconnect(const ConnectParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& address = params.Address.Value();

        DeviceImpl* device = Find(Bluetooth::Address(address.c_str()));
        if (device != nullptr) {
            result = device->Disconnect();
        }

        return (result);
    }

    // Method: pair - Pairs a Bluetooth device
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown device
    //  - ERROR_ALREADY_CONNECTED: Device already paired
    //  - ERROR_GENERAL: Failed to pair the device
    uint32_t BluetoothControl::endpoint_pair(const PairParamsData& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& address = params.Address.Value();
        const uint16_t& timeout = params.Timeout.Value();

        DeviceImpl* device = Find<DeviceImpl>(Bluetooth::Address(address.c_str()));
        if (device != nullptr) {
            result = device->Pair(IBluetooth::DISPLAY_YES_NO, (timeout == 0? 20 : timeout));
        }

        return (result);
    }

    // Method: unpair - Unpairs a Bluetooth device
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown device
    //  - ERROR_ALREADY_RELEASED: Device not paired
    uint32_t BluetoothControl::endpoint_unpair(const ConnectParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& address = params.Address.Value();

        DeviceImpl* device = Find<DeviceImpl>(Bluetooth::Address(address.c_str()));
        if (device != nullptr) {
            result = device->Unpair();
        }

        return (result);
    }

    // Method: abortpairing - Aborts the pairing process
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown device
    //  - ERROR_ILLEGAL_STATE: Device not currently pairing
    uint32_t BluetoothControl::endpoint_abortpairing(const ConnectParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& address = params.Address.Value();

        DeviceImpl* device = Find<DeviceImpl>(Bluetooth::Address(address.c_str()));
        if (device != nullptr) {
            result = device->AbortPairing();
        }

        return (result);
    }

    // Method: pincode - Specifies a PIN code for authentication during a legacy pairing process
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown device
    //  - ERROR_ILLEGAL_STATE: Device not currently pairing or PIN code has not been requested
    uint32_t BluetoothControl::endpoint_pincode(const PincodeParamsData& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& address = params.Address.Value();
        const string& secret = params.Secret.Value();

        DeviceRegular* device = Find<DeviceRegular>(Bluetooth::Address(address.c_str()));
        if (device != nullptr) {
            device->PINCode(secret);
            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Method: passkey - Specifies a passkey for authentication during a pairing process
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown device
    //  - ERROR_ILLEGAL_STATE: Device not currently pairing or a passkey has not been requested
    uint32_t BluetoothControl::endpoint_passkey(const PasskeyParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& address = params.Address.Value();
        const uint32_t& secret = params.Secret.Value();

        DeviceImpl* device = Find<DeviceImpl>(Bluetooth::Address(address.c_str()));
        if (device != nullptr) {
            device->Passkey(secret);
            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Method: passkeyconfirm - Confirms a passkey for authentication during a pairing process
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown device
    //  - ERROR_ILLEGAL_STATE: Device is currently not pairing or passkey confirmation has not been requested
    uint32_t BluetoothControl::endpoint_confirmpasskey(const ConfirmpasskeyParamsData& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& address = params.Address.Value();
        const bool& iscorrect = params.Iscorrect.Value();

        DeviceImpl* device = Find<DeviceImpl>(Bluetooth::Address(address.c_str()));
        if (device != nullptr) {
            device->ConfirmPasskey(iscorrect);
            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Property: adapters - List of local Bluetooth adapters
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t BluetoothControl::get_adapters(Core::JSON::ArrayType<Core::JSON::DecUInt16>& response) const
    {
        for (const uint16_t& adapter : _adapters) {
            Core::JSON::DecUInt16 adapter_id = adapter;
            response.Add(adapter_id);
        }

        return (Core::ERROR_NONE);
    }

    // Property: adapter - Local Bluetooth adapter information
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t BluetoothControl::get_adapter(const string& index, AdapterData& response) const
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;

        if (atoi(index.c_str()) == _btInterface) {
            Bluetooth::ManagementSocket::Info info(_application.Settings());
            response.Interface = ("hci" + Core::ToString(_btInterface));
            response.Address = info.Address().ToString();
            response.Version = info.Version();
            response.Manufacturer = info.Manufacturer();
            if (info.Name().empty() == false) {
                response.Name = info.Name();
            }
            if (info.ShortName().empty() == false) {
                response.Shortname = info.ShortName();
            }

            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Property: devices - List of known remote Bluetooth devices
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t BluetoothControl::get_devices(Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        for (DeviceImpl* device : _devices) {
            Core::JSON::String address;
            address = device->Address().ToString();
            response.Add(address);
        }

        return (Core::ERROR_NONE);
    }

    // Property: device - Remote Bluetooth device information
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown device
    uint32_t BluetoothControl::get_device(const string& index, JsonData::BluetoothControl::DeviceData& response) const
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;

        DeviceImpl* device = Find(Bluetooth::Address(index.c_str()));
        if (device != nullptr) {
            response.Type = (device->LowEnergy()? DevicetypeType::LOWENERGY : DevicetypeType::CLASSIC);
            if (device->Name().empty() == false) {
                response.Name = device->Name();
            }
            if (device->Class() != 0) {
                response.Class = device->Class();
            }
            response.Connected = device->IsConnected();
            response.Paired = device->IsBonded();

            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Event: scancomplete - Notifies about scan completion
    void BluetoothControl::event_scancomplete()
    {
        Notify(_T("scancomplete"));
    }

    // Event: devicestatechange - Notifies about device state change
    void BluetoothControl::event_devicestatechange(const string& address, const DevicestatechangeParamsData::DevicestateType& state,
                                                   const DevicestatechangeParamsData::DisconnectreasonType& disconnectreason)
    {
        DevicestatechangeParamsData params;
        params.Address = address;
        params.State = state;
        if (state == DevicestatechangeParamsData::DevicestateType::DISCONNECTED) {
            params.Disconnectreason = disconnectreason;
        }

        Notify(_T("devicestatechange"), params);
    }

    // Event: pincoderequest - Notifies about a PIN code request
    void BluetoothControl::event_pincoderequest(const string& address)
    {
        ConnectParamsInfo params;
        params.Address = address;

        Notify(_T("pincoderequest"), params);
    }

    // Event: passkeyrequest - Notifies about a passkey request
    void BluetoothControl::event_passkeyrequest(const string& address)
    {
        ConnectParamsInfo params;
        params.Address = address;

        Notify(_T("passkeyrequest"), params);
    }

    // Event: passkeyconfirmrequest - Notifies about a passkey confirmation request
    void BluetoothControl::event_passkeyconfirmrequest(const string& address, const uint32_t& secret)
    {
        PasskeyParamsInfo params;
        params.Address = address;
        params.Secret = secret;

        Notify(_T("passkeyconfirmrequest"), params);
    }

} // namespace Plugin

}

