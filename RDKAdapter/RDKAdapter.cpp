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
 
#include "RDKAdapter.h"

namespace WPEFramework {
namespace Plugin {

    namespace {

        static Metadata<RDKAdapter> metadata(
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

    const string RDKAdapter::Initialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        // On success return empty, to indicate there is no error text.
        return (EMPTY_STRING);
    }

    void RDKAdapter::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
    }

    string RDKAdapter::Information() const
    {
        return (EMPTY_STRING);
    }

    uint32_t RDKAdapter::Test() const 
    {
        TRACE(Trace::Information, (_T("RDKAdapter test called") ));
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
