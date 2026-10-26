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

#include <interfaces/IMath.h>

#include <open_telemetry/open_telemetry.h>

#include <algorithm>
#include <vector>

namespace Thunder {
namespace Plugin {

    class CalculatorImplementation : public Exchange::IMath
        , public Exchange::IMathExtended {
    public:

        PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)

        CalculatorImplementation()
        : _adminLock()
        ,_notifications()
        , _resultjob(*this)
        {
            string config{ R"({"sampler":"ParentOn", "logger": {"loggerenabled" : true},"componentname":"Thunder Calculator", "componentversion":"1.0.1"})" };

            Telemetry::OpenTelemetry::Config telemetryConfig;
            telemetryConfig.FromString(config);
            Telemetry::OpenTelemetry::Configure(telemetryConfig);
            Telemetry::OpenTelemetry::Instance().Initialize();
        }

        POP_WARNING()

        CalculatorImplementation(const CalculatorImplementation&) = delete;
        CalculatorImplementation& operator=(const CalculatorImplementation&) = delete;

        ~CalculatorImplementation() override
        {
            _resultjob.Revoke();
            Telemetry::OpenTelemetry::Instance().Deinitialize();
        }

        uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
            sum = static_cast<uint16_t>(A + B);
            return Core::ERROR_NONE;
        }

        uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
            sum = static_cast<uint16_t>(A - B);
            return Core::ERROR_NONE;
        }

        Core::hresult Register(Exchange::IMathExtended::INotification* const notification) override
        {
           	auto span = Telemetry::OpenTelemetry::Instance().StartSpan("Calculator Multiply Register");
            auto scope = Telemetry::OpenTelemetry::Instance().StartScopeFromJobContext();
		    Telemetry::OpenTelemetry::Instance().Logger()->Log(opentelemetry::logs::Severity::kInfo, "notification set");

            Core::hresult result = Core::ERROR_NONE;
            if (notification == nullptr) {
                result = Core::ERROR_BAD_REQUEST;
            } else {
                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
                if (std::find(_notifications.begin(), _notifications.end(), notification) != _notifications.end()) {
                    result = Core::ERROR_ALREADY_CONNECTED;
                } else {
                    notification->AddRef();
                    _notifications.push_back(notification);
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
         	auto span = Telemetry::OpenTelemetry::Instance().StartSpan("Calculator Multiply");

            auto logrecord = Telemetry::OpenTelemetry::Instance().CreateTraceFilledLogRecord();
            logrecord->SetSeverity(opentelemetry::logs::Severity::kInfo);
            string values;
            values += std::to_string(A);
            values += _T(" and ");
            values += std::to_string(B);
            logrecord->SetAttribute("Values", values);
            Telemetry::OpenTelemetry::Instance().Logger()->EmitLogRecord(std::move(logrecord));

            const uint32_t result = static_cast<uint32_t>(A) * static_cast<uint32_t>(B);
            _resultjob.Revoke();
            static_cast<MultiplayResultJob&>(_resultjob).SetContext(result, Core::JobContext::Instance().TraceContextW3C());
            _resultjob.Submit();

            return Core::ERROR_NONE;
        }


        class MultiplayResultJob 
        {
        public:
            MultiplayResultJob(CalculatorImplementation& parent) 
            : _parent(parent)
            , _result(0)
            , _tracecontext()
            {
            }

            void SetContext(const uint32_t result, const Core::JobContext::W3CTraceContext& context)
            {
                _tracecontext = context;
                _result = result;
            }

            void Dispatch() const
            {
                Core::JobContext::Instance().TraceContextW3C() = _tracecontext;
             	auto span = Telemetry::OpenTelemetry::Instance().StartSpan("Calculator Multiply Sending results");
                {
                    Core::SafeSyncType<Core::CriticalSection> lock(_parent._adminLock);
                    for (auto* notification : _parent._notifications) {
                        notification->MultiplyResult(_result);
                    }
                }
                Core::JobContext::Instance().TraceContextW3C().ClearTraceContext();
            }

        private:
            CalculatorImplementation& _parent;
            uint32_t _result;
            Core::JobContext::W3CTraceContext _tracecontext;
        };

        BEGIN_INTERFACE_MAP(CalculatorImplementation)
            INTERFACE_ENTRY(Exchange::IMath)
            INTERFACE_ENTRY(Exchange::IMathExtended)
        END_INTERFACE_MAP

    private:
        Core::CriticalSection _adminLock;
        std::vector<Exchange::IMathExtended::INotification*> _notifications;
        Core::WorkerPool::JobType<MultiplayResultJob> _resultjob;            


    };

    SERVICE_REGISTRATION(CalculatorImplementation, 1, 0)

} // namespace Plugin
} // namespace Thunder