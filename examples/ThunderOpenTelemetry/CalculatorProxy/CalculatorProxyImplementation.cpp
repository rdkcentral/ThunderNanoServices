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

#include "Module.h"

#include <interfaces/IConfiguration.h>
#include <interfaces/IMath.h>

#include <interfaces/json/JMathExtended.h>

#include <opentelemetry/opentelemetry.h>

#include <algorithm>
#include <vector>

namespace Thunder {
namespace Plugin {

    class CalculatorProxyImplementation : public Exchange::IMath
        , public Exchange::IMathExtended
        , public Exchange::IMathExtended::INotification  
        , public Exchange::IConfiguration     
    {
    public:
        CalculatorProxyImplementation()
            : _service(nullptr)
            , _adminLock()
            , _notifications()
            , _registeredfornotifications(false)
        {
            string config{ R"({"sampler":"ParentOn", "logger": {"loggerenabled" : true},"componentname":"Thunder CalculatorProxy":"1.0.1"})" };

            Telemetry::OpenTelemetry::Config telemetryConfig;
            telemetryConfig.FromString(config);
            Telemetry::OpenTelemetry::Configure(telemetryConfig);
            Telemetry::OpenTelemetry::Instance().Initialize();
        }
        CalculatorProxyImplementation(const CalculatorProxyImplementation&) = delete;
        CalculatorProxyImplementation& operator=(const CalculatorProxyImplementation&) = delete;

        ~CalculatorProxyImplementation() override
        {
            ASSERT(_service != nullptr);

            if(_registeredfornotifications == true) {
                Exchange::IMathExtended* imathExtended = _service->QueryInterfaceByCallsign<Exchange::IMathExtended>(_T("Calculator"));
                if (imathExtended != nullptr) {
                    imathExtended->Unregister(this);
                    imathExtended->Release();
                }
            }
            
            if (_service != nullptr) {
                _service->Release();
            }

            Telemetry::OpenTelemetry::Instance().Deinitialize();
        }

        uint32_t Configure(PluginHost::IShell* shell) override     
        {
            ASSERT(shell != nullptr);
            ASSERT(_service == nullptr);

            _service = shell;
            static_cast<void>(_service->AddRef());

            return Core::ERROR_NONE;
        }

        uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            Exchange::IMath* imath = _service->QueryInterfaceByCallsign<Exchange::IMath>(_T("Calculator"));
            if (imath != nullptr) {
                result = imath->Add(A, B, sum);
                imath->Release();
            }
            return result;
        }

        uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            Exchange::IMath* imath = _service->QueryInterfaceByCallsign<Exchange::IMath>(_T("Calculator"));
            if (imath != nullptr) {
                result = imath->Sub(A, B, sum);
                imath->Release();
            }
            return result;
        }

        Core::hresult Register(Exchange::IMathExtended::INotification* const notification) override
        {

            Core::hresult result = Core::ERROR_BAD_REQUEST;
            if (notification != nullptr) {
                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
                if (std::find(_notifications.begin(), _notifications.end(), notification) != _notifications.end()) {
                    result = Core::ERROR_ALREADY_CONNECTED;
                } else {
                    notification->AddRef();
                    _notifications.push_back(notification);
                    result = Core::ERROR_NONE;
                    if((_notifications.size() == 1) && (_registeredfornotifications == false)) {
                        Exchange::IMathExtended* imathExtended = _service->QueryInterfaceByCallsign<Exchange::IMathExtended>(_T("Calculator"));
                        if (imathExtended != nullptr) {
                            imathExtended->Register(this);
                            imathExtended->Release();
                            _registeredfornotifications = true;
                        }

                    }
                }
            }

            return result;
        }

        Core::hresult Unregister(const Exchange::IMathExtended::INotification* const notification) override
        {
            Exchange::IMathExtended::INotification* registered = nullptr;
            Core::hresult result = Core::ERROR_UNKNOWN_KEY;
            {
                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
                auto index = std::find(_notifications.begin(), _notifications.end(), notification);
                if (index != _notifications.end()) {
                    registered = *index;
                    _notifications.erase(index);
                    result = Core::ERROR_NONE;
                }
            }

            if (registered != nullptr) {
                registered->Release();
            }

            return result;
        }

        Core::hresult Multiply(const uint16_t A, const uint16_t B) override
        {
          	auto span = Telemetry::OpenTelemetry::Instance().StartSpan("CalculatorProxy Multiply");

            uint32_t result = Core::ERROR_UNAVAILABLE;

            Exchange::IMathExtended* imathExtended = _service->QueryInterfaceByCallsign<Exchange::IMathExtended>(_T("Calculator"));
            if (imathExtended != nullptr) {
                _adminLock.Lock();
                if(_registeredfornotifications == false){

                  	auto span = Telemetry::OpenTelemetry::Instance().StartSpan("CalculatorProxy Multiply Registering");
                   (*span)->SetAttribute("Test","Nested spans in same thread");

                    imathExtended->Register(this);
                   _registeredfornotifications = true;
                }
                _adminLock.Unlock();
                result = imathExtended->Multiply(A, B);
                imathExtended->Release();
            }

            return result;
        }

        void MultiplyResult(const uint32_t result) override
        {
          	auto span = Telemetry::OpenTelemetry::Instance().StartSpan("CalculatorProxy Multiply Sending results");

            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            for (auto* notification : _notifications) {
                notification->MultiplyResult(result);
            }

            Exchange::IMathExtended::INotification* internalnotification = _service->QueryInterface<Exchange::IMathExtended::INotification>();
            if(internalnotification != nullptr){
                internalnotification->MultiplyResult(result);
            }
        }

        BEGIN_INTERFACE_MAP(CalculatorProxyImplementation)
            INTERFACE_ENTRY(Exchange::IMath)
            INTERFACE_ENTRY(Exchange::IMathExtended)
            INTERFACE_ENTRY(Exchange::IMathExtended::INotification)
            INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        Core::CriticalSection _adminLock;
        std::vector<Exchange::IMathExtended::INotification*> _notifications;
        bool _registeredfornotifications;
    };

    SERVICE_REGISTRATION(CalculatorProxyImplementation, 1, 0)

} // namespace Plugin
} // namespace Thunder
