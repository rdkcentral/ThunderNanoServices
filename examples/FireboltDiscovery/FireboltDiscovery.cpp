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

#include "FireboltDiscovery.h"
#include <interfaces/IConfiguration.h>

namespace WPEFramework {

namespace Plugin
{

    namespace {

        static Metadata<FireboltDiscovery> metadata(
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


    FireboltDiscovery::FireboltDiscovery()
        : _connectionId(0)
        , _service(nullptr)
        , _fireboltDiscovery(nullptr)
    {
    }

#if 0
    PluginHost::JSONRPC::classification FireboltDiscovery::CheckToken(const string& token, const string& method, const string& parameters){
        SYSLOG(Logging::Startup, (_T("Received Token: %s method: %s, Params: %s"), token.c_str(), method.c_str(), parameters.c_str()));
        return PluginHost::JSONRPC::classification::VALID;
    }
#endif

    const string FireboltDiscovery::Initialize(PluginHost::IShell * service)
    {
        string message;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_fireboltDiscovery == nullptr);
        _service = service;
        _service->AddRef();

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.

        _fireboltDiscovery = service->Root<Exchange::IFDiscovery>(_connectionId, RPC::CommunicationTimeOut, _T("FireboltDiscoveryImplementation"));
        if (_fireboltDiscovery != nullptr) {

            Exchange::IConfiguration* configFireboltDiscovery = _fireboltDiscovery->QueryInterface<Exchange::IConfiguration>();
            if (configFireboltDiscovery != nullptr) {
                if (configFireboltDiscovery->Configure(service) != Core::ERROR_NONE) {
                    message = _T("FireboltDiscovery could not be configured.");
                }
                configFireboltDiscovery->Release();
                configFireboltDiscovery = nullptr;
            } 
        }
        else {
            message = _T("FireboltDiscovery could not be instantiated.");
        }
        //Exchange::JSONRPC::JFireboltDiscovery::Register(*this, _fireboltDiscovery);
        Exchange::JSONRPC::JFireboltDiscovery::Register(*this, this);

        // On success return empty, to indicate there is no error text.
        return (message);
    }

    void FireboltDiscovery::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            if (_fireboltDiscovery != nullptr) {
                Exchange::JSONRPC::JFireboltDiscovery::Unregister(*this);

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
                VARIABLE_IS_NOT_USED uint32_t result = _fireboltDiscovery->Release();
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
                _fireboltDiscovery = nullptr;

                // The connection can disappear in the meantime...
                if (connection != nullptr) {
                    // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                    connection->Terminate();
                    connection->Release();
                }
            }

            _service->Release();
            _service = nullptr;
            _connectionId = 0;
        }
    }

    string FireboltDiscovery::Information() const
    {
        // No additional info to report.
        return (string());
    }

    Core::hresult FireboltDiscovery::SignIn(const Core::JSONRPC::Context& context, const bool param ) {
            std::cout<<"Context appId: "<<context.Token().c_str();
            std::cout<<" `Context param: "<<param <<'\n';
        return Core::ERROR_NONE;
    }

    void FireboltDiscovery::Deactivated(RPC::IRemoteConnection* connection)
    {
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (_connectionId == connection->Id()) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    

} // namespace Plugin
} // namespace WPEFramework

