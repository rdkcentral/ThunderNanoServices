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

#include "CECControl.h"

#include <CECIAccessor.h>

namespace Thunder {

namespace Plugin {

    namespace {

        static Metadata<CECControl> metadata(
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

    const string CECControl::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        _config.FromString(service->ConfigLine());

        auto index(_config.Interfaces.Elements());

        while (index.Next() == true) {
            Config::Interface iface;
            iface.FromString(index.Current().Value());

            ASSERT(iface.Name.Value().empty() == false);
            ASSERT(iface.Config.Value().empty() == false);

            Thunder::CEC::IAccessor::Instance()->Announce(iface.Name.Value(), iface.Config.Value());
        }

        return string();
    }

    void CECControl::Deinitialize(PluginHost::IShell*)
    {
        auto index(_config.Interfaces.Elements());

        while (index.Next() == true) {
            Config::Interface iface;
            iface.FromString(index.Current().Value());

            ASSERT(iface.Name.Value().empty() == false);
            ASSERT(iface.Config.Value().empty() == false);

            Thunder::CEC::IAccessor::Instance()->Revoke(iface.Name.Value());
        }
    }

    string CECControl::Information() const
    {
        return string(); // No additional info to report.
    }

} // namespace Plugin
} // namespace Thunder
