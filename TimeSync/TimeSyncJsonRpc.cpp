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

#include <interfaces/json/JsonData_TimeSync.h>
#include "TimeSync.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::TimeSync;

    // Registration
    //

    void TimeSync::RegisterAll()
    {
        Register<void,void>(_T("synchronize"), &TimeSync::endpoint_synchronize, this);
        Property<SynctimeData>(_T("synctime"), &TimeSync::get_synctime, nullptr, this);
        Property<Core::JSON::String>(_T("time"), &TimeSync::get_time, &TimeSync::set_time, this);
    }

    void TimeSync::UnregisterAll()
    {
        Unregister(_T("synchronize"));
        Unregister(_T("time"));
        Unregister(_T("synctime"));
    }

    // API implementation
    //

    // Method: synchronize - Synchronizes time
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INPROGRESS: Returned when the method is called while previously triggered synchronization is in progress.
    //  - ERROR_INCOMPLETE_CONFIG: Returned when the source configuration is missing or invalid.
    uint32_t TimeSync::endpoint_synchronize()
    {
       uint32_t result = _client->Synchronize();

        if ((result != Core::ERROR_NONE) && (result != Core::ERROR_INPROGRESS) && (result != Core::ERROR_INCOMPLETE_CONFIG)) {
            result = Core::ERROR_INCOMPLETE_CONFIG;
        }

        return result;
    }

    // Property: synctime - Most recent synchronized time
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t TimeSync::get_synctime(SynctimeData& response) const
    {
        response.Time = Core::Time(_client->SyncTime()).ToISO8601(/* local */ true);

        if (response.Time.Value().length() != 0) {
            response.Source = _client->Source();
        }

        return Core::ERROR_NONE;
    }

    // Property: time - Current system time
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t TimeSync::get_time(Core::JSON::String& response) const
    {
        response = Core::Time::Now().ToISO8601();
        return Core::ERROR_NONE;
    }

    // Property: time - Current system time
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_BAD_REQUEST: The time is invalid
    uint32_t TimeSync::set_time(const Core::JSON::String& param)
    {
        uint32_t result = Core::ERROR_NONE;
        Core::Time newTime(0);

        if (param.IsSet() && !param.Value().empty()) {
            const string& time = param.Value();
            newTime.FromISO8601(time);

            if (!newTime.IsValid()) {
                result = Core::ERROR_BAD_REQUEST;
            }
        }

        if (result == Core::ERROR_NONE) {
            // Stop automatic synchronisation
            _client->Cancel();
            Core::IWorkerPool::Instance().Revoke(_activity);

            if (newTime.IsValid()) {
                Core::SystemInfo::Instance().SetTime(newTime);
            }

            EnsureSubsystemIsActive();
        }

        return result;
    }

    // Event: timechange - Signals a time change
    void TimeSync::event_timechange()
    {
        Notify(_T("timechange"));
    }

} // namespace Plugin

}

