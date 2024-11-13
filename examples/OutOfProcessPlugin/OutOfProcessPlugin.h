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
 
#ifndef __OUTOFPROCESSTEST_H
#define __OUTOFPROCESSTEST_H

#include "Module.h"
#include <interfaces/IBrowser.h>
#include <interfaces/IMemory.h>

#include <interfaces/json/JBrowserResources.h>

namespace Thunder {
namespace Plugin {

    class OutOfProcessPlugin : public PluginHost::IPluginExtended, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:

        class Notification : public Exchange::IBrowser::INotification,
                             public PluginHost::IStateControl::INotification,
                             public RPC::IRemoteConnection::INotification,
                             public PluginHost::IPlugin::INotification {

        public:
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            explicit Notification(OutOfProcessPlugin* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
                TRACE(Trace::Information, (_T("================ Constructing the Notification =================\n")));

            }
            ~Notification() override
            {
                TRACE(Trace::Information, (_T("================ Destructing the Notification =================\n")));
                TRACE(Trace::Information, (_T("WebServer::Notification destructed. Line: %d"), __LINE__));
            }

        public:
             void LoadFinished(const string& URL) override
            {
                _parent.LoadFinished(URL);
            }
            void URLChanged(const string& URL) override
            {
                _parent.URLChanged(URL);
            }
            void Hidden(const bool hidden) override
            {
                _parent.Hidden(hidden);
            }
            void StateChange(const PluginHost::IStateControl::state value) override
            {
                _parent.StateChange(value);
            }
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
            void Closure() override
            {
            }

            void Activated(const string& callsign, PluginHost::IShell* plugin) override
            {
                _parent.Activated(callsign, plugin);
            }
            void Deactivated(const string& callsign, PluginHost::IShell* plugin) override
            {
                _parent.Deactivated(callsign, plugin);
            }
            void Unavailable(const string&, PluginHost::IShell*) override
            {
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::IBrowser::INotification)
            INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            OutOfProcessPlugin& _parent;
        };

    public:
        class Data : public Core::JSON::Container {

        public:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

            Data()
                : Core::JSON::Container()
                , URL()
                , FPS()
                , Suspended(false)
                , Hidden(false)
            {
                Add(_T("url"), &URL);
                Add(_T("fps"), &FPS);
                Add(_T("suspended"), &Suspended);
                Add(_T("hidden"), &Hidden);
            }

            Data(const string& url)
                : Core::JSON::Container()
                , URL()
                , FPS()
                , Suspended(false)
                , Hidden(false)
            {
                Add(_T("url"), &URL);
                Add(_T("fps"), &FPS);
                Add(_T("suspended"), &Suspended);
                Add(_T("hidden"), &Hidden);
                URL = url;
            }
            ~Data() override = default;

        public:
            Core::JSON::String URL;
            Core::JSON::DecUInt32 FPS;
            Core::JSON::Boolean Suspended;
            Core::JSON::Boolean Hidden;
        };

    public:
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        OutOfProcessPlugin(const OutOfProcessPlugin&) = delete;
        OutOfProcessPlugin& operator=(const OutOfProcessPlugin&) = delete;

        OutOfProcessPlugin()
            : _adminLock()
            , _skipURL(0)
            , _connectionId(0)
            , _webPath("")
            , _service(nullptr)
            , _browser(nullptr)
            , _browserresources(nullptr)
            , _memory(nullptr)
            , _notification(Core::ServiceType<Notification>::Create<Notification>(this))
            , _state(nullptr)
            , _subscriber(nullptr)
            , _hidden(false)
        {
        }
POP_WARNING()
        ~OutOfProcessPlugin() override
        {
            _notification->Release();
        }

    public:
        BEGIN_INTERFACE_MAP(OutOfProcessPlugin)
        INTERFACE_ENTRY(IPlugin)
        INTERFACE_ENTRY(IPluginExtended)
        INTERFACE_ENTRY(IWeb)
        INTERFACE_AGGREGATE(Exchange::IBrowser, _browser)
        INTERFACE_AGGREGATE(Exchange::IBrowserResources, _browserresources)
        INTERFACE_AGGREGATE(PluginHost::IStateControl, _browser)
        INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        using PluginHost::JSONRPC::Attach;
        using PluginHost::JSONRPC::Detach;

        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------

        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        const string Initialize(PluginHost::IShell* service) override; 

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        string Information() const override;

        // ================================== CALLED ON COMMUNICATION THREAD =====================================
        // Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
        // Whenever the channel is closed, it is reported via the detach method.
        bool Attach(PluginHost::Channel& channel) override;
        void Detach(PluginHost::Channel& channel) override;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        void LoadFinished(const string& URL);
        void URLChanged(const string& URL);
        void Hidden(const bool hidden);
        void StateChange(const PluginHost::IStateControl::state value);
        void Deactivated(RPC::IRemoteConnection* connection);

        static const char* PluginStateStr(const PluginHost::IShell::state state);
        void Activated(const string& callsign, PluginHost::IShell* plugin);
        void Deactivated(const string& callsign, PluginHost::IShell* plugin);

    private:
        Core::CriticalSection _adminLock;
        uint8_t _skipURL;
        uint32_t _connectionId;
        string _webPath;
        PluginHost::IShell* _service;
        Exchange::IBrowser* _browser;
        Exchange::IBrowserResources* _browserresources;
        Exchange::IMemory* _memory;
        Notification* _notification;
        PluginHost::IStateControl* _state;
        PluginHost::Channel* _subscriber;
        bool _hidden;
    };
}
}

#endif // __OUTOFPROCESSTEST_H
