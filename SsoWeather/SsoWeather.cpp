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
 
#include "SsoWeather.h"

namespace WPEFramework {
namespace Plugin {
     namespace {

        static Metadata<SsoWeather> metadata(
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
    
    const string SsoWeather::Initialize(PluginHost::IShell* service)
    {
        string message = EMPTY_STRING;

        ASSERT (_service == nullptr);
        ASSERT (service != nullptr);
        ASSERT (_implementation == nullptr);
        ASSERT (_connectionId == 0);

        _service = service;
        _service->AddRef();
        _service->Register(&_notification);

        _implementation = _service->Root<Exchange::ISsoWeather>(_connectionId, 2000, _T("SsoWeatherImplementation"));
        if (_implementation == nullptr) {
            message = _T("Couldn't create SsoWeather instance");
        } else {
            _implementation->Register(&_weatherNotification);
            Exchange::JSsoWeather::Register(*this, _implementation);
        }

        return (message);
    }

    /* virtual */ void SsoWeather::Deinitialize(PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT (_service == service);

            service->Unregister(&_notification);

            if (_implementation != nullptr) {

                Exchange::JSsoWeather::Unregister(*this);
                _implementation->Unregister(&_weatherNotification);

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
                VARIABLE_IS_NOT_USED uint32_t result = _implementation->Release();
                _implementation = nullptr;

                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                if (connection != nullptr) {
                    connection->Terminate();
                    connection->Release();
                }
            }

            _service->Release();
            _service = nullptr;
            _connectionId = 0;
        }
    }
    
    /* virtual */ string SsoWeather::Information() const
    {
        return (EMPTY_STRING);
    }

    void SsoWeather::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin
} // namespace WPEFramework