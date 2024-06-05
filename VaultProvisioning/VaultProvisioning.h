/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological
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

namespace Thunder {
namespace Plugin {

    class VaultProvisioning : public PluginHost::IPlugin {
    public:
        VaultProvisioning()
            : _implementation(nullptr)
            , _connectionId(0)
            , _service(nullptr)
            , _connectionNotification(this)
        {
        }

        ~VaultProvisioning() override = default;
        VaultProvisioning(const VaultProvisioning&) = delete;
        VaultProvisioning& operator=(const VaultProvisioning&) = delete;

    public:
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override { return {}; }

    private:
        class ConnectionNotification : public RPC::IRemoteConnection::INotification {
        public:
            explicit ConnectionNotification(VaultProvisioning* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            ~ConnectionNotification() override = default;
            ConnectionNotification() = delete;
            ConnectionNotification(const ConnectionNotification&) = delete;
            ConnectionNotification& operator=(const ConnectionNotification&) = delete;

        public:
            void Activated(RPC::IRemoteConnection*) override
            {
            }
            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }

        public:
            BEGIN_INTERFACE_MAP(ConnectionNotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            VaultProvisioning& _parent;
        }; // class ConnectionNotification

    public:
        BEGIN_INTERFACE_MAP(VaultProvisioning)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_AGGREGATE(Exchange::IConfiguration, _implementation)
        END_INTERFACE_MAP

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        Exchange::IConfiguration* _implementation;
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Core::SinkType<ConnectionNotification> _connectionNotification;
    }; // class VaultProvisioning

} // namespace Plugin
}
