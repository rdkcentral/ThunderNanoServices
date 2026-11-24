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


//********************************** */


// NOTE generated with AI, might not adhere to all proper Thunder conventions


 //********************************** */




#define MODULE_NAME CalculatorExtendedClientCOMRPC

#include <com/com.h>
#include <core/core.h>
#include <interfaces/IMath.h>
#include <plugins/plugins.h>
#include <open_telemetry/open_telemetry.h>

#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <cstdint>

#if defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#endif

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace {

    using namespace WPEFramework;

    uint64_t CurrentRssBytes()
    {
#if defined(__linux__)
        const int statm = open("/proc/self/statm", O_RDONLY);
        if (statm < 0) {
            return 0;
        }

        char buffer[128];
        const ssize_t bytesRead = read(statm, buffer, sizeof(buffer) - 1);
        close(statm);
        if (bytesRead <= 0) {
            return 0;
        }

        buffer[bytesRead] = '\0';
        char* cursor = buffer;
        std::strtoull(cursor, &cursor, 10); // Skip total virtual-memory pages.
        const uint64_t residentPages = std::strtoull(cursor, nullptr, 10);
        const long pageSize = sysconf(_SC_PAGESIZE);
        return (pageSize > 0) ? residentPages * static_cast<uint64_t>(pageSize) : 0;
#else
        return 0;
#endif
    }

    class CalculatorExtendedClientCOMRPC : private RPC::SmartInterfaceType<Exchange::IMathExtended> {
    private:
        using BaseClass = RPC::SmartInterfaceType<Exchange::IMathExtended>;

        class Notification : public Exchange::IMathExtended::INotification {
        public:
            explicit Notification(CalculatorExtendedClientCOMRPC& parent)
                : _parent(parent)
            {
            }
            ~Notification() override = default;

            void MultiplyResult(const uint32_t result) override
            {
	            auto span = Telemetry::OpenTelemetry::Instance().StartSpan("COMRPC Application Multiply Received");
                span->Activate();
                std::string textresult{"Multiply result ="};
                textresult += std::to_string(result) ;
                Telemetry::OpenTelemetry::Instance().Logger()->Log(opentelemetry::logs::Severity::kInfo, textresult);
                span->End();
                _parent.MultiplyResult(result);
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(Exchange::IMathExtended::INotification)
            END_INTERFACE_MAP

        private:
            CalculatorExtendedClientCOMRPC& _parent;
        };

    public:
        CalculatorExtendedClientCOMRPC()
            : BaseClass()
            , _notification(*this)
            , _interface(nullptr)
            , _received(false)
            , _result(0)
        {
        }
        CalculatorExtendedClientCOMRPC(const CalculatorExtendedClientCOMRPC&) = delete;
        CalculatorExtendedClientCOMRPC& operator=(const CalculatorExtendedClientCOMRPC&) = delete;

        ~CalculatorExtendedClientCOMRPC()
        {
            Close();
        }

        uint32_t Open(const string& callsign)
        {

            uint32_t result = BaseClass::Open(3000, BaseClass::Connector(), callsign);

            if (result == Core::ERROR_NONE) {
                _interface = BaseClass::Interface();
                if (_interface == nullptr) {
                    result = Core::ERROR_UNAVAILABLE;
                } else {
                    result = _interface->Register(&_notification);
                    if (result != Core::ERROR_NONE) {
                        _interface->Release();
                        _interface = nullptr;
                    }
                }
            }

            return result;
        }

        void Close()
        {
            if (_interface != nullptr) {
                _interface->Unregister(&_notification);
                _interface->Release();
                _interface = nullptr;
            }
            BaseClass::Close(Core::infinite);
        }

        uint32_t Multiply(const uint16_t A, const uint16_t B)
        {
            if (_interface == nullptr) {
                return Core::ERROR_UNAVAILABLE;
            }

            {
                std::lock_guard<std::mutex> lock(_adminLock);
                _received = false;
            }

            return _interface->Multiply(A, B);
        }

        bool WaitForResult(uint32_t& result)
        {
            std::unique_lock<std::mutex> lock(_adminLock);
            const bool received = _resultAvailable.wait_for(lock, std::chrono::seconds(5), [this]() {
                return _received;
            });

            if (received == true) {
                result = _result;
            }

            return received;
        }

    private:
        void MultiplyResult(const uint32_t result)
        {

            printf("Multiply result received: %u\n", result);

            {
                std::lock_guard<std::mutex> lock(_adminLock);
                _result = result;
                _received = true;
            }
            _resultAvailable.notify_one();
        }

    private:
        Core::SinkType<Notification> _notification;
        Exchange::IMathExtended* _interface;
        std::mutex _adminLock;
        std::condition_variable _resultAvailable;
        bool _received;
        uint32_t _result;
    };

} // namespace

int main(int argc, char* argv[])
{
    string callsign(_T("CalculatorProxy"));

    if(argc >= 2)
    {
        callsign = argv[1];
    }

    printf("*** Calculator using %s ***\n\n", callsign.c_str());

    string config{ R"({"sampler":"ParentOn", "metrics": {"enabled" : false, "intervalms" : 1000, "timeoutms" : 2000}, "resources" : {"schemaURL" : "url", "attributes":[{"key":"1", "value":"V1"},{"key":"2", "value":"V2"}]}, "exporter" : "Http", "tracerendpoint" : "127.0.0.1:4318/v1/traces", "logger" : {"enabled" : true, "endpoint" : "127.0.0.1:4318/v1/logs"}, "componentname":"Thunder COMRPC test app","componentversion":"1.0.1"})" };

    Telemetry::OpenTelemetry::Config telemetryConfig;
    Core::OptionalType<Core::JSON::Error> error;
    telemetryConfig.FromString(config, error);
    if(error.IsSet() == true) {
        printf("\nError parsing config: %s\n\n", error.Value().Message().c_str());
    }
    Telemetry::OpenTelemetry::Configure(telemetryConfig);

    Telemetry::OpenTelemetry::Instance().Initialize();

    auto tracestate = opentelemetry::trace::TraceState::GetDefault();
    tracestate = tracestate->Set("rdk", "somevalue");  // note uppercase not allowed
    string tracestatesheader = tracestate->ToHeader();
    Core::JobContext::Instance().TraceContextW3C().TraceState(tracestatesheader);

    {
        auto scope = Telemetry::OpenTelemetry::Instance().StartScopeFromJobContext();

		Telemetry::OpenTelemetry::Instance().Logger()->Log(opentelemetry::logs::Severity::kInfo, "trace state set");
    }

    CalculatorExtendedClientCOMRPC client;
    uint32_t result = client.Open(callsign);
    if (result != WPEFramework::Core::ERROR_NONE) {
        std::fprintf(stderr, "Failed to connect to %s: %u\n", callsign.c_str(), result);
        return 5;
    }

    {
        auto multiplications = Telemetry::OpenTelemetry::Instance().Meter()->CreateUInt64Counter("multiplications", "Number of multiplications processed", "{multiplications}");
        auto latency = Telemetry::OpenTelemetry::Instance().Meter()->CreateDoubleHistogram("multiplications.duration", "multiplications processing duration", "ms");

        std::map<std::string, std::string> attrs{{"multiplications.provider", "thunder"}};
        auto attrs_view = opentelemetry::common::KeyValueIterableView<decltype(attrs)>{attrs};

        constexpr uint32_t operationCount = 50;
        uint32_t product = 0;
        for (uint32_t operation = 0; operation < operationCount; ++operation) {
            const uint64_t rssBeforeBytes = CurrentRssBytes();
            const auto spanCreationStart = std::chrono::steady_clock::now();
            auto span = Telemetry::OpenTelemetry::Instance().StartSpan("COMRPC Application Multiply");
            const double spanCreationNs = std::chrono::duration<double, std::nano>(
                std::chrono::steady_clock::now() - spanCreationStart).count();
            const uint64_t rssAfterStartSpanBytes = CurrentRssBytes();
            (*span)->SetAttribute("span.creation_duration_ns", spanCreationNs);
            std::printf("StartSpan call for operation %u took %.0f ns\n", operation + 1, spanCreationNs);
            (*span)->SetAttribute("Multiply", "20x20");
            (*span)->SetAttribute("operation.number", static_cast<int64_t>(operation + 1));

            const auto start = std::chrono::steady_clock::now();
            result = client.Multiply(20, 20);

            if (result != WPEFramework::Core::ERROR_NONE) {
               std::fprintf(stderr, "Multiply failed: %u\n", result);
               return 6;
            }

            uint32_t product = 0;
            if (client.WaitForResult(product) == false) {
               std::fprintf(stderr, "Timed out waiting for MultiplyResult.\n");
               return 7;
            }

            span->Activate();

            const uint64_t rssBeforeEndBytes = CurrentRssBytes();
            const auto spanEndStart = std::chrono::steady_clock::now();
            span->End();
            const double spanEndCallNs = std::chrono::duration<double, std::nano>(
                std::chrono::steady_clock::now() - spanEndStart).count();
            const uint64_t rssAfterEndBytes = CurrentRssBytes();
            std::printf("span->End() call for operation %u took %.0f ns\n", operation + 1, spanEndCallNs);
            if (rssBeforeBytes != 0 && rssAfterStartSpanBytes != 0 && rssBeforeEndBytes != 0 && rssAfterEndBytes != 0) {
                const double rssBeforeMiB = static_cast<double>(rssBeforeBytes) / (1024.0 * 1024.0);
                const double rssAfterStartSpanMiB = static_cast<double>(rssAfterStartSpanBytes) / (1024.0 * 1024.0);
                const double rssBeforeEndMiB = static_cast<double>(rssBeforeEndBytes) / (1024.0 * 1024.0);
                const double rssAfterEndMiB = static_cast<double>(rssAfterEndBytes) / (1024.0 * 1024.0);
                std::printf("RSS operation %u: before=%.2f MiB, after StartSpan=%.2f MiB (%+.2f MiB), before End=%.2f MiB, after End=%.2f MiB (%+.2f MiB)\n",
                    operation + 1, rssBeforeMiB, rssAfterStartSpanMiB, rssAfterStartSpanMiB - rssBeforeMiB,
                    rssBeforeEndMiB, rssAfterEndMiB, rssAfterEndMiB - rssBeforeEndMiB);
            } else {
                std::printf("RSS unavailable for operation %u (this sampler requires Linux /proc).\n", operation + 1);
            }

            const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            multiplications->Add(1, attrs_view);
            latency->Record(elapsed, attrs_view, opentelemetry::context::Context{});
            std::printf("Operation %u/%u: %u * %u = %u\n", operation + 1, operationCount, 20, 20, product);
        }
    }

    client.Close();

    Telemetry::OpenTelemetry::Instance().Deinitialize();

    char c;
    std::cin >> c;

    return 0;
}
