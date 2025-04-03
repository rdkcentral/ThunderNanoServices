/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 [PLEASE ADD COPYRIGHT NAME!]
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

#include "TestPlugin.h"

namespace Thunder {
namespace Plugin {
    
    namespace {
        
        static Metadata<TestPlugin>metadata(
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
    
    const string TestPlugin::Initialize(PluginHost::IShell* service) {
        string message;
        
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_implTestTextOptions == nullptr);
        ASSERT(_implTestLegacy == nullptr);
        ASSERT(_implTestKeep == nullptr);
        ASSERT(_implTestCustom == nullptr);
        ASSERT(_connectionId == 0);
        
        _service = service;
        _service->AddRef();
        _service->Register(&_notification);
        
        _implTestTextOptions = service->Root<QualityAssurance::ITestTextOptions>(_connectionId, timeout, _T("TestPluginImplementation"));
        if (_implTestTextOptions == nullptr) {
            message = _T("Couldn't create instance of implTestTextOptions");
        } else {
            QualityAssurance::JTestTextOptions::Register(*this, _implTestTextOptions);
            _implTestLegacy = _implTestTextOptions->QueryInterface<QualityAssurance::ITestTextOptions::ITestLegacy>();
            if (_implTestLegacy == nullptr) {
                message = _T("Couldn't create instance of _implITestLegacy");
            } else {
                QualityAssurance::TestTextOptions::JTestLegacy::Register(*this, _implTestLegacy);            
                _implTestKeep = _implTestTextOptions->QueryInterface<QualityAssurance::ITestTextOptions::ITestKeep>();
                if (_implTestKeep == nullptr) {
                    message = _T("Couldn't create instance of _implITestKeep");
                } else {
                    QualityAssurance::TestTextOptions::JTestKeep::Register(*this, _implTestKeep);                   
                    _implTestCustom = _implTestTextOptions->QueryInterface<QualityAssurance::ITestTextOptions::ITestCustom>();
                    if (_implTestCustom == nullptr) {
                        message = _T("Couldn't create instance of _implITestCustom");
                    } else {
                        QualityAssurance::TestTextOptions::JTestCustom::Register(*this, _implTestCustom);
                    }
                }
            }
        }
        
        return (message);
    }
    
    void TestPlugin::Deinitialize(PluginHost::IShell* service) {
        
        ASSERT(_service == service);
        
        _service->Unregister(&_notification);
        
        if (_implTestTextOptions != nullptr) {
            QualityAssurance::JTestTextOptions::Unregister(*this);
            
            if (_implTestLegacy != nullptr) {
                QualityAssurance::TestTextOptions::JTestLegacy::Unregister(*this);
                _implTestLegacy->Release();
                _implTestLegacy = nullptr;
            }
            
            if (_implTestKeep != nullptr) {
                QualityAssurance::TestTextOptions::JTestKeep::Unregister(*this);
                _implTestKeep->Release();
                _implTestKeep = nullptr;
            }
            
            if (_implTestCustom != nullptr) {
                QualityAssurance::TestTextOptions::JTestCustom::Unregister(*this);
                _implTestCustom->Release();
                _implTestCustom = nullptr;
            }
            
            RPC::IRemoteConnection* connection(service->RemoteConnection(_connectionId));
            VARIABLE_IS_NOT_USED uint32_t result = _implTestTextOptions->Release();
            _implTestTextOptions = nullptr;
            
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
            
            // The process can disappear in the meantime...
            if (connection != nullptr) {
                // But if it did not disappear in the meantime, forcefully terminate it. Shoot to kill
                connection->Terminate();
                connection->Release();
            }
        }
        _service->Release();
        _service = nullptr;
        _connectionId = 0;
    }
    
    string TestPlugin::Information() const {
        return (string());
    }

    
    void TestPlugin::Deactivated(RPC::IRemoteConnection* connection) {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
    
} // Plugin
} // Thunder