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

#include "DeviceProvisioning.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceProvisioning, 1, 0);

    const string DeviceProvisioning::Initialize(PluginHost::IShell* service)
    {
        string message;

        ASSERT(_service == nullptr);
        ASSERT(_deviceprovisioning == nullptr);
        ASSERT(_connectionId == 0);

        _service = service;
        _service->Register(&_notification);

        _deviceprovisioning = _service->Root<Exchange::IConfiguration>(_connectionId, 2000, _T("DeviceProvisioningImplementation"));
        if (_deviceprovisioning != nullptr) {

            PluginHost::IStateControl* stateControl(_deviceprovisioning->QueryInterface<PluginHost::IStateControl>());
            if (stateControl != nullptr) {
                stateControl->Register(&_notification);
                stateControl->Configure(_service);
                stateControl->Release();
            } else {
                _deviceprovisioning->Release();
                _deviceprovisioning = nullptr;
            }
        }
        if (_deviceprovisioning == nullptr) {
            message = _T("DeviceProvisioning could not be instantiated.");
            _service->Unregister(&_notification);
            _service = nullptr;
            ConnectionTerminate(_connectionId);
        }
        return message;
    }

    void DeviceProvisioning::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_deviceprovisioning != nullptr);

        PluginHost::IStateControl* stateControl(_deviceprovisioning->QueryInterface<PluginHost::IStateControl>());
        if (stateControl != nullptr) {
            stateControl->Unregister(&_notification);
            stateControl->Release();
        }
        _deviceprovisioning->Release();

        _service->Unregister(&_notification);
        ConnectionTerminate(_connectionId);

        _deviceprovisioning = nullptr;
        _service = nullptr;
    }

    string DeviceProvisioning::Information() const
    {
        return {};
    }

    void DeviceProvisioning::ConnectionTerminate(uint32_t connectionId)
    {
        if (connectionId != 0) {
            RPC::IRemoteConnection* connection(_service->RemoteConnection(connectionId));
            if (connection != nullptr) {
                connection->Terminate();
                connection->Release();
            }
        }
    }

    void DeviceProvisioning::StateChange(const PluginHost::IStateControl::state state)
    {
        switch (state) {
        case PluginHost::IStateControl::EXITED:
            Core::IWorkerPool::Instance().Submit(
                PluginHost::IShell::Job::Create(_service,
                    PluginHost::IShell::DEACTIVATED,
                    PluginHost::IShell::REQUESTED));
            break;
        default:
            ASSERT(false);
            break;
        }
    }

    void DeviceProvisioning::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {

            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(
                PluginHost::IShell::Job::Create(_service,
                    PluginHost::IShell::DEACTIVATED,
                    PluginHost::IShell::FAILURE));
        }
    }
}
} // namespace
