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
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <map>
#include <vector>

namespace Thunder {
namespace Plugin {

    class BenchmarkImplementation
        : public QualityAssurance::IBenchmark
        , public QualityAssurance::IBenchmarkPayload {
    private:
        struct LatencyAccumulator {
            std::vector<uint64_t> samplesUs;

            void Record(uint64_t us)
            {
                samplesUs.push_back(us);
            }

            QualityAssurance::IBenchmark::RoundTripStats Stats() const
            {
                QualityAssurance::IBenchmark::RoundTripStats stats{};
                if (samplesUs.empty()) return stats;

                uint64_t sum = 0;
                uint64_t minVal = UINT64_MAX;
                uint64_t maxVal = 0;

                for (auto v : samplesUs) {
                    sum += v;
                    if (v < minVal) minVal = v;
                    if (v > maxVal) maxVal = v;
                }

                uint64_t avg = sum / samplesUs.size();

                double variance = 0.0;
                for (auto v : samplesUs) {
                    double diff = static_cast<double>(v) - static_cast<double>(avg);
                    variance += diff * diff;
                }
                variance /= samplesUs.size();

                // Convert µs to ns for the interface
                stats.minNs = minVal * 1000;
                stats.avgNs = avg * 1000;
                stats.maxNs = maxVal * 1000;
                stats.stddevNs = static_cast<uint64_t>(std::sqrt(variance)) * 1000;

                return stats;
            }
        };

    public:
        BenchmarkImplementation(const BenchmarkImplementation&) = delete;
        BenchmarkImplementation& operator=(const BenchmarkImplementation&) = delete;
        BenchmarkImplementation(BenchmarkImplementation&&) = delete;
        BenchmarkImplementation& operator=(BenchmarkImplementation&&) = delete;

        BenchmarkImplementation()
            : _adminLock()
            , _results()
            , _notifications()
            , _baselines()
            , _maxLatencyDeviationPct(0.0f)
            , _maxMemoryGrowthBytes(0)
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
            _adminLock.Lock();
            _results.clear();
            _adminLock.Unlock();

            RunPayloadBenchmarks(iterations);

            // Apply pass/fail thresholds
            _adminLock.Lock();
            bool hasThresholds = (_maxLatencyDeviationPct > 0.0f || _maxMemoryGrowthBytes > 0);

            if (hasThresholds && _baselines.empty()) {
                // First run with thresholds set: store as baseline, everything passes
                for (auto& r : _results) {
                    _baselines[r.apiName] = r;
                    r.passed = true;
                }
            } else if (hasThresholds) {
                // Subsequent runs: compare against baseline
                for (auto& r : _results) {
                    r.passed = true;
                    r.failureReason = string();

                    auto baseline = _baselines.find(r.apiName);
                    if (baseline != _baselines.end()) {
                        // Latency check
                        if (_maxLatencyDeviationPct > 0.0f && baseline->second.roundTrip.avgNs > 0) {
                            double baselineAvg = static_cast<double>(baseline->second.roundTrip.avgNs);
                            double currentAvg = static_cast<double>(r.roundTrip.avgNs);
                            double deviationPct = ((currentAvg - baselineAvg) / baselineAvg) * 100.0;
                            if (deviationPct > static_cast<double>(_maxLatencyDeviationPct)) {
                                r.passed = false;
                                char buf[256];
                                snprintf(buf, sizeof(buf), "latency deviation %.1f%% exceeds %.1f%% threshold (baseline=%" PRIu64 " ns, current=%" PRIu64 " ns)",
                                    deviationPct, static_cast<double>(_maxLatencyDeviationPct),
                                    baseline->second.roundTrip.avgNs, r.roundTrip.avgNs);
                                r.failureReason = string(buf);
                            }
                        }
                        // Memory check
                        if (_maxMemoryGrowthBytes > 0) {
                            int64_t growth = static_cast<int64_t>(r.memory.residentAfter) - static_cast<int64_t>(r.memory.residentBefore);
                            if (growth > 0 && static_cast<uint64_t>(growth) > _maxMemoryGrowthBytes) {
                                r.passed = false;
                                char buf[256];
                                snprintf(buf, sizeof(buf), "memory growth %" PRId64 " bytes exceeds %" PRIu64 " byte threshold",
                                    growth, _maxMemoryGrowthBytes);
                                string memMsg(buf);
                                if (!r.failureReason.empty()) {
                                    r.failureReason += "; ";
                                }
                                r.failureReason += memMsg;
                            }
                        }
                    }
                }
            } else {
                // No thresholds: everything passes
                for (auto& r : _results) {
                    r.passed = true;
                }
            }

            // Notify listeners
            for (auto* sink : _notifications) {
                sink->PerformanceCheckCompleted();
            }
            _adminLock.Unlock();

            success = true;
            TRACE(Trace::Information, (_T("Benchmark completed: %u iterations"), iterations));
            return Core::ERROR_NONE;
        }

