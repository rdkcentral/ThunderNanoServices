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

#include "Module.h"
#include "DIALServer.h"
#include <interfaces/json/JsonData_DIALServer.h>

namespace Thunder {

namespace Plugin {

    using namespace JsonData::DIALServer;

    // Registration
    //

    void DIALServer::RegisterAll()
    {
        Property<Core::JSON::EnumType<StateType>>(_T("state"), &DIALServer::get_state, &DIALServer::set_state, this);
    }

    void DIALServer::UnregisterAll()
    {
        Unregister(_T("state"));
    }

    // API implementation
    //

    // Property: state - Current application running state
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Specified application does not exist
    uint32_t DIALServer::get_state(const string& index, Core::JSON::EnumType<StateType>& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto entry(_appInfo.find(index));
        if (entry == _appInfo.end()) {
            result = Core::ERROR_UNKNOWN_KEY;
        } else {
            const AppInformation& app = entry->second;
            if (app.IsRunning() == true) {
                if ((app.HasHide() == true) && (app.IsHidden() == true)) {
                    response = StateType::HIDDEN;
                } else {
                    response = StateType::STARTED;
                }
            } else {
                response = StateType::STOPPED;
            }
        }

        return (result);
    }

    // Property: state - Current application running state
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Specified application does not exist
    //  - ERROR_ILLEGAL_STATE: Specified application is running in *active mode*
    uint32_t DIALServer::set_state(const string& index, const Core::JSON::EnumType<StateType>& param)
    {
        uint32_t result = Core::ERROR_NONE;

        auto entry(_appInfo.find(index));
        if (entry == _appInfo.end()) {
            result = Core::ERROR_UNKNOWN_KEY;
        } else {
            AppInformation& app = entry->second;
            switch (param) {
                default:
                    result = Core::ERROR_GENERAL;
                    break;
                case StateType::STOPPED:
                    app.Running(false);
                    app.Hidden(false);
                    break;
                case StateType::STARTED:
                    app.Running(true);
                    app.Hidden(false);
                    break;
                case StateType::HIDDEN:
                    app.Running(true);
                    app.Hidden(true);
                    break;
            }
        }

        return (result);
    }

    // Event: hide - Signals that application hide was requested over DIAL
    void DIALServer::event_hide(const string& application)
    {
        HideParamsInfo params;
        params.Application = application;

        Notify(_T("hide"), params);
    }

    // Event: show - Signals that application show was requested over DIAL (DEPRECATED)
    void DIALServer::event_show(const string& application)
    {
        HideParamsInfo params;
        params.Application = application;

        Notify(_T("show"), params);
    }

    // Event: start - Signals that application launch (or show if previously hidden) was requested over DIAL
    void DIALServer::event_start(const string& application, const string& parameters, const string& payload)
    {
        StartParamsData params;
        params.Application = application;
        if (parameters.empty() == false) {
            params.Parameters = parameters;
        }
        if (payload.empty() == false) {
            params.Payload = payload;
        }

        Notify(_T("start"), params);
    }

    // Event: change - Signals that application URL change was requested over DIAL (DEPRECATED)
    void DIALServer::event_change(const string& application, const string& parameters, const string& payload)
    {
        StartParamsData params;
        params.Application = application;
        if (parameters.empty() == false) {
            params.Parameters = parameters;
        }
        if (payload.empty() == false) {
            params.Payload = payload;
        }

        Notify(_T("change"), params);
    }

    // Event: stop - Signals that application stop was requested over DIAL
    void DIALServer::event_stop(const string& application, const string& parameters)
    {
        StopParamsData params;
        params.Application = application;
        params.Parameters = parameters;

        Notify(_T("stop"), params);
    }

} // namespace Plugin

}

