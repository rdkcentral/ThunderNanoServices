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

#include "Svalbard.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(Svalbard, 1, 0);

    const string Svalbard::Initialize(PluginHost::IShell* service) /* override */ 
    {
        string message;

        ASSERT(_service == nullptr);
        ASSERT(_svalbard == nullptr);
        ASSERT(_connectionId == 0);

        _service = service;
        _service->Register(&_notification);
        _svalbard = _service->Root<Exchange::IConfiguration>(_connectionId, Core::infinite, _T("CryptographyImplementation"));

        if (_svalbard == nullptr) {
            message = _T("Svalbard could not be instantiated.");
            _service->Unregister(&_notification);
            _service = nullptr;
        } else {
            _svalbard->Configure(_service);
        }

        return message;
    }

    void Svalbard::Deinitialize(PluginHost::IShell* service)  /* override */
    {
        ASSERT(_service == service);

        _service->Unregister(&_notification);

        RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

        VARIABLE_IS_NOT_USED uint32_t result = _svalbard->Release();

        // It should have been the last reference we are releasing,
        // so it should end up in a DESCRUCTION_SUCCEEDED, if not we
        // are leaking...
        ASSERT(result == Core::ERROR_DESCRUCTION_SUCCEEDED);

        // If this was running in a (container) process...
        if (connection != nullptr) {
            // Lets trigger the cleanup sequence for
            // out-of-process code. Which will guard
            // that unwilling processes, get shot if
            // not stopped friendly :~)
            connection->Terminate();
            connection->Release();
        }

        _svalbard = nullptr;
        _service = nullptr;
    }

    string Svalbard::Information() const /* override */
    {
        return string();
    }

    void Svalbard::Deactivated(RPC::IRemoteConnection* connection)
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
