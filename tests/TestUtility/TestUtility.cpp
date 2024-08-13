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

#include "TestUtility.h"

namespace Thunder {
namespace TestUtility {

    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection)
    {
        class MemoryObserverImpl : public Exchange::IMemory {
        private:
            MemoryObserverImpl();
            MemoryObserverImpl(const MemoryObserverImpl&);
            MemoryObserverImpl& operator=(const MemoryObserverImpl&);

        public:
            MemoryObserverImpl(const RPC::IRemoteConnection* connection)
                : _main(connection  == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId())
            {

            }
            ~MemoryObserverImpl() = default;
        public:
            virtual uint64_t Resident() const { return _main.Resident(); }

            virtual uint64_t Allocated() const { return _main.Allocated(); }

            virtual uint64_t Shared() const { return _main.Shared(); }

            virtual uint8_t Processes() const { return (IsOperational() ? 1 : 0); }

            virtual bool IsOperational() const { return _main.IsActive(); }

            BEGIN_INTERFACE_MAP(MemoryObserverImpl)
            INTERFACE_ENTRY(Exchange::IMemory)
            END_INTERFACE_MAP

        private:
            Core::ProcessInfo _main;
        };

        Exchange::IMemory* memory_observer = (Core::ServiceType<MemoryObserverImpl>::Create<Exchange::IMemory>(connection));

        return memory_observer;
    }
} // namespace TestUtility

namespace Plugin {

    namespace {

        static Metadata<TestUtility> metadata(
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

    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<Core::JSON::String>>> jsonStringListFactory(1);

    /* virtual */ const string TestUtility::Initialize(PluginHost::IShell* service)
    {
        string message;
        Config config;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_testUtilityImp == nullptr);
        ASSERT(_memory == nullptr);
        ASSERT(_connection == 0);

        _service = service;
        _service->AddRef();
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

        config.FromString(_service->ConfigLine());

        _service->Register(&_notification);
        _testUtilityImp = _service->Root<QualityAssurance::ITestUtility>(_connection, ImplWaitTime, _T("TestUtilityImp"));

        if (_testUtilityImp != nullptr) {
            Thunder::QualityAssurance::JTestUtility::Register(*this, _testUtilityImp);

            RPC::IRemoteConnection* remoteConnection = _service->RemoteConnection(_connection);
            if (remoteConnection) {
                _memory = Thunder::TestUtility::MemoryObserver(remoteConnection);
                ASSERT(_memory != nullptr);
                remoteConnection->Release();
            }

        } else {
            TRACE(Trace::Fatal, (_T("*** TestUtility could not be instantiated ***")));
            message = _T("TestUtility could not be instantiated.");
        }

        return message;
    }

    /* virtual */ void TestUtility::Deinitialize(VARIABLE_IS_NOT_USED PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            _service->Unregister(&_notification);

            if (_testUtilityImp != nullptr) {
                Thunder::QualityAssurance::JTestUtility::Unregister(*this);

                if (_memory != nullptr) {
                    if (_memory->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
                        TRACE(Trace::Information, (_T("Memory observer in TestUtility is not properly destructed")));
                    }
                    _memory = nullptr;
                }

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connection));
                VARIABLE_IS_NOT_USED uint32_t result = _testUtilityImp->Release();
                _testUtilityImp = nullptr;
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                // The connection can disappear in the meantime...
                if (connection != nullptr) {
                    // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                    connection->Terminate();
                    connection->Release();
                }           
            }

            _skipURL = 0;
            _connection = 0;
            _service->Release();
            _service = nullptr;
        }
    }

    /* virtual */ string TestUtility::Information() const
    {
        // No additional info to report.
        return ((_T("The purpose of this plugin is provide ability to execute functional tests.")));
    }

    void TestUtility::Deactivated(RPC::IRemoteConnection* process)
    {
        if (_connection == process->Id()) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
} // namespace Plugin
} // namespace Thunder
