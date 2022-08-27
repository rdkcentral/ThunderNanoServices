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
 
#pragma once

#include "Module.h"
#include <interfaces/ISwitchBoard.h>

namespace WPEFramework {
namespace Plugin {

    class SwitchBoard : public PluginHost::IPlugin, PluginHost::IWeb, Exchange::ISwitchBoard {

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            enum enumSource {
                Internal,
                External
            };

        public:
            Config()
                : Core::JSON::Container()
                , Default()
                , Callsigns()
            {
                Add(_T("default"), &Default);
                Add(_T("callsigns"), &Callsigns);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Default;
            Core::JSON::ArrayType<Core::JSON::String> Callsigns;
        };

    private:
        SwitchBoard(const SwitchBoard&) = delete;
        SwitchBoard& operator= (const SwitchBoard&) = delete;

        // Trace class for internal information of the SwitchBoard 
        class Switching {
        private:
            Switching() = delete;
            Switching(const Switching& a_Copy) = delete;
            Switching& operator=(const Switching& a_RHS) = delete;

        public:
            Switching(const TCHAR formatter[], ...) {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            Switching(const string& text) : _text(Core::ToString(text)) {
            }
            ~Switching() {
            }

        public:
            inline const char* Data() const {
                return (_text.c_str());
            }
            inline uint16_t Length() const {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };

        class Notification :
            public PluginHost::IPlugin::INotification,
            public PluginHost::IStateControl::INotification {

        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            explicit Notification(SwitchBoard* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }

        public:
            void Activated(const string& callsign, PluginHost::IShell* plugin) override
            {
                _parent.Activated(callsign, plugin);
            }
            void Deactivated(const string& callsign, PluginHost::IShell* plugin) override
            {
                _parent.Deactivated(callsign, plugin);
            }
            void Unavailable(const string& callsign VARIABLE_IS_NOT_USED, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED) override
            {
            }
            void StateChange(const PluginHost::IStateControl::state newState) override
            {
                _parent.StateChange(newState);
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
                INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
            END_INTERFACE_MAP

        private:
            SwitchBoard& _parent;
        };

        class Entry
        {
        private:
            Entry() = delete;
            Entry& operator= (const Entry&) = delete;

        public:
            Entry(PluginHost::IShell* entry)
                : _shell(entry)
            {
                ASSERT(_shell != nullptr);
                _shell->AddRef();
            }
            Entry(const Entry& copy)
                : _shell(copy._shell)
            {
                ASSERT(_shell != nullptr);
                _shell->AddRef();
            }
            ~Entry()
            {
                _shell->Release();
            }

        public:
            inline string Callsign() const
            {
                return (_shell->Callsign());
            }
            bool IsActive() const
            {
                bool running((_shell->State() == PluginHost::IShell::ACTIVATED) ||
                             (_shell->State() == PluginHost::IShell::PRECONDITION));

                if (running == true) {

                    PluginHost::IStateControl* control = _shell->QueryInterface<PluginHost::IStateControl>();

                    if (control != nullptr) {
                        running = ((control->State() == PluginHost::IStateControl::RESUMED) ||
                                   (control->State() == PluginHost::IStateControl::UNINITIALIZED));
                        control->Release();
                    }
                }

                return (running);
            }
            uint32_t Activate()
            {
                uint32_t result = Core::ERROR_NONE;

                TRACE(Switching, (_T("Activating plugin [%s] in plugin state [%d]"), _shell->Callsign().c_str(), _shell->State()));

                if (_shell->State() != PluginHost::IShell::ACTIVATED) {
                    if (_shell->AutoStart() == true) {
                        TRACE(Trace::Error, (Trace::Format("Activation of %s required although it should be autostarted.", _shell->Callsign().c_str())));
                    }

                    result = _shell->Activate(PluginHost::IShell::REQUESTED);

                    TRACE(Switching, (_T("Activated plugin [%s], result [%d]"), _shell->Callsign().c_str(), result));
                }

                if ((result == Core::ERROR_NONE) && (_shell->State() == PluginHost::IShell::ACTIVATED)) {
                    PluginHost::IStateControl* control(_shell->QueryInterface<PluginHost::IStateControl>());

                    if (control != nullptr) {

                        if (control->State() != PluginHost::IStateControl::RESUMED) {

                            result = control->Request(PluginHost::IStateControl::RESUME);
                            TRACE(Switching, (_T("Resumed plugin [%s], result [%d]"), _shell->Callsign().c_str(), result));
                        }

                        control->Release();

                    }
                }

                return ((result == Core::ERROR_PENDING_CONDITIONS) ? static_cast<uint32_t>(Core::ERROR_NONE) : result);
            }

            uint32_t Deactivate() {

                uint32_t result = Core::ERROR_NONE;

                TRACE(Switching, (_T("Deactivating plugin [%s] on plugin state [%d]"), _shell->Callsign().c_str(), _shell->State()));

                if (_shell->State() == PluginHost::IShell::ACTIVATED) {

                    PluginHost::IStateControl* control(_shell->QueryInterface<PluginHost::IStateControl>());

                    if ((control != nullptr) && (control->State() == PluginHost::IStateControl::RESUMED)) {

                        result = control->Request(PluginHost::IStateControl::SUSPEND);
                        TRACE(Switching, (_T("Suspended plugin [%s], result [%d]"), _shell->Callsign().c_str(), result));

                        control->Release();

                    } else if (_shell->AutoStart() == true) {
                        TRACE(Trace::Error, (Trace::Format("Deactivation of %s required although it is autostarted, but has *NO* IStateControl.", _shell->Callsign().c_str())));
                    }

                    if ((result != Core::ERROR_NONE) || (_shell->AutoStart() == false) || (control == nullptr)) {

                        result = _shell->Deactivate(PluginHost::IShell::REQUESTED);

                        TRACE(Switching, (_T("Deactivated plugin [%s], result [%d]"), _shell->Callsign().c_str(), result));
                    }
                }

                return (result);
            }
            void Register(PluginHost::IStateControl::INotification* sink) {

                PluginHost::IStateControl* control(_shell->QueryInterface<PluginHost::IStateControl>());

                if (control != nullptr) {

                    control->Register(sink);
                    control->Release();
                }
            }
            void Unregister(PluginHost::IStateControl::INotification* sink) {

                PluginHost::IStateControl* control(_shell->QueryInterface<PluginHost::IStateControl>());

                if (control != nullptr) {

                    control->Unregister(sink);
                    control->Release();
                }
            }

        private:
            PluginHost::IShell* _shell;
        };

    public:
        class Iterator {
        public:
            Iterator()
                : _container(nullptr)
                , _iterator()
                , _index(0)
            {
            }
            Iterator(const std::map<string, Entry>& container)
                : _container(&container)
                , _iterator(container.begin())
                , _index(0)
            {
            }
            Iterator(const Iterator& copy)
                : _container(copy._container)
                , _iterator(copy._iterator)
                , _index(copy._index)
            {
            }
            ~Iterator()
            {
            }

            Iterator& operator=(const Iterator& RHS)
            {
                _container = RHS._container;
                _iterator = RHS._iterator;
                _index = RHS._index;

                return (*this);
            }

        public:
            bool IsValid() const
            {
                return ((_index > 0) && (_index <= Count()));
            }

            void Reset()
            {
                if (_container != nullptr) {
                    _iterator = _container->begin();
                    _index = 0;
                }
            }

            bool Next()
            {
                if ((_container != NULL) && (_index != Count() + 1)) {
                    _index++;

                    if (_index != 1) {
                        _iterator++;

                        ASSERT((_index != (Count() + 1)) || (_iterator == _container->end()));
                    }
                }
                return (IsValid());
            }

            uint32_t Index() const
            {
                return (_index);
            }

            uint32_t Count() const
            {
                return (_container == NULL ? 0 : static_cast<uint32_t>(_container->size()));
            }

            inline const string& Callsign() const
            {
                ASSERT(IsValid());

                return (_iterator->first);
            }

        private:
            const std::map<string, Entry>* _container;
            mutable std::map<string, Entry>::const_iterator _iterator;
            mutable uint32_t _index;
        };

        enum state {
            INACTIVE,
            IDLE,
            INPROGRESS
        };

    public:
        SwitchBoard();
        ~SwitchBoard() override;

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

        // The plugin is unloaded from the Framework. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        string Information() const override;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        // Whenever a request is received, it might carry some additional data in the body. This method allows
        // the plugin to attach a deserializable data object (ref counted) to be loaded with any potential found
        // in the body of the request.
        void Inbound(Web::Request& request) override;

        // If everything is received correctly, the request is passed to us, on a thread from the thread pool, to
        // do our thing and to return the result in the response object. Here the actual specific module work,
        // based on a a request is handled.
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        //  IUnknown methods
        // -------------------------------------------------------------------------------------------------------

        // ISwitchBoard interface
        void Register(Exchange::ISwitchBoard::INotification* notification) override;
        void Unregister(Exchange::ISwitchBoard::INotification* notification) override;
        bool IsActive(const string& callsign) const override;
        uint32_t Activate(const string& callsign) override;
        uint32_t Deactivate(const string& callsign) override;

        BEGIN_INTERFACE_MAP(SwitchBoard)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(Exchange::ISwitchBoard)
        END_INTERFACE_MAP

    private:
        uint32_t Initialize();
        uint32_t Deinitialize();
        void Evaluate();
        void Activated(Entry& entry);
        void Activated(const string& callsign, PluginHost::IShell* plugin);
        void Deactivated(const string& callsign, PluginHost::IShell* plugin);
        void StateChange(PluginHost::IStateControl::state newState);

        friend Core::ThreadPool::JobType<SwitchBoard&>;
        void Dispatch()
        {
            TRACE(Trace::Information, (_T("%s: Job is Dispatched"), _service->Callsign().c_str()));
            Evaluate();
        }

    private:
        Core::CriticalSection _adminLock;
        uint8_t _skipURL;
        Entry* _defaultCallsign;
        Entry* _activeCallsign;
        std::map<string, Entry> _switches;
        std::list<Exchange::ISwitchBoard::INotification*> _notificationClients;
        Core::Sink<Notification> _sink;
        PluginHost::IShell* _service;
        volatile state _state;

        Core::WorkerPool::JobType<SwitchBoard&> _job;
    };

}
}  // namespace WPEFramework::Plugin
