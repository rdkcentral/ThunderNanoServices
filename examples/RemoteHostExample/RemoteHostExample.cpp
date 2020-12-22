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
 
#include "RemoteHostExample.h"
#include "interfaces/IRemoteInvocation.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(RemoteHostExample, 1, 0);

    const string RemoteHostExample::Initialize(PluginHost::IShell* service) 
    {
        string errorMessage = "";

        service->AddRef();
        _shell = service;

        Config config;
        config.FromString(service->ConfigLine());

        string name = config.Name.Value();
        uint32_t connectionId = 0;   

        _implementation = service->Root<Exchange::IRemoteHostExample>(connectionId, Core::infinite, "RemoteHostExampleImpl", ~0);

        if (_implementation != nullptr) {

            if (service->RemoteConnection(connectionId)->ConnectionType() == RPC::IRemoteConnection::Type::Local) {

                // code run only on plugin host
                _implementation->Initialize(service);
            } else {
                // code run only on proxy side
                if (_implementation->SubscribeTimeUpdates(this) != Core::ERROR_NONE) {
                    errorMessage = "Failed to subscribe updates";
                }
            }
            
        } else {
            errorMessage = "Failed to initialize RemoteHostExample implementaiton";
        }

        return errorMessage;
    }

    void RemoteHostExample::Deinitialize(PluginHost::IShell* service) 
    {
        if (_implementation != nullptr) {
            _implementation->UnsubscribeTimeUpdates(this);

            // Close remote connection
            _implementation->QueryInterface<RPC::RemoteLinker>()->Unlink();

            uint32_t result = _implementation->Release();

            _implementation = nullptr;
        }
    }

    string RemoteHostExample::Information() const 
    {
        return (string());
    }

    uint32_t RemoteHostExample::TimeUpdate(string time) 
    {
        TRACE(Trace::Information, (_T("#### Current time is: %s ####"), time.c_str()));

        return Core::ERROR_NONE;
    }
}
}
