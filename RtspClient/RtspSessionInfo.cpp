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

#include "RtspSessionInfo.h"

namespace WPEFramework {
namespace Plugin {

    RtspSessionInfo::RtspSessionInfo()
        : bSrmIsRtspProxy(true)
        , sessionTimeout(-1)
        , ctrlSessionTimeout(-1)
        , duration(0)
        , bookmark(0)
        , npt(0)
        , scale(0)
    {
        defaultSessionTimeout = 0;
        defaultCtrlSessionTimeout = 0;
    }

    void RtspSessionInfo::reset()
    {
        sessionId = std::string();
        ctrlSessionId = std::string();
        npt = 0;
        duration = 0;
        bookmark = 0;
        scale = 0;
    }
}
} // WPEFramework::Plugin
