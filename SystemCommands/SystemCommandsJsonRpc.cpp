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
 
// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#include <interfaces/json/JsonData_SystemCommands.h>
#include "SystemCommands.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::SystemCommands;

    // Registration
    //

    void SystemCommands::RegisterAll()
    {
        Register<UsbresetParamsData,void>(_T("usbreset"), &SystemCommands::endpoint_usbreset, this);
    }

    void SystemCommands::UnregisterAll()
    {
        Unregister(_T("usbreset"));
    }

    // API implementation
    //

    // Resets a USB device.
    uint32_t SystemCommands::endpoint_usbreset(const UsbresetParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if(params.Device.IsSet() == true) {
            result = USBReset(params.Device.Value());
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        return result;
    }

} // namespace Plugin

} // namespace WPEFramework

