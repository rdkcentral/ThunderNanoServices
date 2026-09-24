#include "TestPerCallsignRegistrationIController.h"

#include "qa_interfaces/json/JPerCallsignRegistrationIControllerTest.h"

#include <algorithm>

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<TestPerCallsignRegistrationIController> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {});

    } // namespace

    TestPerCallsignRegistrationIController::TestPerCallsignRegistrationIController()
        : _service(nullptr)
        , _controller(nullptr)
        , _state(PluginHost::IStateControl::UNINITIALIZED)
        , _notificationCount(0)
    {
    }

    TestPerCallsignRegistrationIController::~TestPerCallsignRegistrationIController()
    {
        ASSERT(_service == nullptr);
        ASSERT(_controller == nullptr);
        ASSERT(_monitoredCallsigns.empty());
        ASSERT(_stateObservers.empty());
    }

    const string TestPerCallsignRegistrationIController::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_controller == nullptr);

        _service = service;
        _service->AddRef();

        /*
         * An empty callsign on QueryInterfaceByCallsign() refers to
         * interfaces exposed by the Controller itself.
         */
        _controller = _service->QueryInterfaceByCallsign<Exchange::Controller::ILifeTime>(_T(""));

        if (_controller == nullptr) {
            _service->Release();
            _service = nullptr;

            return _T("Unable to obtain Controller ILifeTime interface.");
        }

        QualityAssurance::JPerCallsignRegistrationIControllerTest::Register(*this, this);

        return {};
    }

    void TestPerCallsignRegistrationIController::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        QualityAssurance::JPerCallsignRegistrationIControllerTest::Unregister(*this);

        _adminLock.Lock();

        std::list<Core::OptionalType<string>> monitoredCallsigns;
        std::list<PluginHost::IStateControl::INotification*> stateObservers;

        monitoredCallsigns.swap(_monitoredCallsigns);
        stateObservers.swap(_stateObservers);

        _adminLock.Unlock();

        for (const auto& callsign : monitoredCallsigns) {
            _controller->Unregister(this, callsign);
        }

        for (auto* observer : stateObservers) {
            observer->Release();
        }

        _controller->Release();
        _controller = nullptr;

        _service->Release();
        _service = nullptr;
    }

    string TestPerCallsignRegistrationIController::Information() const
    {
        return _T("Plugin used to test per-callsign IController state notification "
                   "registration.");
    }

    Core::hresult TestPerCallsignRegistrationIController::Configure(PluginHost::IShell*)
    {
        return Core::ERROR_NONE;
    }

    PluginHost::IStateControl::state TestPerCallsignRegistrationIController::State() const
    {
        _adminLock.Lock();
        const PluginHost::IStateControl::state state = _state;
        _adminLock.Unlock();

        return state;
    }

    Core::hresult TestPerCallsignRegistrationIController::Request(const PluginHost::IStateControl::command command)
    {
        PluginHost::IStateControl::state state;
        bool changed = false;

        _adminLock.Lock();

        state = _state;

        if ((command == PluginHost::IStateControl::RESUME) && ((_state == PluginHost::IStateControl::UNINITIALIZED) || (_state == PluginHost::IStateControl::SUSPENDED))) {
            _state = PluginHost::IStateControl::RESUMED;
            changed = true;
        } else if ((command == PluginHost::IStateControl::SUSPEND) && ((_state == PluginHost::IStateControl::UNINITIALIZED) || (_state == PluginHost::IStateControl::RESUMED))) {
            _state = PluginHost::IStateControl::SUSPENDED;
            changed = true;
        }

        state = _state;

        std::list<PluginHost::IStateControl::INotification*> observers;
        if (changed) {
            for (auto* observer : _stateObservers) {
                observer->AddRef();
                observers.push_back(observer);
            }
        }

        _adminLock.Unlock();

        for (auto* observer : observers) {
            observer->StateChange(state);
            observer->Release();
        }

        return Core::ERROR_NONE;
    }

    void TestPerCallsignRegistrationIController::Register(PluginHost::IStateControl::INotification* notification)
    {
        ASSERT(notification != nullptr);

        _adminLock.Lock();
        ASSERT(std::find(_stateObservers.begin(), _stateObservers.end(), notification) == _stateObservers.end());
        notification->AddRef();
        _stateObservers.push_back(notification);
        _adminLock.Unlock();
    }

    void TestPerCallsignRegistrationIController::Unregister(PluginHost::IStateControl::INotification* notification)
    {
        ASSERT(notification != nullptr);

        _adminLock.Lock();

        for (auto index = _stateObservers.begin(); index != _stateObservers.end(); ++index) {
            if (*index == notification) {
                (*index)->Release();
                _stateObservers.erase(index);
                break;
            }
        }

        _adminLock.Unlock();
    }

    void TestPerCallsignRegistrationIController::StateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason)
    {
        RecordNotification(_T("StateChange:") + callsign + _T(":") + StateToString(state) + _T(":") + ReasonToString(reason));
    }

    void TestPerCallsignRegistrationIController::StateControlStateChange(const string& callsign, const Exchange::Controller::ILifeTime::state& state)
    {
        RecordNotification(_T("StateControlStateChange:") + callsign + _T(":") + StateControlStateToString(state));
    }

    Core::hresult TestPerCallsignRegistrationIController::Monitor(const Core::OptionalType<string>& callsign)
    {
        ASSERT(_controller != nullptr);

        Core::OptionalType<string> selection(callsign);

        /*
         * Treat an explicitly supplied empty string as "all plugins",
         * matching the optional-index semantics used by the IShell test.
         */
        if ((selection.IsSet() == true) && (selection.Value().empty() == true)) {
            selection.Clear();
        }

        _adminLock.Lock();

        for (const auto& monitoredCallsign : _monitoredCallsigns) {
            if (monitoredCallsign == selection) {
                _adminLock.Unlock();

                return Core::ERROR_ALREADY_CONNECTED;
            }
        }

        _monitoredCallsigns.push_back(selection);

        _adminLock.Unlock();

        /*
         * This is the feature under test:
         *
         *   selection set   -> monitor one callsign
         *   selection unset -> monitor all callsigns
         */
        const Core::hresult result = _controller->Register(this, selection);

        if (result != Core::ERROR_NONE) {
            _adminLock.Lock();

            for (auto index = _monitoredCallsigns.begin(); index != _monitoredCallsigns.end(); ++index) {
                if (*index == selection) {
                    _monitoredCallsigns.erase(index);
                    break;
                }
            }

            _adminLock.Unlock();
        }

        return result;
    }

    Core::hresult TestPerCallsignRegistrationIController::StopMonitoring(const Core::OptionalType<string>& callsign)
    {
        ASSERT(_controller != nullptr);

        Core::OptionalType<string> selection(callsign);

        if ((selection.IsSet() == true) && (selection.Value().empty() == true)) {
            selection.Clear();
        }

        _adminLock.Lock();

        for (auto index = _monitoredCallsigns.begin(); index != _monitoredCallsigns.end(); ++index) {
            if (*index == selection) {
                _monitoredCallsigns.erase(index);

                _adminLock.Unlock();

                _controller->Unregister(this, selection);

                return Core::ERROR_NONE;
            }
        }

        _adminLock.Unlock();

        return Core::ERROR_NOT_EXIST;
    }

    Core::hresult TestPerCallsignRegistrationIController::ClearNotifications()
    {
        _adminLock.Lock();

        _notificationCount = 0;
        _lastNotification.clear();

        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    Core::hresult TestPerCallsignRegistrationIController::NotificationCount(uint32_t& count) const
    {
        _adminLock.Lock();

        count = _notificationCount;

        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    Core::hresult TestPerCallsignRegistrationIController::LastNotification(string& notification) const
    {
        _adminLock.Lock();

        notification = _lastNotification;

        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    void TestPerCallsignRegistrationIController::RecordNotification(const string& notification)
    {
        _adminLock.Lock();

        ++_notificationCount;
        _lastNotification = notification;

        _adminLock.Unlock();
    }

    string TestPerCallsignRegistrationIController::StateToString(const PluginHost::IShell::state state)
    {
        switch (state) {
        case PluginHost::IShell::UNAVAILABLE:
            return _T("UNAVAILABLE");

        case PluginHost::IShell::DEACTIVATED:
            return _T("DEACTIVATED");

        case PluginHost::IShell::ACTIVATED:
            return _T("ACTIVATED");

        case PluginHost::IShell::DEACTIVATION:
            return _T("DEACTIVATION");

        case PluginHost::IShell::ACTIVATION:
            return _T("ACTIVATION");

        case PluginHost::IShell::PRECONDITION:
            return _T("PRECONDITION");

        case PluginHost::IShell::HIBERNATED:
            return _T("HIBERNATED");

        case PluginHost::IShell::DESTROYED:
            return _T("DESTROYED");

        default:
            return _T("UNKNOWN");
        }
    }

    string TestPerCallsignRegistrationIController::ReasonToString(const PluginHost::IShell::reason reason)
    {
        switch (reason) {
        case PluginHost::IShell::REQUESTED:
            return _T("REQUESTED");

        case PluginHost::IShell::AUTOMATIC:
            return _T("AUTOMATIC");

        case PluginHost::IShell::FAILURE:
            return _T("FAILURE");

        case PluginHost::IShell::MEMORY_EXCEEDED:
            return _T("MEMORY_EXCEEDED");

        case PluginHost::IShell::STARTUP:
            return _T("STARTUP");

        case PluginHost::IShell::SHUTDOWN:
            return _T("SHUTDOWN");

        case PluginHost::IShell::CONDITIONS:
            return _T("CONDITIONS");

        case PluginHost::IShell::WATCHDOG_EXPIRED:
            return _T("WATCHDOG_EXPIRED");

        case PluginHost::IShell::INITIALIZATION_FAILED:
            return _T("INITIALIZATION_FAILED");

        case PluginHost::IShell::INSTANTIATION_FAILED:
            return _T("INSTANTIATION_FAILED");

        default:
            return _T("UNKNOWN");
        }
    }

    string TestPerCallsignRegistrationIController::StateControlStateToString(const Exchange::Controller::ILifeTime::state state)
    {
        switch (state) {
        case Exchange::Controller::ILifeTime::UNKNOWN:
            return _T("UNKNOWN");

        case Exchange::Controller::ILifeTime::SUSPENDED:
            return _T("SUSPENDED");

        case Exchange::Controller::ILifeTime::RESUMED:
            return _T("RESUMED");

        default:
            return _T("UNKNOWN");
        }
    }

} // namespace Plugin
} // namespace Thunder