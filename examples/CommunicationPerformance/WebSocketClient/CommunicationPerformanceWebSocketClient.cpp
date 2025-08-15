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

#include "CommunicationPerformanceWebSocketClient.h"

namespace Thunder {
namespace Plugin {

// TODO: Mandatory, why? What does it do?
namespace {

static Metadata<CommunicationPerformanceWebSocketClient> metadata (
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

std::string CommunicationPerformanceWebSocketClient::Classname() const
{
    // This name is appended with 'Implementation' at instantiation of the out-ouf-process part
    return "SimplePluginWebSocketClient";
}

} } // namespace Thunder::Plugin
