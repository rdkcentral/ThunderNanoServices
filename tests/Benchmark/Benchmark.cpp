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

#include "Benchmark.h"
#include <qa_interfaces/json/JBenchmark.h>

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <time.h>

namespace Thunder {
namespace BenchmarkMemory {
    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection)
    {
        class MemoryObserverImpl : public Exchange::IMemory {
        public:
            MemoryObserverImpl() = delete;
            MemoryObserverImpl(const MemoryObserverImpl&) = delete;
            MemoryObserverImpl& operator=(const MemoryObserverImpl&) = delete;

            MemoryObserverImpl(const RPC::IRemoteConnection* connection)
                : _main(connection == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId())
            {
            }
            ~MemoryObserverImpl() = default;

        public:
            uint64_t Resident() const override
            {
                return _main.Resident();
            }
            uint64_t Allocated() const override
            {
                return _main.Allocated();
            }
            uint64_t Shared() const override
            {
                return _main.Shared();
            }
            uint8_t Processes() const override
            {
                return (IsOperational() ? 1 : 0);
            }
            bool IsOperational() const override
            {
                return _main.IsActive();
            }

            BEGIN_INTERFACE_MAP(MemoryObserverImpl)
                INTERFACE_ENTRY(Exchange::IMemory)
            END_INTERFACE_MAP

        private:
            Core::ProcessInfo _main;
        };

        Exchange::IMemory* result = Core::ServiceType<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);
        return (result);
    }
}
namespace Plugin {

    namespace {

        static Metadata<Benchmark> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    namespace {

        struct LatencyAccumulator {
            std::vector<uint64_t> samplesNs;  // true nanosecond samples

            void Record(uint64_t ns)
            {
                samplesNs.push_back(ns);
            }

            QualityAssurance::IBenchmark::RoundTripStats Stats() const
            {
                QualityAssurance::IBenchmark::RoundTripStats stats{};
                if (samplesNs.empty()) return stats;

                uint64_t sum = 0;
                uint64_t minVal = UINT64_MAX;
                uint64_t maxVal = 0;

                for (auto v : samplesNs) {
                    sum += v;
                    if (v < minVal) minVal = v;
                    if (v > maxVal) maxVal = v;
                }

                const uint64_t n = samplesNs.size();
                const uint64_t avg = sum / n;

                double variance = 0.0;
                for (auto v : samplesNs) {
                    double diff = static_cast<double>(v) - static_cast<double>(avg);
                    variance += diff * diff;
                }
                // sample variance (n-1); falls back to n when n==1 (n>0 guaranteed by early return above)
                variance /= static_cast<double>(n > 1 ? n - 1 : n);

                stats.minNs    = minVal;
                stats.avgNs    = avg;
                stats.maxNs    = maxVal;
                stats.stddevNs = static_cast<uint64_t>(std::sqrt(variance));

                return stats;
            }
        };

        // Read CLOCK_MONOTONIC directly for true nanosecond resolution.
        // Core::StopWatch uses SystemInfo::Ticks() which returns microseconds;
        // multiplying by 1000 gives µs-precision values dressed as ns.
        // Returns 0 on clock_gettime failure; callers must treat 0 as invalid.
        inline uint64_t NowNs()
        {
            struct timespec ts{};
            if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
                return 0;
            }
            return static_cast<uint64_t>(ts.tv_sec) * UINT64_C(1000000000) + static_cast<uint64_t>(ts.tv_nsec);
        }

        template<typename CALLABLE>
        QualityAssurance::IBenchmark::BenchmarkResult MeasurePayloadMethod(
            const string& apiName, uint32_t iterations, Exchange::IMemory* memory, CALLABLE&& fn)
        {
            ASSERT(memory != nullptr);

            uint64_t residentBefore = memory->Resident();
            uint64_t allocatedBefore = memory->Allocated();

            LatencyAccumulator acc;
            uint32_t samplesRecorded = 0;   // iterations with a valid timing sample in acc
            uint32_t fnCallsSucceeded = 0;  // fn() calls that returned ERROR_NONE

            for (uint32_t i = 0; i < iterations; i++) {
                const uint64_t t0 = NowNs();
                uint32_t hr = fn();
                const uint64_t t1 = NowNs();
                if (hr != Core::ERROR_NONE) {
                    SYSLOG(Logging::Error, (_T("COM-RPC call '%s' failed on iteration %u with error 0x%08X"),
                        apiName.c_str(), i, hr));
                    break;
                }
                fnCallsSucceeded++;
                // Skip sample if either clock read failed (returns 0) or the clock regressed.
                // Each iteration pays two clock_gettime calls (t0 and t1); for very fast
                // in-process calls this overhead is a significant part of the measured interval.
                if (t0 == 0 || t1 == 0 || t1 < t0) {
                    SYSLOG(Logging::Error, (_T("Clock anomaly on '%s' iteration %u -- sample skipped"),
                        apiName.c_str(), i));
                    continue;
                }
                acc.Record(t1 - t0);
                samplesRecorded++;
            }

            uint64_t residentAfter = memory->Resident();
            uint64_t allocatedAfter = memory->Allocated();

            QualityAssurance::IBenchmark::BenchmarkResult result{};
            result.apiName = apiName;
            // iterations = number of timing samples in roundTrip (consistent with the stats).
            // passed checks fnCallsSucceeded so a run with clock anomalies (skipped samples)
            // still fails correctly rather than masking the partial data as a success.
            result.iterations = samplesRecorded;
            result.roundTrip = acc.Stats();
            result.passed = (fnCallsSucceeded == iterations);
            result.memory.residentBefore  = residentBefore;
            result.memory.residentAfter   = residentAfter;
            result.memory.allocatedBefore = allocatedBefore;
            result.memory.allocatedAfter  = allocatedAfter;

            return result;
        }

    } // anonymous namespace

