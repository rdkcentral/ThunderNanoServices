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

#include <interfaces/IMath.h>

namespace Thunder {

namespace Plugin {

    class Castor : public PluginHost::IPlugin
                 , public PluginHost::JSONRPC {

    private:
        class Notification : public RPC::IRemoteConnection::INotification {
        public:
            Notification(Castor& parent)
                : _parent(parent)
            {
            }
            ~Notification() override = default;

            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;
            Notification(Notification&&) = delete;
            Notification& operator=(Notification&&) = delete;

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
            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            Castor& _parent;
        }; // class Notification

    public:
        Castor()
            : _service(nullptr)
            , _notification(*this)
            , _connectionId(0)
            , _mathImplementation(nullptr)
        {
        }
        ~Castor() override = default;

        Castor(const Castor&) = delete;
        Castor &operator=(const Castor&) = delete;
        Castor(Castor&&) = delete;
        Castor &operator=(Castor&&) = delete;

    public:
        // PluginHost::IPlugin overrides
        const string Initialize(PluginHost::IShell *service) override;
        void Deinitialize(PluginHost::IShell *service) override;
        string Information() const override;

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    public:
        BEGIN_INTERFACE_MAP(Castor)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IMath, _mathImplementation)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell *_service;
        Core::SinkType<Notification> _notification;
        uint32_t _connectionId;
        Exchange::IMath *_mathImplementation;
    }; // class Castor

} // namespace Plugin

}
