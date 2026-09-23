#pragma once

#include "Module.h"
#include "qa_interfaces/IPerCallsignRegistrationIShellTest.h"

#include <list>

namespace Thunder {
namespace Plugin {

class TestPerCallsignRegistrationIShell
    : public PluginHost::IPlugin
    , public PluginHost::JSONRPC
    , public PluginHost::IPlugin::INotification
    , public QualityAssurance::IPerCallsignRegistrationIShellTest {
public:
    TestPerCallsignRegistrationIShell();
    ~TestPerCallsignRegistrationIShell() override;

    TestPerCallsignRegistrationIShell(
        const TestPerCallsignRegistrationIShell&) = delete;

    TestPerCallsignRegistrationIShell& operator=(
        const TestPerCallsignRegistrationIShell&) = delete;

    BEGIN_INTERFACE_MAP(TestPerCallsignRegistrationIShell)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_ENTRY(QualityAssurance::IPerCallsignRegistrationIShellTest)
    END_INTERFACE_MAP

    // IPlugin
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

    // IPlugin::INotification
    void Activated(
        const string& callsign,
        PluginHost::IShell* plugin) override;

    void Deactivated(
        const string& callsign,
        PluginHost::IShell* plugin) override;

    void Unavailable(
        const string& callsign,
        PluginHost::IShell* plugin) override;

    Core::hresult Monitor(
        const Core::OptionalType<string>& callsign) override;

    Core::hresult StopMonitoring(
        const Core::OptionalType<string>& callsign) override;

    Core::hresult ClearNotifications() override;

    Core::hresult NotificationCount(uint32_t& count) const override;

    Core::hresult LastNotification(string& notification) const override;

private:
    void RecordNotification(
        const string& callsign,
        const string& event);

private:
    PluginHost::IShell* _service;

    mutable Core::CriticalSection _adminLock;

    std::list<Core::OptionalType<string>> _monitoredCallsigns;

    uint32_t _notificationCount;
    string _lastNotification;
};

} // namespace Plugin
} // namespace Thunder