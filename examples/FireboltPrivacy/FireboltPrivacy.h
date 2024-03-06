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

#include <interfaces/IFireboltPrivacy.h>
#include <interfaces/json/JFireboltPrivacy.h>
#include <interfaces/json/JsonData_FireboltPrivacy.h>

namespace WPEFramework {
namespace Plugin {

    class FireboltPrivacy : public PluginHost::IPlugin,
                           public PluginHost::JSONRPC {
    private:
        class ConnectionNotification : public RPC::IRemoteConnection::INotification, public Exchange::IFireboltPrivacy::INotification {
        public:
            ConnectionNotification() = delete;
            ConnectionNotification(const ConnectionNotification&) = delete;
            ConnectionNotification(ConnectionNotification&&) = delete;
            ConnectionNotification& operator=(const ConnectionNotification&) = delete;
            ConnectionNotification& operator=(ConnectionNotification&&) = delete;

            ConnectionNotification(FireboltPrivacy& parent)
                : _parent(parent)
            {
            }
            ~ConnectionNotification() override = default;

        public:
            void Activated(RPC::IRemoteConnection*  /* connection */) override
            {
            }
            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }
            void Terminated (RPC::IRemoteConnection* /* connection */) override
            {
            }


            void OnAllowResumePointsChanged(const bool allowResumePoint) override
            {
                _parent.OnAllowResumePointsChanged(allowResumePoint);
            }
            BEGIN_INTERFACE_MAP(ConnectionNotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                INTERFACE_ENTRY(Exchange::IFireboltPrivacy::INotification)
            END_INTERFACE_MAP

        private:
            FireboltPrivacy& _parent;
        };
    public:
        FireboltPrivacy(const FireboltPrivacy&) = delete;
        FireboltPrivacy(FireboltPrivacy&&) = delete;
        FireboltPrivacy& operator=(const FireboltPrivacy&) = delete;
        FireboltPrivacy& operator=(FireboltPrivacy&) = delete;

        FireboltPrivacy();
        ~FireboltPrivacy() override = default;

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(FireboltPrivacy)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IFireboltPrivacy, _fireboltPrivacy)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;


    private:
        void Deactivated(RPC::IRemoteConnection* connection);
        void OnAllowResumePointsChanged(const bool allowResumePoint);

    private:
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Exchange::IFireboltPrivacy* _fireboltPrivacy;
        Core::SinkType<ConnectionNotification> _connectionNotification;
        
    };

} // namespace Plugin
} // namespace WPEFramework
