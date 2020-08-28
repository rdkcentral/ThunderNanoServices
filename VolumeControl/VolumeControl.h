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
#include <interfaces/IVolumeControl.h>
#include <interfaces/json/JVolumeControl.h>

namespace WPEFramework {
namespace Plugin {

    class VolumeControl : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        VolumeControl(const VolumeControl&) = delete;
        VolumeControl& operator=(const VolumeControl&) = delete;

        VolumeControl()
            : _implementation(nullptr)
            , _connectionId(0)
            , _service(nullptr)
            , _connectionNotification(this)
            , _volumeNotification(this)
        {
        }

        ~VolumeControl() override
        {
        }

        BEGIN_INTERFACE_MAP(VolumeControl)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::IVolumeControl, _implementation)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        class ConnectionNotification : public RPC::IRemoteConnection::INotification {
        public:
            explicit ConnectionNotification(VolumeControl* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            ~ConnectionNotification() override
            {
            }

            ConnectionNotification() = delete;
            ConnectionNotification(const ConnectionNotification&) = delete;
            ConnectionNotification& operator=(const ConnectionNotification&) = delete;

            void Activated(RPC::IRemoteConnection*) override
            {
            }

            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }

            BEGIN_INTERFACE_MAP(ConnectionNotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            VolumeControl& _parent;
        };

        class VolumeNotification : public Exchange::IVolumeControl::INotification {
        public:
            explicit VolumeNotification(VolumeControl* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            ~VolumeNotification() override
            {
            }

            VolumeNotification() = delete;
            VolumeNotification(const VolumeNotification&) = delete;
            VolumeNotification& operator=(const VolumeNotification&) = delete;

            void Volume(const uint8_t volume) override
            {
                Exchange::JVolumeControl::Event::Volume(_parent, volume);
            }

            void Muted(const bool muted) override
            {
                Exchange::JVolumeControl::Event::Muted(_parent, muted);
            }

            BEGIN_INTERFACE_MAP(VolumeNotification)
                INTERFACE_ENTRY(Exchange::IVolumeControl::INotification)
            END_INTERFACE_MAP

        private:
            VolumeControl& _parent;
        };

        void Deactivated(RPC::IRemoteConnection* connection);

        Exchange::IVolumeControl* _implementation;
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Core::Sink<ConnectionNotification> _connectionNotification;
        Core::Sink<VolumeNotification> _volumeNotification;

    };

} // namespace Plugin
} // namespace WPEFramework
