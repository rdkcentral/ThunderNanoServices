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

#include "TestStateControl.h"

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<TestStateControl> metadata(
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

    const string TestStateControl::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);
        _service = service;
        _service->AddRef();

        QualityAssurance::JTestStateControl::Register(*this, this);

        return {};
    }

    void TestStateControl::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ASSERT(_service == service);

        QualityAssurance::JTestStateControl::Unregister(*this);

        _job.Revoke();

        _adminLock.Lock();
        _state = PluginHost::IStateControl::EXITED;
        std::list<PluginHost::IStateControl::INotification*> snapshot;
        snapshot.swap(_observers);
        _adminLock.Unlock();

        for (auto* observer : snapshot) {
            observer->StateChange(PluginHost::IStateControl::EXITED);
            observer->Release();
        }

        _service->Release();
        _service = nullptr;
    }

    string TestStateControl::Information() const
    {
        return {};
    }

    Core::hresult TestStateControl::Configure(PluginHost::IShell* /* framework */)
    {
        return Core::ERROR_NONE;
    }

    PluginHost::IStateControl::state TestStateControl::State() const
    {
        _adminLock.Lock();
        auto state = _state;
        _adminLock.Unlock();
        return state;
    }

    Core::hresult TestStateControl::Request(const PluginHost::IStateControl::command command)
    {
        _adminLock.Lock();
        _requestedCommand = command;
        _job.Submit();
        _adminLock.Unlock();
        return Core::ERROR_NONE;
    }

    void TestStateControl::Dispatch()
    {
        bool changed = false;

        _adminLock.Lock();

        if (_requestedCommand == PluginHost::IStateControl::RESUME) {
            if (_state == PluginHost::IStateControl::UNINITIALIZED
                || _state == PluginHost::IStateControl::SUSPENDED) {
                _state = PluginHost::IStateControl::RESUMED;
                changed = true;
            }
        } else {
            if (_state == PluginHost::IStateControl::RESUMED
                || _state == PluginHost::IStateControl::UNINITIALIZED) {
                _state = PluginHost::IStateControl::SUSPENDED;
                changed = true;
            }
        }

        if (changed) {
            // Snapshot the observer list under the lock (AddRef each entry) so
            // callbacks can be invoked without holding _adminLock.  This prevents
            // a deadlock if an observer calls back into Register/Unregister/State.
            std::vector<PluginHost::IStateControl::INotification*> snapshot;
            snapshot.reserve(_observers.size());
            for (auto* observer : _observers) {
                observer->AddRef();
                snapshot.push_back(observer);
            }

            const PluginHost::IStateControl::state currentState = _state;
            _adminLock.Unlock();

            for (auto* observer : snapshot) {
                observer->StateChange(currentState);
                observer->Release();
            }

            const string stateStr = StateToString(currentState);
            QualityAssurance::JTestStateControl::Event::StateChanged(*this, stateStr);
        } else {
            _adminLock.Unlock();
        }
    }

    void TestStateControl::Register(PluginHost::IStateControl::INotification* notification)
    {
        ASSERT(notification != nullptr);

        _adminLock.Lock();
        auto it = std::find(_observers.begin(), _observers.end(), notification);
        ASSERT(it == _observers.end());
        if (it == _observers.end()) {
            notification->AddRef();
            _observers.push_back(notification);
        }
        _adminLock.Unlock();
    }

    void TestStateControl::Unregister(PluginHost::IStateControl::INotification* notification)
    {
        ASSERT(notification != nullptr);

        _adminLock.Lock();
        auto it = std::find(_observers.begin(), _observers.end(), notification);
        ASSERT(it != _observers.end());
        if (it != _observers.end()) {
            (*it)->Release();
            _observers.erase(it);
        }
        _adminLock.Unlock();
    }

    SERVICE_REGISTRATION(TestStateControl, 1, 0)

    // -------------------------------------------------------------------------
    // ITestStateControl — JSON-RPC surface
    // -------------------------------------------------------------------------

    Core::hresult TestStateControl::State(string& state) const
    {
        state = StateToString(State());
        return Core::ERROR_NONE;
    }

    Core::hresult TestStateControl::Request(const string& command)
    {
        if (command == "resume") {
            return Request(PluginHost::IStateControl::RESUME);
        } else if (command == "suspend") {
            return Request(PluginHost::IStateControl::SUSPEND);
        }
        return Core::ERROR_INCORRECT_URL;
    }

    Core::hresult TestStateControl::Register(QualityAssurance::ITestStateControl::INotification* notification)
    {
        ASSERT(notification != nullptr);
        _adminLock.Lock();
        auto it = std::find(_jsonObservers.begin(), _jsonObservers.end(), notification);
        ASSERT(it == _jsonObservers.end());
        if (it == _jsonObservers.end()) {
            notification->AddRef();
            _jsonObservers.push_back(notification);
        }
        _adminLock.Unlock();
        return Core::ERROR_NONE;
    }

    Core::hresult TestStateControl::Unregister(QualityAssurance::ITestStateControl::INotification* notification)
    {
        ASSERT(notification != nullptr);
        _adminLock.Lock();
        auto it = std::find(_jsonObservers.begin(), _jsonObservers.end(), notification);
        ASSERT(it != _jsonObservers.end());
        if (it != _jsonObservers.end()) {
            (*it)->Release();
            _jsonObservers.erase(it);
        }
        _adminLock.Unlock();
        return Core::ERROR_NONE;
    }

    // -------------------------------------------------------------------------
    // Shared helper
    // -------------------------------------------------------------------------

    // static
    string TestStateControl::StateToString(PluginHost::IStateControl::state state)
    {
        switch (state) {
        case PluginHost::IStateControl::RESUMED:       return "resumed";
        case PluginHost::IStateControl::SUSPENDED:     return "suspended";
        case PluginHost::IStateControl::EXITED:        return "exited";
        case PluginHost::IStateControl::UNINITIALIZED: // fall-through
        default:                                       return "uninitialized";
        }
    }

} // namespace Plugin
} // namespace Thunder
