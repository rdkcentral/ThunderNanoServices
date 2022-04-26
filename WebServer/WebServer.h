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
 
#ifndef __PLUGINWEBSERVER_H
#define __PLUGINWEBSERVER_H

#include "Module.h"
#include <interfaces/IMemory.h>
#include <interfaces/IWebServer.h>

namespace WPEFramework {
namespace Plugin {

    class WebServer : public PluginHost::IPlugin, public PluginHost::IWeb{
    private:

        class Notification : public RPC::IRemoteConnection::INotification, public PluginHost::IStateControl::INotification {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            explicit Notification(WebServer* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification() override
            {
                TRACE(Trace::Information, (_T("WebServer::Notification destructed. Line: %d"), __LINE__));
            }

        public:
            void Activated(RPC::IRemoteConnection* /* connection */)
            {
            }
            void Deactivated(RPC::IRemoteConnection* connectionId)
            {
                _parent.Deactivated(connectionId);
            }


            void StateChange(const PluginHost::IStateControl::state state) override {
                _parent.StateChange(state);
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            INTERFACE_ENTRY (PluginHost::IStateControl::INotification)
            END_INTERFACE_MAP

        private:
            WebServer& _parent;
        };

    public:

        WebServer(const WebServer&) = delete;
        WebServer& operator=(const WebServer&) = delete;
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        WebServer()
            : _connectionId(0)
            , _service(nullptr)
            , _server(nullptr)
            , _memory(nullptr)
            , _notification(this)
        {
        }
POP_WARNING()
        ~WebServer() override = default;

        BEGIN_INTERFACE_MAP(WebServer)
        INTERFACE_ENTRY(IPlugin)
        INTERFACE_ENTRY(IWeb)
        INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        INTERFACE_AGGREGATE(Exchange::IWebServer, _server)
        INTERFACE_AGGREGATE(PluginHost::IStateControl, _server)
        END_INTERFACE_MAP

    public:
        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        const string Initialize(PluginHost::IShell* service) override;

        // The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        string Information() const override;

        void StateChange(PluginHost::IStateControl::state);


        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request &request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request &request) override;

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        uint32_t _skipURL;
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Exchange::IWebServer* _server;
        Exchange::IMemory* _memory;
        Core::Sink<Notification> _notification;
    };
}
}

#endif // __PLUGINWEBSERVER_H
