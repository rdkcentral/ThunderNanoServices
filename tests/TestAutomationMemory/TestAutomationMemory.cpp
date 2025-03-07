/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
 
#include "TestAutomationMemory.h"


namespace Thunder {
namespace TestAutomationMemory {
    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection)
    {
        class MemoryObserverImpl : public Exchange::IMemory {
        public:
            MemoryObserverImpl() = delete;
            MemoryObserverImpl(const MemoryObserverImpl&) = delete;
            MemoryObserverImpl& operator=(const MemoryObserverImpl&) = delete;

            MemoryObserverImpl(const RPC::IRemoteConnection* connection)
                : _main(connection  == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId())
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

        Exchange::IMemory* result = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);

        return (result);
    }
}
namespace Plugin {

    namespace {

        static Metadata<TestAutomationMemory> metadata(
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

    const string TestAutomationMemory::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_memory == nullptr);
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_connectionId == 0);


        _service = service;
        _service->AddRef();

        _service->Register(&_notification);

        _memoryTestInterface = _service->Root<QualityAssurance::IMemory>(_connectionId, 2000, _T("TestAutomationMemoryImplementation"));

        string result;
        if (_memoryTestInterface == nullptr) {
            result = _T("Couldn't create TestAutomationMemory instance");
        } else {
            QualityAssurance::JMemory::Register(*this, _memoryTestInterface);

            // If we are configured to run OOP, the _connectionId != 0!
            if (_connectionId == 0) {
                _memory = Thunder::TestAutomationMemory::MemoryObserver(nullptr);
            }
            else {
                const RPC::IRemoteConnection *connection = _service->RemoteConnection(_connectionId);
                if (connection == nullptr) {
                    result = _T("Test Automation Memory crashed at initialize!");
                }
                else {
                    _memory = Thunder::TestAutomationMemory::MemoryObserver(connection);
                    ASSERT(_memory != nullptr);
                    connection->Release();
                }
            }
            if (_memory == nullptr) {
                result = _T("Test Automation Memory could not intantiate a Memory observer!");
            }

            // this->AddRef();
        }
        
        return (result);
    }


    void TestAutomationMemory::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        
        ASSERT(_service == service);

        _service->Unregister(&_notification);

        if (_memory != nullptr) {
            _memory->Release();
            _memory = nullptr;
        }

        QualityAssurance::JMemory::Unregister(*this);
        if (_memoryTestInterface != nullptr) {
            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            VARIABLE_IS_NOT_USED uint32_t result =  _memoryTestInterface->Release();
        
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

            if(connection != nullptr){
                connection->Terminate();
                connection->Release();
            }
            
            _memoryTestInterface = nullptr;
        }
        
        _service->Release();
        _service = nullptr;
        _connectionId = 0;        
    }

    string TestAutomationMemory::Information() const
    {
        return string();
    }
    
    void TestAutomationMemory::Deactivated(RPC::IRemoteConnection* connection)
    {
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (_connectionId == connection->Id()) {

            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
   
}
}
