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

#pragma once

#include "Module.h"
#include <qa_interfaces/IBenchmark.h>
#include <interfaces/IMemory.h>
#include <qa_interfaces/IBenchmarkPayload.h>
#include <atomic>
#include <map>
#include <vector>

namespace Thunder {
namespace Plugin {

    class Benchmark : public PluginHost::IPlugin
                    , public PluginHost::JSONRPC
                    , public QualityAssurance::IBenchmark {
    private:
        class Notification : public RPC::IRemoteConnection::INotification {
        public:
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;
            Notification(Notification&&) = delete;
            Notification& operator=(Notification&&) = delete;

            explicit Notification(Benchmark& parent)
                : _parent(parent)
            {
            }
            ~Notification() override = default;

        public:
            void Activated(RPC::IRemoteConnection* /* connection */) override
            {
            }
            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }
            void Terminated(RPC::IRemoteConnection* /* connection */) override
            {
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            Benchmark& _parent;
        };

    public:
        Benchmark(const Benchmark&) = delete;
        Benchmark& operator=(const Benchmark&) = delete;
        Benchmark(Benchmark&&) = delete;
        Benchmark& operator=(Benchmark&&) = delete;

        Benchmark()
            : _benchmark(nullptr)
            , _connectionId(0)
            , _memory(nullptr)
            , _service(nullptr)
            , _notification(*this)
            , _payloadProxy(nullptr)
            , _maxLatencyDeviationPct(0)
            , _maxMemoryGrowthBytes(0)
        {
        }

        ~Benchmark() override = default;

        BEGIN_INTERFACE_MAP(Benchmark)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(QualityAssurance::IBenchmark)
            INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        END_INTERFACE_MAP

        // IPlugin
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        // IBenchmark
        Core::hresult Trigger(const uint32_t iterations) override;
        Core::hresult CollectData(QualityAssurance::IBenchmark::IBenchmarkResultIterator*& report) const override;
        Core::hresult LatencyThreshold(const uint32_t maxLatencyDeviationPct) override;
        Core::hresult LatencyThreshold(uint32_t& maxLatencyDeviationPct) const override;
        Core::hresult MemoryThreshold(const uint64_t maxMemoryGrowthBytes) override;
        Core::hresult MemoryThreshold(uint64_t& maxMemoryGrowthBytes) const override;
        Core::hresult SetBaseline(QualityAssurance::IBenchmark::IBenchmarkResultIterator* baseline) override;
        Core::hresult Register(QualityAssurance::IBenchmark::INotification* sink) override;
        Core::hresult Unregister(QualityAssurance::IBenchmark::INotification* sink) override;

    private:
        void Deactivated(RPC::IRemoteConnection* connection);
        void RunPayloadBenchmarks(uint32_t iterations);
        void ApplyThresholds();

    private:
        QualityAssurance::IBenchmark* _benchmark;
        uint32_t _connectionId;
        Exchange::IMemory* _memory;
        PluginHost::IShell* _service;
        Core::SinkType<Notification> _notification;
        QualityAssurance::IBenchmarkPayload* _payloadProxy;
        mutable Core::CriticalSection _adminLock;
        std::vector<QualityAssurance::IBenchmark::BenchmarkResult> _results;
        std::map<string, QualityAssurance::IBenchmark::BenchmarkResult> _baselines;
        std::vector<QualityAssurance::IBenchmark::INotification*> _notifications;
        uint32_t _maxLatencyDeviationPct;
        uint64_t _maxMemoryGrowthBytes;
        std::atomic_flag _triggerRunning = ATOMIC_FLAG_INIT;
    };

} // namespace Plugin
} // namespace Thunder
