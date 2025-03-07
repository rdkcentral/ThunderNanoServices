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
 
#include "TestAutomationUtils.h"


namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<TestAutomationUtils> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    const string TestAutomationUtils::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_implementation == nullptr);
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_connectionId == 0);

        _service = service;
        _service->AddRef(); 

        _service->Register(&_notification);

        _implementation = _service->Root<QualityAssurance::ITestUtils>(_connectionId, 2000, _T("TestAutomationUtilsImplementation"));
        
        string result;
        if (_implementation == nullptr) {
            result = _T("Couldn't create TestAutomationUtils instance");
        } else {
            QualityAssurance::JTestUtils::Register(*this, _implementation);

            if (_connectionId == 0){ //Running this plugin in process does not make any sense. It will crash Thunder!
                result = _T("Running this plugin in process does not make any sense. It will crash Thunder!");
            }
        }
        return (result);
    }


    void TestAutomationUtils::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ASSERT(_service == service);
        
        _service->Unregister(&_notification);

        if (_implementation != nullptr) {
            QualityAssurance::JTestUtils::Unregister(*this);
            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            VARIABLE_IS_NOT_USED uint32_t result =  _implementation->Release();
            _implementation = nullptr;

            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

            if(connection != nullptr){
                connection->Terminate();
                connection->Release();
            }
        }
        
        _service->Release();
        _service = nullptr;
        _connectionId = 0;        
    }

    string TestAutomationUtils::Information() const
    {
        return string();
    }
    
    void TestAutomationUtils::Deactivated(RPC::IRemoteConnection* connection)
    {
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (_connectionId == connection->Id()) {

            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
   
}
}
