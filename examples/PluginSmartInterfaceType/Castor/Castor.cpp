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

#include "Castor.h"
#include <interfaces/json/JMath.h>
#include <interfaces/IConfiguration.h>

namespace Thunder {

namespace Plugin {

    namespace {
        static Metadata<Castor> metadata(
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

    const string Castor::Initialize(PluginHost::IShell *service)
    {
        string message{};

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_mathImplementation == nullptr);
        ASSERT(_connectionId == 0);

        _service = service;
        _service->AddRef();
        _service->Register(&_notification);

        _mathImplementation = _service->Root<Exchange::IMath>(_connectionId, 2000, _T("CastorImplementation"));
        ASSERT(_mathImplementation != nullptr);

        if (_mathImplementation == nullptr) {
            message = _T("Castor could not be instantiated");
        } else {
            Exchange::IConfiguration* configure = _mathImplementation->QueryInterface<Exchange::IConfiguration>();
            if (configure != nullptr) {
                uint32_t result = configure->Configure(_service);
                configure->Release();
                if(result != Core::ERROR_NONE) {
                message = _T("Castor could not be configured");
                }
            } else {
                message = _T("Castor implementation did not provide a configuration interface");
            }

            Exchange::JMath::Register(*this, _mathImplementation);
        }

        return (message);
    }

    void Castor::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            if (_mathImplementation != nullptr) {
                Exchange::JMath::Unregister(*this);

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
                VARIABLE_IS_NOT_USED const uint32_t result = _mathImplementation->Release();
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                _mathImplementation = nullptr;

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

    string Castor::Information() const
    {
        return {};
    }

    void Castor::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin

}
