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

#ifndef PLUGIN_NETWORKCONTROL_H
#define PLUGIN_NETWORKCONTROL_H

#include "Module.h"
#include "DHCPClient.h"

#include <interfaces/IIPNetwork.h>
#include <interfaces/INetworkControl.h>
#include <interfaces/json/JNetworkControl.h>

namespace WPEFramework {
namespace Plugin {

    class NetworkControl : public PluginHost::IPlugin,
                           public PluginHost::IWeb,
                           public PluginHost::JSONRPC {
    private:
        class ConnectionNotification : public RPC::IRemoteConnection::INotification {
        public:
            ConnectionNotification() = delete;
            ConnectionNotification(const ConnectionNotification&) = delete;
            ConnectionNotification& operator=(const ConnectionNotification&) = delete;

            explicit ConnectionNotification(NetworkControl* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~ConnectionNotification() override = default;

        public:
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
            NetworkControl& _parent;
        };

        class NetworkNotification : public Exchange::INetworkControl::INotification {
        public:
            explicit NetworkNotification(NetworkControl* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            ~NetworkNotification() override
            {
            }

            NetworkNotification() = delete;
            NetworkNotification(const NetworkNotification&) = delete;
            NetworkNotification& operator=(const NetworkNotification&) = delete;

            void Update(const string& interface) override
            {
                Exchange::JNetworkControl::Event::Update(_parent, interface);
            }

            BEGIN_INTERFACE_MAP(NetworkNotification)
                INTERFACE_ENTRY(Exchange::INetworkControl::INotification)
            END_INTERFACE_MAP

        private:
            NetworkControl& _parent;
        };

    public:
        class StatusData : public Core::JSON::Container {
        public:
            StatusData()
                : Core::JSON::Container()
            {
                Add(_T("statustype"), &StatusType);
            }

            StatusData(const StatusData&) = delete;
            StatusData& operator=(const StatusData&) = delete;

        public:
            Core::JSON::EnumType<Exchange::INetworkControl::StatusType> StatusType;
        };

    public:
        NetworkControl(const NetworkControl&) = delete;
        NetworkControl& operator=(const NetworkControl&) = delete;

        NetworkControl();
        ~NetworkControl() override = default;

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(NetworkControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::INetworkControl, _networkControl)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index) const;
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        uint16_t _skipURL;
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Exchange::INetworkControl* _networkControl;
        Core::Sink<ConnectionNotification> _connectionNotification;
        Core::Sink<NetworkNotification> _networkNotification;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // PLUGIN_NETWORKCONTROL_H
