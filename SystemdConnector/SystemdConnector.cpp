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

#include "SystemdConnector.h"

#include <systemd/sd-daemon.h>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(SystemdConnector, 1, 0);

    SystemdConnector::SystemdConnector()
    {
    }

    /* virtual */ SystemdConnector::~SystemdConnector()
    {
    }

    /* virtual */ const string SystemdConnector::Initialize(PluginHost::IShell* /* service */)
    {
        TRACE(Trace::Information, (_T("Notify systemd that the Platform is up and running.\n")));

        int rc = sd_notifyf(0,
            "READY=1\n"
            "STATUS=Platform Server is Ready (from WPE Framework Compositor Plugin)\n"
            "MAINPID=%lu",
            ::getpid());
        if (rc) {
            TRACE(Trace::Error, (_T("Notify Nexus Server Ready to systemd: FAILED (%d)\n"), rc));
        } else {
            TRACE(Trace::Information, (_T("Notify Nexus Server Ready to systemd: OK\n")));
        }
        // On success return empty, to indicate there is no error text.
        return (string());
    }

    /* virtual */ void SystemdConnector::Deinitialize(PluginHost::IShell* /* servicei */)
    {
        //Strangely enough systemd has no notification for shutting down, so do nothing.
    }

    /* virtual */ string SystemdConnector::Information() const
    {
        // No additional info to report.
        return (string());
    }

} // namespace Plugin
} // namespace WPEFramework
