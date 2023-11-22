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
 
#ifndef __TESTAUTOMATIONTOOLS_H
#define __TESTAUTOMATIONTOOLS_H


#include "Module.h"
#include <interfaces/ITestAutomationTools.h>
#include <interfaces/json/JTestAutomationTools.h>
#include <interfaces/IMemory.h>

namespace WPEFramework {
namespace Plugin {

    class TestAutomationTools : public PluginHost::IPlugin, public Exchange::ITestAutomationTools, public PluginHost::JSONRPC {
    public:
        TestAutomationTools(const TestAutomationTools&) = delete;
        TestAutomationTools& operator=(const TestAutomationTools&) = delete;

        TestAutomationTools()
            : _implementation(nullptr)
            , _connectionId(0)
            , _service(nullptr)
            , _Notification(this)

        {
        }

        ~TestAutomationTools() override
        {
        }
       

        BEGIN_INTERFACE_MAP(TestAutomationTools)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::ITestAutomationTools)
            INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        END_INTERFACE_MAP

        //   ITestAutomationTools Methods
        void Register(Exchange::ITestAutomationTools::INotification* observer) override;
        void Unregister(const Exchange::ITestAutomationTools::INotification* observer) override;
        Core::hresult AllocateMemory(const uint32_t size) override;
        Core::hresult FreeAllocatedMemory() override;
        Core::hresult TestBigString(const uint32_t length) override;

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
  
    private:

        class Notification : public RPC::IRemoteConnection::INotification, public Exchange::ITestAutomationTools::INotification {

        public:
            explicit Notification(TestAutomationTools* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            ~Notification() override
            {
            }

            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            void Activated(RPC::IRemoteConnection*) override
            {
            }

            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }

            void TestNotification() override  
            {
                printf("Test Virtual");
            }


        BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            INTERFACE_ENTRY(Exchange::ITestAutomationTools::INotification)
        END_INTERFACE_MAP

        private:
            TestAutomationTools& _parent;
        };

            void Deactivated(RPC::IRemoteConnection* connection);

            Exchange::ITestAutomationToolsInternal* _implementation;
            uint32_t _connectionId;
            PluginHost::IShell* _service;
            Core::Sink<Notification> _Notification;

            //Added
            mutable Core::CriticalSection _adminLock;
            std::vector<Exchange::ITestAutomationTools::INotification*> _notifications;
            Exchange::IMemory* _memory;


    };

} // Namespace Plugin.
}// namespace WPEFramework



#endif // __TESTAUTOMATIONTOOLS_H