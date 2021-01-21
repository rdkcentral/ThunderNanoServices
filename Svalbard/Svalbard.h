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
#include <cryptography/cryptography.h>
#include <interfaces/IConfiguration.h>

namespace WPEFramework {
namespace Plugin {
    class Svalbard : public PluginHost::IPlugin {
    private:
        Svalbard(const Svalbard&) = delete;
        Svalbard& operator=(const Svalbard&) = delete;

        class Notification : public RPC::IRemoteConnection::INotification {

        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            explicit Notification(Svalbard* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }

        public:
            virtual void Activated(RPC::IRemoteConnection*)
            {
            }
            virtual void Deactivated(RPC::IRemoteConnection* connection)
            {
                _parent.Deactivated(connection);
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            Svalbard& _parent;
        };

    public:
        Svalbard()
            : _connectionId(0)
            , _service(nullptr)
            , _svalbard(nullptr)
            , _notification(this)
        {
        }

        virtual ~Svalbard()
        {
        }

        BEGIN_INTERFACE_MAP(Svalbard)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service);
        virtual void Deinitialize(PluginHost::IShell* service);
        virtual string Information() const;
    
    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Exchange::IConfiguration* _svalbard;
        Core::Sink<Notification> _notification;
    };

} // Namespace Plugin.
}
