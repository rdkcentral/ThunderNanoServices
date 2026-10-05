#include "TestPerCallsignRegistration.h"

#include "qa_interfaces/json/JPerCallsignRegistrationTest.h"

namespace Thunder {
namespace Plugin {

namespace {

static Metadata<TestPerCallsignRegistration> metadata(
    1, 0, 0,
    {},
    {},
    {});

Core::OptionalType<string> NormalizeCallsign(const Core::OptionalType<string>& callsign)
{
    Core::OptionalType<string> selection(callsign);
    if ((selection.IsSet() == true) && (selection.Value().empty() == true)) {
        selection.Clear();
    }
    return selection;
}

} // namespace

TestPerCallsignRegistration::TestPerCallsignRegistration()
    : _service(nullptr)
    , _controller(nullptr)
    , _shellNotificationCount(0)
    , _controllerNotificationCount(0)
{
}

TestPerCallsignRegistration::~TestPerCallsignRegistration()
{
    ASSERT(_service == nullptr);
    ASSERT(_controller == nullptr);
    ASSERT(_shellCallsigns.empty());
    ASSERT(_controllerCallsigns.empty());
}

const string TestPerCallsignRegistration::Initialize(PluginHost::IShell* service)
{
    ASSERT(service != nullptr);
    ASSERT(_service == nullptr);
    ASSERT(_controller == nullptr);

    _service = service;
    _service->AddRef();

    _controller = _service->QueryInterfaceByCallsign<Exchange::Controller::ILifeTime>(_T(""));
    if (_controller == nullptr) {
        _service->Release();
        _service = nullptr;
        return _T("Unable to obtain Controller ILifeTime interface.");
    }

    QualityAssurance::JPerCallsignRegistrationTest::Register(*this, this);
    return {};
}

void TestPerCallsignRegistration::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);

    QualityAssurance::JPerCallsignRegistrationTest::Unregister(*this);

    _adminLock.Lock();
    std::list<Core::OptionalType<string>> shellCallsigns;
    std::list<Core::OptionalType<string>> controllerCallsigns;
    shellCallsigns.swap(_shellCallsigns);
    controllerCallsigns.swap(_controllerCallsigns);
    _adminLock.Unlock();

    for (const auto& callsign : shellCallsigns) {
        _service->Unregister(this, callsign);
    }
    for (const auto& callsign : controllerCallsigns) {
        _controller->Unregister(this, callsign);
    }

    _controller->Release();
    _controller = nullptr;
    _service->Release();
    _service = nullptr;
}

string TestPerCallsignRegistration::Information() const
{
    return _T("Per-callsign IShell and IController notification test plugin.");
}

void TestPerCallsignRegistration::Activated(const string& callsign, PluginHost::IShell*)
{
    RecordShellNotification(callsign, _T("Activated"));
}

void TestPerCallsignRegistration::Deactivated(const string& callsign, PluginHost::IShell*)
{
    RecordShellNotification(callsign, _T("Deactivated"));
}

void TestPerCallsignRegistration::Unavailable(const string& callsign, PluginHost::IShell*)
{
    RecordShellNotification(callsign, _T("Unavailable"));
}

void TestPerCallsignRegistration::StateChange(
    const string& callsign,
    const PluginHost::IShell::state& state,
    const PluginHost::IShell::reason& reason)
{
    RecordControllerNotification(
        _T("StateChange:") + callsign + _T(":") + StateToString(state) + _T(":") + ReasonToString(reason));
}

void TestPerCallsignRegistration::StateControlStateChange(
    const string& callsign,
    const Exchange::Controller::ILifeTime::state& state)
{
    RecordControllerNotification(
        _T("StateControlStateChange:") + callsign + _T(":") + StateControlStateToString(state));
}

Core::hresult TestPerCallsignRegistration::MonitorShell(
    const Core::OptionalType<string>& callsign)
{
    ASSERT(_service != nullptr);
    const Core::OptionalType<string> selection = NormalizeCallsign(callsign);

    _adminLock.Lock();
    for (const auto& current : _shellCallsigns) {
        if (current == selection) {
            _adminLock.Unlock();
            return Core::ERROR_ALREADY_CONNECTED;
        }
    }
    _shellCallsigns.push_back(selection);
    _adminLock.Unlock();

    _service->Register(this, selection);
    return Core::ERROR_NONE;
}

Core::hresult TestPerCallsignRegistration::StopMonitoringShell(
    const Core::OptionalType<string>& callsign)
{
    ASSERT(_service != nullptr);
    const Core::OptionalType<string> selection = NormalizeCallsign(callsign);

    _adminLock.Lock();
    for (auto index = _shellCallsigns.begin(); index != _shellCallsigns.end(); ++index) {
        if (*index == selection) {
            _shellCallsigns.erase(index);
            _adminLock.Unlock();
            _service->Unregister(this, selection);
            return Core::ERROR_NONE;
        }
    }
    _adminLock.Unlock();
    return Core::ERROR_NOT_EXIST;
}

Core::hresult TestPerCallsignRegistration::ClearShellNotifications()
{
    _adminLock.Lock();
    _shellNotificationCount = 0;
    _lastShellNotification.clear();
    _adminLock.Unlock();
    return Core::ERROR_NONE;
}

Core::hresult TestPerCallsignRegistration::ShellNotificationCount(uint32_t& count) const
{
    _adminLock.Lock();
    count = _shellNotificationCount;
    _adminLock.Unlock();
    return Core::ERROR_NONE;
}

