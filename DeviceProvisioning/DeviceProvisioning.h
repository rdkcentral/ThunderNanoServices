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

#pragma once

#include "Module.h"
#include <interfaces/IConfiguration.h>

namespace WPEFramework {
namespace Plugin {

    class DeviceProvisioning : public PluginHost::IPlugin {
    private:
        DeviceProvisioning(const DeviceProvisioning&) = delete;
        DeviceProvisioning& operator=(const DeviceProvisioning&) = delete;

        class Notification : public RPC::IRemoteConnection::INotification,
                             public PluginHost::IStateControl::INotification {
        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            Notification(DeviceProvisioning* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }
            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            void StateChange(const PluginHost::IStateControl::state state) override
            {
                _parent.StateChange(state);
            }
            void Activated(RPC::IRemoteConnection*) override
            {
            }
            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }

        private:
            DeviceProvisioning& _parent;
        };

    public:
        DeviceProvisioning()
            : _connectionId(0)
            , _deviceprovisioning(nullptr)
            , _service(nullptr)
            , _notification(this)
        {
        }
        ~DeviceProvisioning() override
        {
        }
        BEGIN_INTERFACE_MAP(DeviceProvisioning)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

    public:
        // IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        void ConnectionTerminate(uint32_t connectionId);
        void StateChange(const PluginHost::IStateControl::state state);
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        uint32_t _connectionId;
        Exchange::IConfiguration* _deviceprovisioning;
        PluginHost::IShell* _service;
        Core::Sink<Notification> _notification;
    };
} // namespace Plugin
} // namespace WPEFramework
