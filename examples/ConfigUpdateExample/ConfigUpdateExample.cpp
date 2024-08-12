/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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
 
#include "ConfigUpdateExample.h"

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<ConfigUpdateExample> metadata(
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

    /* virtual */ const string ConfigUpdateExample::Initialize(PluginHost::IShell* service)
    {
        string message;
        ASSERT(_service == nullptr);

        // Setup skip URL for right offset.
        _service = service;

        Config config;
        config.FromString(service->ConfigLine());
        
        if (config.Plugins.IsSet() == true) {
            Core::JSON::ArrayType<Config::Plugin>::Iterator index(config.Plugins.Elements());
            while (index.Next() == true) {
                const string& callsign(index.Current().Callsign.Value());
                PluginHost::IShell* shell = service->QueryInterfaceByCallsign<PluginHost::IShell>(callsign);
                if (shell != nullptr) {
                    TRACE(Trace::Information, (_T("Current SystemRootPath[%s] = %s"), callsign.c_str(), shell->SystemRootPath().c_str()));
                    if (index.Current().SystemRootPath.IsSet() == true) {
                        shell->SystemRootPath(index.Current().SystemRootPath.Value());
                        TRACE(Trace::Information, (_T("New SystemRootPath[%s] = %s"), callsign.c_str(), shell->SystemRootPath().c_str()));
                    }
                }
                Exchange::Controller::IConfiguration* controller = service->QueryInterfaceByCallsign<Exchange::Controller::IConfiguration>(EMPTY_STRING);
                if (controller != nullptr) {
                    controller->Persist();
                    controller->Release();
                }
            }
        }
        return message;
    }

    /* virtual */ void ConfigUpdateExample::Deinitialize(VARIABLE_IS_NOT_USED PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        _service = nullptr;
    }

    /* virtual */ string ConfigUpdateExample::Information() const
    {
        // No additional info to report.
        return (string());
    }

} //namespace Plugin
} // namespace Thunder
