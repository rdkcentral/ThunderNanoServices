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
#include "InputSwitch.h"
#include <interfaces/json/JsonData_InputSwitch.h>

/*
    // Copy the code below to InputSwitch class definition
    // Note: The InputSwitch class must inherit from PluginHost::JSONRPC

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_channel(const JsonData::InputSwitch::ChannelParamsInfo& params);
        uint32_t endpoint_status(const JsonData::InputSwitch::SelectParamsData& params, Core::JSON::ArrayType<JsonData::InputSwitch::ChannelParamsInfo>& response);
*/

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::InputSwitch;

    // Registration
    //

    void InputSwitch::RegisterAll()
    {
        Register<ChannelParamsInfo,void>(_T("channel"), &InputSwitch::endpoint_channel, this);
        Register<SelectParamsInfo,void>(_T("select"), &InputSwitch::endpoint_select, this);
        Register<SelectParamsInfo,Core::JSON::ArrayType<ChannelParamsInfo>>(_T("status"), &InputSwitch::endpoint_status, this);
    }

    void InputSwitch::UnregisterAll()
    {
        Unregister(_T("status"));
        Unregister(_T("select"));
        Unregister(_T("channel"));
    }

    // API implementation
    //

    // Method: channel - Enable or Disable the throughput throough the given channel
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Failed to scan
    uint32_t InputSwitch::endpoint_channel(const ChannelParamsInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;
        Exchange::IInputSwitch::mode enabled = params.Enabled.Value() ? 
                                               Exchange::IInputSwitch::ENABLED : 
                                               Exchange::IInputSwitch::DISABLED;

        if (params.Name.IsSet() == true) {
            if (ChannelExists(params.Name.Value()) == false) {
                result = Core::ERROR_UNKNOWN_KEY;
            }
            else {
                Consumer(params.Name.Value(), enabled);
            }
        }
        else {
            PluginHost::VirtualInput::Iterator index (_handler->Consumers());
            while (index.Next() == true) {
                Consumer(index.Name(), enabled);
            }
        }

        return result;
    }

    // Method: status - Check the status of the requested channel
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Could not find the designated channel
    uint32_t InputSwitch::endpoint_status(const SelectParamsInfo& params, Core::JSON::ArrayType<ChannelParamsInfo>& response)
    {
        uint32_t result = Core::ERROR_NONE;

        if (params.Name.IsSet() == true) {
            if (ChannelExists(params.Name.Value()) == false) {
                result = Core::ERROR_UNKNOWN_KEY;
            }
            else {
                ChannelParamsInfo& element(response.Add());

                element.Name = params.Name.Value();
                element.Enabled = Consumer(params.Name.Value());
            }
        }
        else {
            PluginHost::VirtualInput::Iterator index (_handler->Consumers());
            // Insert all channels with there status..
            while (index.Next() == true) {
                ChannelParamsInfo& element(response.Add());

                element.Name = index.Name();
                element.Enabled = Consumer(index.Name());
            }
        }

        return result;
    }

    // Method: status - Check the status of the requested channel
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Could not find the designated channel
    uint32_t InputSwitch::endpoint_select(const SelectParamsInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if (params.Name.IsSet() == true) {
            if (Select(params.Name.Value()) != Core::ERROR_NONE) {
                result = Core::ERROR_UNKNOWN_KEY;
            }
        }
        return (result);
    }

} // namespace Plugin

}

