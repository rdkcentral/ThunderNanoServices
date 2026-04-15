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
            QualityAssurance::JBenchmark::Register(*this, _benchmark);

            _benchmark->Register(&_benchmarkNotification);

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

            if (_memory == nullptr) {
                result = _T("Benchmark could not instantiate a Memory observer!");
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
            _benchmark->Unregister(&_benchmarkNotification);

            QualityAssurance::JBenchmark::Unregister(*this);

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

    void Benchmark::BenchmarkCompleted()
    {
        QualityAssurance::JBenchmark::Event::PerformanceCheckCompleted(*this);
    }

}
}
