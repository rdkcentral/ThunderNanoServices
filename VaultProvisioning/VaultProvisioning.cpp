/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological
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

#include "Module.h"
#include "VaultProvisioning.h"

#include <interfaces/IConfiguration.h>

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<VaultProvisioning> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::PLATFORM },
            // Terminations
            {},
            // Controls
            { subsystem::PROVISIONING }
        );
    }

    const string VaultProvisioning::Initialize(PluginHost::IShell* service)
    {
        string message;

        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_implementation == nullptr);
        ASSERT(_connectionId == 0);

        _service = service;
        _service->AddRef();
        _service->Register(&_connectionNotification);

        _implementation = _service->Root<Exchange::IConfiguration>(_connectionId, 2000, _T("VaultProvisioningImplementation"));

        if (_implementation == nullptr) {
            message = _T("Failed to create a vault provisioning instance");
        }
        else {
            if (_implementation->Configure(service) != Core::ERROR_NONE) {
                message = _T("Failed to configure vault provisioning");
            }
        }

        return (message);
    }

    void VaultProvisioning::Deinitialize(PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            service->Unregister(&_connectionNotification);

            if (_implementation != nullptr) {
                RPC::IRemoteConnection* connection = _service->RemoteConnection(_connectionId);

                VARIABLE_IS_NOT_USED uint32_t result = _implementation->Release();
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
                _implementation = nullptr;

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

    void VaultProvisioning::Deactivated(RPC::IRemoteConnection* connection)
    {
        using IShell = PluginHost::IShell;

        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(IShell::Job::Create(_service, IShell::DEACTIVATED, IShell::FAILURE));
        }
    }

} // namespace Plugin
}
