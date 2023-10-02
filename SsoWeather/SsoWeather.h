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

#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class SsoWeather : public PluginHost::IPlugin {
    public:
        class Notification : public RPC::IRemoteConnection::INotification {
        public:
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            explicit Notification(SsoWeather* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification() override = default;

        public:
        /*Stany pluginu*/
            void Activated(RPC::IRemoteConnection* /* connection */) override
            {
                printf("STD: Notification Activated\n");
                fprintf(stderr,"STDERR: Notification Activated\n");
            }

            void Deactivated(RPC::IRemoteConnection* connectionId) override
            {
                printf("STD: Notification Deactivated\n");
                fprintf(stderr,"STDERR: Notification Deactivated\n");
            }

            void Terminated(RPC::IRemoteConnection* /* connection */) override
            {
                printf("STD: Notification Terminated\n");
                fprintf(stderr,"STDERR: Notification Terminated\n");
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            SsoWeather& _parent;
        };

    public:
        SsoWeather(const SsoWeather&) = delete;
        SsoWeather& operator=(const SsoWeather&) = delete;

        SsoWeather()
            : _service(nullptr)
            , _connectionId(0)
            , _notification(this)
        {
        }

        ~SsoWeather() override = default;

        BEGIN_INTERFACE_MAP(SsoWeather)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        PluginHost::IShell* _service;
        uint32_t _connectionId;
        Core::Sink<Notification> _notification;
    };

} // namespace Plugin
} // namespace WPEFramework
