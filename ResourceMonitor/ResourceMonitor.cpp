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

        SERVICE_REGISTRATION(ResourceMonitor, 1, 0);

        const string ResourceMonitor::Initialize(PluginHost::IShell *service)
        {
            string message;
            _connectionId = 0;

            ASSERT(_service == nullptr);
            ASSERT(_monitor == nullptr);

            _service = service;

            _skipURL = static_cast<uint32_t>(_service->WebPrefix().length());

            _monitor = _service->Root<Exchange::IResourceMonitor>(_connectionId, 2000, _T("ResourceMonitorImplementation"));

            if (_monitor == nullptr)
            {
                _service = nullptr;
                message = _T("ResourceMonitor could not be instantiated.");
            }
            else
            {
                if(_monitor->Configure(service) == Core::ERROR_INCOMPLETE_CONFIG){
                    _service = nullptr;
                    message = _T("ResourceMonitor could not be instantiated.");
                }
            }

            return message;
        }

        void ResourceMonitor::Deinitialize(PluginHost::IShell *service)
        {
            ASSERT(_service == _service);

            _monitor->Release();

            if (_connectionId != 0)
            {
                RPC::IRemoteConnection *connection(_service->RemoteConnection(_connectionId));

                // The process can disappear in the meantime...
                if (connection != nullptr)
                {
                    // Connection is still there.
                    connection->Terminate();
                    connection->Release();
                }
            }

            _service = nullptr;
        }

        string ResourceMonitor::Information() const
        {
            return "";
        }

        /* static */ Core::ProxyPoolType<Web::TextBody> ResourceMonitor::webBodyFactory(4);
    } // namespace Plugin
} // namespace WPEFramework
