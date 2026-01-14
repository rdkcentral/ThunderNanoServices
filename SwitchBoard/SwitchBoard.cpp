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
 
#include "SwitchBoard.h"

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<SwitchBoard> metadata(
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

    static Core::ProxyPoolType<Web::JSONBodyType<SwitchBoard::Config> > jsonBodySwitchFactory(1);

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    SwitchBoard::SwitchBoard()
        : _skipURL(0)
        , _defaultCallsign(nullptr)
        , _activeCallsign(nullptr)
        , _sink(this)
        , _service(nullptr)
        , _state(INACTIVE)
        , _job(*this)
    {
    }

POP_WARNING()
    /* virtual */ SwitchBoard::~SwitchBoard()
    {
        ASSERT(_service == nullptr);
        ASSERT(_switches.size() == 0);
    }

    /* virtual */ const string SwitchBoard::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);

        Config config;
        config.FromString(service->ConfigLine());

        if ((config.Callsigns.IsSet() == true) && (config.Callsigns.Length() > 1)) {

            const string& defaultCallsign(config.Default.Value());
            Core::JSON::ArrayType<Core::JSON::String>::Iterator index(config.Callsigns.Elements());

            _service = service;
            _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

            _service->AddRef();
            _service->Register(&_sink);

            while (index.Next() == true) {

                const string& callsign(index.Current().Value());
                PluginHost::IShell* shell(service->QueryInterfaceByCallsign<PluginHost::IShell>(callsign));

                if (shell != nullptr) {

                    _switches.insert(std::pair<string, Entry>(callsign, Entry(shell)));

                    shell->Release();
                }
            }

            if (defaultCallsign.empty() == false) {
                std::map<string, Entry>::iterator base(_switches.find(defaultCallsign));

                if (base != _switches.end()) {
                    _defaultCallsign = &(base->second);
                }
            }

            if (_switches.size() == 0) {
                _service->Unregister(&_sink);
                _service->Release();
                _service = nullptr;
                _switches.clear();
            }
            else {
                Exchange::JSwitchBoard::Register(*this, this);
            }
        }

        Initialize();

        // On succes return a name as a Callsign to be used in the URL, after the "service"prefix
        return (_service != nullptr ? _T("") : _T("No group of callsigns available."));
    }

    /* virtual */ void SwitchBoard::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);
            ASSERT(_switches.size() > 1);

            Exchange::JSwitchBoard::Unregister(*this);

            for (auto& index: _switches) {
                index.second.Unregister(&_sink);
            }
            _switches.clear();

            Deinitialize();

            _service->Unregister(&_sink);
            _service->Release();
            _service = nullptr;
        }
    }

    /* virtual */ string SwitchBoard::Information() const
    {
        // No additional info to report.
        return (string());
    }

    Core::hresult SwitchBoard::Register(Exchange::ISwitchBoard::INotification* const notification)
    {
        Core::hresult result = Core::ERROR_ALREADY_CONNECTED;

        ASSERT(notification != nullptr);

        _adminLock.Lock();

        auto it = std::find(_notificationClients.begin(), _notificationClients.end(), notification);
        ASSERT(it == _notificationClients.end());

        if (it == _notificationClients.end()) {
            notification->AddRef();

            _notificationClients.push_back(notification);

            result = Core::ERROR_NONE;
        }

        _adminLock.Unlock();

        return (result);
    }

    Core::hresult SwitchBoard::Unregister(const Exchange::ISwitchBoard::INotification* const notification)
    {
        Core::hresult result = Core::ERROR_ALREADY_RELEASED;

        ASSERT(notification != nullptr);

        _adminLock.Lock();

        auto it = std::find(_notificationClients.cbegin(), _notificationClients.cend(), notification);
        ASSERT(it != _notificationClients.end());

        if (it != _notificationClients.end()) {
            (*it)->Release();

            _notificationClients.erase(it);

            result = Core::ERROR_NONE;
        }

        _adminLock.Unlock();

        return (result);
    }

    Core::hresult SwitchBoard::Switches(Exchange::ISwitchBoard::IStringIterator*& switches) const
    {
        std::vector<string> list;

        for (const auto& entry : _switches) {
            list.push_back(entry.first);
        }

        using Implementation = RPC::IteratorType<RPC::IStringIterator>;
        switches = Core::ServiceType<Implementation>::Create<RPC::IStringIterator>(list);

        return (Core::ERROR_NONE);
    }

    Core::hresult SwitchBoard::Default(string& callsign) const
    {
        Core::hresult result = Core::ERROR_NONE;

        if (_defaultCallsign != nullptr) {
            callsign = _defaultCallsign->Callsign();
        }
        else {
            result = Core::ERROR_UNAVAILABLE;
        }

        return (result);
    }

    /* virtual */ Core::hresult SwitchBoard::IsActive(const string& callsign, bool& active) const
    {
        Core::hresult result = Core::ERROR_NONE;
        std::map<string, Entry>::const_iterator index(_switches.find(callsign));

        if (index != _switches.end()) {
            active = index->second.IsActive();
        }
        else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return (result);
    }

    /* virtual */ Core::hresult SwitchBoard::Activate(const string& callsign)
    {
        Core::hresult result = (_state == INACTIVE ? Core::ERROR_ILLEGAL_STATE : Core::ERROR_INPROGRESS);

        _adminLock.Lock();

        if (_state == IDLE) {

            _state = INPROGRESS;

            _adminLock.Unlock();

            result = Core::ERROR_UNAVAILABLE;

            std::map<string, Entry>::iterator activate(_switches.find(callsign));

            if (activate != _switches.end()) {

                result = Core::ERROR_NONE;

                if ((&(activate->second) != _activeCallsign) || (activate->second.IsActive() == false)) {

                    if ((_activeCallsign != nullptr) && (&(activate->second) != _activeCallsign)) {

                        if ((result = _activeCallsign->Deactivate()) == Core::ERROR_NONE) {
                            _activeCallsign = nullptr;
                        }
                    }

                    if ((result == Core::ERROR_NONE) &&
                        ((result = activate->second.Activate()) == Core::ERROR_NONE)) {

                        Activated(activate->second);
                    }
                }
            }

            _adminLock.Lock();

            _state = IDLE;
        }

        _adminLock.Unlock();

        return (result);
    }

    /* virtual */ Core::hresult SwitchBoard::Deactivate(const string& callsign)
    {
        Core::hresult result = (_state == INACTIVE ? Core::ERROR_ILLEGAL_STATE : Core::ERROR_INPROGRESS);

        _adminLock.Lock();

        if (_state == IDLE) {

            _state = INPROGRESS;
            _adminLock.Unlock();
            result = Core::ERROR_UNAVAILABLE;
            std::map<string, Entry>::iterator deactivate(_switches.find(callsign));

            if (deactivate != _switches.end()) {

                result = Core::ERROR_NONE;
                if (deactivate->second.IsActive() == true) {

                    // Check if it is not the currently active call sign which is also the default one..
                    if (((&(deactivate->second) != _activeCallsign) || (&(deactivate->second) != _defaultCallsign)) &&
                        ((result = deactivate->second.Deactivate()) == Core::ERROR_NONE) &&
                        (&(deactivate->second) != _activeCallsign)) {

                        // Oops we deactivated the active callsign, clear it and i
                        // check if we can start a default one.
                        _activeCallsign = nullptr;

                        if ((_defaultCallsign != nullptr) &&
                            ((result = _defaultCallsign->Activate()) == Core::ERROR_NONE)) {

                            Activated(*_defaultCallsign);
                        }
                    }
                }
            }

            _adminLock.Lock();
            _state = IDLE;
        }

        _adminLock.Unlock();

        return (result);
    }

    uint32_t SwitchBoard::Initialize()
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        ASSERT(_state == INACTIVE);

        if (_state == INACTIVE) {

            std::map<string, Entry>::iterator index(_switches.begin());
            result = Core::ERROR_NONE;

            while ((index != _switches.end()) && (result == Core::ERROR_NONE)) {

            if ((index->second.IsActive() == true) && (&(index->second) != _defaultCallsign)) {
                    result = index->second.Deactivate();
                }

                index++;
            }

            _state = IDLE;
            if ((_defaultCallsign != nullptr) && ((result = _defaultCallsign->Activate()) == Core::ERROR_NONE)) {
                Activated(*_defaultCallsign);
            }
        }

        return(result);
    }

    uint32_t SwitchBoard::Deinitialize()
    {
        uint32_t result = Core::ERROR_NONE;

        _adminLock.Lock();

        if (_state != INACTIVE) {
            while (_state != IDLE) {

                _adminLock.Unlock();
                SleepMs(0);
                _adminLock.Lock();
            }

            _state = INACTIVE;
        }

        _adminLock.Unlock();
        if (_activeCallsign != nullptr) {
            if ((result = _activeCallsign->Deactivate()) == Core::ERROR_NONE) {
                _activeCallsign = nullptr;
            }
        }

        return(result);
    }

    void SwitchBoard::Activated(Entry& entry) {

        if (entry.IsActive() == true) {

            string callsign(entry.Callsign());
            TRACE(Switching, (_T("Reporting activation of [%s]"), callsign.c_str()));
            _activeCallsign = &entry;
            std::list<Exchange::ISwitchBoard::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end())
            {
                (*index)->Activated(callsign);
                index++;
            }

            Exchange::JSwitchBoard::Event::Activated(*this, callsign);
        }
    }

    void SwitchBoard::Evaluate()
    {
        TRACE(Switching, (_T("Re-evaluating active callsign [%s]"), (_activeCallsign == nullptr ? _T("Null") : _activeCallsign->Callsign().c_str())));

        // Check if the ActiveCallsign is still active, and we are not inprogress, if not try to start the default plugin..
        if ((_activeCallsign != nullptr) && (_defaultCallsign != nullptr) && (_activeCallsign->IsActive() == false)) {

            if (_activeCallsign != _defaultCallsign) {

                // Deactivate, maybe we are only suspended and need to go to the deactivated mode.
                _activeCallsign->Deactivate();
            }

            // Oops looks like the active plugin is no longer active. Time to switch to the default.
            _activeCallsign = nullptr;

            if (_defaultCallsign->Activate() == Core::ERROR_NONE) {

                Activated(*_defaultCallsign);
            }
        }

        _adminLock.Lock();
        _state = IDLE;
        _adminLock.Unlock();
    }

    void SwitchBoard::Activated(const string& callsign, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED)
    {
        _adminLock.Lock();
        std::map<string, Entry>::iterator index(_switches.find(callsign));

        if (index != _switches.end()) {
            index->second.Register(&_sink);
        }
        _adminLock.Unlock();
    }

    void SwitchBoard::Deactivated(const string& callsign, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED)
    {
        _adminLock.Lock();
        std::map<string, Entry>::iterator index(_switches.find(callsign));

        if (index != _switches.end()) {

            index->second.Unregister(&_sink);
            if ((_state == IDLE) && (&(index->second) == _activeCallsign)) {
                _state = INPROGRESS;
                _job.Submit();
            }
        }
        _adminLock.Unlock();
    }

    void SwitchBoard::StateChange(PluginHost::IStateControl::state newState)
    {
        if ((newState == PluginHost::IStateControl::SUSPENDED) && (_state == IDLE)) {

            _adminLock.Lock();

            if (_state == IDLE) {
                _state = INPROGRESS;
                _job.Submit();
            }

            _adminLock.Unlock();
        }
    }
}
}  // namespace 
