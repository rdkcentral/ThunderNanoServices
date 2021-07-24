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

#include "SimpleCOMRPCPluginServer.h"

#ifdef __CORE_EXCEPTION_CATCHING__
#include <stdexcept>
#endif

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(SimpleCOMRPCPluginServer, 1, 0);

    //IPlugin
    const string SimpleCOMRPCPluginServer::Initialize(PluginHost::IShell* service)  /* override */
    {
        ASSERT(service != nullptr);
        service->Register(&_comNotificationSink);
        return (EMPTY_STRING);
    }

    void SimpleCOMRPCPluginServer::Deinitialize(PluginHost::IShell* service)  /* override */
    {
        ASSERT(service != nullptr);
        service->Unregister(&_comNotificationSink);
    }

    string SimpleCOMRPCPluginServer::Information() const /* override */
    {
        return (EMPTY_STRING);
    }
    //Private methods

    //IWallClock
    uint32_t SimpleCOMRPCPluginServer::Arm(const uint16_t seconds, ICallback* callback)
    {   
        uint32_t result = _notifier.Arm(seconds, callback);
        TRACE(Trace::Information, (_T("Armed a callback, it fires in %d seconds. Result: %d"), seconds, result));
        return (result);
    }
    uint32_t SimpleCOMRPCPluginServer::Disarm(const Exchange::IWallClock::ICallback* callback)
    {
        uint32_t result = _notifier.Disarm(callback);
        TRACE(Trace::Information, (_T("Disarmed a callback, it will not fire anymore. Result: %d"), result));
        return (result);
    }
    uint64_t SimpleCOMRPCPluginServer::Now() const
    {
        Core::Time result = Core::Time::Now();
        TRACE(Trace::Information, (_T("The time has been requested, we return a clock time of: %s"), result.ToRFC1123().c_str()));
        return result.Ticks();
    }

    void SimpleCOMRPCPluginServer::OnRevoke(const Exchange::IWallClock::ICallback* remote) {
        // Looks like we need to stop the callback from being called, it is a dead object now anyway :-)
        _notifier.Disarm(remote);
    }

}
} // namespace Plugin
