/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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
#include <interfaces/IYang.h>
#include <interfaces/JYang.h>

namespace Thunder {

namespace Plugin {

    class Yang : public PluginHost::IPlugin
               , public PluginHost::JSONRPC {
    private:
        class Notification : public RPC::IRemoteConnection::INotification
                           , public Exchange::IYang::INotification {
        public:
            Notification(Yang& parent)
                : _parent(parent) {
            }
            ~Notification() override = default;

            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            // RPC::IRemoteConnection::INotification overrides
            void Activated(RPC::IRemoteConnection*) override
            {
            }
            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }

        public:
            // Exchange::IYang::INotification overrides
            void BalanceChanged(const uint8_t percentage)
            {
                // Upstream the notification...
                Exchange::JYang::Event::BalanceChanged(_parent, percentage);
            }

        public:
            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            Yang& _parent;
        }; // class Notification

    public:
        Yang()
            : _service(nullptr)
            , _notification(*this)
            , _connectionId(0)
            , _yangImplementation(nullptr)
        {
        }
        ~Yang() override = default;

        Yang(const Yang&) = delete;
        Yang &operator=(const Yang&) = delete;

    public:
        // PluginHost::IPlugin overrides
        const string Initialize(PluginHost::IShell *service) override;
        void Deinitialize(PluginHost::IShell *service) override;
        string Information() const override;

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    public:
        BEGIN_INTERFACE_MAP(Yang)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IYang, _yangImplementation)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell *_service;
        Core::SinkType<Notification> _notification;
        uint32_t _connectionId;
        Exchange::IYang *_yangImplementation;
    }; // class Yang

} // namespace Plugin

}
