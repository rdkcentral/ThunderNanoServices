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
 
#ifndef __POWER_H
#define __POWER_H

#include "Module.h"
#include <interfaces/IPower.h>
#include <interfaces/json/JsonData_Power.h>

namespace WPEFramework {
namespace Plugin {

    class Power 
        : public PluginHost::IPlugin 
        , public PluginHost::IWeb
        , public Exchange::IPower
        , public PluginHost::JSONRPC {

    private:
        class Notification 
            : public PluginHost::IPlugin::INotification
            , public PluginHost::VirtualInput::INotifier {

        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            explicit Notification(Power* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }

        public:
            virtual void Dispatch(const IVirtualInput::KeyData::type type, const uint32_t code) override
            {
                if (type == IVirtualInput::KeyData::RELEASED) {
                    _parent.KeyEvent(code);
                }
            }
            virtual void StateChange(PluginHost::IShell* plugin)
            {
                _parent.StateChange(plugin);
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            Power& _parent;
        };

        class Entry {
        private:
            Entry() = delete;
            Entry(const Entry& copy) = delete;
            Entry& operator=(const Entry&) = delete;

        public:
            Entry(PluginHost::IStateControl* entry)
                : _shell(entry)
                , _lastStateResumed(false)
            {
                ASSERT(_shell != nullptr);
                _shell->AddRef();
            }
            ~Entry()
            {
                _shell->Release();
            }

        public:
            bool Suspend()
            {
                bool succeeded(true);
                if (_shell->State() == PluginHost::IStateControl::RESUMED) {
                    _lastStateResumed = true;
                    succeeded = (_shell->Request(PluginHost::IStateControl::SUSPEND) == Core::ERROR_NONE);
                }
                return (succeeded);
            }
            bool Resume()
            {
                bool succeeded(true);
                if (_lastStateResumed == true) {
                    _lastStateResumed = false;
                    succeeded = (_shell->Request(PluginHost::IStateControl::RESUME) == Core::ERROR_NONE);
                }
                return (succeeded);
            }

        public:
            PluginHost::IStateControl* _shell;
            bool _lastStateResumed;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , PowerKey(0)
                , OffMode(Exchange::IPower::PCState::SuspendToRAM)
                , ControlClients(true)
            {
                Add(_T("powerkey"), &PowerKey);
                Add(_T("offmode"), &OffMode);
                Add(_T("control"), &ControlClients);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt32 PowerKey;
            Core::JSON::EnumType<Exchange::IPower::PCState> OffMode;
            Core::JSON::Boolean ControlClients;
        };

        typedef std::map<const string, Entry> Clients;

    public:
        class Data : public Core::JSON::Container {
        private:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

        public:
            Data()
                : Core::JSON::Container()
                , Status()
                , PowerState()
                , Timeout()
            {
                Add(_T("Status"), &Status);
                Add(_T("PowerState"), &PowerState);
                Add(_T("Timeout"), &Timeout);
            }
            ~Data()
            {
            }

        public:
            Core::JSON::DecUInt32 Status;
            Core::JSON::DecUInt32 PowerState;
            Core::JSON::DecUInt32 Timeout;
        };

    public:
        Power(const Power&) = delete;
        Power& operator=(const Power&) = delete;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
        Power()
            : _adminLock()
            , _skipURL(0)
            , _service(nullptr)
            , _clients()
            , _sink(this)
            , _notificationClients()
            , _powerKey(0)
            , _controlClients(true)
            , _powerOffMode(Exchange::IPower::PCState::SuspendToRAM)
        {
            RegisterAll();
        }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
        virtual ~Power()
        {
            UnregisterAll();
        }

    public:
        BEGIN_INTERFACE_MAP(Power)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(Exchange::IPower)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
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

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        //  IPower methods
        // -------------------------------------------------------------------------------------------------------
        void Register(IPower::INotification* sink) override;
        void Unregister(IPower::INotification* sink) override;
        PCState GetState() const override;
        uint32_t SetState(const PCState, const uint32_t) override;
        void PowerKey() override;

        void PowerChange(const Exchange::IPower::PCState state);

    private:
        void KeyEvent(const uint32_t keyCode);
        void StateChange(PluginHost::IShell* plugin);
        void ControlClients(Exchange::IPower::PCState state);

        void RegisterAll();
        void UnregisterAll();
        inline bool InRange(Exchange::IPower::PCState value);
        inline Exchange::IPower::PCState TranslateIn(JsonData::Power::StateType value);
        inline JsonData::Power::StateType TranslateOut(Exchange::IPower::PCState value) const;
        uint32_t endpoint_set(const JsonData::Power::PowerData& params);
        uint32_t get_state(Core::JSON::EnumType<JsonData::Power::StateType>& response) const;

    private:
        Core::CriticalSection _adminLock;
        uint32_t _skipURL;
        PluginHost::IShell* _service;
        Clients _clients;
        Core::Sink<Notification> _sink;
        std::list<Exchange::IPower::INotification*> _notificationClients;
        uint32_t _powerKey;
        bool _controlClients;
        Exchange::IPower::PCState _powerOffMode;
    };
} //namespace Plugin
} //namespace WPEFramework

#endif // __POWER_H
