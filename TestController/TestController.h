#pragma once

#include "Module.h"

#include <interfaces/IMemory.h>
#include <interfaces/ITestController.h>

#include "Core/TestMetadata.h"

namespace WPEFramework {
namespace Plugin {

    class TestController : public PluginHost::IPlugin, public PluginHost::IWeb {
    public:
        // maximum wait time for process to be spawned
        static constexpr uint32_t ImplWaitTime = 1000;

    private:
        class Notification : public RPC::IRemoteProcess::INotification {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;

            explicit Notification(TestController* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~Notification() {}

        public:
            virtual void Activated(RPC::IRemoteProcess* process) { _parent.Activated(process); }

            virtual void Deactivated(RPC::IRemoteProcess* process) { _parent.Deactivated(process); }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
            END_INTERFACE_MAP

        private:
            TestController& _parent;
        };

        class MetadataCategory : public Core::JSON::Container {
        public:
            MetadataCategory(const MetadataCategory&) = delete;
            MetadataCategory& operator=(const MetadataCategory&) = delete;

            MetadataCategory()
                : Core::JSON::Container()
                , TestCategories()
            {
                Add(_T("testCategories"), &TestCategories);
            }
            ~MetadataCategory() {}

        public:
            Core::JSON::ArrayType<Core::JSON::String> TestCategories;
        };

        class MetadataTest : public Core::JSON::Container {
        public:
            MetadataTest(const MetadataTest&) = delete;
            MetadataTest& operator=(const MetadataTest&) = delete;

            MetadataTest()
                : Core::JSON::Container()
                , Tests()
            {
                Add(_T("tests"), &Tests);
            }
            ~MetadataTest() {}

        public:
            Core::JSON::ArrayType<Core::JSON::String> Tests;
        };

        class OverallTestResults : public Core::JSON::Container {
        private:
            OverallTestResults(const OverallTestResults&) = delete;
            OverallTestResults& operator=(const OverallTestResults&) = delete;

        public:
            OverallTestResults()
                : Core::JSON::Container()
                , Results()
            {
                Add(_T("testsResults"), &Results);
            }

            ~OverallTestResults() = default;

        public:
            Core::JSON::ArrayType<TestCore::TestResult> Results;
        };

    public:
        TestController()
            : _service(nullptr)
            , _notification(this)
            , _memory(nullptr)
            , _testControllerImp(nullptr)
            , _skipURL(0)
            , _pid(0)
            , _prevCategory(EMPTY_STRING)
        {
        }

        virtual ~TestController() {}

        BEGIN_INTERFACE_MAP(TestController)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        INTERFACE_AGGREGATE(Exchange::ITestController, _testControllerImp)
        END_INTERFACE_MAP

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

        TestController(const TestController&) = delete;
        TestController& operator=(const TestController&) = delete;

    private:
        void Activated(RPC::IRemoteProcess* process);
        void Deactivated(RPC::IRemoteProcess* process);

        void ProcessTermination(uint32_t pid);

        void TestPreparation(Exchange::ITestController::ICategory* const category, const string& categoryName);
        string /*JSON*/ HandleRequest(Web::Request::type type, const string& path, const uint8_t skipUrl, const string& body /*JSON*/);
        string /*JSON*/ TestCategories(Exchange::ITestController::ICategory::IIterator* categories);
        string /*JSON*/ Tests(Exchange::ITestController::ITest::IIterator* tests);
        string /*JSON*/ RunAll(const string& body, const string& categoryName = EMPTY_STRING);
        string /*JSON*/ RunTest(const string& body, const string& categoryName, const string& testName);

        PluginHost::IShell* _service;
        Core::Sink<Notification> _notification;
        Exchange::IMemory* _memory;
        Exchange::ITestController* _testControllerImp;
        uint8_t _skipURL;
        uint32_t _pid;
        string _prevCategory;
    };

} // namespace Plugin
} // namespace WPEFramework
