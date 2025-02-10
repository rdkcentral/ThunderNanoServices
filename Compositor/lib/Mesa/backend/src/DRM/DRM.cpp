/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
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

#include "../Module.h"
#include "IGpu.h"
#include "Connector.h"

#include <interfaces/IComposition.h>

MODULE_NAME_ARCHIVE_DECLARATION

// clang-format on
// good reads
// https://docs.nvidia.com/jetson/l4t-multimedia/group__direct__rendering__manager.html
// http://github.com/dvdhrm/docs
// https://drmdb.emersion.fr
//

namespace Thunder {

namespace Compositor {

    Core::ProxyType<IOutput> CreateBuffer(const string& connectorName, const Exchange::IComposition::Rectangle& rectangle, const PixelFormat& format, IOutput::ICallback* feedback)
    {
        Core::ProxyType<IOutput> result;

        ASSERT(drmAvailable() == 1);
        ASSERT(connectorName.empty() == false);

        static Core::ProxyMapType<string, Connector> connectors;

        TRACE_GLOBAL(Trace::Backend, ("Requesting connector '%s'", connectorName.c_str()));

        Core::ProxyType<Connector> connector = connectors.Instance<Connector>(connectorName, connectorName, rectangle, format, feedback);

        if ( (connector.IsValid()) && (connector->IsValid()) ) {
            result = Core::ProxyType<IOutput>(connector);
        }

        return result;
    }

} // namespace Compositor
} // Thunder
