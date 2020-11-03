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
 
#include "VolumeControl.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(VolumeControl, 1, 0);

    const string VolumeControl::Initialize(PluginHost::IShell* service)
    {
        ASSERT (_service == nullptr);
        ASSERT (service != nullptr);

        _service = service;
        _service->Register(&_connectionNotification);

        string result;
        _implementation = _service->Root<Exchange::IVolumeControl>(_connectionId, 2000, _T("VolumeControlImplementation"));
        if (_implementation == nullptr) {
            result = _T("Couldn't create volume control instance");
        } else {
          _implementation->Register(&_volumeNotification);
          Exchange::JVolumeControl::Register(*this, _implementation);
        }

        return (result);
    }

    void VolumeControl::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        Exchange::JVolumeControl::Unregister(*this);

        _service->Unregister(&_connectionNotification);
        _implementation->Unregister(&_volumeNotification);

        _implementation->Release();

        if(_connectionId != 0){
            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            // The process can disappear in the meantime...
            if (connection != nullptr) {
                // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                connection->Terminate();
                connection->Release();
            }
        }

        _service = nullptr;
        _implementation = nullptr;
    }

    string VolumeControl::Information() const
    {
        return string();
    }

    void VolumeControl::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin
} // namespace WPEFramework
