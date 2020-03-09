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
 
#include "ProcessMonitor.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(ProcessMonitor, 1, 0);

const string ProcessMonitor::Initialize(PluginHost::IShell* service)
{
    Config config;
    config.FromString(service->ConfigLine());

    _notification.Open(service, config.ExitTimeout.Value());

    return (_T(""));
}

void ProcessMonitor::Deinitialize(PluginHost::IShell* service)
{
    _notification.Close();
}

string ProcessMonitor::Information() const
{
    string emptyString;
    return emptyString;
}
}
}
