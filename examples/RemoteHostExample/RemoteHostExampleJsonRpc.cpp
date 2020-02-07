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
#include "RemoteHostExample.h"
#include <interfaces/json/JsonData_RemoteHostExample.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::RemoteHostExample;

    // Registration
    //

    void RemoteHostExample::RegisterAll()
    {
        Register<GreetParamsData,GreetResultData>(_T("greet"), &RemoteHostExample::endpoint_greet, this);
    }

    void RemoteHostExample::UnregisterAll()
    {
        Unregister(_T("greet"));
    }

    // API implementation
    //

    // Method: greet - Sends greetings to other device
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_TIMEDOUT: Could not reach out the other side 
    uint32_t RemoteHostExample::endpoint_greet(const GreetParamsData& params, GreetResultData& response)
    {
        const string& message = params.Message.Value();

        string resp;
        uint32_t result = _implementation->Greet(message, resp);
        response.Response = resp;

        if (result == Core::ERROR_TIMEDOUT || result == Core::ERROR_CONNECTION_CLOSED) {
            // connection to remote is closed. Closing plugin...
            printf("Exiting plugin...\n");
            _shell->Deactivate(PluginHost::IShell::reason::AUTOMATIC);
            printf("Exiting plugin2...\n");         
        }

        return result;
    }

} // namespace Plugin

}

