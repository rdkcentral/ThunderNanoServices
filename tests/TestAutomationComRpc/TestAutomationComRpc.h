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
 
#ifndef __TESTAUTOMATIONCOMRPC_H
#define __TESTAUTOMATIONCOMRPC_H


#include "Module.h"
#include <qa_interfaces/ITestAutomation.h>
#include <qa_interfaces/json/JComRpc.h>


namespace Thunder {
namespace Plugin {

    class TestAutomationComRpc : public PluginHost::IPlugin, public QualityAssurance::IComRpc, public PluginHost::JSONRPC {
    public:
        TestAutomationComRpc(const TestAutomationComRpc&) = delete;
        TestAutomationComRpc& operator=(const TestAutomationComRpc&) = delete;
        TestAutomationComRpc(TestAutomationComRpc&&) = delete;
        TestAutomationComRpc& operator=(TestAutomationComRpc&&) = delete;

        TestAutomationComRpc()
            : _implementation(nullptr)
            , _connectionId(0)
            , _service(nullptr)

        {
        }

        ~TestAutomationComRpc() override
        {
        }
       

        BEGIN_INTERFACE_MAP(TestAutomationComRpc)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(QualityAssurance::IComRpc)
        END_INTERFACE_MAP

        //   ITestAutomationComRpc Methods
        Core::hresult TestBigString(const uint32_t length) override;

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
  
    private:
       void Deactivated(RPC::IRemoteConnection* connection);

        QualityAssurance::IComRpc::IComRpcInternal* _implementation;

        uint32_t _connectionId;
        PluginHost::IShell* _service;

    };

} // Namespace Plugin.
}// namespace Thunder



#endif // __TESTAUTOMATIONCOMRPC_H