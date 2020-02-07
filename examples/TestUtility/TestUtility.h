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

#include "CommandCore/TestCommandController.h"
#include <interfaces/IMemory.h>
#include <interfaces/ITestUtility.h>
#include <interfaces/json/JsonData_TestUtility.h>

namespace WPEFramework {
namespace Plugin {

    class TestUtility : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        // maximum wait time for process to be spawned
        static constexpr uint32_t ImplWaitTime = 1000;

    public:
        TestUtility(const TestUtility&) = delete;
        TestUtility& operator=(const TestUtility&) = delete;

    private:
        class Notification : public RPC::IRemoteConnection::INotification {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;

        public:
            explicit Notification(TestUtility* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~Notification() = default;

        public:
            virtual void Activated(RPC::IRemoteConnection* process) { /*_parent.Activated(process);*/ }

            virtual void Deactivated(RPC::IRemoteConnection* process) { _parent.Deactivated(process); }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            TestUtility& _parent;
        };

        class Metadata : public Core::JSON::Container {
        public:
            Metadata(const Metadata&) = delete;
            Metadata& operator=(const Metadata&) = delete;

        public:
            Metadata()
                : Core::JSON::Container()
                , TestCommands()
            {
                Add(_T("testCommands"), &TestCommands);
            }
            ~Metadata() = default;

        public:
            Core::JSON::ArrayType<Core::JSON::String> TestCommands;
        };

    public:
        TestUtility()
            : _service(nullptr)
            , _notification(this)
            , _memory(nullptr)
            , _testUtilityImp(nullptr)
            , _skipURL(0)
            , _connection(0)
        {
            RegisterAll();
        }

        virtual ~TestUtility()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(TestUtility)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        INTERFACE_AGGREGATE(Exchange::ITestUtility, _testUtilityImp)
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

    private:
        void Deactivated(RPC::IRemoteConnection* process);

        void ProcessTermination(uint32_t _connection);
        string /*JSON*/ HandleRequest(Web::Request::type type, const string& path, const uint8_t skipUrl, const string& body /*JSON*/);
        string /*JSON*/ TestCommands(void);

        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_runmemory(const JsonData::TestUtility::RunmemoryParamsData& params, JsonData::TestUtility::RunmemoryResultData& response);
        uint32_t endpoint_runcrash(const JsonData::TestUtility::RuncrashParamsData& params);
        uint32_t get_commands(Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_description(const string& index, JsonData::TestUtility::DescriptionData& response) const;
        uint32_t get_parameters(const string& index, JsonData::TestUtility::ParametersData& response) const;
        uint32_t set_shutdowntimeout(const Core::JSON::DecUInt32& param);

        PluginHost::IShell* _service;
        Core::Sink<Notification> _notification;
        Exchange::IMemory* _memory;
        Exchange::ITestUtility* _testUtilityImp;
        uint8_t _skipURL;
        uint32_t _connection;
    };

} // namespace Plugin
} // namespace WPEFramework
