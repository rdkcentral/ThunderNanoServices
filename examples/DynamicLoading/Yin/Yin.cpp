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
#include "Yin.h"

namespace Thunder {

namespace Plugin {

    namespace {
        static Metadata<Yin> metadata(
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

    const string Yin::Initialize(PluginHost::IShell *service)
    {
        string message{};

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_yinImplementation == nullptr);
        ASSERT(_connectionId == 0);

        _service = service;
        _service->AddRef();
        _service->Register(&_notification);

        _yinImplementation = _service->Root<Exchange::IYin>(_connectionId, 2000, _T("YinImplementation"));
        ASSERT(_yinImplementation != nullptr);

        if (_yinImplementation == nullptr) {
            message = _T("Yin could not be instantiated");
        } else {
            _yinImplementation->Configure(service);
            _yinImplementation->Register(&_notification);
            Exchange::JYin::Register(*this, _yinImplementation);
        }

        return (message);
    }

    void Yin::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            if (_yinImplementation != nullptr) {
                _yinImplementation->Unregister(&_notification);
                Exchange::JYin::Unregister(*this);

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
                VARIABLE_IS_NOT_USED const uint32_t result = _yinImplementation->Release();
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                _yinImplementation = nullptr;

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

    string Yin::Information() const
    {
        return {};
    }

    void Yin::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin

}
