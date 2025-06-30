/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 [PLEASE ADD COPYRIGHT NAME!]
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
#include <interfaces/IPluginAsyncStateControl.h>

namespace Thunder {

namespace Trace {
    class ActivationQueueInfo {
    public:
        ActivationQueueInfo(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit ActivationQueueInfo(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~ActivationQueueInfo() = default;

        ActivationQueueInfo(const ActivationQueueInfo&) = delete;
        ActivationQueueInfo& operator=(const ActivationQueueInfo&) = delete;
        ActivationQueueInfo(ActivationQueueInfo&&) = delete;
        ActivationQueueInfo& operator=(ActivationQueueInfo&&) = delete;

    public:
        const char* Data() const
        {
            return (_text.c_str());
        }
        uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };
}

namespace Plugin {
    
    class PluginInitializerService : public PluginHost::IPlugin, public Exchange::IPluginAsyncStateControl {
    public:
        PluginInitializerService(const PluginInitializerService&) = delete;
        PluginInitializerService& operator=(const PluginInitializerService&) = delete;
        PluginInitializerService(PluginInitializerService&&) = delete;
        PluginInitializerService& operator=(PluginInitializerService&&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        PluginInitializerService()
            : PluginHost::IPlugin()
            , Exchange::IPluginAsyncStateControl()
            , _maxparallel(2)
            , _maxretries(5)
            , _delay(500)
            , _pluginInitList()
            , _sink(*this)
            , _service(nullptr)
            , _adminLock()
            , _deactivateNotificationsJob(*this)
        {
        }
POP_WARNING()

        ~PluginInitializerService() override = default;
    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config(Config&&) = delete;
            Config& operator=(Config&&) = delete;
            
            Config()
                : Core::JSON::Container()
                , MaxParallel()
                , MaxRetries(5)
                , Delay(500)
            {
                Add(_T("maxparallel"), &MaxParallel);
                Add(_T("maxretries"), &MaxRetries);
                Add(_T("delay"), &Delay);
            }
            ~Config() override = default;
        public:
            Core::JSON::DecUInt8 MaxParallel;
            Core::JSON::DecUInt8 MaxRetries;
            Core::JSON::DecUInt16 Delay;
        };

    private:

        class PluginStarter {
        public:
            enum class ActiveState {
                NotActive,
                Active,
                WaitingForPrecondition
            };


            PluginStarter(PluginHost::IShell* requestedPluginShell
                         , uint8_t maxnumberretries
                         , uint16_t delay
                         , IPluginAsyncStateControl::IActivationCallback* callback
                         , PluginInitializerService& initservice)
                : _callsign()
                , _requestedPluginShell(requestedPluginShell)
                , _attempt(0)
                , _maxnumberretries(maxnumberretries)
                , _delay(delay)
                , _callback(callback)
                , _initializerservice(initservice)
                , _delayJob()
            {
                ASSERT(_requestedPluginShell != nullptr);
                _callsign = _requestedPluginShell->Callsign();
                _requestedPluginShell->AddRef();
                if (_callback != nullptr) {
                    _callback->AddRef();
                }
            }
            ~PluginStarter()
            {
                if (_requestedPluginShell != nullptr) {
                    _requestedPluginShell->Release();
                    _requestedPluginShell = nullptr;
                }
                if (_callback != nullptr) {
                    _callback->Release();
                    _callback = nullptr;
                }

                ASSERT((_delayJob.IsValid() == false) || (_delayJob->IsIdle() == true));
            }

            PluginStarter(const PluginStarter&) = delete;
            PluginStarter& operator=(const PluginStarter&) = delete;
            PluginStarter(PluginStarter&& other)
                : _callsign(std::move(other._callsign))
                , _requestedPluginShell(other._requestedPluginShell)
                , _attempt(other._attempt)
                , _maxnumberretries(other._maxnumberretries)
                , _delay(other._delay)
                , _callback(other._callback)
                , _initializerservice(other._initializerservice)
                , _delayJob(std::move(other._delayJob))
            {
                other._requestedPluginShell = nullptr;
                other._callback = nullptr;
            }
            PluginStarter& operator=(PluginStarter&& other)
            {
                if (this != &other) {
                    _callsign = std::move(other._callsign);
                    _requestedPluginShell = other._requestedPluginShell;
                    _attempt = other._attempt;
                    _maxnumberretries = other._maxnumberretries;
                    _delay = other._delay;
                    _callback = other._callback;
                    _delayJob = std::move(other._delayJob);
                    other._requestedPluginShell = nullptr;
                    other._callback = nullptr;
                    ASSERT(&_initializerservice == &other._initializerservice); // there should only be one...
                }
                return *this;
            }

