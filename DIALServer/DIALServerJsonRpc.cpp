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
#include "DIALServer.h"
#include <interfaces/json/JsonData_DIALServer.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::DIALServer;

    // API implementation
    //

    // Event: hide - Signals that application hide was requested over DIAL *(passive mode only)*
    void DIALServer::event_hide(const string& application)
    {
        HideParamsData params;
        params.Application = application;

        Notify(_T("hide"), params);
    }

    // Event: show - Signals that application show was requested over DIAL *(passive mode only)*
    void DIALServer::event_show(const string& application)
    {
        HideParamsData params;
        params.Application = application;

        Notify(_T("show"), params);
    }

    // Event: start - Signals that application start was requested over DIAL *(passive mode only)*
    void DIALServer::event_start(const string& application, const string& parameters, const string& payload)
    {
        StartParamsData params;
        params.Application = application;
        params.Parameters = parameters;
        params.Payload = payload;

        Notify(_T("start"), params);
    }

    // Event: change - Signals that application change was requested over DIAL *(passive mode only)*
    void DIALServer::event_change(const string& application, const string& parameters, const string& payload)
    {
        StartParamsData params;
        params.Application = application;
        params.Parameters = parameters;
        params.Payload = payload;

        Notify(_T("change"), params);
    }

    // Event: stop - Signals that application stop was requested over DIAL *(passive mode only)*
    void DIALServer::event_stop(const string& application, const string& parameters)
    {
        StopParamsData params;
        params.Application = application;
        params.Parameters = parameters;

        Notify(_T("stop"), params);
    }

} // namespace Plugin

}

