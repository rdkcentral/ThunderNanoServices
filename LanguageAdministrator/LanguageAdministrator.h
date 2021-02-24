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
#include <interfaces/ILanguageTag.h>
#include <interfaces/IApplication.h>
#include <interfaces/json/JLanguageTag.h>

namespace WPEFramework {
namespace Plugin {

    class LanguageAdministrator : public PluginHost::IPlugin, public PluginHost::JSONRPC {

    public:
        LanguageAdministrator(const LanguageAdministrator&) = delete;
        LanguageAdministrator& operator=(const LanguageAdministrator&) = delete;

        LanguageAdministrator()
            :_lock() 
            , _skipURL(0)  
            , _service(nullptr)                      
            ,_impl(nullptr)
            , _connectionId(0)
            , _sink(this)
            , _LanguageTagNotification(this)
        {
        }

        ~LanguageAdministrator() override
        {
        }

    public:
        BEGIN_INTERFACE_MAP(LanguageAdministrator)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::ILanguageTag, _impl)
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


    private:

        class Config: public Core::JSON::Container {
        private: 
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config() : LanguageTag("en_US") 
            {
                Add(_T("languagetag"), &LanguageTag);
            }

            ~Config()
            {
            }

        public:
            Core::JSON::String LanguageTag;
        };

        /*class ApplicationEntry {
        private:
            ApplicationEntry() = delete;
            ApplicationEntry(const ApplicationEntry &copy) = delete;
            ApplicationEntry& operator=(const ApplicationEntry&) = delete;
        public: 
            ApplicationEntry(Exchange::Application* entry) 
                             : _application(entry) 
            {
                ASSERT(entry != nullptr);
                _application->AddRef();

            }
            ~ApplicationEntry() 
            {
                _application->Release();
            }

            Exchange::IApplication* _application;
        };*/

        class Notification: public RPC::IRemoteConnection::INotification,
                            public PluginHost::IPlugin::INotification {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            explicit Notification(LanguageAdministrator* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            
            ~Notification() override
            {
            }

            
            //PluginHost::IPlugin::INotification
            void StateChange(PluginHost::IShell* plugin, const string& callsign) override
            {
                _parent.StateChange(plugin, callsign);
            }

            //RPC::IRemoteConnection::INotification
            void Activated(RPC::IRemoteConnection*) override
            {
            }

            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }            


            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            //INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)            
            END_INTERFACE_MAP

        private:
            LanguageAdministrator& _parent;
        };

        class LanguageTagNotification : public Exchange::ILanguageTag::INotification {
        public:
            explicit LanguageTagNotification(LanguageAdministrator* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            ~LanguageTagNotification() override {
            }

            LanguageTagNotification() = delete;
            LanguageTagNotification(const LanguageTagNotification&) = delete;
            LanguageTagNotification& operator=(const LanguageTagNotification&) = delete;

            void LanguageChanged(const string& lang) override {
                _parent.NotifyLanguageChangesToApps(lang);
                Exchange::JLanguageTag::Event::LanguageChanged(_parent, lang);
            }

            BEGIN_INTERFACE_MAP(LanguageTagNotification)
                INTERFACE_ENTRY(Exchange::ILanguageTag::INotification)
            END_INTERFACE_MAP

        private:
            LanguageAdministrator& _parent;
        };

    
    private:
        
        void StateChange(PluginHost::IShell* plugin, const string& callsign);
        void Deactivated(RPC::IRemoteConnection* connection);
        void ConnectionTermination(uint32_t connectionId);
        std::string GetCurrentLanguageFromPersistentStore();
        void UpdateLanguageUsed(const string& language);
        void NotifyLanguageChangesToApps(const std::string& language);

        Core::CriticalSection  _lock;
        uint32_t _skipURL;
        PluginHost::IShell* _service;
        
        Exchange::ILanguageTag* _impl;
        uint32_t _connectionId;
        std::string _language;
        Core::Sink<Notification> _sink;
        Core::Sink<LanguageTagNotification> _LanguageTagNotification;

        std::unordered_map<std::string, Exchange::IApplication*> _appMap; 
    };
} //namespace Plugin
} //namespace WPEFramework