            friend bool operator==(const PluginStarter& lhs, const PluginStarter& rhs)
            {
                return lhs._callsign == rhs._callsign;
            }
            friend bool operator!=(const PluginStarter& lhs, const PluginStarter& rhs)
            {
                return !(lhs == rhs);
            }
            friend bool operator==(const PluginStarter& lhs, const string& callsign)
            {
                return lhs._callsign == callsign;
            }
            friend bool operator!=(const PluginStarter& lhs, const string& callsign)
            {
                return !(lhs == callsign);
            }
            friend bool operator==(const string& callsign, const PluginStarter& lhs)
            {
                return lhs._callsign == callsign;
            }
            friend bool operator!=(const string& callsign, const PluginStarter& lhs)
            {
                return !(lhs == callsign);
            }

            void Activate()
            {
                ASSERT(_delayJob.IsValid() == false);
                ASSERT(_requestedPluginShell != nullptr);
                TRACE(Trace::Information, (_T("Start activating plugin [%s]"), Callsign().c_str()));
                _delayJob = Core::ProxyType<Core::WorkerPool::JobType<DelayJob>>::Create(_requestedPluginShell); // indicate have activated this PluginStarter
                if (_requestedPluginShell->State() == PluginHost::IShell::DEACTIVATED) { // (other states here can be DEACTIVATION, ACTIVATION and PRECONDITION) if the plugin is still not yet fully deactivated or activated we can only restart it ones it has fully deactivated or activation failed, we'll do that then when the Deinitialized notification is received (either because it fully deactivated or the activation failed
                    // we'll not keep the job, when activation is actualy started, aborting after that might not always abort the plugin activation (does not make sense, could always cross eachother anyway, and otherwisse we need to keep the job)
                    ++_attempt; // indicate we have attempted to start this plugin
                    Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_requestedPluginShell, PluginHost::IShell::ACTIVATED, PluginHost::IShell::REQUESTED));
                }
            }
            void Abort()
            {
                ASSERT(_delayJob.IsValid() == true);
                TRACE(Trace::Information, (_T("Aborting activating plugin [%s]"), Callsign().c_str()));
                if (_delayJob->IsIdle() == false) {
                    _delayJob->Revoke();
                }
                if (_callback != nullptr) {
                    TRACE(Trace::Information, (_T("Result callback aborted called for plugin [%s]"), Callsign().c_str()));
                    _callback->Finished(Callsign(), Exchange::IPluginAsyncStateControl::IActivationCallback::state::ABORTED, Retries());
                }
            }
            uint8_t Retries() const
            {
                return (_attempt == 0 ? 0 : _attempt - 1); 
            }
            const string& Callsign() const
            {
                return _callsign;
            }
            ActiveState Active() const
            {
                ActiveState state = ActiveState::NotActive;

                ASSERT(_requestedPluginShell != nullptr);
                
                // if the plugin is waiting for preconditions it is not eating bread here, so we can add another in parallel (risk is that if it fails to Initialize after the subsystems become avialable there might more parallel activations than max allowed, for now we just accept this small chance )
                if (_delayJob.IsValid() == true) {
                    if (_requestedPluginShell->State() != PluginHost::IShell::PRECONDITION) {
                        state = ActiveState::Active;
                    } else {
                        state = ActiveState::WaitingForPrecondition;
                    }
                }
                return state;
            }
            void Activated()
            {
                if (_delayJob.IsValid() == true) {
                    TRACE(Trace::Information, (_T("Plugin [%s] was activated"), Callsign().c_str()));
                } else {
                    TRACE(Trace::Warning, (_T("Plugin [%s] was activated but this was initiated from the PluginInitializerService!!"), Callsign().c_str()));
                }
                if (_callback != nullptr) {
                    // for now let's not decouple. In the future if users prove to be unreliable we might however to not block Thunder internally
                    TRACE(Trace::Information, (_T("Result callback success called for plugin [%s]"), Callsign().c_str()));
                    _callback->Finished(Callsign(), Exchange::IPluginAsyncStateControl::IActivationCallback::state::SUCCESS, Retries());
                }
                // we will be removed and destroyed from the caller
            }
            bool Deinitialized() // returns true when fully failed and we should stop retrying
            {
                // we can get here if:
                // 1) state was DEACTIVATION and now plugin is fully deactivated -> let's try to startup now if this one was actually Activated (this notification can of course also happen in this situation in case it was not yest Activated)
                // 2) state was PRECONDITION (triggered by ourselves or in that state when request came in) and when preconditions were met startup failed -> let's try to restart, next attempt
                // 3) initial state was ACTIVATION -> although activate was not triggered by this service let's try to restart anyway as that is now requested
                // 3) at Activation the activation failed -> let's try to restart, next attempt

                bool fullyfailed = false;

                if (_delayJob.IsValid() == true) { // Correct case, we are indeed trying to activate this PluginStarter

                    TRACE(Trace::Warning, (_T("Plugin [%s] was deinitialzed"), Callsign().c_str()));
                    if (_attempt <= _maxnumberretries) { // first attempt not included, that is not a retry...
                        if ((_attempt == 0) || (_delay == 0)) { // _attempt == 0 means we could not yet activate the plugin when Activate was called so we can start without delay (or of course delay is just 0)
                            ++_attempt;
                            TRACE(Trace::Information, (_T("Retrying to re-activate Plugin [%s] now (retries %u)"), Callsign().c_str(), Retries()));
                            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_requestedPluginShell, PluginHost::IShell::ACTIVATED, PluginHost::IShell::REQUESTED));
                        } else {
                            // okay we might need to delay now
                            TRACE(Trace::Information, (_T("Delaying re-activating Plugin [%s] (retries %u)"), Callsign().c_str(), Retries()));
                            ++_attempt;
                            _delayJob->Reschedule(Core::Time::Now().Add(_delay));
                        }
                    } else {
                        TRACE(Trace::Error, (_T("Plugin [%s] could not be restarted within the allowed number of retries (retries %u)"), Callsign().c_str(), Retries()));
                        if (_callback != nullptr) {
                            // for now let's not decouple. In the future if users prove to be unreliable we might however to not block Thunder internally
                            TRACE(Trace::Information, (_T("Result callback failed called for plugin [%s]"), Callsign().c_str()));
                            _callback->Finished(Callsign(), Exchange::IPluginAsyncStateControl::IActivationCallback::state::FAILURE, Retries());
                            fullyfailed = true;
                        }
                    }
                } else {
                    TRACE(Trace::Warning, (_T("Plugin [%s] Deinitialzed notification received but not yet activated, will be ignored..."), Callsign().c_str())); // apperently this plugin failed to start or was deactivated without us being involved, we just ignore and when there isn a slot available we will try to start it anyway
                }

