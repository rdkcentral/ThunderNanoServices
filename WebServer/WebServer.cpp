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
 
#include "WebServer.h"

namespace Thunder {

namespace WebServer {

    // An implementation file needs to implement this method to return an operational webserver, wherever that would be :-)
    extern Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection);
}

namespace Plugin {

    namespace {

        static Metadata<WebServer> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            { subsystem::WEBSOURCE }
        );
    }

    /* virtual */ const string WebServer::Initialize(PluginHost::IShell* service)
    {
        string message;

        ASSERT(_server == nullptr);
        ASSERT(_memory == nullptr);
        ASSERT(_statecontrol == nullptr);
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_connectionId == 0);

        // Setup skip URL for right offset.
        _service = service;
        _service->AddRef();
        _skipURL = static_cast<uint32_t>(_service->WebPrefix().length());

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notification);

        _server = _service->Root<Exchange::IWebServer>(_connectionId, 2000, _T("WebServerImplementation"));

        if (_server != nullptr) {
            _statecontrol = _server->QueryInterface<PluginHost::IStateControl>();

            // We see that sometimes the implementation crashes before it reaches this point, than there is
            // no StateControl. Cope with this situation.
            if (_statecontrol == nullptr) {
                message = _T("WebServer Couldn't get StateControl.");

            } else {
                _statecontrol->Register(&_notification);
                uint32_t result = _statecontrol->Configure(_service);
                if (result != Core::ERROR_NONE) {
                    message = _T("WebServer could not be configured.");
                }
                else {
                    RPC::IRemoteConnection* connection = _service->RemoteConnection(_connectionId);
                    if (connection != nullptr) {
                        _memory = Thunder::WebServer::MemoryObserver(connection);
                        ASSERT(_memory != nullptr);
                        connection->Release();
                    }

                    PluginHost::ISubSystem* subSystem = service->SubSystems();
                    if (subSystem != nullptr) {
                        if (subSystem->IsActive(PluginHost::ISubSystem::WEBSOURCE) == true) {
                            SYSLOG(Logging::Startup, (_T("WebSource is not defined as External !!")));
                        }
                        else {
                            subSystem->Set(PluginHost::ISubSystem::WEBSOURCE, nullptr);
                        }
                        subSystem->Release();
                    }
                }
            }
        } else {
            message = _T("WebServer could not be instantiated.");
        }

        return message;
    }

    /* virtual */ void WebServer::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == _service);

            _service->Unregister(&_notification);

            if (_server != nullptr) {

                if (_statecontrol != nullptr) {
                    _statecontrol->Unregister(&_notification);
                    _statecontrol->Release();
                    _statecontrol = nullptr;
                }

                if (_memory != nullptr) {
                    _memory->Release();
                    _memory = nullptr;
                }

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

                // Stop processing of the browser:
                VARIABLE_IS_NOT_USED uint32_t result = _server->Release();
                _server = nullptr;
                // It should have been the last reference we are releasing,
                // so it should endup in a DESTRUCTION_SUCCEEDED, if not we
                // are leaking...
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                // If this was running in a (container) process...
                if (connection != nullptr) {
                    // Lets trigger a cleanup sequence for
                    // out-of-process code. Which will guard
                    // that unwilling processes, get shot if
                    // not stopped friendly :~)
                    connection->Terminate();
                    connection->Release();
                }
            }

            _service->Release();
            _service = nullptr;
            _connectionId = 0;
        }
    }

    /* virtual */ string WebServer::Information() const
    {
        // No additional info to report.
        return (string());
    }

    void WebServer::Deactivated(RPC::IRemoteConnection* connection)
    {
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (_connectionId == connection->Id()) {

            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
    void WebServer::StateChange(const PluginHost::IStateControl::state state)
    {
        switch (state) {
        case PluginHost::IStateControl::RESUMED:
            TRACE(Trace::Information,
                    (string(_T("StateChange: { \"suspend\":false }"))));
            _service->Notify("{ \"suspended\":false }");
            break;
        case PluginHost::IStateControl::SUSPENDED:
            TRACE(Trace::Information,
                    (string(_T("StateChange: { \"suspend\":true }"))));
            _service->Notify("{ \"suspended\":true }");
            break;
        case PluginHost::IStateControl::EXITED:
            Core::IWorkerPool::Instance().Submit(
                    PluginHost::IShell::Job::Create(_service,
                            PluginHost::IShell::DEACTIVATED,
                            PluginHost::IShell::REQUESTED));
            break;
        case PluginHost::IStateControl::UNINITIALIZED:
            break;
        default:
            ASSERT(false);
            break;
        }
    }



}
}
