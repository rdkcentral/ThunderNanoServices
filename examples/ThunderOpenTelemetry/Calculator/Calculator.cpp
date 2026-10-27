/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include "Calculator.h"

#include <interfaces/json/JMath.h>
#include <interfaces/json/JMathExtended.h>

namespace Thunder {
namespace Plugin {

    namespace {
        Metadata<Calculator> metadata(
            1, 0, 0,
            {},
            {},
            {});
    }

    Calculator::Calculator()
        : _service(nullptr)
        , _notification(*this)
        , _connectionId(0)
        , _mathImplementation(nullptr)
        , _mathExtendedImplementation(nullptr)
    {
    }

    const string Calculator::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        string message;
        _service = service;
        _service->AddRef();
        _service->Register(&_notification);

        _mathImplementation = _service->Root<Exchange::IMath>(_connectionId, Core::infinite, _T("CalculatorImplementation"));
        if (_mathImplementation == nullptr) {
            message = _T("Calculator implementation could not be instantiated");
        } else {
            Exchange::JMath::Register(*this, _mathImplementation);
            _mathExtendedImplementation = _mathImplementation->QueryInterface<Exchange::IMathExtended>();
            if (_mathExtendedImplementation == nullptr) {
                message = _T("Calculator implementation does not expose IMathExtended");
            } else {
                Exchange::JMathExtended::Register(*this, _mathExtendedImplementation);
                _mathExtendedImplementation->Register(&_notification);
            }
        }

        return message;
    }

    void Calculator::Deinitialize(PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            if (_mathExtendedImplementation != nullptr) {
                _mathExtendedImplementation->Unregister(&_notification);    
                Exchange::JMathExtended::Unregister(*this);
                _mathExtendedImplementation->Release();
                _mathExtendedImplementation = nullptr;
            }
            if (_mathImplementation != nullptr) {
                Exchange::JMath::Unregister(*this);

                RPC::IRemoteConnection* connection = _service->RemoteConnection(_connectionId);
                const uint32_t result = _mathImplementation->Release();
                ASSERT((result == Core::ERROR_CONNECTION_CLOSED) || (result == Core::ERROR_DESTRUCTION_SUCCEEDED));
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

    string Calculator::Information() const
    {
        return {};
    }

    void Calculator::Notification::Deactivated(RPC::IRemoteConnection* connection)
    {
        _parent.Deactivated(connection);
    }

    void Calculator::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(
                _service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    void Calculator::Notification::MultiplyResult(const uint32_t result)
    {
        Exchange::JMathExtended::Event::MultiplyResult(_parent, result);
    }

} // namespace Plugin
} // namespace Thunder