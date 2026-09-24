
#include "TestPerCallsignRegistrationIShell.h"

#include "qa_interfaces/json/JPerCallsignRegistrationIShellTest.h"

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<TestPerCallsignRegistrationIShell> metadata(
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

TestPerCallsignRegistrationIShell::TestPerCallsignRegistrationIShell()
    : _service(nullptr)
    , _notificationCount(0)
{
}

TestPerCallsignRegistrationIShell::~TestPerCallsignRegistrationIShell()
{
    ASSERT(_service == nullptr);
    ASSERT(_monitoredCallsigns.empty());
}

const string TestPerCallsignRegistrationIShell::Initialize(PluginHost::IShell* service)
{
    ASSERT(service != nullptr);
    ASSERT(_service == nullptr);

    _service = service;
    _service->AddRef();

    QualityAssurance::JPerCallsignRegistrationIShellTest::Register(
        *this,
        this);

    return {};
}

void TestPerCallsignRegistrationIShell::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);

    QualityAssurance::JPerCallsignRegistrationIShellTest::Unregister(
        *this);

    _adminLock.Lock();

    std::list<Core::OptionalType<string>> monitoredCallsigns;

    monitoredCallsigns.swap(_monitoredCallsigns);

    _adminLock.Unlock();

    for (const auto& callsign : monitoredCallsigns) {
        _service->Unregister(this, callsign);
    }

    _service->Release();
    _service = nullptr;
}

string TestPerCallsignRegistrationIShell::Information() const
{
    return _T("Plugin used to test per-callsign IShell notification registration.");
}

void TestPerCallsignRegistrationIShell::Activated(
    const string& callsign,
    PluginHost::IShell*)
{
    RecordNotification(callsign, _T("Activated"));
}

void TestPerCallsignRegistrationIShell::Deactivated(
    const string& callsign,
    PluginHost::IShell*)
{
    RecordNotification(callsign, _T("Deactivated"));
}

void TestPerCallsignRegistrationIShell::Unavailable(
    const string& callsign,
    PluginHost::IShell*)
{
    RecordNotification(callsign, _T("Unavailable"));
}

Core::hresult TestPerCallsignRegistrationIShell::Monitor(
    const Core::OptionalType<string>& callsign)
{
    ASSERT(_service != nullptr);

    Core::OptionalType<string> selection(callsign);
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

    _service->Register(this, selection);

    return Core::ERROR_NONE;
}

Core::hresult TestPerCallsignRegistrationIShell::StopMonitoring(
    const Core::OptionalType<string>& callsign)
{
    ASSERT(_service != nullptr);

    Core::OptionalType<string> selection(callsign);
    if ((selection.IsSet() == true) && (selection.Value().empty() == true)) {
        selection.Clear();
    }

    _adminLock.Lock();

    for (auto index = _monitoredCallsigns.begin();
         index != _monitoredCallsigns.end();
         ++index) {

        if (*index == selection) {
            _monitoredCallsigns.erase(index);
            _adminLock.Unlock();

            _service->Unregister(this, selection);
            return Core::ERROR_NONE;
        }
    }

    _adminLock.Unlock();

    return Core::ERROR_NOT_EXIST;
}

Core::hresult TestPerCallsignRegistrationIShell::ClearNotifications()
{
    _adminLock.Lock();
    _notificationCount = 0;
    _lastNotification.clear();
    _adminLock.Unlock();

    return Core::ERROR_NONE;
}

Core::hresult TestPerCallsignRegistrationIShell::NotificationCount(uint32_t& count) const
{
    _adminLock.Lock();
    count = _notificationCount;
    _adminLock.Unlock();

    return Core::ERROR_NONE;
}

Core::hresult TestPerCallsignRegistrationIShell::LastNotification(string& notification) const
{
    _adminLock.Lock();
    notification = _lastNotification;
    _adminLock.Unlock();

    return Core::ERROR_NONE;
}

void TestPerCallsignRegistrationIShell::RecordNotification(
    const string& callsign,
    const string& event)
{
    _adminLock.Lock();
    ++_notificationCount;
    _lastNotification = event + _T(":") + callsign;
    _adminLock.Unlock();
}

} // namespace Plugin
} // namespace Thunder