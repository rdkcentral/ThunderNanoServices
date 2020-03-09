/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 
#pragma once

#include "Module.h"

#include <interfaces/IMemory.h>
#include <interfaces/ITestController.h>
#include <interfaces/json/JsonData_TestController.h>

#include "Core/TestMetadata.h"

namespace WPEFramework {
namespace Plugin {

    class TestController : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        // maximum wait time for process to be spawned
        static constexpr uint32_t ImplWaitTime = 1000;

    private:
        class Notification : public RPC::IRemoteConnection::INotification {
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
            virtual void Activated(RPC::IRemoteConnection* process) { _parent.Activated(process); }

            virtual void Deactivated(RPC::IRemoteConnection* process) { _parent.Deactivated(process); }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            TestController& _parent;
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
            , _connection(0)
            , _prevCategory(EMPTY_STRING)
        {
            RegisterAll();
        }

        virtual ~TestController()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(TestController)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
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
        void Activated(RPC::IRemoteConnection* process);
        void Deactivated(RPC::IRemoteConnection* process);

        void ProcessTermination(uint32_t pid);

        void TestPreparation(Exchange::ITestController::ICategory* const category, const string& categoryName);
        string /*JSON*/ HandleRequest(Web::Request::type type, const string& path, const uint8_t skipUrl, const string& body /*JSON*/);
        Core::JSON::ArrayType<Core::JSON::String> /*JSON*/ TestCategories(Exchange::ITestController::ICategory::IIterator* categories) const;
        Core::JSON::ArrayType<Core::JSON::String> /*JSON*/ Tests(Exchange::ITestController::ITest::IIterator* tests) const;
        string /*JSON*/ RunAll(const string& body, const string& categoryName = EMPTY_STRING);
        string /*JSON*/ RunTest(const string& body, const string& categoryName, const string& testName);

        void RegisterAll();
        void UnregisterAll();
        Core::JSON::ArrayType<JsonData::TestController::RunResultData> TestResults(const string& results);
        uint32_t endpoint_run(const JsonData::TestController::RunParamsData& params, Core::JSON::ArrayType<JsonData::TestController::RunResultData>& response);
        uint32_t get_categories(Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_tests(const string& index, Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_description(const string& index, JsonData::TestController::DescriptionData& response) const;

        PluginHost::IShell* _service;
        Core::Sink<Notification> _notification;
        Exchange::IMemory* _memory;
        Exchange::ITestController* _testControllerImp;
        uint8_t _skipURL;
        uint32_t _connection;
        string _prevCategory;
    };

} // namespace Plugin
} // namespace WPEFramework
