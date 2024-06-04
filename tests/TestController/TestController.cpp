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
            RegisterAll();
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
            message = _T("TestUtility could not be instantiated.");
        }

        return message;
    }

    /* virtual */ void TestController::Deinitialize(VARIABLE_IS_NOT_USED PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            _service->Unregister(&_notification);
 
            if (_testControllerImp != nullptr) {
                UnregisterAll();
                _testControllerImp->TearDown();

                if (_memory != nullptr){
                    if (_memory->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
                        TRACE(Trace::Information, (_T("Memory observer in TestUtility is not properly destructed")));
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

    static Core::ProxyPoolType<Web::TextBody> _testControllerMetadata(2);

    /* virtual */ void TestController::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_POST) {
            request.Body(_testControllerMetadata.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> TestController::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        if (_testControllerImp != nullptr) {
            Core::ProxyType<Web::TextBody> body(_testControllerMetadata.Element());
            string requestBody = EMPTY_STRING;

            if ((request.Verb == Web::Request::HTTP_POST) && (request.HasBody())) {
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

    Core::JSON::ArrayType<Core::JSON::String> /*JSON*/ TestController::TestCategories(QualityAssurance::ITestController::ICategory::IIterator* categories) const
    {
        Core::JSON::ArrayType<Core::JSON::String> testCategories;

        ASSERT(categories != nullptr);

        if (categories != nullptr) {
            while (categories->Next()) {
                Core::JSON::String name;
                name = categories->Category()->Name();
                testCategories.Add(name);
            }
        }
        return testCategories;
    }

    Core::JSON::ArrayType<Core::JSON::String> /*JSON*/ TestController::Tests(QualityAssurance::ITestController::ITest::IIterator* tests) const
    {
        Core::JSON::ArrayType<Core::JSON::String> testsItems;

        ASSERT(tests != nullptr);

        if (tests != nullptr) {
            while (tests->Next()) {
                Core::JSON::String name;
                name = tests->Test()->Name();
                testsItems.Add(name);
            }
        }

        return testsItems;
    }

    string /*JSON*/ TestController::RunAll(const string& body, const string& categoryName)
    {
        string result = EMPTY_STRING;
        OverallTestResults jsonResults;

        if (categoryName == EMPTY_STRING) {
            QualityAssurance::ITestController::ICategory::IIterator* categories = _testControllerImp->Categories();

            if (categories != nullptr) {
                while (categories->Next()) {
                    TestPreparation(categories->Category(), categories->Category()->Name());

                    auto tests = categories->Category()->Tests();
                    while (tests->Next()) {
                        result = tests->Test()->Execute(body);

                        TestCore::TestResult ret;
                        if (ret.FromString(result)) {
                            jsonResults.Results.Add(ret);
                        }
                    }
                }
            }
        } else {
            QualityAssurance::ITestController::ICategory* category = _testControllerImp->Category(categoryName);

            if (category != nullptr) {
                TestPreparation(category, categoryName);

                auto tests = category->Tests();
                while (tests->Next()) {
                    result = tests->Test()->Execute(body);

                    TestCore::TestResult ret;
                    if (ret.FromString(result)) {
                        jsonResults.Results.Add(ret);
                    }
                }
            }
        }

        result = EMPTY_STRING;
        jsonResults.ToString(result);
        return result;
    }

    string /*JSON*/ TestController::RunTest(const string& body, const string& categoryName, const string& testName)
    {
        string response = EMPTY_STRING;
        OverallTestResults jsonResults;

        QualityAssurance::ITestController::ICategory* category = _testControllerImp->Category(categoryName);

        if (category != nullptr) {
            TestPreparation(category, categoryName);

            auto test = category->Test(testName);

            if (test != nullptr) {
                response = test->Execute(body);
                TestCore::TestResult ret;
                if (ret.FromString(response)) {
                    jsonResults.Results.Add(ret);
                }
            }
        }

        response = EMPTY_STRING;
        jsonResults.ToString(response);
        return response;
    }

    void TestController::TestPreparation(QualityAssurance::ITestController::ICategory* const category, const string& categoryName)
    {
        if (_prevCategory != categoryName) {
            if (_prevCategory != EMPTY_STRING) {
                QualityAssurance::ITestController::ICategory* prevCategory = _testControllerImp->Category(_prevCategory);
                if (prevCategory != nullptr)
                    prevCategory->TearDown();
            }
            category->Setup();
            _prevCategory = categoryName;
        }
    }

    string /*JSON*/ TestController::HandleRequest(Web::Request::type type, const string& path, const uint8_t skipUrl, const string& body /*JSON*/)
    {
        bool executed = false;
        // Return empty result in case of issue
        string /*JSON*/ response = EMPTY_STRING;

        Core::TextSegmentIterator index(Core::TextFragment(path, skipUrl, path.length() - skipUrl), false, '/');

        index.Next();
        if (index.Next() == true) {
            // Here process request other than:
            // GET /Service/<CALLSIGN>/TestCategories
            // GET /Service/<CALLSIGN>/<TEST_CATEGORY>/Tests
            // GET /Service/<CALLSIGN>/<TEST_CATEGORY>/<TEST_NAME>/Description
            // POST/PUT /Service/<CALLSIGN>/TestCategories/Run
            // POST/PUT /Service/<CALLSIGN>/<TEST_CATEGORY>/Run
            // POST/PUT /Service/<CALLSIGN>/<TEST_CATEGORY>/<TEST_NAME>

            if (index.Current().Text() == _T("TestCategories")) {
                if (type == Web::Request::HTTP_GET) {
                    if (!index.Next()) {
                        auto categories = _testControllerImp->Categories();
                        ASSERT(categories != nullptr);

                        // Get list of Category
                        TestCategories(categories).ToString(response);
                        executed = true;
                    }
                } else if ((type == Web::Request::HTTP_POST) || (type == Web::Request::HTTP_PUT)) {
                    index.Next();
                    if (index.Current().Text() == _T("Run")) {
                        if (!index.Next()) {
                            response = RunAll(body);
                            executed = true;
                        }
                    }
                }
            } else {
                string testCategory = index.Current().Text();

                auto category = _testControllerImp->Category(testCategory);

                if (category != nullptr) {
                    index.Next();
                    if (type == Web::Request::HTTP_GET) {
                        if (index.Current().Text() == _T("Tests")) {
                            if (!index.Next()) {
                                // Get Tests list per Category
                                Tests(category->Tests()).ToString(response);
                                executed = true;
                            }
                        } else {
                            auto test = category->Test(index.Current().Text());

                            if (test != nullptr) {
                                if (index.Current().Text() == test->Name()) {
                                    if (index.Next()) {
                                        if (index.Current().Text() == _T("Description")) {
                                            if (!index.Next()) {
                                                response = test->Description();
                                                executed = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else if ((type == Web::Request::HTTP_POST) || (type == Web::Request::HTTP_PUT)) {
                        string request = index.Current().Text();
                        if (request == _T("Run")) {
                            if (!index.Next()) {
                                response = RunAll(body, category->Name());
                                executed = true;
                            }
                        } else {
                            if (!index.Next()) {
                                //Process particular test requests if it is valid
                                response = RunTest(body, category->Name(), request);
                                executed = true;
                            }
                        }
                    }
                }
            }
        }

        if (!executed) {
            TRACE(Trace::Fatal, (_T("*** Wrong request !!! ***")));
        }

        return response;
    }
} // namespace Plugin
} // namespace Thunder
