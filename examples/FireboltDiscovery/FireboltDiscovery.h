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

#include <interfaces/IFireboltDiscovery.h>
#include <interfaces/json/JFireboltDiscovery.h>
#include <interfaces/json/JsonData_FireboltDiscovery.h>

namespace WPEFramework {
namespace Plugin {

    class FireboltDiscovery : public PluginHost::IPlugin
                            , public PluginHost::JSONRPC
                            , public Exchange::JSONRPC::IFireboltDiscovery {
    private:
#if 0
        class NotificationHandler : public RPC::IRemoteConnection::INotification, public Exchange::IFireboltDiscovery::INotification {
        public:
            NotificationHandler() = delete;
            NotificationHandler(const NotificationHandler&) = delete;
            NotificationHandler(NotificationHandler&&) = delete;
            NotificationHandler& operator=(const NotificationHandler&) = delete;
            NotificationHandler& operator=(NotificationHandler&&) = delete;

            NotificationHandler(FireboltDiscovery& parent)
                : _parent(parent)
            {
            }
            ~NotificationHandler() override = default;

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
            BEGIN_INTERFACE_MAP(NotificationHandler)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                INTERFACE_ENTRY(Exchange::IFireboltDiscovery::INotification)
            END_INTERFACE_MAP

        private:
            FireboltDiscovery& _parent;
        };
#endif
    public:
        FireboltDiscovery(const FireboltDiscovery&) = delete;
        FireboltDiscovery(FireboltDiscovery&&) = delete;
        FireboltDiscovery& operator=(const FireboltDiscovery&) = delete;
        FireboltDiscovery& operator=(FireboltDiscovery&) = delete;

        FireboltDiscovery();
        ~FireboltDiscovery() override = default;

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(FireboltDiscovery)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            //INTERFACE_AGGREGATE(Exchange::IFDiscovery, _fireboltDiscovery)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
        Core::hresult SignIn(const Core::JSONRPC::Context& context, const bool param ) override;


    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Exchange::IFDiscovery* _fireboltDiscovery;
        
    };

} // namespace Plugin
} // namespace WPEFramework
