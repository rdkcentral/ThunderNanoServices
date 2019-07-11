#include "TestController.h"

namespace WPEFramework {
namespace TestController {

    Exchange::IMemory* MemoryObserver(const uint32_t pid)
    {
        class MemoryObserverImpl : public Exchange::IMemory {
        public:
            MemoryObserverImpl() = delete;
            MemoryObserverImpl(const MemoryObserverImpl&) = delete;
            MemoryObserverImpl& operator=(const MemoryObserverImpl&) = delete;

            MemoryObserverImpl(const uint32_t id)
                : _main(id == 0 ? Core::ProcessInfo().Id() : id)
                , _observable(false)
            {
            }
            ~MemoryObserverImpl() {}

        public:
            virtual void Observe(const uint32_t pid)
            {
                if (pid == 0) {
                    _observable = false;
                } else {
                    _observable = true;
                    _main = Core::ProcessInfo(pid);
                }
            }
            virtual uint64_t Resident() const { return (_observable == false ? 0 : _main.Resident()); }

            virtual uint64_t Allocated() const { return (_observable == false ? 0 : _main.Allocated()); }

            virtual uint64_t Shared() const { return (_observable == false ? 0 : _main.Shared()); }

            virtual uint8_t Processes() const { return (IsOperational() ? 1 : 0); }

            virtual const bool IsOperational() const { return (_observable == false) || (_main.IsActive()); }

            BEGIN_INTERFACE_MAP(MemoryObserverImpl)
            INTERFACE_ENTRY(Exchange::IMemory)
            END_INTERFACE_MAP

        private:
            Core::ProcessInfo _main;
            bool _observable;
        };

        return (Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(pid));
    }
} // namespace TestController

namespace Plugin {
    SERVICE_REGISTRATION(TestController, 1, 0);

    /* virtual */ const string TestController::Initialize(PluginHost::IShell* service)
    {
        /*Assume that everything is OK*/
        string message = EMPTY_STRING;
        Config config;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_testControllerImp == nullptr);
        ASSERT(_memory == nullptr);

        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        _service->Register(&_notification);
        _testControllerImp = _service->Root<Exchange::ITestController>(_pid, ImplWaitTime, _T("TestControllerImp"));

        if ((_testControllerImp != nullptr) && (_service != nullptr)) {
            _memory = WPEFramework::TestController::MemoryObserver(_pid);
            ASSERT(_memory != nullptr);
            _memory->Observe(_pid);
            _testControllerImp->Setup();
        } else {
            ProcessTermination(_pid);
            _service = nullptr;
            _testControllerImp = nullptr;
            _service->Unregister(&_notification);

            TRACE(Trace::Fatal, (_T("*** TestController could not be instantiated ***")))
            message = _T("TestUtility could not be instantiated.");
        }

        return message;
    }

    /* virtual */ void TestController::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_testControllerImp != nullptr);
        ASSERT(_memory != nullptr);

        _testControllerImp->TearDown();

        if (_testControllerImp->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
            TRACE_L1("TestController Plugin is not properly destructed. %d", _pid);
            ProcessTermination(_pid);
        }

        _testControllerImp = nullptr;
        _memory->Release();
        _memory = nullptr;
        _service->Unregister(&_notification);
        _service = nullptr;
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
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

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

    void TestController::ProcessTermination(uint32_t pid)
    {
        RPC::IRemoteProcess* process(_service->RemoteProcess(pid));
        if (process != nullptr) {
            process->Terminate();
            process->Release();
        }
    }

    void TestController::Activated(RPC::IRemoteProcess* /*process*/)
    {
        return;
    }

    void TestController::Deactivated(RPC::IRemoteProcess* process)
    {
        if (_pid == process->Id()) {
            ASSERT(_service != nullptr);
            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    Core::JSON::ArrayType<Core::JSON::String> /*JSON*/ TestController::TestCategories(Exchange::ITestController::ICategory::IIterator* categories)
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

    Core::JSON::ArrayType<Core::JSON::String> /*JSON*/ TestController::Tests(Exchange::ITestController::ITest::IIterator* tests)
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
            Exchange::ITestController::ICategory::IIterator* categories = _testControllerImp->Categories();
            ASSERT(categories != nullptr);

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
            Exchange::ITestController::ICategory* category = _testControllerImp->Category(categoryName);
            ASSERT(category != nullptr);

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

        Exchange::ITestController::ICategory* category = _testControllerImp->Category(categoryName);
        ASSERT(category != nullptr);

        if (category != nullptr) {
            TestPreparation(category, categoryName);

            auto test = category->Test(testName);

            if (test != nullptr) {
                response = test->Execute(body);
            }
        }

        return response;
    }

    void TestController::TestPreparation(Exchange::ITestController::ICategory* const category, const string& categoryName)
    {
        if (_prevCategory != categoryName) {
            if (_prevCategory != EMPTY_STRING) {
                Exchange::ITestController::ICategory* prevCategory = _testControllerImp->Category(_prevCategory);
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
            TRACE(Trace::Fatal, (_T("*** Wrong request !!! ***")))
        }

        return response;
    }
} // namespace Plugin
} // namespace WPEFramework
