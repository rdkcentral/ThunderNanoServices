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

namespace Thunder {
namespace Plugin {

    class Benchmark : public PluginHost::IPlugin, public PluginHost::JSONRPC {
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

        class BenchmarkNotification : public QualityAssurance::IBenchmark::INotification {
        public:
            BenchmarkNotification(const BenchmarkNotification&) = delete;
            BenchmarkNotification& operator=(const BenchmarkNotification&) = delete;

            explicit BenchmarkNotification(Benchmark& parent)
                : _parent(parent)
            {
            }
            ~BenchmarkNotification() override = default;

            void PerformanceCheckCompleted() override
            {
                _parent.BenchmarkCompleted();
            }

            BEGIN_INTERFACE_MAP(BenchmarkNotification)
                INTERFACE_ENTRY(QualityAssurance::IBenchmark::INotification)
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
            , _benchmarkNotification(*this)
        {
        }

        ~Benchmark() override = default;

        BEGIN_INTERFACE_MAP(Benchmark)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(QualityAssurance::IBenchmark, _benchmark)
            INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        void Deactivated(RPC::IRemoteConnection* connection);
        void BenchmarkCompleted();

    private:
        QualityAssurance::IBenchmark* _benchmark;
        uint32_t _connectionId;
        Exchange::IMemory* _memory;
        PluginHost::IShell* _service;
        Core::SinkType<Notification> _notification;
        Core::SinkType<BenchmarkNotification> _benchmarkNotification;
    };

} // namespace Plugin
} // namespace Thunder
