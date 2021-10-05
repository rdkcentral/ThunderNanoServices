/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "Module.h"
#include <interfaces/json/JsonData_SecureShellServer.h>
#include "SecureShellServer.h"

namespace WPEFramework {
namespace Plugin {

    using namespace JsonData::SecureShellServer;

    // Registration
    void SecureShellServer::RegisterAll()
    {
        Register<void, Core::JSON::DecUInt32>(_T("activesessionscount"), &SecureShellServer::endpoint_activesessionscount, this);
        Register<void, Core::JSON::ArrayType<SessioninfoData>>(_T("activesessionsinfo"), &SecureShellServer::endpoint_activesessionsinfo, this);
        Register<JsonData::SecureShellServer::CloseclientcessionParamsData,void>(_T("closeclientsession"), &SecureShellServer::endpoint_closeclientsession, this);
    }

    void SecureShellServer::UnregisterAll()
    {
        Unregister(_T("activesessionsinfo"));
        Unregister(_T("activesessionscount"));
        Unregister(_T("closeclientsession"));
    }


    // Method: Get number of active ssh sessions
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: failure in getting the number of active sessions
   uint32_t SecureShellServer::endpoint_activesessionscount(Core::JSON::DecUInt32& response)
    {
        ASSERT(_secureShellServer != nullptr);
        uint32_t result = Core::ERROR_NONE;

        if(_secureShellServer){
            response = _secureShellServer->GetActiveSessionsCount();

        } else {
            result = Core::ERROR_GENERAL;
        }
        return result;
    }

    // Method: Get details of active ssh sessions
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: failure in getting the information
    uint32_t SecureShellServer::endpoint_activesessionsinfo(Core::JSON::ArrayType<SessioninfoData>& response)
    {
        ASSERT(_secureShellServer != nullptr);
        uint32_t result = Core::ERROR_NONE;

        std::list<SessioninfoData> allInfos;

        result = GetActiveSessionsInfo(allInfos);
        if (result == Core::ERROR_NONE) {
            for (auto& session : allInfos){
                response.Add(session);
            }
        }
        
        return result;
    }

    // Method: Close an active ssh Session
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: failure in closing the requested session 
    uint32_t SecureShellServer::endpoint_closeclientsession(const JsonData::SecureShellServer::CloseclientcessionParamsData& params)
    {
        ASSERT(_secureShellServer != nullptr);
        uint32_t result = Core::ERROR_NONE;

        if(_secureShellServer!=nullptr && params.Pid.IsSet() == true) {
            TRACE(Trace::Information, (_T("closing client session with pid: %u"), params.Pid.Value()));
            result = _secureShellServer->CloseClientSession(params.Pid.Value());
        } else {
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

} // namespace Plugin
} // namespace WPEFramework

