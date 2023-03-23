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
 
#include "RDKAdapter.h"

namespace WPEFramework {
namespace Plugin {

    namespace {

        static Metadata<RDKAdapter> metadata(
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

    const string RDKAdapter::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        _subsystems = service->SubSystems();

        if(_subsystems != nullptr) {
            _subsystems->Register(&_sink);
            _connected = _subsystems->IsActive(PluginHost::ISubSystem::INTERNET);
        }

        // On success return empty, to indicate there is no error text.
        return (EMPTY_STRING);
    }

    void RDKAdapter::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if(_subsystems != nullptr) {
            _subsystems->Unregister(&_sink);
            _subsystems->Release();
            _subsystems = nullptr;
        }
    }

    string RDKAdapter::Information() const
    {
        return (EMPTY_STRING);
    }

    Core::hresult RDKAdapter::Register(Exchange::IRDKAdapter::INotification* sink) {
        _adminLock.Lock();

        ASSERT(std::find(_listeners.begin(), _listeners.end(), sink) == _listeners.end());

        _listeners.emplace_back(sink);

        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    Core::hresult RDKAdapter::Unregister(Exchange::IRDKAdapter::INotification* sink) {
        _adminLock.Lock();

        auto item = std::find(_listeners.begin(), _listeners.end(), sink);

        ASSERT(item != _listeners.end());

        _listeners.erase(item);

        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    Core::hresult RDKAdapter::Connected(bool& connected) const {
        connected = _connected;
        return Core::ERROR_NONE;
    }

    void RDKAdapter::SubsystemUpdate() {
        _adminLock.Lock();

        bool connected = _subsystems->IsActive(PluginHost::ISubSystem::INTERNET);

        if(connected != _connected) {
            _connected = connected;

            for (auto l : _listeners) {
                l->ConnectionUpdate(_connected);
            }
        }

        _adminLock.Unlock();
       
    }

} // namespace Plugin
} // namespace WPEFramework
