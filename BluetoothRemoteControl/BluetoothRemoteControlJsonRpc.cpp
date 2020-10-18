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
        Property<Core::JSON::Boolean>(_T("voice"), &BluetoothRemoteControl::get_voicecontrol, &BluetoothRemoteControl::set_voicecontrol, this);
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
        Unregister(_T("voice"));
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
        const string& address = params.Address.Value();
        return (Assign(address));
    }

    // Method: revoke - Revokes the current remote control assignment
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: Remote not assigned
    uint32_t BluetoothRemoteControl::endpoint_revoke()
    {
        return (Revoke());
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
            response = _gattRemote->Address();
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
            if (_gattRemote->ModelNumber().empty() == false) {
                response.Model = _gattRemote->ModelNumber();
            }
            if (_gattRemote->SerialNumber().empty() == false) {
                response.Serial = _gattRemote->SerialNumber();
            }
            if (_gattRemote->FirmwareRevision().empty() == false) {
                response.Firmware = _gattRemote->FirmwareRevision();
            }
            if (_gattRemote->SoftwareRevision().empty() == false) {
                response.Software = _gattRemote->SoftwareRevision();
            }
            if (_gattRemote->ManufacturerName().empty() == false) {
                response.Manufacturer = _gattRemote->ManufacturerName();
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
            response = BatteryLevel();
            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Property: voice - Get the status of the Voice flow
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: Failed to retrieve battery level
    uint32_t BluetoothRemoteControl::get_voicecontrol(Core::JSON::Boolean& response) const
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if (_gattRemote != nullptr) {
            response = _gattRemote->VoiceOutput();
            result = Core::ERROR_NONE;
        }

        return (result);
    }

    // Property: voice - Get the status of the Voice flow
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: Failed to retrieve battery level
    uint32_t BluetoothRemoteControl::set_voicecontrol(const Core::JSON::Boolean& response)
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if (_gattRemote != nullptr) {
            result = Core::ERROR_GENERAL;
            if (_gattRemote->VoiceOutput(response.Value()) == Core::ERROR_NONE) {
                result = Core::ERROR_NONE;
            };
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
            result = Core::ERROR_NONE;
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
        if(_gattRemote != nullptr) {
            result = Core::ERROR_UNKNOWN_KEY;
            Exchange::IVoiceProducer::IProfile* profile = _gattRemote->SelectedProfile();

            if (profile != nullptr) {
                profile->Release();
            }
        }

        return (result);
    }

    // Event: audiotransmission - Notifies about new audio data transmission
    void BluetoothRemoteControl::event_audiotransmission(const string& profile)
    {
        AudiotransmissionParamsData params;
        if (profile.empty() == false) {
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

    // Event: batterylevelchange - Notifies about battery level changes
    void BluetoothRemoteControl::event_batterylevelchange(const uint8_t& level)
    {
        BatterylevelchangeParamsData params;
        params.Level = level;

        Notify(_T("batterylevelchange"), params);
    }
} // namespace Plugin

}

