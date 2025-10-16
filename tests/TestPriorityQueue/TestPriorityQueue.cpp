/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 [PLEASE ADD COPYRIGHT NAME!]
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

#include "TestPriorityQueue.h"

namespace Thunder {
namespace Plugin {
    
    namespace {
        
        static Metadata<TestPriorityQueue>metadata(
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

    const string TestPriorityQueue::Initialize(PluginHost::IShell* service)
    {
        string message;

        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_implMath == nullptr);
        ASSERT(_connectionId == 0);

        _service = service;
        _service->AddRef();
        _service->Register(&_notification);
        
        _implMath = service->Root<Exchange::IMath>(_connectionId, timeout, _T("TestPriorityQueueImplementation"));
        if (_implMath == nullptr) {
            message = _T("Couldn't create instance of implMath");
        }
        /*else {
            StartFlood(100000, std::max(1u, std::thread::hardware_concurrency()));
        }*/

        return (message);
    }

    void TestPriorityQueue::Deinitialize(PluginHost::IShell* service)
    {
        //StopFlood();

        ASSERT(_service == service);
        
        _service->Unregister(&_notification);
        
        if (_implMath != nullptr) {
            
            RPC::IRemoteConnection* connection(service->RemoteConnection(_connectionId));
            VARIABLE_IS_NOT_USED uint32_t result = _implMath->Release();
            _implMath = nullptr;
            
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
            
            // The process can disappear in the meantime...
            if (connection != nullptr) {
                // But if it did not disappear in the meantime, forcefully terminate it. Shoot to kill
                connection->Terminate();
                connection->Release();
            }
        }
        _service->Release();
        _service = nullptr;
        _connectionId = 0;
    }
    
    string TestPriorityQueue::Information() const {
        return (string());
    }
    
    void TestPriorityQueue::StartFlood(uint32_t totalCalls, uint32_t parallelism)
    {
        StopFlood();

        _stop.store(false, std::memory_order_release);

        const uint32_t perThread = (totalCalls / parallelism);
        _workers.reserve(parallelism);

        for (uint32_t i = 0; i < parallelism; ++i) {
            _workers.emplace_back([this, perThread]() {
                uint16_t res = 0;

                for (uint32_t n = 0; ((n < perThread) && (_stop.load(std::memory_order_acquire) == false)); ++n) {
                    const uint16_t a = static_cast<uint16_t>(n * 2);
                    const uint16_t b = static_cast<uint16_t>(n);
                    _implMath->Add(a, b, res);
                    _implMath->Sub(a, b, res);
                }
            });
        }
    }

    void TestPriorityQueue::StopFlood()
    {
        _stop.store(true, std::memory_order_release);

        for (auto& w : _workers) {
            if (w.joinable()) {
                w.join();
            }
        }
        _workers.clear();
    }

    void TestPriorityQueue::Deactivated(RPC::IRemoteConnection* connection) {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // Plugin
} // Thunder