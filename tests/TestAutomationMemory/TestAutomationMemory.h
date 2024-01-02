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
#include <interfaces/ITestAutomation.h>
#include <interfaces/json/JTestAutomationMemory.h>
#include <interfaces/IMemory.h>

namespace WPEFramework {
namespace Plugin {

    class TestAutomationMemory : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        TestAutomationMemory(const TestAutomationMemory&) = delete;
        TestAutomationMemory& operator=(const TestAutomationMemory&) = delete;

        TestAutomationMemory()
            : _memoryTestInterface(nullptr)
            , _connectionId(0)
            , _memory(nullptr)
            , _service(nullptr)

        {
        }

        ~TestAutomationMemory() override
        {
        }
       

        BEGIN_INTERFACE_MAP(TestAutomationMemory)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::TestAutomation::IMemory, _memoryTestInterface)
            INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        END_INTERFACE_MAP


        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
  
    private:
       void Deactivated(RPC::IRemoteConnection* connection);

        Exchange::TestAutomation::IMemory* _memoryTestInterface;
        uint32_t _connectionId;
        Exchange::IMemory* _memory;
        PluginHost::IShell* _service;

    };

} // Namespace Plugin.
}// namespace WPEFramework

#endif // __TESTAUTOMATIONMEMORY_H