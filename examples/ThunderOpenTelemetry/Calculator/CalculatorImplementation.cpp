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

#include <open_telemetry/open_telemetry.h>

#include "../common/TelemetryConfig.h"
#include "../common/TimedSpan.h"

#include <algorithm>
#include <vector>

namespace Thunder {
namespace Plugin {

    class CalculatorImplementation : public Exchange::IMath
        , public Exchange::IMathExtended
        , public Exchange::IConfiguration {
    private:
        // Load-test settings from the "configuration" section of Calculator.json
        class LoadConfig : public Core::JSON::Container {
        public:
            LoadConfig(const LoadConfig&) = delete;
            LoadConfig& operator=(const LoadConfig&) = delete;

            LoadConfig()
                : Core::JSON::Container()
                , ExtraSpans(0)
                , ExtraSpanPayload(0)
            {
                Add(_T("extraspans"), &ExtraSpans);
                Add(_T("extraspanpayload"), &ExtraSpanPayload);
            }
            ~LoadConfig() override = default;

            Core::JSON::DecUInt16 ExtraSpans;
            Core::JSON::DecUInt32 ExtraSpanPayload;
        };

    public:

        PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)

        CalculatorImplementation()
        : _adminLock()
        ,_notifications()
        , _resultjob(*this)
        , _telemetryInitialized(false)
        , _extraSpans(0)
        , _extraSpanPayload(0)
        {
        }

        POP_WARNING()

        CalculatorImplementation(const CalculatorImplementation&) = delete;
        CalculatorImplementation& operator=(const CalculatorImplementation&) = delete;

        ~CalculatorImplementation() override
        {
            _resultjob.Revoke();
            if (_telemetryInitialized == true) {
                Telemetry::OpenTelemetry::Instance().Deinitialize();
            }
        }

        uint32_t Configure(PluginHost::IShell* shell) override
        {
            ASSERT(shell != nullptr);
            ASSERT(_telemetryInitialized == false);

            string config{ R"({"sampler":"ParentOn", "logger": {"enabled" : true},"componentname":"Thunder Calculator", "componentversion":"1.0.1"})" };

            Telemetry::OpenTelemetry::Config telemetryConfig;
            telemetryConfig.FromString(config);

            string error;
            if (Telemetry::ApplyPluginConfig(telemetryConfig, shell->ConfigLine(), error) == false) {
                SYSLOG(Logging::ParsingError, (_T("Parsing telemetry plugin config failed: %s"), error.c_str()));
            }
            LoadConfig pluginConfig;
            pluginConfig.FromString(shell->ConfigLine());
            _extraSpans = pluginConfig.ExtraSpans.Value();
            _extraSpanPayload = pluginConfig.ExtraSpanPayload.Value();

            fprintf(stderr, "Calculator OpenTelemetry %s, extraspans=%u extraspanpayload=%u bytes\n",
                Telemetry::Describe(telemetryConfig).c_str(), _extraSpans, _extraSpanPayload);
            fflush(stderr);

            Telemetry::OpenTelemetry::Configure(telemetryConfig);
            Telemetry::OpenTelemetry::Instance().Initialize();
            _telemetryInitialized = true;

            return Core::ERROR_NONE;
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
           	Telemetry::TimedSpan span("Calculator Multiply Register");
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

            span.End();

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
         	Telemetry::TimedSpan span("Calculator Multiply");

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

            // Extra spans to raise memory usage; all kept alive (nested) until Multiply is done.
            // Created after the log record and result job so those still belong to "Calculator Multiply".
            std::vector<std::shared_ptr<Telemetry::OpenTelemetry::Span>> extraSpans;
            extraSpans.reserve(_extraSpans);
            const string payload(_extraSpanPayload, 'x');
            for (uint16_t index = 0; index < _extraSpans; ++index) {
                const string name = _T("Calculator Span Spam Multiply ") + std::to_string(index + 1);
                extraSpans.push_back(Telemetry::OpenTelemetry::Instance().StartSpan(name));
                if (payload.empty() == false) {
                    (*extraSpans.back())->SetAttribute("payload", opentelemetry::nostd::string_view(payload.data(), payload.size()));
                }
            }
            
            // Each span restores the previous trace context when it ends, so end them in reverse order.
            while (extraSpans.empty() == false) {
                extraSpans.pop_back();
            }
            span.End();

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
             	Telemetry::TimedSpan span("Calculator Multiply Sending results");
                {
                    Core::SafeSyncType<Core::CriticalSection> lock(_parent._adminLock);
                    for (auto* notification : _parent._notifications) {
                        notification->MultiplyResult(_result);
                    }
                }
                span.End();
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
            INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

    private:
        Core::CriticalSection _adminLock;
        std::vector<Exchange::IMathExtended::INotification*> _notifications;
        Core::WorkerPool::JobType<MultiplayResultJob> _resultjob;            
        bool _telemetryInitialized;
        uint16_t _extraSpans;
        uint32_t _extraSpanPayload;


    };

    SERVICE_REGISTRATION(CalculatorImplementation, 1, 0)

} // namespace Plugin
} // namespace Thunder