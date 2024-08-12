/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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
#include "Yang.h"

namespace Thunder {

namespace Plugin {

    namespace {
        static Metadata<Yang> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { },
            // Terminations
            { },
            // Controls
            { }
        );
    }

    const string Yang::Initialize(PluginHost::IShell *service)
    {
        string message{};

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_yangImplementation == nullptr);
        ASSERT(_connectionId == 0);

        _service = service;
        _service->AddRef();
        _service->Register(&_notification);

        _yangImplementation = _service->Root<Exchange::IYang>(_connectionId, 2000, _T("YangImplementation"));
        ASSERT(_yangImplementation != nullptr);

        if (_yangImplementation == nullptr) {
            message = _T("Yang could not be instantiated");
        } else {
            _yangImplementation->Configure(service);
            _yangImplementation->Register(&_notification);
            Exchange::JYang::Register(*this, _yangImplementation);
        }

        return (message);
    }

    void Yang::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            if (_yangImplementation != nullptr) {
                _yangImplementation->Unregister(&_notification);
                Exchange::JYang::Unregister(*this);

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
                VARIABLE_IS_NOT_USED const uint32_t result = _yangImplementation->Release();
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                _yangImplementation = nullptr;

                if (connection != nullptr) {
                    connection->Terminate();
                    connection->Release();
                }
            }

            _service->Unregister(&_notification);

            _connectionId = 0;

            _service->Release();
            _service = nullptr;
        }
    }

    string Yang::Information() const
    {
        return {};
    }

    void Yang::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin

}
