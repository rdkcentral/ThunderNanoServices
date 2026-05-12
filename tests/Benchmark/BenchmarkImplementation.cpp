/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Metrological
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
#include <qa_interfaces/IBenchmark.h>
#include <qa_interfaces/IBenchmarkPayloadCOMRPC.h>

#include <algorithm>
#include <vector>

namespace Thunder {
namespace Plugin {

    class BenchmarkImplementation
        : public QualityAssurance::IBenchmark
        , public QualityAssurance::IBenchmarkPayload {
    public:
        BenchmarkImplementation(const BenchmarkImplementation&) = delete;
        BenchmarkImplementation& operator=(const BenchmarkImplementation&) = delete;
        BenchmarkImplementation(BenchmarkImplementation&&) = delete;
        BenchmarkImplementation& operator=(BenchmarkImplementation&&) = delete;

        BenchmarkImplementation()
            : _adminLock()
            , _notifications()
        {
        }

        ~BenchmarkImplementation() override = default;

        BEGIN_INTERFACE_MAP(BenchmarkImplementation)
            INTERFACE_ENTRY(QualityAssurance::IBenchmark)
            INTERFACE_ENTRY(QualityAssurance::IBenchmarkPayload)
        END_INTERFACE_MAP

        // -------------------------------------------------------------------
        // IBenchmark
        // -------------------------------------------------------------------

        Core::hresult Trigger(const uint32_t iterations, bool& success) override
        {
            // Measurement is performed on the plugin shell side via the IBenchmarkPayload COM-RPC proxy.
            // This method is retained for interface compliance and notification forwarding.
            _adminLock.Lock();
            std::vector<IBenchmark::INotification*> sinks(_notifications);
            for (auto* sink : sinks) {
                sink->AddRef();
            }
            _adminLock.Unlock();

            for (auto* sink : sinks) {
                sink->PerformanceCheckCompleted();
                sink->Release();
            }

            success = true;
            TRACE(Trace::Information, (_T("Benchmark Trigger called: %u iterations (measurement on shell side)"), iterations));
            return Core::ERROR_NONE;
        }

        Core::hresult CollectData(IBenchmarkResultIterator*& report) const override
        {
            // Results are stored on the plugin shell side.
            std::vector<BenchmarkResult> empty;
            using Iterator = RPC::IteratorType<IBenchmarkResultIterator>;
            report = Core::ServiceType<Iterator>::Create<IBenchmarkResultIterator>(empty);

            return Core::ERROR_NONE;
        }

        Core::hresult Register(const IBenchmark::INotification* sink) override
        {
            _adminLock.Lock();
            auto it = std::find(_notifications.begin(), _notifications.end(), sink);
            if (it == _notifications.end()) {
                _notifications.push_back(const_cast<IBenchmark::INotification*>(sink));
                sink->AddRef();
            }
            _adminLock.Unlock();
            return Core::ERROR_NONE;
        }

        Core::hresult Unregister(const IBenchmark::INotification* sink) override
        {
            _adminLock.Lock();
            auto it = std::find(_notifications.begin(), _notifications.end(), sink);
            if (it != _notifications.end()) {
                (*it)->Release();
                _notifications.erase(it);
            }
            _adminLock.Unlock();
            return Core::ERROR_NONE;
        }

        Core::hresult SetThreshold(const float /* maxLatencyDeviationPct */, const uint64_t /* maxMemoryGrowthBytes */, bool& success) override
        {
            // Thresholds are managed on the plugin shell side.
            success = true;
            return Core::ERROR_NONE;
        }

        // -------------------------------------------------------------------
        // IBenchmarkPayload — echo/loopback endpoints for COM-RPC measurement
        // -------------------------------------------------------------------

        uint32_t GetPayloadTypes(IPayloadTypeIterator*& types) const override
        {
            std::vector<PayloadType> payloads = {
                PAYLOAD_SMALL, PAYLOAD_MEDIUM, PAYLOAD_LARGE
            };
            using Iterator = RPC::IteratorType<IPayloadTypeIterator>;
            types = Core::ServiceType<Iterator>::Create<IPayloadTypeIterator>(payloads);
            return Core::ERROR_NONE;
        }

        uint32_t SendUint16(const uint16_t /* value */) override { return Core::ERROR_NONE; }
        uint32_t SendUint32(const uint32_t /* value */) override { return Core::ERROR_NONE; }
        uint32_t SendUint64(const uint64_t /* value */) override { return Core::ERROR_NONE; }
        uint32_t SendBool(const bool /* value */) override { return Core::ERROR_NONE; }
        uint32_t SendFloat(const float /* value */) override { return Core::ERROR_NONE; }
        uint32_t SendDouble(const double /* value */) override { return Core::ERROR_NONE; }
        uint32_t SendString(const string& /* value */) override { return Core::ERROR_NONE; }

        uint32_t SendSampleData(const QualityAssurance::SampleData& /* data */) override
        {
            return Core::ERROR_NONE;
        }

        uint32_t SendWithNoParameters() override { return Core::ERROR_NONE; }

        uint32_t SendBuffer(const uint16_t /* bufferSize */, const uint8_t[] /* buffer */) override
        {
            return Core::ERROR_NONE;
        }

        uint32_t SendReceiveUint16(const uint16_t input, uint16_t& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        uint32_t SendReceiveUint32(const uint32_t input, uint32_t& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        uint32_t SendReceiveUint64(const uint64_t input, uint64_t& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        uint32_t SendReceiveBool(const bool input, bool& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        uint32_t SendReceiveFloat(const float input, float& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        uint32_t SendReceiveDouble(const double input, double& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        uint32_t SendReceiveString(const string& input, string& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        uint32_t SendReceiveSampleData(const QualityAssurance::SampleData& input, QualityAssurance::SampleData& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        uint32_t SendReceiveBuffer(uint16_t& bufferSize, uint8_t buffer[]) const override
        {
            // Echo — buffer and size remain unchanged
            (void)bufferSize;
            (void)buffer;
            return Core::ERROR_NONE;
        }

        uint32_t Add(const uint32_t a, const uint32_t b, uint32_t& result) const override
        {
            result = a + b;
            return Core::ERROR_NONE;
        }

    private:
        mutable Core::CriticalSection _adminLock;
        std::vector<IBenchmark::INotification*> _notifications;
    };

    SERVICE_REGISTRATION(BenchmarkImplementation, 1, 0)

} // namespace Plugin
} // namespace Thunder
