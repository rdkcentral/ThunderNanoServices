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
#include "ResourceMonitor.h"

namespace WPEFramework
{

    namespace Plugin
    {
        namespace {

            static Metadata<ResourceMonitor> metadata(
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

        const string ResourceMonitor::Initialize(PluginHost::IShell *service)
        {
            string message;

            ASSERT(service != nullptr);
            ASSERT(_service == nullptr);
            ASSERT(_monitor == nullptr);
            ASSERT(_connectionId == 0);

            _service = service;
            _service->AddRef();
            _monitor = _service->Root<Exchange::IResourceMonitor>(_connectionId, 2000, _T("ResourceMonitorImplementation"));

            if (_monitor == nullptr) {
                message = _T("ResourceMonitor could not be instantiated.");
            }
            else {
                if(_monitor->Configure(service) == Core::ERROR_INCOMPLETE_CONFIG){
                    message = _T("ResourceMonitor could not be Configured.");
                }
                _monitor->Register(&_notification);
                Exchange::JResourceMonitor::Register(*this, _monitor);
            }

            return message;
        }

        void ResourceMonitor::Deinitialize(PluginHost::IShell *service VARIABLE_IS_NOT_USED)
        {
            if (_service != nullptr) {
                ASSERT(_service == _service);

                if (_monitor != nullptr) {
                    Exchange::JResourceMonitor::Unregister(*this);
                    _monitor->Unregister(&_notification);
                    RPC::IRemoteConnection *connection(_service->RemoteConnection(_connectionId));
                    VARIABLE_IS_NOT_USED uint32_t result = _monitor->Release();
                    _monitor = nullptr;
                    ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                    // The process can disappear in the meantime...
                    if (connection != nullptr) {
                        // Connection is still there.
                        connection->Terminate();
                        connection->Release();
                    }
                }

                _connectionId = 0;
                _service->Release();
                _service = nullptr;
            }
        }

        string ResourceMonitor::Information() const
        {
            return "";
        }
    } // namespace Plugin
} // namespace Thunder