    const string Benchmark::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_benchmark == nullptr);
        ASSERT(_memory == nullptr);
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_connectionId == 0);

        _service = service;
        _service->AddRef();

        _service->Register(&_notification);

        _benchmark = _service->Root<QualityAssurance::IBenchmark>(_connectionId, 2000, _T("BenchmarkImplementation"));

        string result;
        if (_benchmark == nullptr) {
            result = _T("Couldn't create Benchmark instance");
        } else {
            _payloadProxy = _benchmark->QueryInterface<QualityAssurance::IBenchmarkPayload>();
            if (_payloadProxy == nullptr) {
                result = _T("Couldn't obtain IBenchmarkPayload interface from Benchmark implementation");
            }

            if (result.empty()) {
                QualityAssurance::JBenchmark::Register(*this, this);

                if (_connectionId == 0) {
                    _memory = Thunder::BenchmarkMemory::MemoryObserver(nullptr);
                } else {
                    const RPC::IRemoteConnection* connection = _service->RemoteConnection(_connectionId);
                    if (connection == nullptr) {
                        result = _T("Benchmark crashed at initialize!");
                    } else {
                        _memory = Thunder::BenchmarkMemory::MemoryObserver(connection);
                        ASSERT(_memory != nullptr);
                        connection->Release();
                    }
                }

                if (_memory == nullptr && result.empty()) {
                    result = _T("Benchmark could not instantiate a Memory observer!");
                }
            }
        }

        return (result);
    }

    void Benchmark::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ASSERT(_service == service);

        _service->Unregister(&_notification);

        if (_memory != nullptr) {
            _memory->Release();
            _memory = nullptr;
        }

        if (_benchmark != nullptr) {
            QualityAssurance::JBenchmark::Unregister(*this);

            _adminLock.Lock();
            for (auto* sink : _notifications) {
                sink->Release();
            }
            _notifications.clear();
            _adminLock.Unlock();

            if (_payloadProxy != nullptr) {
                _payloadProxy->Release();
                _payloadProxy = nullptr;
            }

            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            VARIABLE_IS_NOT_USED uint32_t result = _benchmark->Release();

            ASSERT((result == Core::ERROR_CONNECTION_CLOSED) || (result == Core::ERROR_DESTRUCTION_SUCCEEDED));

            if (connection != nullptr) {
                connection->Terminate();
                connection->Release();
            }

            _benchmark = nullptr;
        }

        _service->Release();
        _service = nullptr;
        _connectionId = 0;
    }

    string Benchmark::Information() const
    {
        return string();
    }

    void Benchmark::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (_connectionId == connection->Id()) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    Core::hresult Benchmark::Trigger(const uint32_t iterations)
    {
        // Reject concurrent triggers: interleaved results would corrupt the result set
        // and produce an incorrect allPassed evaluation.
        if (_triggerRunning.test_and_set()) {
            return Core::ERROR_INPROGRESS;
        }
        _adminLock.Lock();
        _results.clear();
        _adminLock.Unlock();

        RunPayloadBenchmarks(iterations);
        ApplyThresholds();

        _adminLock.Lock();
        std::vector<QualityAssurance::IBenchmark::INotification*> sinks(_notifications);
        for (auto* sink : sinks) sink->AddRef();
        bool allPassed = std::all_of(_results.begin(), _results.end(),
            [](const QualityAssurance::IBenchmark::BenchmarkResult& r) { return r.passed; });
        _adminLock.Unlock();

        for (auto* sink : sinks) {
            sink->PerformanceCheckCompleted();
            sink->Release();
        }

        QualityAssurance::JBenchmark::Event::PerformanceCheckCompleted(*this);

        TRACE(Trace::Information, (_T("Benchmark completed: %u iterations via COM-RPC proxy"), iterations));
        const Core::hresult hr = allPassed ? Core::ERROR_NONE : Core::ERROR_GENERAL;
        _triggerRunning.clear();
        return hr;
    }

    Core::hresult Benchmark::CollectData(QualityAssurance::IBenchmark::IBenchmarkResultIterator*& report) const
    {
        _adminLock.Lock();
        std::vector<QualityAssurance::IBenchmark::BenchmarkResult> snapshot(_results);
        _adminLock.Unlock();

        using Iterator = RPC::IteratorType<QualityAssurance::IBenchmark::IBenchmarkResultIterator>;
        report = Core::ServiceType<Iterator>::Create<QualityAssurance::IBenchmark::IBenchmarkResultIterator>(snapshot);
        return Core::ERROR_NONE;
    }

    Core::hresult Benchmark::LatencyThreshold(const uint32_t maxLatencyDeviationPct)
    {
        _adminLock.Lock();
        _maxLatencyDeviationPct = maxLatencyDeviationPct;
        _baselines.clear();
        _adminLock.Unlock();
        TRACE(Trace::Information, (_T("Latency threshold set: %u millipercent"), maxLatencyDeviationPct));
        return Core::ERROR_NONE;
    }

    Core::hresult Benchmark::LatencyThreshold(uint32_t& maxLatencyDeviationPct) const
    {
        _adminLock.Lock();
        maxLatencyDeviationPct = _maxLatencyDeviationPct;
        _adminLock.Unlock();
        return Core::ERROR_NONE;
    }

    Core::hresult Benchmark::MemoryThreshold(const uint64_t maxMemoryGrowthBytes)
    {
        _adminLock.Lock();
        _maxMemoryGrowthBytes = maxMemoryGrowthBytes;
        _baselines.clear();
        _adminLock.Unlock();
        TRACE(Trace::Information, (_T("Memory threshold set: %" PRIu64 " bytes"), maxMemoryGrowthBytes));
        return Core::ERROR_NONE;
    }

    Core::hresult Benchmark::MemoryThreshold(uint64_t& maxMemoryGrowthBytes) const
    {
        _adminLock.Lock();
        maxMemoryGrowthBytes = _maxMemoryGrowthBytes;
        _adminLock.Unlock();
        return Core::ERROR_NONE;
    }

    Core::hresult Benchmark::Register(QualityAssurance::IBenchmark::INotification* sink)
    {
        if (sink == nullptr) return Core::ERROR_BAD_REQUEST;
        _adminLock.Lock();
        auto it = std::find(_notifications.begin(), _notifications.end(), sink);
        if (it == _notifications.end()) {
            sink->AddRef();
            _notifications.push_back(sink);
        }
        _adminLock.Unlock();
        return Core::ERROR_NONE;
    }

    Core::hresult Benchmark::Unregister(QualityAssurance::IBenchmark::INotification* sink)
    {
        if (sink == nullptr) return Core::ERROR_BAD_REQUEST;
        _adminLock.Lock();
        auto it = std::find(_notifications.begin(), _notifications.end(), sink);
        if (it != _notifications.end()) {
            (*it)->Release();
            _notifications.erase(it);
        }
        _adminLock.Unlock();
        return Core::ERROR_NONE;
    }

    void Benchmark::RunPayloadBenchmarks(uint32_t iterations)
    {
        ASSERT(_payloadProxy != nullptr);

        auto addResult = [this](QualityAssurance::IBenchmark::BenchmarkResult&& r) {
            _adminLock.Lock();
            _results.push_back(std::move(r));
            _adminLock.Unlock();
        };

        addResult(MeasurePayloadMethod("SendUint32", iterations, _memory, [this]() -> uint32_t {
            return _payloadProxy->SendUint32(42);
        }));

        addResult(MeasurePayloadMethod("SendUint64", iterations, _memory, [this]() -> uint32_t {
            return _payloadProxy->SendUint64(UINT64_C(123456789));
        }));

        addResult(MeasurePayloadMethod("GetPayloadTypes", iterations, _memory, [this]() -> uint32_t {
            QualityAssurance::IBenchmarkPayload::IPayloadTypeIterator* iter = nullptr;
            uint32_t hr = _payloadProxy->GetPayloadTypes(iter);
            if (iter != nullptr) {
                iter->Release();
            }
            return hr;
        }));

        addResult(MeasurePayloadMethod("SendString", iterations, _memory, [this]() -> uint32_t {
            return _payloadProxy->SendString(_T("benchmark_payload_string"));
        }));

        addResult(MeasurePayloadMethod("SendSampleData", iterations, _memory, [this]() -> uint32_t {
            QualityAssurance::IBenchmarkPayload::SampleData data;
            data.id = 1;
            data.value = 100;
            data.name = _T("sample");
            return _payloadProxy->SendSampleData(data);
        }));

        addResult(MeasurePayloadMethod("SendNoPayload", iterations, _memory, [this]() -> uint32_t {
            return _payloadProxy->SendNoPayload();
        }));

        addResult(MeasurePayloadMethod("SendReceiveUint32", iterations, _memory, [this]() -> uint32_t {
            uint32_t out = 0;
            return _payloadProxy->SendReceiveUint32(42, out);
        }));

        addResult(MeasurePayloadMethod("SendReceiveString", iterations, _memory, [this]() -> uint32_t {
            string out;
            return _payloadProxy->SendReceiveString(_T("benchmark_roundtrip"), out);
        }));

        addResult(MeasurePayloadMethod("SendReceiveSampleData", iterations, _memory, [this]() -> uint32_t {
            QualityAssurance::IBenchmarkPayload::SampleData in;
            in.id = 1;
            in.value = 200;
            in.name = _T("roundtrip");
            QualityAssurance::IBenchmarkPayload::SampleData out;
            return _payloadProxy->SendReceiveSampleData(in, out);
        }));

        addResult(MeasurePayloadMethod("SendUint32Vector", iterations, _memory, [this]() -> uint32_t {
            const std::vector<uint32_t> data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
            return _payloadProxy->SendUint32Vector(data);
        }));

        addResult(MeasurePayloadMethod("SendReceiveUint32Vector", iterations, _memory, [this]() -> uint32_t {
            const std::vector<uint32_t> input = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
            std::vector<uint32_t> output;
            return _payloadProxy->SendReceiveUint32Vector(input, output);
        }));

        addResult(MeasurePayloadMethod("Add", iterations, _memory, [this]() -> uint32_t {
            uint32_t result = 0;
            return _payloadProxy->Add(17, 25, result);
        }));
    }

    void Benchmark::ApplyThresholds()
    {
        _adminLock.Lock();
        bool hasThresholds = (_maxLatencyDeviationPct > 0 || _maxMemoryGrowthBytes > 0);

        if (hasThresholds && _baselines.empty()) {
            // First run with thresholds: store as baseline
            for (auto& r : _results) {
                _baselines[r.apiName] = r;
                // Preserve passed=false from COM-RPC call failures
            }
        } else if (hasThresholds) {
            // Subsequent runs: compare against baseline
            for (auto& r : _results) {
                if (r.passed == false) {
                    // COM-RPC call failed during measurement; don't override
                    continue;
                }
                r.failureReason.Clear();
                bool latencyFailed = false;
                bool memoryFailed = false;

                auto baseline = _baselines.find(r.apiName);
                if (baseline != _baselines.end()) {
                    // Latency check (threshold is in millipercent: 1000 = 1%)
                    if (_maxLatencyDeviationPct > 0 && baseline->second.roundTrip.avgNs > 0) {
                        double baselineAvg = static_cast<double>(baseline->second.roundTrip.avgNs);
                        double currentAvg = static_cast<double>(r.roundTrip.avgNs);
                        double deviationMillipct = ((currentAvg - baselineAvg) / baselineAvg) * 100000.0;
                        if (deviationMillipct > static_cast<double>(_maxLatencyDeviationPct)) {
                            latencyFailed = true;
                        }
                    }
                    // Memory check
                    if (_maxMemoryGrowthBytes > 0) {
                        int64_t growth = static_cast<int64_t>(r.memory.residentAfter) - static_cast<int64_t>(r.memory.residentBefore);
                        if (growth > 0 && static_cast<uint64_t>(growth) > _maxMemoryGrowthBytes) {
                            memoryFailed = true;
                        }
                    }

                    if (latencyFailed && memoryFailed) {
                        r.passed = false;
                        r.failureReason = QualityAssurance::IBenchmark::LATENCY_AND_MEMORY_THRESHOLD_EXCEEDED;
                    } else if (latencyFailed) {
                        r.passed = false;
                        r.failureReason = QualityAssurance::IBenchmark::LATENCY_THRESHOLD_EXCEEDED;
                    } else if (memoryFailed) {
                        r.passed = false;
                        r.failureReason = QualityAssurance::IBenchmark::MEMORY_THRESHOLD_EXCEEDED;
                    }
                }
            }
        } else {
            // No thresholds: mark as passed only if COM-RPC calls succeeded
            for (auto& r : _results) {
                if (r.passed != false) {
                    r.passed = true;
                }
            }
        }
        _adminLock.Unlock();
    }

}
}
