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

#ifndef __BROWSER_H
#define __BROWSER_H

#include "Module.h"
#include <interfaces/IBrowser.h>
#include <interfaces/IComposition.h>
#include <interfaces/IMemory.h>
#include <interfaces/json/JsonData_Browser.h>
#include <interfaces/json/JsonData_StateControl.h>

namespace WPEFramework {
namespace Plugin {

    class WebKitBrowser : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:
        WebKitBrowser(const WebKitBrowser&);
        WebKitBrowser& operator=(const WebKitBrowser&);

        class Notification : public RPC::IRemoteConnection::INotification,
                             public PluginHost::IStateControl::INotification,
                             public Exchange::IBrowser::INotification {

        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            explicit Notification(WebKitBrowser* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~Notification()
            {
            }

        public:
            virtual void LoadFinished(const string& URL) override
            {
                _parent.LoadFinished(URL);
            }
            virtual void URLChanged(const string& URL) override
            {
                _parent.URLChanged(URL);
            }
            virtual void Hidden(const bool hidden) override
            {
                _parent.Hidden(hidden);
            }
            virtual void Closure() override
            {
                _parent.Closure();
            }
            virtual void StateChange(const PluginHost::IStateControl::state state) override
            {
                _parent.StateChange(state);
            }
            virtual void Activated(RPC::IRemoteConnection* /* connection */) override
            {
            }
            virtual void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::IBrowser::INotification)
            INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            WebKitBrowser& _parent;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , OutOfProcess(true)
                , EnvironmentOverride(false)
            {
                Add(_T("outofprocess"), &OutOfProcess);
                Add(_T("environmentoverride"), &EnvironmentOverride);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::Boolean OutOfProcess;
            Core::JSON::Boolean EnvironmentOverride;
        };

    public:
        class Data : public Core::JSON::Container {
        private:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

        public:
            Data()
                : Core::JSON::Container()
                , URL()
                , FPS()
                , Suspended(false)
                , Hidden(false)
                , Path()
            {
                Add(_T("url"), &URL);
                Add(_T("fps"), &FPS);
                Add(_T("suspended"), &Suspended);
                Add(_T("hidden"), &Hidden);
                Add(_T("path"), &Path);
            }
            ~Data()
            {
            }

        public:
            Core::JSON::String URL;
            Core::JSON::DecUInt32 FPS;
            Core::JSON::Boolean Suspended;
            Core::JSON::Boolean Hidden;
            Core::JSON::String Path;
        };

    public:
        WebKitBrowser()
            : _skipURL(0)
            , _hidden(false)
            , _service(nullptr)
            , _browser(nullptr)
            , _memory(nullptr)
            , _notification(this)
            , _jsonBodyDataFactory(2)
        {
            RegisterAll();
        }

        virtual ~WebKitBrowser()
        {
            UnregisterAll();
            TRACE_L1("Destructor WebKitBrowser.%d", __LINE__);
        }

        inline static bool EnvironmentOverride(const bool configFlag)
        {
            bool result = configFlag;

            if (result == false) {
                string value;
                Core::SystemInfo::GetEnvironment(_T("WPE_ENVIRONMENT_OVERRIDE"), value);
                result = (value == "1");
            }
            return (result);
        }

    public:
        BEGIN_INTERFACE_MAP(WebKitBrowser)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(PluginHost::IStateControl, _browser)
        INTERFACE_AGGREGATE(Exchange::IBrowser, _browser)
        INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
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
        virtual const string Initialize(PluginHost::IShell* service);

        // The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

    private:
        void Deactivated(RPC::IRemoteConnection* connection);
        void LoadFinished(const string& URL);
        void URLChanged(const string& URL);
        void Hidden(const bool hidden);
        void Closure();
        void StateChange(const PluginHost::IStateControl::state state);

        // JsonRpc
        void RegisterAll();
        void UnregisterAll();
        uint32_t get_url(Core::JSON::String& response) const; // Browser
        uint32_t set_url(const Core::JSON::String& param); // Browser
        uint32_t get_visibility(Core::JSON::EnumType<JsonData::Browser::VisibilityType>& response) const; // Browser
        uint32_t set_visibility(const Core::JSON::EnumType<JsonData::Browser::VisibilityType>& param); // Browser
        uint32_t get_fps(Core::JSON::DecUInt32& response) const; // Browser
        uint32_t get_state(Core::JSON::EnumType<JsonData::StateControl::StateType>& response) const; // StateControl
        uint32_t set_state(const Core::JSON::EnumType<JsonData::StateControl::StateType>& param); // StateControl
        uint32_t endpoint_delete(const JsonData::Browser::DeleteParamsData& params);
        uint32_t delete_dir(const string& path);
        void event_urlchange(const string& url, const bool& loaded); // Browser
        void event_visibilitychange(const bool& hidden); // Browser
        void event_pageclosure(); // Browser
        void event_statechange(const bool& suspended); // StateControl

    private:
        uint8_t _skipURL;
        uint32_t _connectionId;
        bool _hidden;
        PluginHost::IShell* _service;
        Exchange::IBrowser* _browser;
        Exchange::IMemory* _memory;
        Core::Sink<Notification> _notification;
        Core::ProxyPoolType<Web::JSONBodyType<WebKitBrowser::Data>> _jsonBodyDataFactory;
        string _persistentStoragePath;
    };
}
}

#endif // __BROWSER_H
