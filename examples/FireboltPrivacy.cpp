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

#include "FireboltPrivacy.h"
#include <interfaces/IConfiguration.h>

namespace WPEFramework {

namespace Plugin
{

    namespace {

        static Metadata<FireboltPrivacy> metadata(
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


    FireboltPrivacy::FireboltPrivacy()
        : JSONRPC(std::bind(&FireboltPrivacy::CheckToken, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
        , _connectionId(0)
        , _service(nullptr)
        , _fireboltPrivacy(nullptr)
        , _notificationSink(*this)
    {
    }

    PluginHost::JSONRPC::classification FireboltPrivacy::CheckToken(const string& token, const string& method, const string& parameters){
        SYSLOG(Logging::Startup, (_T("Received Token: %s method: %s, Params: %s"), token.c_str(), method.c_str(), parameters.c_str()));
        return PluginHost::JSONRPC::classification::VALID;
    }

    const string FireboltPrivacy::Initialize(PluginHost::IShell * service)
    {
        string message;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_fireboltPrivacy == nullptr);
        _service = service;
        _service->AddRef();

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notificationSink);

        _fireboltPrivacy = service->Root<Exchange::IFireboltPrivacy>(_connectionId, RPC::CommunicationTimeOut, _T("FireboltPrivacyImplementation"));
        if (_fireboltPrivacy != nullptr) {
            _fireboltPrivacy->Register(&_notificationSink);

            Exchange::IConfiguration* configFireboltPrivacy = _fireboltPrivacy->QueryInterface<Exchange::IConfiguration>();
            if (configFireboltPrivacy != nullptr) {
                if (configFireboltPrivacy->Configure(service) != Core::ERROR_NONE) {
                    message = _T("FireboltPrivacy could not be configured.");
                }
                configFireboltPrivacy->Release();
                configFireboltPrivacy = nullptr;
            } 
        }
        else {
            message = _T("FireboltPrivacy could not be instantiated.");
        }
        Exchange::JFireboltPrivacy::Register(*this, _fireboltPrivacy);

        // On success return empty, to indicate there is no error text.
        return (message);
    }

    void FireboltPrivacy::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);
            _service->Unregister(&_notificationSink);

            if (_fireboltPrivacy != nullptr) {
                _fireboltPrivacy->Unregister(&_notificationSink);
                Exchange::JFireboltPrivacy::Unregister(*this);

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
                VARIABLE_IS_NOT_USED uint32_t result = _fireboltPrivacy->Release();
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
                _fireboltPrivacy = nullptr;

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

    string FireboltPrivacy::Information() const
    {
        // No additional info to report.
        return (string());
    }


    void FireboltPrivacy::Deactivated(RPC::IRemoteConnection* connection)
    {
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (_connectionId == connection->Id()) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    void FireboltPrivacy::OnAllowResumePointsChanged(const bool allowResumePoint) {
        Exchange::JFireboltPrivacy::Event::OnAllowResumePointsChanged(*this, allowResumePoint);
    }
} // namespace Plugin
} // namespace WPEFramework