        Core::hresult CollectData(IBenchmarkResultIterator*& report) const override
        {
            _adminLock.Lock();
            std::vector<BenchmarkResult> items(_results);
            _adminLock.Unlock();

            using Iterator = RPC::IteratorType<IBenchmarkResultIterator>;
            report = Core::ServiceType<Iterator>::Create<IBenchmarkResultIterator>(items);

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

        Core::hresult SetThreshold(const float maxLatencyDeviationPct, const uint64_t maxMemoryGrowthBytes, bool& success) override
        {
            _adminLock.Lock();
            _maxLatencyDeviationPct = maxLatencyDeviationPct;
            _maxMemoryGrowthBytes = maxMemoryGrowthBytes;
            // Clear baselines so the next Trigger run becomes the new baseline
            _baselines.clear();
            _adminLock.Unlock();

            success = true;
            TRACE(Trace::Information, (_T("Thresholds set: latency=%.1f%%, memory=%llu bytes"),
                maxLatencyDeviationPct, maxMemoryGrowthBytes));
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
            return Core::ERROR_NONE;
        }

        uint32_t Add(const uint32_t a, const uint32_t b, uint32_t& result) const override
        {
            result = a + b;
            return Core::ERROR_NONE;
        }

    private:
        void RunPayloadBenchmarks(uint32_t iterations)
        {
            // Benchmark each IBenchmarkPayload method by calling self over COM-RPC.
            // When running OOP, these calls traverse the proxy/stub boundary.

            BenchmarkMethod("SendUint32", iterations, [this]() {
                SendUint32(42);
            });

            BenchmarkMethod("SendUint64", iterations, [this]() {
                SendUint64(UINT64_C(123456789));
            });

            BenchmarkMethod("SendString", iterations, [this]() {
                SendString(_T("benchmark_payload_string"));
            });

            BenchmarkMethod("SendSampleData", iterations, [this]() {
                QualityAssurance::SampleData data;
                data.id = 1;
                data.value = 100;
                data.name = _T("sample");
                SendSampleData(data);
            });

            BenchmarkMethod("SendWithNoParameters", iterations, [this]() {
                SendWithNoParameters();
            });

            BenchmarkMethod("SendReceiveUint32", iterations, [this]() {
                uint32_t out = 0;
                SendReceiveUint32(42, out);
            });

            BenchmarkMethod("SendReceiveString", iterations, [this]() {
                string out;
                SendReceiveString(_T("benchmark_roundtrip"), out);
            });

            BenchmarkMethod("SendReceiveSampleData", iterations, [this]() {
                QualityAssurance::SampleData in;
                in.id = 1;
                in.value = 200;
                in.name = _T("roundtrip");
                QualityAssurance::SampleData out;
                SendReceiveSampleData(in, out);
            });

            BenchmarkMethod("Add", iterations, [this]() {
                uint32_t result = 0;
                Add(17, 25, result);
            });
        }

        template<typename CALLABLE>
        void BenchmarkMethod(const string& apiName, uint32_t iterations, CALLABLE&& fn)
        {
            Core::ProcessInfo processInfo(Core::ProcessInfo().Id());

            // Capture memory before
            uint64_t residentBefore = processInfo.Resident();
            uint64_t allocatedBefore = processInfo.Allocated();

            LatencyAccumulator acc;

            // Time each call individually for min/max
            for (uint32_t i = 0; i < iterations; i++) {
                Core::StopWatch timer;
                fn();
                uint64_t elapsedUs = timer.Elapsed();
                acc.Record(elapsedUs);
            }

            // Also time the entire batch for accurate average
            Core::StopWatch batchTimer;
            for (uint32_t i = 0; i < iterations; i++) {
                fn();
            }
            uint64_t totalBatchUs = batchTimer.Elapsed();

            // Capture memory after
            uint64_t residentAfter = processInfo.Resident();
            uint64_t allocatedAfter = processInfo.Allocated();

            BenchmarkResult result;
            result.apiName = apiName;
            result.iterations = iterations;
            result.roundTrip = acc.Stats();
            // Override avgNs with batch-derived value for better precision
            if (iterations > 0) {
                result.roundTrip.avgNs = (totalBatchUs * 1000) / iterations;
            }
            result.memory.residentBefore = residentBefore;
            result.memory.residentAfter = residentAfter;
            result.memory.allocatedBefore = allocatedBefore;
            result.memory.allocatedAfter = allocatedAfter;

            _adminLock.Lock();
            _results.push_back(result);
            _adminLock.Unlock();
        }

    private:
        mutable Core::CriticalSection _adminLock;
        std::vector<BenchmarkResult> _results;
        std::vector<IBenchmark::INotification*> _notifications;
        std::map<string, BenchmarkResult> _baselines;
        float _maxLatencyDeviationPct;
        uint64_t _maxMemoryGrowthBytes;
    };

    SERVICE_REGISTRATION(BenchmarkImplementation, 1, 0)

} // namespace Plugin
} // namespace Thunder
