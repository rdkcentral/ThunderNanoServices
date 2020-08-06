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
 
#include "Power.h"
#include "Implementation.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(Power, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<Power::Data>> jsonBodyDataFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<Power::Data>> jsonResponseFactory(4);

    extern "C" {

    static void PowerStateChange(void* userData, enum WPEFramework::Exchange::IPower::PCState newState) {
        reinterpret_cast<Power*>(userData)->PowerChange(newState);
    }

    }

    /* virtual */ const string Power::Initialize(PluginHost::IShell* service)
    {
        string message;
        WPEFramework::Exchange::IPower::PCState persistedState = power_get_persisted_state();

        ASSERT(_service == nullptr);

        // Setup skip URL for right offset.
        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

        Config config;

        config.FromString(_service->ConfigLine());

        _powerKey = config.PowerKey.Value();
        _powerOffMode = config.OffMode.Value();
        _controlClients = config.ControlClients.Value();
        if (_powerKey != KEY_RESERVED) {
            PluginHost::VirtualInput* keyHandler(PluginHost::InputHandler::Handler());

            ASSERT(keyHandler != nullptr);

            keyHandler->Register(&_sink, _powerKey);
        }

        // Receive all plugin information on state changes.
        _service->Register(&_sink);

        power_initialize(PowerStateChange, this, _service->ConfigLine().c_str(), persistedState);

        return message;
    }

    /* virtual */ void Power::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        // No need to monitor the Process::Notification anymore, we will kill it anyway.
        _service->Unregister(&_sink);

        // Remove all registered clients
        _clients.clear();

        if (_powerKey != KEY_RESERVED) {
            // Also we are nolonger interested in the powerkey events, we have been requested to shut down our services!
            PluginHost::VirtualInput* keyHandler(PluginHost::InputHandler::Handler());

            ASSERT(keyHandler != nullptr);
            keyHandler->Unregister(&_sink, _powerKey);
        }

        power_deinitialize();
        _service = nullptr;
    }

    /* virtual */ string Power::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void Power::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_POST)
            request.Body(jsonBodyDataFactory.Element());
    }

    /* virtual */ Core::ProxyType<Web::Response> Power::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::TextSegmentIterator index(
            Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = "Unknown error";

        if ((request.Verb == Web::Request::HTTP_GET) && ((index.Next() == true) && (index.Next() == true))) {
            result->ErrorCode = Web::STATUS_OK;
            result->Message = "OK";
            if (index.Remainder() == _T("State")) {
                Core::ProxyType<Web::JSONBodyType<Data>> response(jsonResponseFactory.Element());
                response->PowerState = power_get_state();
                if (response->PowerState) {
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                } else {
                    result->Message = "Invalid State";
                }
            } else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = "Unknown error";
            }
        } else if ((request.Verb == Web::Request::HTTP_POST) && (index.Next() == true) && (index.Next() == true)) {
            result->ErrorCode = Web::STATUS_OK;
            result->Message = "OK";
            if (index.Remainder() == _T("State")) {
                uint32_t timeout = request.Body<const Data>()->Timeout.Value();
                Exchange::IPower::PCState state = static_cast<Exchange::IPower::PCState>(request.Body<const Data>()->PowerState.Value());

                ControlClients(state);

                Core::ProxyType<Web::JSONBodyType<Data>> response(jsonResponseFactory.Element());
                response->Status = SetState(state, timeout);
                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            } else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = "Unknown error";
            }
        }

        return result;
    }

    void Power::Register(Exchange::IPower::INotification* sink)
    {
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(std::find(_notificationClients.begin(), _notificationClients.end(), sink) == _notificationClients.end());

        _notificationClients.push_back(sink);
        sink->AddRef();

        _adminLock.Unlock();

        TRACE(Trace::Information, (_T("Registered a sink on the power")));
    }

    void Power::Unregister(Exchange::IPower::INotification* sink)
    {
        _adminLock.Lock();

        std::list<Exchange::IPower::INotification*>::iterator index(std::find(_notificationClients.begin(), _notificationClients.end(), sink));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _notificationClients.end());

        if (index != _notificationClients.end()) {
            (*index)->Release();
            _notificationClients.erase(index);
            TRACE(Trace::Information, (_T("Unregistered a sink on the power")));
        }

        _adminLock.Unlock();
    }

    Exchange::IPower::PCState Power::GetState() const /* override */ {
        return (power_get_state());
    }

    uint32_t Power::SetState(const Exchange::IPower::PCState state, const uint32_t waitTime) /* override */ {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if (power_get_state() == state) {
            result = Core::ERROR_DUPLICATE_KEY;
            TRACE(Trace::Information, (_T("No need to change power states, we are already at this stage!")));
        } else if (is_power_state_supported(state)) {
            _adminLock.Unlock();

            std::list<Exchange::IPower::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end()) {
                (*index)->StateChange(state);
                index++;
            }

            _adminLock.Unlock();
 
            if (state != Exchange::IPower::PCState::On) {
                ControlClients(state);
            }

            /* Better save target state before triggering the transition (Zero delay ?). */
            power_set_persisted_state(state);

            if ( (result = power_set_state(state, waitTime)) != Core::ERROR_NONE) {
                TRACE(Trace::Information, (_T("Could not change the power state, error: %d"), result));
            }
        }

        return (result);
    }
    void Power::PowerChange(const Exchange::IPower::PCState state) {

        if (state == Exchange::IPower::PCState::On) {
            ControlClients(state);
        }

        _adminLock.Lock();

        std::list<Exchange::IPower::INotification*>::iterator index(_notificationClients.begin());

        while (index != _notificationClients.end()) {
            (*index)->StateChange(state);
            index++;
        }

        _adminLock.Unlock();

        /* May be resuming from another power state; lets update persisted state. */
        power_set_persisted_state(state);
    }
    void Power::PowerKey() /* override */ {
        if (power_get_state() == Exchange::IPower::PCState::On) {
            // Maybe this value should be coming from the config :-)
            SetState(_powerOffMode, ~0);
        }
        else {
            SetState(Exchange::IPower::PCState::On, 0);
        }
    }
    void Power::KeyEvent(const uint32_t keyCode)
    {
        // We only subscribed for the KEY_POWER event so do not
        // expect anything else !!!
        ASSERT(keyCode == KEY_POWER);

        if (keyCode == KEY_POWER) {
            PowerKey();
        }
    }
    void Power::StateChange(PluginHost::IShell* plugin)
    {
        const string callsign(plugin->Callsign());

        _adminLock.Lock();

        Clients::iterator index(_clients.find(callsign));

        if (plugin->State() == PluginHost::IShell::ACTIVATED) {

            if (index == _clients.end()) {
                PluginHost::IStateControl* stateControl(plugin->QueryInterface<PluginHost::IStateControl>());

                if (stateControl != nullptr) {
                    _clients.emplace(std::piecewise_construct,
                        std::forward_as_tuple(callsign),
                        std::forward_as_tuple(stateControl));
                    TRACE(Trace::Information, (_T("%s plugin is add to power control list"), callsign.c_str()));
                    stateControl->Release();
                }
            }
        } else if (plugin->State() == PluginHost::IShell::DEACTIVATION) {

            if (index != _clients.end()) { // Remove from the list, if it is already there
                _clients.erase(index);
                TRACE(Trace::Information, (_T("%s plugin is removed from power control list"), plugin->Callsign().c_str()));
            }
        }

        _adminLock.Unlock();
    }

    void Power::ControlClients(Exchange::IPower::PCState state)
    {
        if ((_controlClients) && (is_power_state_supported(state))) {
            Clients::iterator client(_clients.begin());

            switch (state) {
                case Exchange::IPower::PCState::On:
                    TRACE(Trace::Information, (_T("Change state to RESUME for")));
                    while (client != _clients.end()) {
                        client->second.Resume();
                        client++;
                    }
                    //Nothing to be done
                    break;
                case Exchange::IPower::PCState::ActiveStandby:
                case Exchange::IPower::PCState::PassiveStandby:
                case Exchange::IPower::PCState::SuspendToRAM:
                case Exchange::IPower::PCState::Hibernate:
                case Exchange::IPower::PCState::PowerOff:
                    while (client != _clients.end()) {
                        client->second.Suspend();
                        client++;
                    }
                    break;
                default:
                    ASSERT(false);
                    break;
            }
        }
    }

} //namespace Plugin
} // namespace WPEFramework
