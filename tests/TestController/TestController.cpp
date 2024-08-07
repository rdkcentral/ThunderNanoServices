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

#include "TestController.h"

namespace Thunder {
namespace TestController {

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
            ~MemoryObserverImpl() {}

        public:
            uint64_t Resident() const override { return _main.Resident(); }

            uint64_t Allocated() const override { return _main.Allocated(); }

            uint64_t Shared() const override { return _main.Shared(); }

            uint8_t Processes() const override { return (IsOperational() ? 1 : 0); }

            bool IsOperational() const override { return _main.IsActive(); }

            BEGIN_INTERFACE_MAP(MemoryObserverImpl)
            INTERFACE_ENTRY(Exchange::IMemory)
            END_INTERFACE_MAP

        private:
            Core::ProcessInfo _main;
        };

        return (Core::ServiceType<MemoryObserverImpl>::Create<Exchange::IMemory>(connection));
    }
} // namespace TestController

namespace Plugin {
    SERVICE_REGISTRATION(TestController, 1, 0)

    /* virtual */ const string TestController::Initialize(PluginHost::IShell* service)
    {
        /*Assume that everything is OK*/
        string message;
        Config config;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_testControllerImp == nullptr);
        ASSERT(_memory == nullptr);
        ASSERT(_connection == 0);

        _service = service;
        _service->AddRef();
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        _service->Register(&_notification);
        _testControllerImp = _service->Root<QualityAssurance::ITestController>(_connection, ImplWaitTime, _T("TestControllerImp"));

        if (_testControllerImp != nullptr) {
            Thunder::QualityAssurance::JTestController::Register(*this, _testControllerImp);

            _testControllerImp->Setup();
            const RPC::IRemoteConnection *connection = _service->RemoteConnection(_connection);

            if (connection != nullptr) {
                _memory = Thunder::TestController::MemoryObserver(connection);
                ASSERT(_memory != nullptr);
                connection->Release();
               
            } else {
                TRACE(Trace::Warning, (_T("Colud not get Remote connection")));
            }
        } else {
            TRACE(Trace::Fatal, (_T("*** TestController could not be instantiated ***")));
            message = _T("TestController could not be instantiated.");
        }

        return message;
    }

    /* virtual */ void TestController::Deinitialize(VARIABLE_IS_NOT_USED PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            _service->Unregister(&_notification);
 
            if (_testControllerImp != nullptr) {
                Thunder::QualityAssurance::JTestController::Unregister(*this);
                _testControllerImp->TearDown();

                if (_memory != nullptr){
                    if (_memory->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
                        TRACE(Trace::Information, (_T("Memory observer in TestController is not properly destructed")));
                    }
                    _memory = nullptr;
                }

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connection));
                VARIABLE_IS_NOT_USED uint32_t result = _testControllerImp->Release();
                _testControllerImp = nullptr;
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                // The connection can disappear in the meantime...
                if (connection != nullptr) {
                    // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                    connection->Terminate();
                    connection->Release();
                }           
            }
            _service->Release();
            _service = nullptr;
            _connection = 0;
        }
    }

    /* virtual */ string TestController::Information() const
    {
        // No additional info to report.
        return ((_T("The purpose of this plugin is provide ability to execute functional tests.")));
    }

    void TestController::ProcessTermination(uint32_t connection)
    {
        RPC::IRemoteConnection* process(_service->RemoteConnection(connection));
        if (process != nullptr) {
            process->Terminate();
            process->Release();
        }
    }

    void TestController::Activated(RPC::IRemoteConnection* /*connection*/)
    {
        return;
    }

    void TestController::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (_connection == connection->Id()) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
} // namespace Plugin
} // namespace Thunder
