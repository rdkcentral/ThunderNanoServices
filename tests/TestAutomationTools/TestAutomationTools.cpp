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
 
#include "TestAutomationTools.h"


namespace WPEFramework {
namespace TestAutomationTools {
    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection)
    {
        class MemoryObserverImpl : public Exchange::IMemory {
        public:
            MemoryObserverImpl() = delete;
            MemoryObserverImpl(const MemoryObserverImpl&) = delete;
            MemoryObserverImpl& operator=(const MemoryObserverImpl&) = delete;

            MemoryObserverImpl(const RPC::IRemoteConnection* connection)
                : _main(connection  == 0 ? Core::ProcessInfo().Id() : connection->RemoteId())
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

        static Metadata<TestAutomationTools> metadata(
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
    
    
    void TestAutomationTools::Register(Exchange::ITestAutomationTools::INotification* notification)
    {
        ASSERT(notification);
        _adminLock.Lock();
        notification->AddRef();
        _notifications.push_back(notification);
        _adminLock.Unlock();
    }

    void TestAutomationTools::Unregister(const Exchange::ITestAutomationTools::INotification* notification)
    {
        ASSERT(notification);
        _adminLock.Lock();
        auto item = std::find(_notifications.begin(), _notifications.end(), notification);
        ASSERT(item != _notifications.end());
        _notifications.erase(item);
        (*item)->Release();
        _adminLock.Unlock();
    }

    const string TestAutomationTools::Initialize(PluginHost::IShell* service)
    {
        ASSERT (_service == nullptr);
        ASSERT (service != nullptr);
        ASSERT(_memory == nullptr);

        _service = service;
        _service->AddRef(); 
        _service->Register(&_Notification);

        _implementation = _service->Root<Exchange::ITestAutomationToolsInternal>(_connectionId, 2000, _T("TestAutomationToolsImplementation"));

        string result;
        if (_implementation == nullptr) {
            result = _T("Couldn't create TestAutomationTools instance");
        } else {
            Exchange::JTestAutomationTools::Register(*this, this);
        }
        const RPC::IRemoteConnection *connection = _service->RemoteConnection(_connectionId);

        if (connection != nullptr) {

            _memory = WPEFramework::TestAutomationTools::MemoryObserver(connection);
            ASSERT(_memory != nullptr);

            connection->Release();
        }
        else {
            result = _T("Test Automation Tools crashed at initialize!");
        }
        
        return (result);
    }


    void TestAutomationTools::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ASSERT(_service == service);

        service->Unregister(&_Notification);
        Exchange::JTestAutomationTools::Unregister(*this);
        if (_implementation != nullptr) {
            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            VARIABLE_IS_NOT_USED uint32_t result =  _implementation->Release();
        
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

            if(connection != nullptr){
                connection->Terminate();
                connection->Release();
            }
            _implementation = nullptr;
        }
        
        _service->Release();
        _service = nullptr;
        
    }

    string TestAutomationTools::Information() const
    {
        return string();
    }

    
    void TestAutomationTools::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }


    Core::hresult TestAutomationTools::AllocateMemory(const uint32_t size)
    {
        ASSERT(_implementation != nullptr);
        TRACE(Trace::Information, (_T("Allocate Memory Called")));
        return _implementation->IncreaseMemory(size);

    }

    Core::hresult TestAutomationTools::FreeAllocatedMemory()
    {
        ASSERT(_implementation != nullptr);
        TRACE(Trace::Information, (_T("Free Allocated Memory Called")));
        return _implementation->FreeMemory();
        
    }

    Core::hresult TestAutomationTools::TestBigString(const uint32_t length)
    {
        ASSERT(_implementation != nullptr);
        const uint32_t stringLength = length * 1024;
        string bigString;
        bigString.resize(stringLength, 'a');
        bigString.replace(0, 8, "testaaaa");
        bigString.replace(bigString.length() - 8, 8, "testzzzz");
 
        TRACE(Trace::Information, (_T("InP: Length Of The String Is " + std::to_string(bigString.length()))));
        TRACE(Trace::Information, (_T("InP: Content Of The String Is " + bigString)));

        return _implementation->BigStringTest(bigString);
     
    }

   
}
}
