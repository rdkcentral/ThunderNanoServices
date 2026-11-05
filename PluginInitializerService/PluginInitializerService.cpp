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

#include "PluginInitializerService.h"

namespace WPEFramework {
namespace Plugin {
    
    namespace {
        
        static Metadata<PluginInitializerService>metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }
    
    const string PluginInitializerService::Initialize(PluginHost::IShell* service) {
        string message;
        
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        Config config;
        config.FromString(service->ConfigLine());

        if (config.MaxParallel.IsSet() == true) {
            _maxparallel = config.MaxParallel.Value();
            if (_maxparallel == 0)  {
                message = _T("maxparallel configured incorrectly");
            }
        } else {
            _maxparallel = 2;
        }
        _maxretries = config.MaxRetries.Value();
        _delay = config.Delay.Value();

        if (message.empty() == true) {
            _service = service;
            _service->AddRef();
        }

        // note we will not register for the plugin state notifications here but only do that when it actually needed later on, and more importantly also stop listening when there are no more plugins to start 
        // (this to make sure hat even if this plugin is not deactivated when there are no more plugins to start it will not give any unnecessary overhead like constantly being notified on plugin state transitions)

        return (message);
    }
    
    void PluginInitializerService::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED) 
    {
        ASSERT((_service == nullptr) || (_service == service));

        CancelAll();

        if (_service != nullptr) {
            _service->Release();
            _service = nullptr;
        }
    }
    
    string PluginInitializerService::Information() const {
        return (string());
    }

    // note we will not specifically handle the connection from the client and the plugin being closed after we stored the callback.
    // worst case: the connection being closed without abort called but in that case we will call the callback on a dead proxy and then 
    // release it, so no leaks (no need to go through the trouble to handle the dangling proxies here)
    Core::hresult PluginInitializerService::Activate(const string& callsign, const uint8_t& maxnumberretries, const uint16_t& delay, IPluginAsyncStateControl::IActivationCallback* const cb)
    {
        TRACE(Trace::Information, (_T("Plugin Activate request received for plugin [%s]"), callsign.c_str()));

        Core::hresult result = Core::ERROR_NONE;

        PluginHost::IShell* requestedpluginShell = _service->QueryInterfaceByCallsign<PluginHost::IShell>(callsign);

        if (requestedpluginShell != nullptr) {
            PluginHost::IShell::state state = requestedpluginShell->State();
            if ((state == PluginHost::IShell::DEACTIVATED) || 
                (state == PluginHost::IShell::DEACTIVATION) || 
                (state == PluginHost::IShell::ACTIVATION) ||  // this and the PRECONDITION are rather special cases, These plugin were already request to activate somewhere else, as we cannot just send a success notification (they might fail to initialize) we'll take them into account, if it was the result of a previous PluginInitializerService we'll find out anyway.
                (state == PluginHost::IShell::PRECONDITION) ) // note PRECONDITION can be both reached during deactivation and activation, for the purpose here it does not matter, we only have to monitor if the activation succeeds and report back or monitor deinitialization failure and retry
            {
                TRACE(Trace::Information, (_T("Plugin Activate request received for plugin [%s] in state [%s]"), callsign.c_str(), Core::EnumerateType<PluginHost::IShell::state>(state).Data()));
                if (NewPluginStarter(requestedpluginShell
                                    , (maxnumberretries > 0 ? maxnumberretries : _maxretries)
                                    , (delay > 0 ? delay : _delay)
                                    , cb) == true) {
                    TRACE(Trace::DetailedInfo, (_T("Plugin start entry created for plugin [%s]"), callsign.c_str()));
                } else {
                    TRACE(Trace::Warning, (_T("Plugin start entry not created for plugin [%s], there was already a pending request for this plugin"), callsign.c_str()));
                    result = Core::ERROR_INPROGRESS;
                }
            } else if ((state == PluginHost::IShell::ACTIVATED) || 
                       (state == PluginHost::IShell::HIBERNATED)) {
                TRACE(Trace::Warning, (_T("Plugin Activate received for plugin [%s] that was already active, state [%s]"), callsign.c_str(), Core::EnumerateType<PluginHost::IShell::state>(state).Data()));

                if (cb != nullptr)
                {
                    TRACE(Trace::DetailedInfo, (_T("Result callback success called for plugin [%s]"), callsign.c_str()));
                    cb->Finished(callsign, Exchange::IPluginAsyncStateControl::IActivationCallback::state::SUCCESS, 0);
                }
            } else { // DESTROYED || UNAVAILABLE
                TRACE(Trace::Error, (_T("Could not start activating plugin [%s] as it is in an illegal state [%s]"), callsign.c_str(), Core::EnumerateType<PluginHost::IShell::state>(state).Data()));
                result = Core::ERROR_ILLEGAL_STATE;
            }

            requestedpluginShell->Release();
            requestedpluginShell = nullptr;

        } else {
            TRACE(Trace::Error, (_T("Could not start activating plugin [%s] as it is unknown"), callsign.c_str()));
            result = Core::ERROR_NOT_EXIST;        
        }

        return result;
    }

    Core::hresult PluginInitializerService::AbortActivate(const string& callsign)
    {
        TRACE(Trace::Information, (_T("Plugin Abort Activate request received for plugin [%s]"), callsign.c_str()));

        Core::hresult result = Core::ERROR_NONE;

        if (CancelPluginStarter(callsign) == true) {
            TRACE(Trace::DetailedInfo, (_T("Plugin Activate request was canceled for plugin [%s]"), callsign.c_str()));
        } else {
            // note this is not necessarily an error, the abort request could just have crossed the successful activation (or failure to do so for that matter) so it was just removed from the list
            TRACE(Trace::Warning, (_T("Plugin Abort Activate request: plugin was not in activation list [%s]"), callsign.c_str()));
            result = Core::ERROR_NOT_EXIST;
        }

        return result;
    }
    
} // Plugin
} // WPEFramework
