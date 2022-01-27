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

#include "Module.h"
#include <interfaces/IConfiguration.h>

namespace WPEFramework {
namespace Plugin {

    class DeviceProvisioningImplementation : public Exchange::IConfiguration,
                                             public PluginHost::IStateControl {
    private:
        DeviceProvisioningImplementation(const DeviceProvisioningImplementation&) = delete;
        DeviceProvisioningImplementation& operator=(const DeviceProvisioningImplementation&) = delete;

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Script()
            {
                Add(_T("script"), &Script);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Script;
        };

        class DeviceProvisioningTask : public Core::Thread {
        private:
            DeviceProvisioningTask(const DeviceProvisioningTask&) = delete;
            DeviceProvisioningTask& operator=(const DeviceProvisioningTask&) = delete;

        public:
            DeviceProvisioningTask(DeviceProvisioningImplementation& parent)
                : Core::Thread(0, _T("DeviceProvisioning"))
                , _parent(parent)
                , _script { }
            {
            }

            ~DeviceProvisioningTask() override
            {
                Block();
                Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
            }

            uint32_t Configure(PluginHost::IShell* service)
            {
                Config config;
                config.FromString(service->ConfigLine());

                if (config.Script.IsSet() == true) {
                    _script = config.Script.Value();
                }

                Run();
                return Core::ERROR_NONE;
            }

        private:
            uint32_t Initialize() override
            {
                return Core::ERROR_NONE;
            }

            uint32_t Worker() override
            {
                if (IsRunning() == true) {
                    if (_script.empty() == false) {
                        system(_script.c_str());
                    }
                }
                Block();

                _parent.StateChange(PluginHost::IStateControl::EXITED);
                return (Core::infinite);
            }

        private:
            DeviceProvisioningImplementation& _parent;
            string _script;
        };

    public:
        DeviceProvisioningImplementation()
            : _task(*this)
            , _adminLock()
            , _state(PluginHost::IStateControl::UNINITIALIZED)
            , _stateControlClients()
        {
        }

        ~DeviceProvisioningImplementation() override
        {
        }

        uint32_t Configure(PluginHost::IShell* service) override
        {
            uint32_t result = _task.Configure(service);

            _state = PluginHost::IStateControl::RESUMED;
            return (result);
        }

        PluginHost::IStateControl::state State() const override
        {
            return (_state);
        }

        uint32_t Request(const PluginHost::IStateControl::command command) override
        {
            return Core::ERROR_NONE;
        }

        void Register(PluginHost::IStateControl::INotification* sink) override
        {
            _adminLock.Lock();

            ASSERT(std::find(_stateControlClients.begin(), _stateControlClients.end(), sink) == _stateControlClients.end());
            _stateControlClients.push_back(sink);
            sink->AddRef();

            _adminLock.Unlock();
        }

        void Unregister(PluginHost::IStateControl::INotification* sink) override
        {
            _adminLock.Lock();

            auto index(std::find(_stateControlClients.begin(), _stateControlClients.end(), sink));
            ASSERT(index != _stateControlClients.end());
            if (index != _stateControlClients.end()) {
                (*index)->Release();
                _stateControlClients.erase(index);
            }

            _adminLock.Unlock();
        }

        BEGIN_INTERFACE_MAP(DeviceProvisioningImplementation)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        INTERFACE_ENTRY(PluginHost::IStateControl)
        END_INTERFACE_MAP

    private:
        void StateChange(const PluginHost::IStateControl::state newState)
        {
            _adminLock.Lock();

            _state = newState;

            auto index(_stateControlClients.begin());
            while (index != _stateControlClients.end()) {
                (*index)->StateChange(newState);
                index++;
            }

            _adminLock.Unlock();
        }

    private:
        DeviceProvisioningTask _task;
        Core::CriticalSection _adminLock;
        PluginHost::IStateControl::state _state;
        std::list<PluginHost::IStateControl::INotification*> _stateControlClients;
    };

    SERVICE_REGISTRATION(DeviceProvisioningImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