                return fullyfailed;
            }

        private:
            class DelayJob {
            public:
                explicit DelayJob(PluginHost::IShell* pluginShell)
                    : _pluginShell(pluginShell)
                {
                    ASSERT(_pluginShell != nullptr);
                    _pluginShell->AddRef();
                }
                ~DelayJob()
                {
                    ASSERT(_pluginShell != nullptr);
                    _pluginShell->Release();
                }

                DelayJob(const DelayJob&) = delete;
                DelayJob& operator=(const DelayJob&) = delete;
                DelayJob(DelayJob&&) = delete;
                DelayJob& operator=(DelayJob&&) = delete;

            private:
                friend Core::ThreadPool::JobType<DelayJob>;
                void Dispatch()
                {
                    TRACE(Trace::Information, (_T("Retrying to re-activate Plugin [%s] after delay"), _pluginShell->Callsign().c_str()));
                    Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_pluginShell, PluginHost::IShell::ACTIVATED, PluginHost::IShell::REQUESTED));
                }

            private:
                PluginHost::IShell* _pluginShell;
            };

        private:
            string _callsign; //as _requestedPluginShell->Callsign(); returns a temp and we need this lots of times so let's keep a copy
            PluginHost::IShell* _requestedPluginShell;
            uint8_t _attempt;
            uint8_t _maxnumberretries;
            uint16_t _delay;
            IPluginAsyncStateControl::IActivationCallback* _callback;
            PluginInitializerService& _initializerservice;
            Core::ProxyType<Core::WorkerPool::JobType<DelayJob>> _delayJob;
        };

        class Notifications : public PluginHost::IPlugin::INotification, public PluginHost::IPlugin::ILifeTime {
        public:
            explicit Notifications(PluginInitializerService& initservice)
                : PluginHost::IPlugin::INotification()
                , PluginHost::IPlugin::ILifeTime()
                , _initservice(initservice)
            {
            }
            ~Notifications() override = default;

            Notifications(const Notifications&) = delete;
            Notifications& operator=(const Notifications&) = delete;
            Notifications(Notifications&&) = delete;
            Notifications& operator=(Notifications&&) = delete;

            // IPlugin::INotification overrides
            void Activated(const string& callsign, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED) override
            {
                _initservice.ActivatedNotification(callsign);
            }
            void Deactivated(const string& callsign, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED) override
            {
                // this event doesn't add much value to use for this use case, it means deactivation has started but is not finihed.
            }
            void Unavailable(const string& callsign VARIABLE_IS_NOT_USED, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED) override
            {
            }

            // IPlugin::ILifeTime overrides
            void Initialize(const string& callsign VARIABLE_IS_NOT_USED, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED) override
            {

            }
            void Deinitialized(const string& callsign VARIABLE_IS_NOT_USED, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED) override
            {
                _initservice.DeinitializedNotification(callsign);
            }

            BEGIN_INTERFACE_MAP(Notifications)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            INTERFACE_ENTRY(PluginHost::IPlugin::ILifeTime)
            END_INTERFACE_MAP

        private:
            PluginInitializerService& _initservice;
        };
        
    public:
        // IPlugin Methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
        
        // IPluginAsyncStateControl methods
        Core::hresult Activate(const string& callsign, const Core::OptionalType<uint8_t>& maxnumberretries, const Core::OptionalType<uint16_t>& delay, IPluginAsyncStateControl::IActivationCallback* const cb) override;
        Core::hresult AbortActivate(const string& callsign) override;
      
        
        BEGIN_INTERFACE_MAP(PluginInitializerService)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(Exchange::IPluginAsyncStateControl)
        END_INTERFACE_MAP

    private:
        void ActivateNotifications()
        {
            ASSERT(_service != nullptr);
            _deactivateNotificationsJob.Revoke(); // no need to check if it is actually posted
            _service->Register(&_sink);
            TRACE(Trace::Information, (_T("Started listening for plugin state notifications")));
        }
        void DeactivateNotifications(bool synchronous = true)
        {
            ASSERT(_service != nullptr);
            if (synchronous == true) {
                _deactivateNotificationsJob.Revoke(); // no need to check if it is actually posted
                _service->Unregister(&_sink);
                TRACE(Trace::Information, (_T("Stopped listening for plugin state notifications")));
            } else {
                _deactivateNotificationsJob.Submit();
            }
        }

        bool NewPluginStarter(PluginHost::IShell* requestedPluginShell, uint8_t maxnumberretries, uint16_t delay, IPluginAsyncStateControl::IActivationCallback* callback)
        {
            bool result = true;
            PluginStarter starter(requestedPluginShell, maxnumberretries, delay, callback, *this);

            _adminLock.Lock();

            //see if this callsign is not yet in the list
            if (std::find(_pluginInitList.cbegin(), _pluginInitList.cend(), starter) == _pluginInitList.cend()) {
                ASSERT(_pluginInitList.size() < std::numeric_limits<uint16_t>::max()); // I'll bet this will fire at some point :)

                _pluginInitList.emplace_back(std::move(starter));
                if (_pluginInitList.size() == 1) {
                    ActivateNotifications();
                }
                ActivateAnotherPlugin();
            }
            else {
                //oops this callsign was already requested...
                result = false;
            }

            _adminLock.Unlock();
            return result;
        }

        // must be called from inside the lock
        void ActivateAnotherPlugin() 
        {
            TRACE(Trace::Information, (_T("Going to try to activate another plugin")));
            PluginStarterContainer::iterator it = _pluginInitList.begin(); // we just start activating from the top, so we try to do it more or less in the order of incoming requests
            uint16_t currentlyActiveCounted = 0;
            while ((it != _pluginInitList.end()) && (currentlyActiveCounted < _maxparallel)) {
                PluginStarter::ActiveState state = it->Active();
                if (state == PluginStarter::ActiveState::NotActive) {
                    ++currentlyActiveCounted;
                    it->Activate();
                    TRACE(Trace::Information, (_T("Activating another plugin")));
                    break; // we activated another we can stop looking... (as this method is only called when added another request or one was handled)
                } else if (state == PluginStarter::ActiveState::Active) {
                    ++currentlyActiveCounted;
                } // so if the state is waiting for preconditions we do not count it as active and try to start another...
                ++it;
            };
            TRACE(Trace::ActivationQueueInfo, (_T("Request queue info: %s"), GetStarterQueueInfo().ToString().c_str()));
        }

        bool CancelPluginStarter(const string& callsign)
        {
            bool result = false;

            _adminLock.Lock();

            PluginStarterContainer::iterator it = std::find(_pluginInitList.begin(), _pluginInitList.end(), callsign);
            if (it != _pluginInitList.end()) {
                PluginStarter toAbort(std::move(*it));
                _pluginInitList.erase(it);
                if (_pluginInitList.size() == 0) {
                    DeactivateNotifications();
                } else {
                    ActivateAnotherPlugin();
                }
                _adminLock.Unlock();
                result = true;
                toAbort.Abort();
            } else {
                _adminLock.Unlock();
            }

            return result;
        }

        void ActivatedNotification(const string& callsign)
        {
            _adminLock.Lock();

            PluginStarterContainer::iterator it = std::find(_pluginInitList.begin(), _pluginInitList.end(), callsign);
            if (it != _pluginInitList.end()) {
                // okay this plugin is activated were done for this one!
                PluginStarter activated(std::move(*it));
                _pluginInitList.erase(it);
                if (_pluginInitList.size() == 0) {
                    DeactivateNotifications(false);
                } else {
                    ActivateAnotherPlugin();
                }
                _adminLock.Unlock();
                activated.Activated();
            } else {
                _adminLock.Unlock();
            }
        }

        void DeinitializedNotification(const string& callsign)
        {
            _adminLock.Lock();

            PluginStarterContainer::iterator it = std::find(_pluginInitList.begin(), _pluginInitList.end(), callsign);
            if (it != _pluginInitList.end()) {
                if (it->Deinitialized() == true) { // note must be inside the lock as it could be in parallel with an abort request. 
                    _pluginInitList.erase(it); // we're done retrying just give up and remove it from the list
                    if (_pluginInitList.size() == 0) {
                        DeactivateNotifications(false);
                    } else {
                        ActivateAnotherPlugin();
                    }
                }
            }
            _adminLock.Unlock();
        }

        void CancelAll()
        {
            _adminLock.Lock();

            TRACE(Trace::Information, (_T("Cancelling all plugin activation requests (plugin deactivate)")));

            if (_pluginInitList.size() != 0) {

                for (PluginStarter& starter : _pluginInitList) {
                    starter.Abort();
                }
                _pluginInitList.clear();
                DeactivateNotifications();
            }

             _adminLock.Unlock();
        }

        struct StarterQueueInfo {
            uint16_t Activating;
            uint16_t WaitingPreconditions;
            uint16_t TotalStarters;

            string ToString() const
            {
                const TCHAR text[] = _T("Requests being activated: %u, Request waiting for preconditions %u, Total number of requests: %u");
                return Core::Format(text, Activating, WaitingPreconditions, TotalStarters);
            }
        };

        // note must be done inside lock
        StarterQueueInfo GetStarterQueueInfo() const
        {
            StarterQueueInfo info{};
            for (const PluginStarter& starter : _pluginInitList) {
                PluginStarter::ActiveState state = starter.Active();
                switch (state) {
                    case PluginStarter::ActiveState::Active : 
                        ++info.Activating;
                        break;
                    case PluginStarter::ActiveState::WaitingForPrecondition : 
                        ++info.WaitingPreconditions;
                        break;
                    case PluginStarter::ActiveState::NotActive:
                    default:
                        break;
                }
            }
            info.TotalStarters = static_cast<uint16_t>(_pluginInitList.size());
            return info;
        }

    private:
        class DeactivateNotificationsJob {
        public:
            DeactivateNotificationsJob(PluginInitializerService& parent)
                : _parent(parent)
            {
            }
            ~DeactivateNotificationsJob() = default;

            DeactivateNotificationsJob(const DeactivateNotificationsJob&) = delete;
            DeactivateNotificationsJob& operator=(const DeactivateNotificationsJob&) = delete;
            DeactivateNotificationsJob(DeactivateNotificationsJob&&) = delete;
            DeactivateNotificationsJob& operator=(DeactivateNotificationsJob&&) = delete;

        private:
            friend Core::ThreadPool::JobType<DeactivateNotificationsJob>;
            void Dispatch()
            {
                TRACE(Trace::Information, (_T("Stopped listening for plugin state notifications (from job)")));
                _parent._service->Unregister(&_parent._sink);
            }

        private:
            PluginInitializerService& _parent;
        };

    private:
        using PluginStarterContainer = std::list<PluginStarter>; // for now we keep them in a list as we more or less agreed to activate plugins in order of requests receieved (if needed we can add a shadow unordered map for quick lookup but we do not expect that many parallel activation requests, at least I hope...)

        uint8_t                                                 _maxparallel;
        uint8_t                                                 _maxretries;
        uint16_t                                                _delay;
        PluginStarterContainer                                  _pluginInitList;
        Core::SinkType<Notifications>                           _sink;
        PluginHost::IShell*                                     _service;
        Core::CriticalSection                                   _adminLock;
        Core::WorkerPool::JobType<DeactivateNotificationsJob>   _deactivateNotificationsJob;

    };
} // Plugin
} // Thunder