Core::hresult TestPerCallsignRegistration::LastShellNotification(string& notification) const
{
    _adminLock.Lock();
    notification = _lastShellNotification;
    _adminLock.Unlock();
    return Core::ERROR_NONE;
}

Core::hresult TestPerCallsignRegistration::MonitorController(
    const Core::OptionalType<string>& callsign)
{
    ASSERT(_controller != nullptr);
    const Core::OptionalType<string> selection = NormalizeCallsign(callsign);

    _adminLock.Lock();
    for (const auto& current : _controllerCallsigns) {
        if (current == selection) {
            _adminLock.Unlock();
            return Core::ERROR_ALREADY_CONNECTED;
        }
    }
    _controllerCallsigns.push_back(selection);
    _adminLock.Unlock();

    const Core::hresult result = _controller->Register(this, selection);
    if (result != Core::ERROR_NONE) {
        _adminLock.Lock();
        for (auto index = _controllerCallsigns.begin(); index != _controllerCallsigns.end(); ++index) {
            if (*index == selection) {
                _controllerCallsigns.erase(index);
                break;
            }
        }
        _adminLock.Unlock();
    }
    return result;
}

Core::hresult TestPerCallsignRegistration::StopMonitoringController(
    const Core::OptionalType<string>& callsign)
{
    ASSERT(_controller != nullptr);
    const Core::OptionalType<string> selection = NormalizeCallsign(callsign);

    _adminLock.Lock();
    for (auto index = _controllerCallsigns.begin(); index != _controllerCallsigns.end(); ++index) {
        if (*index == selection) {
            _controllerCallsigns.erase(index);
            _adminLock.Unlock();
            return _controller->Unregister(this, selection);
        }
    }
    _adminLock.Unlock();
    return Core::ERROR_NOT_EXIST;
}

Core::hresult TestPerCallsignRegistration::ClearControllerNotifications()
{
    _adminLock.Lock();
    _controllerNotificationCount = 0;
    _lastControllerNotification.clear();
    _adminLock.Unlock();
    return Core::ERROR_NONE;
}

Core::hresult TestPerCallsignRegistration::ControllerNotificationCount(uint32_t& count) const
{
    _adminLock.Lock();
    count = _controllerNotificationCount;
    _adminLock.Unlock();
    return Core::ERROR_NONE;
}

Core::hresult TestPerCallsignRegistration::LastControllerNotification(string& notification) const
{
    _adminLock.Lock();
    notification = _lastControllerNotification;
    _adminLock.Unlock();
    return Core::ERROR_NONE;
}

void TestPerCallsignRegistration::RecordShellNotification(const string& callsign, const string& event)
{
    _adminLock.Lock();
    ++_shellNotificationCount;
    _lastShellNotification = event + _T(":") + callsign;
    _adminLock.Unlock();
}

void TestPerCallsignRegistration::RecordControllerNotification(const string& notification)
{
    _adminLock.Lock();
    ++_controllerNotificationCount;
    _lastControllerNotification = notification;
    _adminLock.Unlock();
}

string TestPerCallsignRegistration::StateToString(PluginHost::IShell::state state)
{
    switch (state) {
    case PluginHost::IShell::UNAVAILABLE: return _T("UNAVAILABLE");
    case PluginHost::IShell::DEACTIVATED: return _T("DEACTIVATED");
    case PluginHost::IShell::ACTIVATED: return _T("ACTIVATED");
    case PluginHost::IShell::DEACTIVATION: return _T("DEACTIVATION");
    case PluginHost::IShell::ACTIVATION: return _T("ACTIVATION");
    case PluginHost::IShell::PRECONDITION: return _T("PRECONDITION");
    case PluginHost::IShell::HIBERNATED: return _T("HIBERNATED");
    case PluginHost::IShell::DESTROYED: return _T("DESTROYED");
    default: return _T("UNKNOWN");
    }
}

string TestPerCallsignRegistration::ReasonToString(PluginHost::IShell::reason reason)
{
    switch (reason) {
    case PluginHost::IShell::REQUESTED: return _T("REQUESTED");
    case PluginHost::IShell::AUTOMATIC: return _T("AUTOMATIC");
    case PluginHost::IShell::FAILURE: return _T("FAILURE");
    case PluginHost::IShell::MEMORY_EXCEEDED: return _T("MEMORY_EXCEEDED");
    case PluginHost::IShell::STARTUP: return _T("STARTUP");
    case PluginHost::IShell::SHUTDOWN: return _T("SHUTDOWN");
    case PluginHost::IShell::CONDITIONS: return _T("CONDITIONS");
    case PluginHost::IShell::WATCHDOG_EXPIRED: return _T("WATCHDOG_EXPIRED");
    case PluginHost::IShell::INITIALIZATION_FAILED: return _T("INITIALIZATION_FAILED");
    case PluginHost::IShell::INSTANTIATION_FAILED: return _T("INSTANTIATION_FAILED");
    default: return _T("UNKNOWN");
    }
}

string TestPerCallsignRegistration::StateControlStateToString(
    Exchange::Controller::ILifeTime::state state)
{
    switch (state) {
    case Exchange::Controller::ILifeTime::SUSPENDED: return _T("SUSPENDED");
    case Exchange::Controller::ILifeTime::RESUMED: return _T("RESUMED");
    default: return _T("UNKNOWN");
    }
}

} // namespace Plugin
} // namespace Thunder