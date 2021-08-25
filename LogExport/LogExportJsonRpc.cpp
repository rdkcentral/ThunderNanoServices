/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological Management
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
#include "LogExport.h"
#include <interfaces/json/JsonData_LogExport.h>

namespace WPEFramework {

namespace Plugin {
    using namespace JsonData::LogExport;

    // Registration
    //

    void LogExport::RegisterAll()
    {
        Register<WPEFramework::JsonData::LogExport::WatchParamsData,void>(_T("watch"), &LogExport::endpoint_watch, this);
        Register<Core::JSON::String,void>(_T("unwatch"), &LogExport::endpoint_unwatch, this);
        Register<Core::JSON::String,void>(_T("dump"), &LogExport::endpoint_dump, this);
    }

    void LogExport::UnregisterAll()
    {
        Unregister(_T("watch"));
        Unregister(_T("unwatch"));
        Unregister(_T("dump"));
    }

    // API implementation
    //
    // Method: watch - Start watching a file
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t LogExport::endpoint_watch(const WPEFramework::JsonData::LogExport::WatchParamsData& params)
    {
        return _implementation->watch(params.Filename.Value(), params.Frombeginning.Value());
    }

    // Method: watch - Start watching a file
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t LogExport::endpoint_unwatch(const Core::JSON::String& filepath)
    {
        return _implementation->unwatch(filepath.Value());
    }

    // Method: watch - Start watching a file
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t LogExport::endpoint_dump(const Core::JSON::String& filepath)
    {
        return _implementation->dump(filepath.Value());
    }
} // namespace Plugin

}

