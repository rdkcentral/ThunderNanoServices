/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 RDK Management
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

    class DetailedInfo {
    public:
        DetailedInfo(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit DetailedInfo(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~DetailedInfo() = default;

        DetailedInfo(const DetailedInfo&) = delete;
        DetailedInfo& operator=(const DetailedInfo&) = delete;
        DetailedInfo(DetailedInfo&&) = delete;
        DetailedInfo& operator=(DetailedInfo&&) = delete;

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

        PluginInitializerService(const PluginInitializerService&) = delete;
        PluginInitializerService& operator=(const PluginInitializerService&) = delete;
        PluginInitializerService(PluginInitializerService&&) = delete;
        PluginInitializerService& operator=(PluginInitializerService&&) = delete;

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

        // class that actually handles Activating the plugin taking into account all the possible Plugin states and Activation call return codes 
        // and at the same time tries to cope as best as possible with plugin state changes triggered externally to the PluginStarter, e.g. Activation and Deactivation requests.
        // One special case is the state PRECONDITION, when the plugin is in this state the PluginStarter is not considered Active and will not be included in the maximum allowed parallel startup activations.
        // This as it is waiting for the preconditions to be met and up to that moment it is not actually Initializing so we could maximize parallelism and activate another plugin. Subsequent advantage: now we can try to start 
        // other plugins that might actually trigger a subsystem the plugin in state PRECONDITION is waiting on and unblock this plugin (this in principle would of course be considered incorrect usage of the PluginInitializerService 
        // because we try to activate plugins in order received but now this will not lead to the plugin startup becoming stuck because of incorrect order of requests icw subsystem dependencies)
        // As we get an Initialize() notification when a plugin waiting for preconditions is starting to actual activate we will use that to set the PluginStarter to actually active state to prevent from that moment on to activate 
        // new PluginStarters beyond the allowed maximum, but of course if there were already activations up to the maximum we will now temporarily exceed the maximum (but as explained above that is accepted behaviour)
        // The ActivateJob will indicate if at the moment the PluginStarter is trying to actively start a plugin, then it will be valid (so will also not be valid if the plugin is waiting for the Initialize to start in case of pending preconditions)
        // a boolean will indicate if the plugin started from this PluginStarter is waiting for preconditions. This is needed so that when in that case the Initialization fails (Deinitialized callback called) we will not automatically retry 
        // the activation, this will only be done when there is slot available for this
        class PluginStarter {

        public:

            enum class ResultCode {
                Continue,
                Failed,
                Paused
            };

        public:

            PluginStarter(PluginHost::IShell* requestedPluginShell
                         , const uint8_t maxnumberretries
                         , const uint16_t delay
                         , IPluginAsyncStateControl::IActivationCallback* const callback
                         , PluginInitializerService& initservice)
                : _callsign()
                , _requestedPluginShell(requestedPluginShell)
                , _attempt(0)
                , _maxnumberretries(maxnumberretries)
                , _delay(delay)
                , _callback(callback)
                , _initializerservice(initservice)
                , _activateJob()
                , _activateResultJob()
                , _waitingPrecondition(false)
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

                ASSERT(_activateJob.IsValid() == false);
                ASSERT(_activateResultJob.IsValid() == false);
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
                , _activateJob(std::move(other._activateJob))
                , _activateResultJob(std::move(other._activateResultJob))
                , _waitingPrecondition(other._waitingPrecondition)
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
                    _activateJob = std::move(other._activateJob);
                    _activateResultJob = std::move(other._activateResultJob);
                    _waitingPrecondition = other._waitingPrecondition;

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

            // returns true when we're done with this PluginStarter, no need to retry again
            bool Activate()
            {
                bool done = false;
                ASSERT(_activateJob.IsValid() == false);
                ASSERT(_activateResultJob.IsValid() == false);
                ASSERT(_requestedPluginShell != nullptr);
                ASSERT(_waitingPrecondition == false);

                PluginHost::IShell::state state = _requestedPluginShell->State(); // let's get the state again, potentially it could have been changed between state check in the IPluginAsyncStateControl::Activate and the addition the crew and also the lazy registration to the notification plays a role here (if it is the first request in the queue the registration for the notificaion will only happen after it was added to the queue). Here we are guaranteed to be in the list and notifications active so we can safely assume we will not miss any state transition)

                TRACE(Trace::Information, (_T("Start activating plugin [%s] current state [%s]"), Callsign().c_str(), Core::EnumerateType<PluginHost::IShell::state>(state).Data()));

                // DEACTIVATED -> we can activate this request
                // DEACTIVATION -> Deinitialize will always follow and we do not need to do anything but await for it to happen and then activate the plugin
                // ACTIVATION -> We will still either get the Activated or Deinitialized callback, again nothing to do here just await these
                // PRECONDITION -> again we will either get the Activated or Deinitialized callback, again nothing to do here just await these
                // ACTIVATED -> we did miss the Activation notification -> report the plugin as activated and we're done!
                // HIBERNATED -> quite unlikely, plugin must have been Activated by another means for which we missed the notification and after that have been Hibernated before we now try to activate the plug. But it means the plugin is activated so -> report the plugin as activated and we're done!
                // DESTROYED -> hmm this would be a very unexpected use case, more a system architecture issue, why would the plugin be requested to be started by this service and then even after that be transitioned into DESTROYED?. We'll report it "failed to activate"
                // UNAVAILABLE -> same reasoning as DESTROYED -> We'll report it "failed to activate"


                switch (state) { // note fall throughs on purpose
                case PluginHost::IShell::DEACTIVATED: 
                    // we'll not keep a reference to the job, that would be just overhead, when activation is actually started, aborting after that might not always abort the plugin activation (does not make sense, could always cross each other anyway, and otherwise we need to keep the job)
                    ++_attempt; // indicate we have attempted to start this plugin
                    TRACE(Trace::DetailedInfo, (_T("Activation job posted for plugin [%s] from Activate call"), Callsign().c_str()));
                    _activateResultJob = ActivateResultJobProxyType::Create(_initializerservice, Callsign()); 
                    _activateJob = ActivateJobProxyType::Create(_requestedPluginShell, _activateResultJob); // indicate have activated this PluginStarter
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(_activateJob)); // let's start this plugin (and from a job, the start call might take long as when the Initialize is badly written, we know they are out there)
                    break;
                 case PluginHost::IShell::ACTIVATED: 
                 case PluginHost::IShell::HIBERNATED:
                    TRACE(Trace::Warning, (_T("Activation started for plugin [%s] that is already Activated (so not started by the PluginInitializerService)"), Callsign().c_str()));
                    NotifyInitiator(Exchange::IPluginAsyncStateControl::IActivationCallback::state::SUCCESS);
                    done = true;
                    break;
                case PluginHost::IShell::DESTROYED:
                case PluginHost::IShell::UNAVAILABLE:
                    ASSERT(false); // this would be so unexpected, lets assert... (but in case ASSERT is not executed or does not lead to abort we must notify the initiator to unblock him)
                    TRACE(Trace::Error, (_T("Activation started for plugin [%s] that is in an invalid state!"), Callsign().c_str()));
                    NotifyInitiator(Exchange::IPluginAsyncStateControl::IActivationCallback::state::FAILURE);
                    done = true;
                    break;
                case PluginHost::IShell::DEACTIVATION:
                case PluginHost::IShell::ACTIVATION:
                    // although not started by us let's consider it is active as we will now monitor and handle full startup in this plugin activator
                    TRACE(Trace::Warning, (_T("Activation started for plugin [%s] that is already in a state that will lead to either a successful Activation or failure to Activate (so not started by the PluginInitializerService), waiting for that state to be reached"), Callsign().c_str()));
                    _activateResultJob = ActivateResultJobProxyType::Create(_initializerservice, Callsign()); 
                    _activateJob = ActivateJobProxyType::Create(_requestedPluginShell, _activateResultJob); // indicate have activated this PluginStarter
                    break;
                case PluginHost::IShell::PRECONDITION:
                    // this state is not considered active (bit strange we get here, the plugin was activated externally or reached this state because a subsystem fell away in case the plugin will be restarted automatically, also in that case we want to keep it in the list of plugins to activate so we can call the callback at the right moment and retry in case the automatic Initialization fails when all subsystems become set)
                    TRACE(Trace::Warning, (_T("Activation started for plugin [%s] that is in state PRECONDITION (so not started by the PluginInitializerService), we'll wait for either a successful start or failure to do so"), Callsign().c_str()));
                    _activateResultJob = ActivateResultJobProxyType::Create(_initializerservice, Callsign()); 
                    _waitingPrecondition = true;
                    break;
                default:
                    ASSERT(false);
                    TRACE(Trace::Error, (_T("Activating started for plugin [%s] but it is in an unexpected state"), Callsign().c_str()));
                    done = true; // best we can do when the assert does not abort is to remove it from the list
                }

                return done;

            }

            // note must not be called within a lock that is also used in the ActivationResultNotification call from the ActivationJob (as it needs to revoke the job)
            void Abort() 
            {
                TRACE(Trace::Information, (_T("Aborting activating plugin [%s]"), Callsign().c_str()));
                SetInactive();
                NotifyInitiator(Exchange::IPluginAsyncStateControl::IActivationCallback::state::ABORTED);
            }

            uint8_t Retries() const
            {
                return (_attempt == 0 ? 0 : _attempt - 1); 
            }

            const string& Callsign() const
            {
                return _callsign;
            }

            // note must not be called within a lock that is also used in the ActivationResultNotification call from the ActivationJob (as it needs to revoke the job)
            void Activated()
            {
                //also revoke the activation job (there could be an external activate call leading to the activate notification, so the job could be active)
                if (_activateJob.IsValid() == true) {
                    TRACE(Trace::Information, (_T("Plugin [%s] was activated"), Callsign().c_str()));
                } else {
                    TRACE(Trace::Warning, (_T("Plugin [%s] was activated but this was not initiated from the PluginInitializerService!!"), Callsign().c_str()));
                }
                SetInactive();
                NotifyInitiator(Exchange::IPluginAsyncStateControl::IActivationCallback::state::SUCCESS);
                // we will be removed and destroyed from the caller
            }

            void Initialized()
            {
                // plugin Initialize called
                if (_waitingPrecondition == true) {
                    ASSERT(_requestedPluginShell->State() == PluginHost::IShell::PRECONDITION); // if this fires there is a bug in this code or more likely somebody changed the order of the callback and setting the state to Activation :) 
                    ASSERT(_activateJob.IsValid() == false);
                    TRACE(Trace::Information, (_T("Initialize received for plugin [%s] while it was waiting for preconditions, activations started"), Callsign().c_str()));
                    _activateJob = ActivateJobProxyType::Create(_requestedPluginShell, _activateResultJob); // the subsystem conditions for this plugin are met so Initialization started... (this can temporarily lead to too many plugins being activated, see explanation at class definition for detailed information on this)
                }

                // note we cannot assert if the job is actually valid if _waitingPrecondition is false as the activation could be triggered externally... This will all be reported/handled when the Deinitialization or Activation notification are called
            }

            ResultCode Deinitialized() // returns true when fully failed and we should stop retrying
            {
                // we can get here if:
                // 1) state was DEACTIVATION and now plugin is fully deactivated -> let's try to startup now if this one was actually Activated (this notification can of course also happen in this situation in case it was not yet Activated)
                // 2) state was PRECONDITION (triggered by ourselves or in that state when request came in) and when preconditions were met startup failed -> let's try to restart, next attempt
                // 3) initial state was ACTIVATION -> although activate was not triggered by this service let's try to restart anyway as that is now requested
                // 3) at first Activation the activation failed -> let's try to restart, next attempt

                ResultCode result = ResultCode::Continue;

                if (_activateJob.IsValid() == true) { // Correct case, we are indeed trying to activate this PluginStarter

                    TRACE(Trace::Warning, (_T("Plugin [%s] was deinitialized"), Callsign().c_str()));


                    if (_attempt > _maxnumberretries) { // first attempt not included, that is not a retry...
                        TRACE(Trace::Error, (_T("Plugin [%s] could not be restarted within the allowed number of retries (retries %u)"), Callsign().c_str(), Retries()));
                        NotifyInitiator(Exchange::IPluginAsyncStateControl::IActivationCallback::state::FAILURE);
                        // we should revoke the job as that might be necessary in case the PluginStarter did not cause this Deinitialized notification but it was because of an external Activation but we cannot do that here (as that might deadlock because we are in the same lock as the job is using)
                        result = ResultCode::Failed; // will be removed and destroyed by caller
                    } else if (_waitingPrecondition == false) {
                        if ((_attempt == 0) || (_delay == 0)) { // _attempt == 0 means we could not yet activate the plugin when Activate was called so we can start without delay (or of course delay is just 0)
                            ++_attempt;
                            TRACE(Trace::DetailedInfo, (_T("Retrying to re-activate Plugin [%s] now (retries %u)"), Callsign().c_str(), Retries()));
                            Core::IWorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(_activateJob)); // note there is very small chance this Deinitialized notification is triggered because there was an external Activation in parallel to out attempt that failed... (as the Submit will assert when the job is already posted we revoke it first, that will work even it is not submitted)
                            Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(_activateJob)); // let's start this plugin (and from a job, the start call might take long as when the Initialize is badly written, we know they are out there)
                        } else {
                            // okay we might need to delay now
                            TRACE(Trace::DetailedInfo, (_T("Delaying re-activating Plugin [%s] (retries %u)"), Callsign().c_str(), Retries()));
                            ++_attempt;
                            Core::IWorkerPool::Instance().Reschedule(Core::Time::Now().Add(_delay), Core::ProxyType<Core::IDispatch>(_activateJob));
                            // note there is very small chance this Deinitialized notification is triggered because there was an external Activation in parallel to out attempt that failed... (Nice thing is the Resubmit will try to Revoke anyway)
                        }
                    } else {
                        // okay we failed from a situation where we were activated because the preconditions were not satisfied yet, we should restart but now as there could be a chance there are already too many activations so we need to postpone that until there is a slot available
                        // we should cancel the job to indicate we are not active anymore and indicate if possible we could start ourselves again (we cannot revoke the job as we are in the lock that also is needed in the job, should not be a problem coming from preconditions there is hardly another way to get to deinitialized other then subsystems being met and even if so it would be no problem the job would fire) 
                        TRACE(Trace::Warning, (_T("Plugin [%s] failed to initialize after waiting for preconditions to be met, attempt to activate again will be made when a slot becomes available"), Callsign().c_str() ));
                        _activateJob = std::move(ActivateJobProxyType()); // not active anymore (let's not use Release() on the proxy as that look a little confusing)                
                        _waitingPrecondition = false; // indicate we are no longer waiting and this starter can be started again...
                        result = ResultCode::Paused; 
                    }
                } else {
                    TRACE(Trace::Warning, (_T("Plugin [%s] Deinitialized notification received but PluginStarter Activate was not called (activation triggered externally), we'll try to restart anyway..."), Callsign().c_str())); // apparently this plugin failed to start or was deactivated without us being involved, we just ignore and when there is a slot available we will try to start it anyway
                }

                return result;
            }

            // note must not be called within a lock that is also used in the ActivationResultNotification call from the ActivationJob (as it needs to revoke the job)
            void Failed()
            {
                SetInactive();
            }

            // note most likely some states will never be handled here as these states are handled in the notification that are triggered from the Activate call itself and will happen before the activate call returns removing the PluginStarter from the queue before the activation results are handled, so this is fully correct and these states are here only for completeness
            ResultCode HandleActivationResult(const Core::hresult result) // returns true when fully failed and we should stop retrying
            {
                ResultCode resultcode = ResultCode::Continue;

                switch (result)  {  
                case Core::ERROR_GENERAL:
                    // -> Initialize failed -> do nothing (already) handled in Deinitialized notification
                    TRACE(Trace::Warning, (_T("Plugin [%s] Activate call returned: failed to start"), Callsign().c_str()));
                    break;
                case Core::ERROR_NONE: 
                    // -> Activation success -> do nothing (already) handled in Activated notification
                    TRACE(Trace::DetailedInfo, (_T("Plugin [%s] Activate call returned: success"), Callsign().c_str()));
                    break;
                case Core::ERROR_INPROGRESS:
                    // startup was already going on, triggered externally, do nothing as it wil succeed or fail and handled via the notifications (so this result might be reported as the notification will be triggered from another Activate and be after we handle the result from ours)
                    // we keep it in our list to restart it if necessary might the externally triggered activation fail (will be removed from the activated or Deinitialized notification in the end)
                    TRACE(Trace::Warning, (_T("Plugin [%s] Activation reported plugin is already being activated (triggered externally)"), Callsign().c_str()));
                    break;
                case Core::ERROR_PENDING_CONDITIONS:
                    // set starter to not activated, but keep it in the list as it will start activating at some point and may fail (then we'll activate ourselves as we will start retrying or reach activated state)
                    // we should now see if we can start more activations
                    TRACE(Trace::Information, (_T("Plugin [%s] Activate call returned: preconditions pending"), Callsign().c_str()));
                    _waitingPrecondition = true;
                    _activateJob = std::move(ActivateJobProxyType()); // not active anymore (let's not use Release() on the proxy as that look a little confusing)                
                    resultcode = ResultCode::Paused;
                    break;
                case Core::ERROR_ILLEGAL_STATE: // quite unexpected as now between posting the Activation job and calling Activate on the plugin it moved to some illegal state, must have been triggered externally...
                case Core::ERROR_UNAVAILABLE:
                    // set starter to not activated, consider startup failed and remove from list
                    TRACE(Trace::Error, (_T("Plugin [%s] Activation failed due to plugin being in state in which it cannot be started [%s][%u]"), Callsign().c_str(), Core::ErrorToString(result), result));
                    NotifyInitiator(Exchange::IPluginAsyncStateControl::IActivationCallback::state::FAILURE);
                    resultcode = ResultCode::Failed; // will result in Failed being called to deactivate the jobs
                    break;
                case Core::ERROR_BAD_REQUEST:
                    // there is a problem to wakeup from hibernation, external to the PluginStarter the plugin must have been set to hibernate (as we would not try to activate it out of hibernation) -> we cannot do anything more, we'll consider it activated (as hibernate is a substate of Activation and report success to unblock the caller here, Activated notification will not be called) 
                    TRACE(Trace::Error, (_T("Plugin [%s] Activation failed because Plugin moved to Hibernate state in the mean time (triggered externally) and could not be awaken"), Callsign().c_str()));
                    NotifyInitiator(Exchange::IPluginAsyncStateControl::IActivationCallback::state::SUCCESS);
                    resultcode = ResultCode::Failed; // will result in Failed being called to deactivate the jobs
                    break;
                default:
                    // result code not expected, nothing else to do then assert (and consider ourselves failed to not block other starters and unblock the user of the IPluginAsyncStateControl interface
                    ASSERT(false);
                    TRACE(Trace::Error, (_T("Plugin [%s] Activation failed due to unexpected reason [%s][%u]"), Callsign().c_str(), Core::ErrorToString(result), result));
                    NotifyInitiator(Exchange::IPluginAsyncStateControl::IActivationCallback::state::FAILURE);
                    resultcode = ResultCode::Failed;  // will result in Failed being called to deactivate the jobs
                }

                return resultcode;
            }

            // (simplified) State diagram for plugin activation (reported state between brackets and values of _activateJob and _waitingPrecondition to identify a state)
            // * in front of state means only relevant for plugin that reported waiting for preconditions when activated
            //
            // not active -> _activateJob invalid, _waitingPrecondition false (NotActive) -> should be started, not counted as active
            // Activate called -> _activateJob valid, _waitingPrecondition false (Activating) -> should not be started, counted as active
            // * waiting for preconditions reported -> _activateJob invalid, _waitingPrecondition true (WaitingForPreconditionNotActive) -> should not be started, not counted as active
            // * initialize notification after waiting for precondition reported -> _activateJob valid, _waitingPrecondition true (WaitingForPreconditionActive) -> should not be started, counted as active
            // 
            // - plugin activated notification -> end state (will be removed from queue)
            // OR
            // - deinitialize notification, so failed (no preconditions), wil be restarted automatically ->  _activateJob valid, _waitingPrecondition false (Active) -> should not be started, counted as active
            // - * deinitialize notification, so failed (as this might be overallocated it should not be automatically restarted) -> _activateJob invalid, _waitingPrecondition false (NotActive) -> should be started again, not counted as active 
            //     so this state will move back to not active and start from there again
            // - deinitialize notification, so failed but failed too many times -> end state (will be removed from queue)

            enum class State {
                NotActive, 
                Activating, 
                WaitingForPreconditionNotActive, 
                WaitingForPreconditionActive 
            };

            State GetState() const
            {
                State state = State::Activating;

                if (_activateJob.IsValid()) {
                    if (_waitingPrecondition == true) {
                        state = State::WaitingForPreconditionActive;
                    }
                } else {
                    if (_waitingPrecondition == true) {
                        state = State::WaitingForPreconditionNotActive;
                    } else {
                        state = State::NotActive;
                    }
                }

                return state;
            }

        private:
            void SetInactive()
            {
                if (_activateResultJob.IsValid() == true) {
                    _activateResultJob->RevokeAndBlock();
                    _activateResultJob = std::move(ActivateResultJobProxyType()); // we can let of our reference (let's not use Release() on the proxy as that look a little confusing)
                }
                // note now awe are sure the _activateResultJob will not longer run at all, even if the _activateJob is running or will run... (so note order of revoking the _activateResultJob and _activateJob is important)
                if (_activateJob.IsValid() == true) {
                    Core::IWorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(_activateJob)); // note this revoke could be while running the ActivationJob itself this is allowed, We can also not skip the revocation as the activation might also be the result of an externally triggered activation
                    _activateJob = std::move(ActivateJobProxyType()); // not active anymore (let's not use Release() on the proxy as that look a little confusing)
                }
            }

            void NotifyInitiator(const Exchange::IPluginAsyncStateControl::IActivationCallback::state state)
            {
                if (_callback != nullptr) {
                    // as the IPluginAsyncStateControl does not have a json interface at this moment no enum conversion generated
                    TRACE(Trace::DetailedInfo, (_T("Result Callback called for plugin [%s] with state [%s]"), Callsign().c_str(), (state == Exchange::IPluginAsyncStateControl::IActivationCallback::state::SUCCESS ? _T("success") : (state == Exchange::IPluginAsyncStateControl::IActivationCallback::state::FAILURE ? _T("failure") : _T("aborted")))));
                    // for now let's not decouple. In the future if users prove to be unreliable we might however to not block Thunder internally
                    _callback->Finished(Callsign(), state, Retries());
                }
            }

        private:
            template <typename IMPLEMENTATION>
            class RevokeAndBlockJobType {
            public:
                RevokeAndBlockJobType(const RevokeAndBlockJobType<IMPLEMENTATION>&) = delete;
                RevokeAndBlockJobType<IMPLEMENTATION>& operator=(const RevokeAndBlockJobType<IMPLEMENTATION>&) = delete;
                RevokeAndBlockJobType(RevokeAndBlockJobType<IMPLEMENTATION>&&) = delete;
                RevokeAndBlockJobType<IMPLEMENTATION>& operator=(RevokeAndBlockJobType<IMPLEMENTATION>&&) = delete;

                template <typename... Args>
                explicit RevokeAndBlockJobType(Args&&... args)
                    : _workerjob(std::forward<Args>(args)...)
                    , _blocked(false)
                    , _adminLock()
                {
                }

                ~RevokeAndBlockJobType()
                {
                }

                bool Submit()
                {
                    bool submitted = false;
                    Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

                    if (_blocked == false) {
                        submitted = _workerjob.Submit();
                    }
                    return submitted;
                }
                bool Reschedule(const Core::Time& time)
                {
                    bool rescheduled = false;

                    Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

                    if (_blocked == false) {
                        rescheduled = _workerjob.Reschedule(time);
                    }
                    return rescheduled;
                }
                void RevokeAndBlock()
                {
                    Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

                    _blocked = true;
                    _workerjob.Revoke();
                }

                operator IMPLEMENTATION&()
                {
                    return _workerjob;
                }
                operator const IMPLEMENTATION&() const
                {
                    return _workerjob;
                }

            private:
                Core::WorkerPool::JobType<IMPLEMENTATION> _workerjob;
                std::atomic_bool _blocked;
                Core::CriticalSection _adminLock;
            };

        private:

            class ActivateResultJob {
            public:
                explicit ActivateResultJob(PluginInitializerService& pluginInitService, const string& callsign)
                    : _initservice(pluginInitService)
                    , _callsign(callsign)
                    , _result(Core::ERROR_NONE)
                {
                }
                ~ActivateResultJob() = default;

                ActivateResultJob(const ActivateResultJob&) = delete;
                ActivateResultJob& operator=(const ActivateResultJob&) = delete;
                ActivateResultJob(ActivateResultJob&&) = delete;
                ActivateResultJob& operator=(ActivateResultJob&&) = delete;

                void Result(const Core::hresult result)
                {
                    _result = result;
                }
                Core::hresult Result()
                {
                    return _result;
                }

            private:
                friend Core::ThreadPool::JobType<ActivateResultJob>;
                void Dispatch() 
                {
                    _initservice.ActivationResultNotification(_callsign, Result());
                }

            private:
                PluginInitializerService& _initservice;
                string _callsign;
                Core::hresult _result;
            };

        // note we cannot make it a WorkerPool::JobType as we need it to work with a ProxyType that we can reset from the job itself (the WorkerPool::JobType then has an internal composite job)
        class ActivateJob : public Core::IDispatch {
        public:
            explicit ActivateJob(PluginHost::IShell* requestedPluginShell, const Core::ProxyType<RevokeAndBlockJobType<ActivateResultJob>>& resultjob)
                : Core::IDispatch()
                , _requestedPluginShell(requestedPluginShell)
                , _resultjob(resultjob)
            {
                ASSERT(requestedPluginShell != nullptr);
                _requestedPluginShell->AddRef();
            }

            ~ActivateJob() override
            {
                _requestedPluginShell->Release();
            }

            ActivateJob(const ActivateJob&) = delete;
            ActivateJob& operator=(const ActivateJob&) = delete;
            ActivateJob(ActivateJob&&) = delete;
            ActivateJob& operator=(ActivateJob&&) = delete;

        private:

            void Dispatch() override
            {
                TRACE(Trace::Information, (_T("Activating plugin form ActivateJob [%s]"), _requestedPluginShell->Callsign().c_str()));
                Core::hresult result = _requestedPluginShell->Activate(PluginHost::IShell::REQUESTED);
                // after the previous call the plugin could reached state Started or fully deactivated (due to the notifications triggered from it) and not be available anymore in the PluginInitializerService queue...
                TRACE(Trace::Information, (_T("Activating plugin job [%s] result [%s](%u)"), _requestedPluginShell->Callsign().c_str(), Core::ErrorToString(result), result));

                ASSERT(_resultjob.IsValid() == true);

                // so we cannot handle the result in this job as we can never guarantee it is running after the PluginStarter has been destructed and removed from the PluginInitializerService queue. Now in itself that is not a problem as we will access the
                // PluginStarter from the PluginInitializerService in a lock and if it was gone it will not be found but the real problem is it could be that we are just shutting down the plugin and the PluginInitializerService itself is no longer available...
                // therefore we do it from a second job from which we can guarantee it will not run at all (either not submitted here after the PluginStarter has been destroyed or fully revoked before it can run)
                ActivateResultJob& resultjob = *(_resultjob);
                resultjob.Result(result);
                _resultjob->Submit();  // note this is not your avarage job submit...
            }

        private:
            PluginHost::IShell* _requestedPluginShell;
            Core::ProxyType<RevokeAndBlockJobType<ActivateResultJob>> _resultjob;
        };

        private:
            using ActivateJobProxyType = Core::ProxyType<ActivateJob>; 
            using ActivateResultJobProxyType = Core::ProxyType<RevokeAndBlockJobType<ActivateResultJob>>; 

            string _callsign; //as _requestedPluginShell->Callsign(); returns a temp and we need this lots of times so let's keep a copy
            PluginHost::IShell* _requestedPluginShell;
            uint8_t _attempt;
            uint8_t _maxnumberretries;
            uint16_t _delay;
            IPluginAsyncStateControl::IActivationCallback* _callback;
            PluginInitializerService& _initializerservice;
            ActivateJobProxyType _activateJob;
            ActivateResultJobProxyType _activateResultJob;
            bool _waitingPrecondition;
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
            void Deactivated(const string& callsign VARIABLE_IS_NOT_USED, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED) override
            {
                // this event doesn't add much value to use for this use case, it means deactivation has started but is not finished.
            }
            void Unavailable(const string& callsign VARIABLE_IS_NOT_USED, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED) override
            {
            }

            // IPlugin::ILifeTime overrides
            void Initialize(const string& callsign VARIABLE_IS_NOT_USED, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED) override
            {
                _initservice.InitializedNotification(callsign);
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

        // must be called from inside the lock
        void ActivateNotificationsIfNeeded()
        {
            ASSERT(_service != nullptr);

            if ((_pluginInitList.size() == 1) && (RevokeDeactivationJob() == false)) { // note size() is constant time (so fast, c++11)
                _service->Register(&_sink);
                TRACE(Trace::Information, (_T("Started listening for plugin state notifications")));
            }
        }

        // must be called from inside the lock
        void DeactivateNotificationsIfPossible(const bool synchronous = true)
        {
            ASSERT(_service != nullptr);

            if (_pluginInitList.size() == 0) { // note size() is constant time (so fast, c++11)
                if ((synchronous == true) && (RevokeDeactivationJob() == true)) {
                    _service->Unregister(&_sink);
                    TRACE(Trace::Information, (_T("Stopped listening for plugin state notifications")));
                } else {
                    SubmitDeactivationJob();
                }
            }
        }

        void SubmitDeactivationJob()
        {
            DeactivateNotificationsJob& job = _deactivateNotificationsJob;
            job.Submit();
        }

        bool RevokeDeactivationJob()  // returns true when unregister was planned but did not happen yet
        {
            DeactivateNotificationsJob& job = _deactivateNotificationsJob;
            return job.Revoke();
        }

        bool NewPluginStarter(PluginHost::IShell* const requestedPluginShell, const uint8_t maxnumberretries, uint16_t const delay, IPluginAsyncStateControl::IActivationCallback* const callback)
        {
            bool result = true;
            PluginStarter starter(requestedPluginShell, maxnumberretries, delay, callback, *this);

            _adminLock.Lock();

            //see if this callsign is not yet in the list
            if (std::find(_pluginInitList.cbegin(), _pluginInitList.cend(), starter) == _pluginInitList.cend()) {
                ASSERT(_pluginInitList.size() < std::numeric_limits<uint16_t>::max()); // I'll bet this will fire at some point :)

                _pluginInitList.emplace_back(std::move(starter));
                ActivateNotificationsIfNeeded();
                ActivateAnotherPlugin();
                DeactivateNotificationsIfPossible(); // could be that the Activation failed/succeeded immediately and the list is again empty here so check if notifications no longer needed
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
            ASSERT(_pluginInitList.size() > 0);

            TRACE(Trace::DetailedInfo, (_T("Going to try to activate another plugin")));
            PluginStarterContainer::iterator it = _pluginInitList.begin(); // we just start activating from the top, so we try to do it more or less in the order of incoming requests
            uint16_t currentlyActiveCounted = 0;
            while ((it != _pluginInitList.end()) && (currentlyActiveCounted < _maxparallel)) {
                PluginStarter::State state = it->GetState();
                if (state == PluginStarter::State::NotActive) {
                    TRACE(Trace::DetailedInfo, (_T("Activating another plugin")));
                    if (it->Activate() == true) {
                        // oops this plugin could not be started, end state reached we can remove it
                        TRACE(Trace::DetailedInfo, (_T("Activating handled in Activate call, removing")));
                        it = _pluginInitList.erase(it);
                    } else {
                        break; // we activated another we can stop looking... (as this method is only called when added another request or one was handled)
                    }
                } else if ((state == PluginStarter::State::Activating) || (state == PluginStarter::State::WaitingForPreconditionActive)) {
                    ++currentlyActiveCounted;
                    ++it;
                } else {
                    ++it;
                } // so if the state is waiting for preconditions we do not count it as active and try to start another...
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
                if (_pluginInitList.size() != 0) {
                    ActivateAnotherPlugin();
                }
                DeactivateNotificationsIfPossible(); // we need to do this whether or not we called ActivateAnotherPlugin here
                _adminLock.Unlock();
                result = true;
                toAbort.Abort(); // note it is essential this is called outside the lock
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
                if (_pluginInitList.size() != 0) {
                    ActivateAnotherPlugin();
                }
                DeactivateNotificationsIfPossible(false); // we need to do this whether or not we called ActivateAnotherPlugin here
                _adminLock.Unlock();
                activated.Activated(); // note it is essential this is called outside the lock
            } else {
                _adminLock.Unlock();
            }
        }

        void DeinitializedNotification(const string& callsign)
        {
            _adminLock.Lock();

            PluginStarterContainer::iterator it = std::find(_pluginInitList.begin(), _pluginInitList.end(), callsign);
            if (it != _pluginInitList.end()) {
                PluginStarter::ResultCode result = it->Deinitialized(); // note must be inside the lock as it could be in parallel with an abort request
                if (result == PluginStarter::ResultCode::Failed) {
                    PluginStarter failed(std::move(*it));
                    _pluginInitList.erase(it); // we're done retrying just give up and remove it from the list
                    if (_pluginInitList.size() != 0) {
                        ActivateAnotherPlugin();
                    }
                    DeactivateNotificationsIfPossible(false); // we need to do this whether or not we called ActivateAnotherPlugin here
                    _adminLock.Unlock();
                    failed.Failed();
                } else if (result == PluginStarter::ResultCode::Paused) {
                    ActivateAnotherPlugin();
                    // no need to check if we need to DeactivateNotificationsIfPossible as the paused one must be there
                    _adminLock.Unlock();
                } else { // continue will be handled in the follow up notification
                    _adminLock.Unlock();
                }
            } else {
                _adminLock.Unlock();
            }
        }

        void InitializedNotification(const string& callsign)
        {
            _adminLock.Lock();

            PluginStarterContainer::iterator it = std::find(_pluginInitList.begin(), _pluginInitList.end(), callsign);
            if (it != _pluginInitList.end()) {
                it->Initialized();  // note must be inside the lock as it could be in parallel with an abort request.
            }
            _adminLock.Unlock();
        }

        void ActivationResultNotification(const string& callsign, const Core::hresult result) {

            _adminLock.Lock();

            PluginStarterContainer::iterator it = std::find(_pluginInitList.begin(), _pluginInitList.end(), callsign);

            ASSERT(it != _pluginInitList.end()); // as this is triggered when the activation call has been done we expect the callsign to always be available (it was probably forgotten to revoke the PluginStarter::ActivateJob job)

            if (it != _pluginInitList.end()) {
                PluginStarter::ResultCode resultcode = it->HandleActivationResult(result); // note must be inside the lock as it could be in parallel with an abort request.
                if (resultcode == PluginStarter::ResultCode::Failed) { 
                    PluginStarter failed(std::move(*it));
                    _pluginInitList.erase(it); // we're done retrying just give up and remove it from the list
                    if (_pluginInitList.size() != 0) {
                        ActivateAnotherPlugin();
                    }
                    DeactivateNotificationsIfPossible(); // we need to do this whether or not we called ActivateAnotherPlugin here
                    _adminLock.Unlock();
                    failed.Failed(); // not strictly necessary but for consistency
                } else if (resultcode == PluginStarter::ResultCode::Paused) {
                    ActivateAnotherPlugin();
                    // no need to check if we need to DeactivateNotificationsIfPossible as the paused one must be there
                    _adminLock.Unlock();
                } else { // we need to continue, handled in the follow up notifications 
                    _adminLock.Unlock();
                }
            } else {
                _adminLock.Unlock();
            }
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
                DeactivateNotificationsIfPossible();
            }

             _adminLock.Unlock();
        }

        struct StarterQueueInfo {
            uint16_t Activating;
            uint16_t WaitingForPreconditionNotActive;
            uint16_t WaitingForPreconditionActive;
            uint16_t TotalStarters;

            string ToString() const
            {
                const TCHAR text[] = _T("Requests being activated: %u, Request waiting for preconditions %u, Request starting after preconditions are met %u, Total number of requests: %u");
                return Core::Format(text, Activating, WaitingForPreconditionNotActive, WaitingForPreconditionActive, TotalStarters);
            }
        };

        // note must be done inside lock
        StarterQueueInfo GetStarterQueueInfo() const
        {
            StarterQueueInfo info{};
            for (const PluginStarter& starter : _pluginInitList) {
                PluginStarter::State state = starter.GetState();
                switch (state) {
                    case PluginStarter::State::Activating : 
                        ++info.Activating;
                        break;
                    case PluginStarter::State::WaitingForPreconditionNotActive: 
                        ++info.WaitingForPreconditionNotActive;
                        break;
                    case PluginStarter::State::WaitingForPreconditionActive:
                        ++info.WaitingForPreconditionActive;
                        break;
                    case PluginStarter::State::NotActive:
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
            explicit DeactivateNotificationsJob(PluginInitializerService& parent)
                : _parent(parent)
                , _waitingAsyncDeactivation(false)
            {
            }
            ~DeactivateNotificationsJob() = default;

            DeactivateNotificationsJob(const DeactivateNotificationsJob&) = delete;
            DeactivateNotificationsJob& operator=(const DeactivateNotificationsJob&) = delete;
            DeactivateNotificationsJob(DeactivateNotificationsJob&&) = delete;
            DeactivateNotificationsJob& operator=(DeactivateNotificationsJob&&) = delete;

        public:
            void Submit()
            {
                _waitingAsyncDeactivation.store(true);
                _parent._deactivateNotificationsJob.Submit();
            }

            bool Revoke() //  returns true when not run but was submitted
            {
                bool revoked = false;
                if (_waitingAsyncDeactivation.load() == true) { // if not true the job was not posted or was already finished
                    _parent._deactivateNotificationsJob.Revoke();

                    revoked = (_waitingAsyncDeactivation.load() == true); // still true, job did not run before revoke
                    _waitingAsyncDeactivation.store(false); // just in case somebody would call revoke again
                }
                return revoked;
            }

        private:
            friend Core::ThreadPool::JobType<DeactivateNotificationsJob>;

            void Dispatch()
            {
                TRACE(Trace::DetailedInfo, (_T("Stopped listening for plugin state notifications (from job)")));
                _parent._service->Unregister(&_parent._sink);
                _waitingAsyncDeactivation.store(false);
            }

        private:
            PluginInitializerService& _parent;
            std::atomic_bool _waitingAsyncDeactivation;
        };

    private:
        using PluginStarterContainer = std::list<PluginStarter>; // for now we keep them in a list as we more or less agreed to activate plugins in order of requests receieved (and not a vector as we will constanlty add/remove items) (if needed we can add a shadow unordered map for quick lookup but we do not expect that many parallel activation requests, at least I hope...)

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