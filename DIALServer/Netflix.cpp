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

#include "DIALServer.h"

#include "interfaces/ISwitchBoard.h"

namespace WPEFramework {
namespace DIALHandlers {

    class Netflix : public Plugin::DIALServer::Default {
    private:
        Netflix() = delete;
        Netflix(const Netflix&) = delete;
        Netflix& operator=(const Netflix&) = delete;

    public:
        Netflix(PluginHost::IShell* service, const Plugin::DIALServer::Config::App& config, Plugin::DIALServer *parent)
            : Default(service, config, parent)
        {
        }
        virtual ~Netflix()
        {
        }

    public:
        uint32_t Start(const string& params) override {
            const string query = params + _T("&source_type=12"); // i.e. launched by DIAL
            Core::SystemInfo::SetEnvironment(_T("ONE_TIME_QUERY_STRING_OVERRIDE"), query.c_str());
            return Default::Start(params);
        }

    };

    static Plugin::DIALServer::ApplicationRegistrationType<Netflix> _netflixHandler;
}
}
