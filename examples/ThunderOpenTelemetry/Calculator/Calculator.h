/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

//********************************** */


// NOTE generated with AI, might not adhere to all proper Thunder conventions  (e.g. Plugin activation/deactivation order)


 //********************************** */




#pragma once

#include "Module.h"

#include <interfaces/IMath.h>

namespace WPEFramework {
namespace Plugin {

    class Calculator : public PluginHost::IPlugin
        , public PluginHost::JSONRPC {
    private:
        class Notification : public RPC::IRemoteConnection::INotification, public Exchange::IMathExtended::INotification {
        public:
            explicit Notification(Calculator& parent)
                : _parent(parent)
            {
            }
            ~Notification() override = default;

            void Activated(RPC::IRemoteConnection* connection VARIABLE_IS_NOT_USED) override
            {
            }
            void Deactivated(RPC::IRemoteConnection* connection) override;
            void MultiplyResult(const uint32_t result) override;

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                INTERFACE_ENTRY(Exchange::IMathExtended::INotification) 
            END_INTERFACE_MAP

        private:
            Calculator& _parent;
        };

    public:
        Calculator();
        Calculator(const Calculator&) = delete;
        Calculator& operator=(const Calculator&) = delete;
        ~Calculator() override = default;

        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        void Deactivated(RPC::IRemoteConnection* connection);

        BEGIN_INTERFACE_MAP(Calculator)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IMath, _mathImplementation)
            INTERFACE_AGGREGATE(Exchange::IMathExtended, _mathExtendedImplementation)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        Core::SinkType<Notification> _notification;
        uint32_t _connectionId;
        Exchange::IMath* _mathImplementation;
        Exchange::IMathExtended* _mathExtendedImplementation;
    };

} // namespace Plugin
} // namespace WPEFramework