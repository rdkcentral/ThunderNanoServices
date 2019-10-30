
#include "Module.h"
#include "BluetoothRemoteControl.h"
#include <interfaces/json/JsonData_BluetoothRemoteControl.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::BluetoothRemoteControl;

    // Registration
    //

    void BluetoothRemoteControl::RegisterAll()
    {
        Register<AssignParamsData,void>(_T("assign"), &BluetoothRemoteControl::endpoint_assign, this);
        Register<void,void>(_T("revoke"), &BluetoothRemoteControl::endpoint_revoke, this);
        Property<Core::JSON::String>(_T("name"), &BluetoothRemoteControl::get_name, nullptr, this);
        Property<Core::JSON::String>(_T("address"), &BluetoothRemoteControl::get_address, nullptr, this);
        Property<InfoData>(_T("info"), &BluetoothRemoteControl::get_info, nullptr, this);
        Property<Core::JSON::DecUInt8>(_T("batterylevel"), &BluetoothRemoteControl::get_batterylevel, nullptr, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("audioprofiles"), &BluetoothRemoteControl::get_audioprofiles, nullptr, this);
        Property<AudioprofileData>(_T("audioprofile"), &BluetoothRemoteControl::get_audioprofile, nullptr, this);
    }

    void BluetoothRemoteControl::UnregisterAll()
    {
        Unregister(_T("revoke"));
        Unregister(_T("assign"));
        Unregister(_T("audioprofile"));
        Unregister(_T("audioprofiles"));
        Unregister(_T("batterylevel"));
        Unregister(_T("info"));
        Unregister(_T("address"));
        Unregister(_T("name"));
    }

    // API implementation
    //

    // Method: assign - Assigns a bluetooth device as a remote control unit
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Bluetooth unavailable
    //  - ERROR_UNKNOWN_KEY: Device unknown
    //  - ERROR_GENERAL: Failed to assign the device
    uint32_t BluetoothRemoteControl::endpoint_assign(const AssignParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& device = params.Device.Value();

        result = Assign(device);

        return result;
    }

    // Method: revoke - Revokes the current remote control assignment
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: Remote not assigned
    uint32_t BluetoothRemoteControl::endpoint_revoke()
    {
        uint32_t result = Core::ERROR_NONE;

        result = Revoke();

        return result;
    }

    // Property: name - Unit name
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: No remote has been assigned
    uint32_t BluetoothRemoteControl::get_name(Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if (_gattRemote != nullptr) {
            response = _gattRemote->Name();
            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Property: address - Bluetooth address
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: No remote has been assigned
    uint32_t BluetoothRemoteControl::get_address(Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if (_gattRemote != nullptr) {
            response = _gattRemote->RemoteId();
            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Property: info - Unit auxiliary information
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: No remote has been assigned
    //  - ERROR_GENERAL: Failed to retrieve information
    uint32_t BluetoothRemoteControl::get_info(InfoData& response) const
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if (_gattRemote != nullptr) {
            if (_gattRemote->Model().length() > 0) {
                response.Model = _gattRemote->Model();
            }
            if (_gattRemote->Serial().length() > 0) {
                response.Serial = _gattRemote->Serial();
            }
            if (_gattRemote->Firmware().length() > 0) {
                response.Firmware = _gattRemote->Firmware();
            }
            if (_gattRemote->Software().length() > 0) {
                response.Software = _gattRemote->Software();
            }
            if (_gattRemote->Manufacturer().length() > 0) {
                response.Manufacturer= _gattRemote->Manufacturer();
            }

            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Property: batterylevel - Battery level
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: No remote has been assigned
    //  - ERROR_GENERAL: Failed to retrieve battery level
    uint32_t BluetoothRemoteControl::get_batterylevel(Core::JSON::DecUInt8& response) const
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if (_gattRemote != nullptr) {
            response = _gattRemote->BatteryLevel();
            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Property: audioprofiles - Supported audio profiles
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: No remote has been assigned
    //  - ERROR_UNAVAILABLE: The unit does not support voice functionality
    uint32_t BluetoothRemoteControl::get_audioprofiles(Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if (_gattRemote != nullptr) {
        }

        return result;
    }

    // Property: audioprofile - Audio profile details
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: No remote has been assigned
    //  - ERROR_UNKNOWN_KEY: The supplied audio profile is unknown
    uint32_t BluetoothRemoteControl::get_audioprofile(const string& index, AudioprofileData& response) const
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if (_gattRemote != nullptr) {
        }

        return result;
    }

    // Event: audiotransmission - Notifies about new audio data transmission
    void BluetoothRemoteControl::event_audiotransmission(const string& profile)
    {
        AudiotransmissionParamsData params;
        if (profile.length() > 0) {
            params.Profile = profile;
        }

        Notify(_T("audiotransmission"), params);
    }

    // Event: audioframe - Notifies about new audio data available
    void BluetoothRemoteControl::event_audioframe(const uint32_t& seq, const string& data)
    {
        AudioframeParamsData params;
        params.Seq = seq;
        params.Data = data;

        Notify(_T("audioframe"), params);
    }

} // namespace Plugin

}

