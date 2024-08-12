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

#include "AVS.h"

namespace Thunder {
namespace Plugin {

     namespace {

        static Metadata<AVS> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::INTERNET },
            // Terminations
            {},
            // Controls
            {}
        );
    }

    const string AVS::Initialize(PluginHost::IShell* service)
    {
        string message;
        Config config;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_AVSClient == nullptr);

        _service = service;
        _service->AddRef();

        _service->Register(&_audiosourceNotification);
        _service->Register(&_connectionNotification);

        ASSERT(service->PersistentPath() != _T(""));
        Core::Directory directory((service->PersistentPath() + _T("db")).c_str());
        if (directory.CreatePath() != true) {
            message = _T("Failed to create Persistent Path");
        }

        config.FromString(service->ConfigLine());

        if (message.empty() == true) {
            _audiosourceName = config.Audiosource.Value();
            if (_audiosourceName.empty() == true) {
                message = _T("Missign audiosource callsing");
            }
        }

        if (message.empty() == true) {
            if (config.EnableSmartScreen.Value() == true) {
                message = CreateInstance(_T("SmartScreen"), config);
            } else {
                message = CreateInstance(_T("AVSDevice"), config);
            }
        }

        if (message.empty() == true) {
            _controller = _AVSClient->Controller();
            if (_controller != nullptr) {
                _controller->AddRef();
                _controller->Register(&_dialogueNotification);
                Exchange::JAVSController::Register(*this, _controller);
            }
        }

        return message;
    }

    void AVS::Deinitialize(PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            _service->Unregister(&_audiosourceNotification);
            _service->Unregister(&_connectionNotification);

            if (_AVSClient != nullptr) {
                TRACE_L1(_T("Deinitializing AVSClient..."));

                if (_controller != nullptr) {
                    _controller->Unregister(&_dialogueNotification);
                    _controller->Release();
                    Exchange::JAVSController::Unregister(*this);
                    _controller = nullptr;
                }

                if (_AVSClient->Deinitialize() == false) {
                    TRACE_L1(_T("AVSClient deinitialize failed!"));
                }

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
                VARIABLE_IS_NOT_USED uint32_t result = _AVSClient->Release();
                _AVSClient = nullptr;
                // It should have been the last reference we are releasing,
                // so it should endup in a DESTRUCTION_SUCCEEDED, if not we
                // are leaking...
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                // The process can disappear in the meantime...
                if (connection != nullptr) {
                    connection->Terminate();
                    connection->Release();
                }
            }

            _service->Release();
            _service = nullptr;
            _connectionId = 0;
        }
    }

    string AVS::Information() const
    {
        return (_T("Alexa Voice Service Client"));
    }

    void AVS::Activated(RPC::IRemoteConnection* /*connection*/)
    {
        return;
    }

    void AVS::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (_connectionId == connection->Id()) {
            ASSERT(_service != nullptr);
            Core::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    const string AVS::CreateInstance(const string& name, const Config& config)
    {
        TRACE_L1(_T("Launching AVSClient - %s..."), name.c_str());

        string message = _T("");
        string configStr;

        if (config.ToString(configStr) != true) {
            message = _T("Failed to convert configuration to string");
        } else {
            _AVSClient = _service->Root<Exchange::IAVSClient>(_connectionId, ImplWaitTime, name);
            if (_AVSClient == nullptr) {
                message = _T("Failed to create the AVSClient - " + name);
            } else {
                if (_AVSClient->Initialize(_service, configStr) != true) {
                    message = _T("Failed to initialize the AVSClient - " + name);
                }
            }
        }

        return message;
    }

} // namespace Plugin
} // namespace Thunder
