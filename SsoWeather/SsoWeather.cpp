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
 
#include "SsoWeather.h"

namespace WPEFramework {
namespace Plugin {
     namespace {

        static Metadata<SsoWeather> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }
    
    const string SsoWeather::Initialize(PluginHost::IShell* service)
    {
        printf("STD: SSOWeather Activated\n");
        fprintf(stderr,"STDERR: SSOWeather Activated\n");
        return (EMPTY_STRING);
    }

    /* virtual */ void SsoWeather::Deinitialize(PluginHost::IShell*)
    {
        printf("STD: SSOWeather Deactivated\n");
        fprintf(stderr,"STDERR: SSOWeather Deactivated\n");
    }
    
    /* virtual */ string SsoWeather::Information() const
    {
        // No additional info to report.
        printf("STD: SSOWeather Information\n");
        fprintf(stderr,"STDERR: SSOWeather Information\n");
        return (EMPTY_STRING);
    }

} // namespace Plugin
} // namespace WPEFramework