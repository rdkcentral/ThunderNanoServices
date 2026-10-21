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

#include "CalculatorProxy.h"

#include <interfaces/IConfiguration.h>
#include <interfaces/json/JMath.h>
#include <interfaces/json/JMathExtended.h>

namespace Thunder {
namespace Plugin {

    namespace {
        Metadata<CalculatorProxy> metadata(
            1, 0, 0,
            {},
            {},
            {});
    }

    CalculatorProxy::CalculatorProxy()
        : _service(nullptr)
        , _notification(*this)
        , _connectionId(0)
        , _math(nullptr)
        , _mathExtended(nullptr)
    {
    }

    const string CalculatorProxy::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        string message;
        _service = service;
        _service->AddRef();
        _service->Register(&_notification);

        _math = _service->Root<Exchange::IMath>(_connectionId, Core::infinite, _T("CalculatorProxyImplementation"));
        if (_math == nullptr) {
            message = _T("Calculator proxy implementation could not be instantiated");
        } else {
            Exchange::IConfiguration* configuration = _math->QueryInterface<Exchange::IConfiguration>();
            if (configuration == nullptr) {
                message = _T("Calculator proxy implementation does not expose IConfiguration");
            } else {
                const uint32_t result = configuration->Configure(_service);
                configuration->Release();

                if (result != Core::ERROR_NONE) {
                    message = _T("Calculator proxy implementation could not be configured");
                } else {
                    Exchange::JMath::Register(*this, _math);

                    _mathExtended = _math->QueryInterface<Exchange::IMathExtended>();
                    if (_mathExtended == nullptr) {
                        message = _T("Calculator proxy implementation does not expose IMathExtended");
                    } else {
                        _mathExtended->Register(&_notification);

                        Exchange::JMathExtended::Register(*this, _mathExtended);
                    }
                }
            }
        }

        return message;
    }

    void CalculatorProxy::Deinitialize(PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            if (_mathExtended != nullptr) {
                _mathExtended->Unregister(&_notification);
                
                Exchange::JMathExtended::Unregister(*this);
                _mathExtended->Release();
                _mathExtended = nullptr;
            }
            if (_math != nullptr) {
                Exchange::JMath::Unregister(*this);

                RPC::IRemoteConnection* connection = _service->RemoteConnection(_connectionId);
                const uint32_t result = _math->Release();
                ASSERT((result == Core::ERROR_CONNECTION_CLOSED) || (result == Core::ERROR_DESTRUCTION_SUCCEEDED));
                _math = nullptr;

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

    string CalculatorProxy::Information() const
    {
        return {};
    }

    void CalculatorProxy::Notification::Deactivated(RPC::IRemoteConnection* connection)
    {
        _parent.Deactivated(connection);
    }

    void CalculatorProxy::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(
                _service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    void CalculatorProxy::Notification::MultiplyResult(const uint32_t result)
    {
        JMathExtended::MultiplyResult(result);
    }


} // namespace Plugin
} // namespace Thunder
