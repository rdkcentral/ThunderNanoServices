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
            virtual uint64_t Resident() const override { return _main.Resident(); }

            virtual uint64_t Allocated() const override { return _main.Allocated(); }

            virtual uint64_t Shared() const override { return _main.Shared(); }

            virtual uint8_t Processes() const override { return (IsOperational() ? 1 : 0); }

            virtual bool IsOperational() const override { return _main.IsActive(); }

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
    SERVICE_REGISTRATION(TestUtility, 1, 0)

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
            RegisterAll();

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

                UnregisterAll();
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

    static Core::ProxyPoolType<Web::TextBody> _testUtilityMetadata(2);

    /* virtual */ void TestUtility::Inbound(Web::Request& request)
    {
        if ((request.Verb == Web::Request::HTTP_POST) || (request.Verb == Web::Request::HTTP_PUT)) {
            request.Body(_testUtilityMetadata.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> TestUtility::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        if (_testUtilityImp != nullptr) {
            Core::ProxyType<Web::TextBody> body(_testUtilityMetadata.Element());
            string requestBody = EMPTY_STRING;

            if (((request.Verb == Web::Request::HTTP_POST) || (request.Verb == Web::Request::HTTP_PUT)) && (request.HasBody())) {
                requestBody = (*request.Body<Web::TextBody>());
            }

            (*body) = HandleRequest(request.Verb, request.Path, _skipURL, requestBody);
            if ((*body) != EMPTY_STRING) {
                result->Body<Web::TextBody>(body);
                result->ErrorCode = Web::STATUS_OK;
                result->Message = (_T("OK"));
                result->ContentType = Web::MIMETypes::MIME_JSON;
            } else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = (_T("Method is not supported"));
            }
        } else {
            result->ErrorCode = Web::STATUS_METHOD_NOT_ALLOWED;
            result->Message = (_T("Test controller does not exist"));
        }
        return result;
    }

    void TestUtility::Deactivated(RPC::IRemoteConnection* process)
    {
        if (_connection == process->Id()) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    string /*JSON*/ TestUtility::TestCommands(void)
    {
        string response = EMPTY_STRING;
        Metadata testCommands;

        QualityAssurance::ITestUtility::ICommand::IIterator* supportedCommands = _testUtilityImp->Commands();
        ASSERT(supportedCommands != nullptr);

        while (supportedCommands->Next()) {
            Core::JSON::String name;
            name = supportedCommands->Command()->Name();
            testCommands.TestCommands.Add(name);
        }
        testCommands.ToString(response);

        return response;
    }

    string /*JSON*/ TestUtility::HandleRequest(Web::Request::type type, const string& path, const uint8_t skipUrl, const string& body /*JSON*/)
    {
        bool executed = false;
        // Return empty result in case of issue
        string /*JSON*/ response = EMPTY_STRING;

        Core::TextSegmentIterator index(Core::TextFragment(path, skipUrl, path.length() - skipUrl), false, '/');
        index.Next();
        index.Next();
        // Here process request other than:
        // /Service/<CALLSIGN>/TestCommands
        // /Service/<CALLSIGN>/<TEST_COMMAND_NAME>/...

        if (index.IsValid()) {
            if (index.Current().Text() == _T("TestCommands")) {
                if ((!index.Next()) && (type == Web::Request::HTTP_GET)) {
                    response = TestCommands();
                    executed = true;
                }
            } else {
                QualityAssurance::ITestUtility::ICommand* command = _testUtilityImp->Command(index.Current().Text());

                if (command) {
                    if (!index.Next() && ((type == Web::Request::HTTP_POST) || (type == Web::Request::HTTP_PUT))) {
                        // Execute test command
                        response = command->Execute(body);
                        executed = true;
                    } else  if (type == Web::Request::HTTP_GET) {
                        if (index.IsValid() && (index.Current().Text() == _T("Description")) && (!index.Next())) {
                            response = command->Description();
                            executed = true;
                        } else if (index.IsValid() && (index.Current().Text() == _T("Parameters")) && (!index.Next())) {
                            response = command->Signature();
                            executed = true;
                        }
                    }
                }
            }
        }

        if (!executed) {
            TRACE(Trace::Fatal, (_T("*** Test case method not found !!! ***")));
        }

        return response;
    }
} // namespace Plugin
} // namespace Thunder
