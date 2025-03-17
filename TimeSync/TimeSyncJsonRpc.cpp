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
#include <interfaces/json/JsonData_TimeSync.h>
#include "TimeSync.h"

namespace Thunder {

namespace Plugin {

    using namespace JsonData::TimeSync;

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

    // Event: timechange - Signals a time change
    void TimeSync::event_timechange()
    {
        Notify(_T("timechange"));
    }

} // namespace Plugin

}

