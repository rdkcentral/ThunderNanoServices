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

#ifndef WIFICONTROL_H
#define WIFICONTROL_H

#include "Module.h"
#include <interfaces/json/JWifiControl.h>

namespace Thunder {
namespace Plugin {

    class WifiControl : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:
        class ConnectionNotification : public RPC::IRemoteConnection::INotification {
        public:
            ConnectionNotification() = delete;
            ConnectionNotification(const ConnectionNotification&) = delete;
            ConnectionNotification& operator=(const ConnectionNotification&) = delete;

            explicit ConnectionNotification(WifiControl* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~ConnectionNotification() override = default;

        public:
            void Activated(RPC::IRemoteConnection* /* connection */) override
            {
            }
            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }
            void Terminated(RPC::IRemoteConnection* /* connection */) override
            {
            }

            BEGIN_INTERFACE_MAP(ConnectionNotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            WifiControl& _parent;
        };
        class WifiNotification : public Exchange::IWifiControl::INotification {
        public:
            explicit WifiNotification(WifiControl* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            ~WifiNotification() override = default;

            WifiNotification() = delete;
            WifiNotification(const WifiNotification&) = delete;
            WifiNotification& operator=(const WifiNotification&) = delete;

            void NetworkChange() override
            {
                Exchange::JWifiControl::Event::NetworkChange(_parent);
                string message("{ \"event\": \"NetworkChange\"}");
                _parent.Service()->Notify(message);
            }

            void ConnectionChange(const string& ssid) override
            {
                Exchange::JWifiControl::Event::ConnectionChange(_parent, ssid);
                string message("{ \"event\": \"ConnectionChange\", \"ssid\": \"" + ssid + "\" }");
                _parent.Service()->Notify(message);
            }

            BEGIN_INTERFACE_MAP(WifiNotification)
                INTERFACE_ENTRY(Exchange::IWifiControl::INotification)
            END_INTERFACE_MAP

        private:
            WifiControl& _parent;
        };

    public:
        WifiControl(const WifiControl&) = delete;
        WifiControl& operator=(const WifiControl&) = delete;

        WifiControl();

        ~WifiControl() override = default;

    public:
        inline PluginHost::IShell* Service()
        {
            return _service;
        }

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

        Core::ProxyType<Web::Response> SetConfig(const Web::Request& request, const string& ssid);
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index);

    public:
        BEGIN_INTERFACE_MAP(WifiControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IWifiControl, _wifiControl)
        END_INTERFACE_MAP

    private:
        uint8_t _skipURL;
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Exchange::IWifiControl* _wifiControl;
        Core::SinkType<ConnectionNotification> _connectionNotification;
        Core::SinkType<WifiNotification> _wifiNotification;
    };

} // namespace Plugin
} // namespace Thunder

#endif // WIFICONTROL_H
