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
 
#ifndef __TESTAUTOMATIONMEMORY_H
#define __TESTAUTOMATIONMEMORY_H


#include "Module.h"
#include <qa_interfaces/ITestAutomation.h>
#include <qa_interfaces/json/JMemory.h>
#include <interfaces/IMemory.h>

namespace Thunder {
namespace Plugin {

    class TestAutomationMemory : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    private:
        class Notification : public RPC::IRemoteConnection::INotification {
        public:
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;
            Notification(Notification&&) = delete;
            Notification& operator=(Notification&&) = delete;

            explicit Notification(TestAutomationMemory& parent)
                : _parent(parent)
            {
            }
            ~Notification() = default;

        public:
            void Activated(RPC::IRemoteConnection* /* connection */) override
            {
            }
            void Deactivated(RPC::IRemoteConnection* connectionId) override
            {
                _parent.Deactivated(connectionId);
            }
            void Terminated(RPC::IRemoteConnection* /* connection */) override
            {
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            TestAutomationMemory& _parent;
        };

    public:
        TestAutomationMemory(const TestAutomationMemory&) = delete;
        TestAutomationMemory& operator=(const TestAutomationMemory&) = delete;
        TestAutomationMemory(TestAutomationMemory&&) = delete;
        TestAutomationMemory& operator=(TestAutomationMemory&&) = delete;

        TestAutomationMemory()
            : _memoryTestInterface(nullptr)
            , _connectionId(0)
            , _memory(nullptr)
            , _service(nullptr)
            , _notification(*this)
        {
        }

        ~TestAutomationMemory() override = default;

        BEGIN_INTERFACE_MAP(TestAutomationMemory)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(QualityAssurance::IMemory, _memoryTestInterface)
            INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        END_INTERFACE_MAP


        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
  
    private:
       void Deactivated(RPC::IRemoteConnection* connection);

    private:
        QualityAssurance::IMemory* _memoryTestInterface;
        uint32_t _connectionId;
        Exchange::IMemory* _memory;
        PluginHost::IShell* _service;
        Core::SinkType<Notification> _notification;

    };

} // Namespace Plugin.
}// namespace Thunder

#endif // __TESTAUTOMATIONMEMORY_H