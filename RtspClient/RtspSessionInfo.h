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

#ifndef RTSPSESSIONINFO_H
#define RTSPSESSIONINFO_H

#include <string>

namespace WPEFramework {
namespace Plugin {

    struct ServerInfo {
        std::string name;
        std::string address;
        uint16_t port;
    };

    class RtspSessionInfo {
    public:
        RtspSessionInfo();
        void reset();
        ServerInfo srm;
        ServerInfo pump;

        std::string sessionId, ctrlSessionId;
        bool bSrmIsRtspProxy;
        int sessionTimeout, ctrlSessionTimeout;
        int defaultSessionTimeout, defaultCtrlSessionTimeout;
        unsigned int frequency;
        unsigned int programNum;
        unsigned int modulation;
        unsigned int symbolRate;
        int duration;
        float bookmark;
        float npt;
        float scale;
    };
}
} // WPEFramework::Plugin

#endif
