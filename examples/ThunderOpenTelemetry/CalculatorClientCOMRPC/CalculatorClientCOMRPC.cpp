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
#include <opentelemetry/opentelemetry.h>

#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <mutex>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace {

    using namespace Thunder;

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

    string config{ R"({"sampler":"AlwaysOn", "loggerenabled" : true ,"componentname":"Thunder COMRPC test app","componentversion":"1.0.1"})" };

    Telemetry::OpenTelemetry::Config telemetryConfig;
    telemetryConfig.FromString(config);
    Telemetry::OpenTelemetry::Configure(telemetryConfig);
    Telemetry::OpenTelemetry::Instance().Initialize();

    auto tracestate = opentelemetry::trace::TraceState::GetDefault();
    tracestate = tracestate->Set("RDK", "tracevalue");
    string tracestatesheader = tracestate->ToHeader();
    Core::JobContext::Instance().TraceState(tracestatesheader);

    {
        auto scope = Telemetry::OpenTelemetry::Instance().StartScopeFromJobContext();

		Telemetry::OpenTelemetry::Instance().Logger()->Log(opentelemetry::logs::Severity::kInfo, "trace state set");
    }

    CalculatorExtendedClientCOMRPC client;
    uint32_t result = client.Open(callsign);
    if (result != Thunder::Core::ERROR_NONE) {
        std::fprintf(stderr, "Failed to connect to %s: %u\n", callsign.c_str(), result);
        return 5;
    }

	auto span = Telemetry::OpenTelemetry::Instance().StartSpan("COMRPC Application Multiply");
    (*span)->SetAttribute("Multiply","20x20");

    result = client.Multiply(20, 20);
    if (result != Thunder::Core::ERROR_NONE) {
        std::fprintf(stderr, "Multiply failed: %u\n", result);
        return 6;
    }

    uint32_t product = 0;
    if (client.WaitForResult(product) == false) {
        std::fprintf(stderr, "Timed out waiting for MultiplyResult.\n");
        return 7;
    }

    span->End();

    std::printf("%u * %u = %u\n", 20, 20, product);

    client.Close();

    return 0;
}
