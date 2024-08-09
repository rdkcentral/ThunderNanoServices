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

namespace Thunder {

namespace Plugin {

    namespace {

        static Metadata<SimpleCOMRPCPluginServer> metadata(
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

    // IPlugin
    const string SimpleCOMRPCPluginServer::Initialize(PluginHost::IShell* service)  /* override */
    {
        ASSERT(service != nullptr);
        service->Register(&_comNotificationSink);
        _wallclock = Core::ServiceType<WallClock>::Create<WallClock>(*this);
        return (EMPTY_STRING);
    }

    void SimpleCOMRPCPluginServer::Deinitialize(PluginHost::IShell* service)  /* override */
    {
        ASSERT(service != nullptr);
        _wallclock->Decouple();
        _wallclock->Release();
        _notifier.Clear();
        service->Unregister(&_comNotificationSink);
    }

    string SimpleCOMRPCPluginServer::Information() const /* override */
    {
        return (EMPTY_STRING);
    }
    //Private methods

    //IWallClock
    uint32_t SimpleCOMRPCPluginServer::WallClock::Arm(const uint16_t seconds, ICallback* callback)
    {   
        uint32_t result = Core::ERROR_BAD_REQUEST;

        _adminLock.Lock();

        if (_parent != nullptr) {
            result = _parent->_notifier.Arm(seconds, callback);
            TRACE(Trace::Information, (_T("Armed a callback, it fires in %d seconds. Result: %d"), seconds, result));
        }
        _adminLock.Unlock();

        return (result);
    }
    uint32_t SimpleCOMRPCPluginServer::WallClock::Disarm(const Exchange::IWallClock::ICallback* callback)
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;

        _adminLock.Lock();

        if (_parent != nullptr) {
            result = _parent->_notifier.Disarm(callback);
            TRACE(Trace::Information, (_T("Disarmed a callback, it will not fire anymore. Result: %d"), result));
        }

        _adminLock.Unlock();

        return (result);
    }
    uint64_t SimpleCOMRPCPluginServer::WallClock::Now() const
    {
        Core::Time result = Core::Time::Now();
        TRACE(Trace::Information, (_T("The time has been requested, we return a clock time of: %s"), result.ToRFC1123().c_str()));

        return (_parent == nullptr ? 0 : result.Ticks());
    }

    void SimpleCOMRPCPluginServer::OnRevoke(const Exchange::IWallClock::ICallback* remote) {
        // Looks like we need to stop the callback from being called, it is a dead object now anyway :-)
        TRACE(Trace::Information, (_T("Revoking a callback, it will not fire anymore")));
        _notifier.Disarm(remote);
    }

}
} // namespace Plugin
