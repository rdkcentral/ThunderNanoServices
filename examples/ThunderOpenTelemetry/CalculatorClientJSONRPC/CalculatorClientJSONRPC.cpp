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

#define MODULE_NAME CalculatorClientJSONRPC

#include <core/core.h>
#include <websocket/websocket.h>

#include <interfaces/json/JsonData_Math.h>
#include <interfaces/json/JsonData_MathExtended.h>

#include <open_telemetry/open_telemetry.h>


#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>

namespace 
{

    using namespace Thunder;

    constexpr uint32_t WaitTime = 5000;

    class MultiplyNotification {
    public:
        MultiplyNotification()
            : _adminLock()
            , _signal()
            , _received(false)
            , _result(0)
        {
        }
        
        void OnModified(const JsonData::MathExtended::MultiplyResultParamsData& parameters)
        {

            printf("\n\n Received Multiply Notification \n\n");

            uint32_t result = parameters.Result.Value();

            auto span = Telemetry::OpenTelemetry::Instance().StartSpan("COMRPC Application Multiply Received");
            span->Activate();
            std::string textresult{"Multiply result ="};
            textresult += std::to_string(result) ;
            Telemetry::OpenTelemetry::Instance().Logger()->Log(opentelemetry::logs::Severity::kInfo, textresult);

            {
                std::lock_guard<std::mutex> guard(_adminLock);
                _received = true;
                _result = result;
            }
            _signal.notify_one();
        }

        bool WaitFor(uint32_t& result) 
        {
            std::unique_lock<std::mutex> lock(_adminLock);
            return _signal.wait_for(lock, std::chrono::milliseconds(WaitTime), [this, &result] {
                result = _result;
                return (_received == true);
            });
        }

    private:
        mutable std::mutex _adminLock;
        std::condition_variable _signal;
        bool _received;
        uint32_t _result;
    };
}

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

int main(int argc, char* argv[])
{

    string callsign(_T("CalculatorProxy"));

    if(argc >= 2)
    {
        callsign = argv[1];
    }

    string config{ R"({"sampler":"ParentOn", "logger": {"loggerenabled" : true}, "componentname":"Thunder JSONRPC test app","componentversion":"1.0.1"})" };

    Telemetry::OpenTelemetry::Config telemetryConfig;
    telemetryConfig.FromString(config);
    Telemetry::OpenTelemetry::Configure(telemetryConfig);
    Telemetry::OpenTelemetry::Instance().Initialize();

    auto tracestate = opentelemetry::trace::TraceState::GetDefault();
    tracestate = tracestate->Set("rdk", "someothervalue");  // note uppercase not allowed
    string tracestatesheader = tracestate->ToHeader();
    Core::JobContext::Instance().TraceContextW3C().TraceState(tracestatesheader);

    {
        auto scope = Telemetry::OpenTelemetry::Instance().StartScopeFromJobContext();

		Telemetry::OpenTelemetry::Instance().Logger()->Log(opentelemetry::logs::Severity::kInfo, "trace state set");
    }

    string access;
    if (Core::SystemInfo::GetEnvironment(_T("THUNDER_ACCESS"), access) == false) {
#ifdef __WINDOWS__
        Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), _T("127.0.0.1:25555"));
#else
        Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), _T("127.0.0.1:80"));
#endif
    }

    printf("*** Calculator using %s ***\n\n", callsign.c_str());

    uint16_t A = 40;
    uint16_t B = 40;


    JSONRPC::LinkType<Core::JSON::IElement> link(callsign);
    MultiplyNotification notification;
    uint32_t result = link.Subscribe<JsonData::MathExtended::MultiplyResultParamsData>(WaitTime, _T("multiplyResult"), &MultiplyNotification::OnModified, &notification);
    if (result != Core::ERROR_NONE) {
        std::fprintf(stderr, "Subscription to multiply event failed: %u\n", result);
        return 1;
    }

	auto span = Telemetry::OpenTelemetry::Instance().StartSpan("COMRPC Application Multiply");
    (*span)->SetAttribute("Multiply","40x40");

    JsonData::MathExtended::MultiplyParamsData parameters;
    parameters.A = A;
    parameters.B = B;
    result = link.Invoke<JsonData::MathExtended::MultiplyParamsData, void>(WaitTime, _T("multiply"), parameters);
    if (result != Core::ERROR_NONE) {
        std::fprintf(stderr, "Multiply failed: %u\n", result);
        link.Unsubscribe(WaitTime, _T("multiplyResult"));
        return 2;
    }

    uint32_t product = 0;
    const bool notified = notification.WaitFor(product);

    span->End();

    if(notified == false) {
        std::fprintf(stderr, "Multiply result notification timed out\n");
        link.Unsubscribe(WaitTime, _T("multiplyResult"));
        return 3;
    }

    link.Unsubscribe(WaitTime, _T("multiplyResult"));

    std::printf("%u * %u = %u\n", A, B, product);

    Telemetry::OpenTelemetry::Instance().Deinitialize();

    char c;
    std::cin >> c;

    return 0;
}
