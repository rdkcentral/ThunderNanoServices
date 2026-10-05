#pragma once

#include "Module.h"
#include "qa_interfaces/IPerCallsignRegistrationTest.h"

#include <list>
#include <plugins/IController.h>

namespace Thunder {
namespace Plugin {

class TestPerCallsignRegistration
    : public PluginHost::IPlugin
    , public PluginHost::JSONRPC
    , public PluginHost::IPlugin::INotification
    , public Exchange::Controller::ILifeTime::INotification
    , public QualityAssurance::IPerCallsignRegistrationTest {
public:
    TestPerCallsignRegistration();
    ~TestPerCallsignRegistration() override;

    TestPerCallsignRegistration(const TestPerCallsignRegistration&) = delete;
    TestPerCallsignRegistration& operator=(const TestPerCallsignRegistration&) = delete;

    BEGIN_INTERFACE_MAP(TestPerCallsignRegistration)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        INTERFACE_ENTRY(Exchange::Controller::ILifeTime::INotification)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_ENTRY(QualityAssurance::IPerCallsignRegistrationTest)
    END_INTERFACE_MAP

    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

    // IShell lifecycle notifications
    void Activated(const string& callsign, PluginHost::IShell* plugin) override;
    void Deactivated(const string& callsign, PluginHost::IShell* plugin) override;
    void Unavailable(const string& callsign, PluginHost::IShell* plugin) override;

    // IController lifetime notifications
    void StateChange(
        const string& callsign,
        const PluginHost::IShell::state& state,
        const PluginHost::IShell::reason& reason) override;
    void StateControlStateChange(
        const string& callsign,
        const Exchange::Controller::ILifeTime::state& state) override;

    Core::hresult MonitorShell(const Core::OptionalType<string>& callsign) override;
    Core::hresult StopMonitoringShell(const Core::OptionalType<string>& callsign) override;
    Core::hresult ClearShellNotifications() override;
    Core::hresult ShellNotificationCount(uint32_t& count) const override;
    Core::hresult LastShellNotification(string& notification) const override;

    Core::hresult MonitorController(const Core::OptionalType<string>& callsign) override;
    Core::hresult StopMonitoringController(const Core::OptionalType<string>& callsign) override;
    Core::hresult ClearControllerNotifications() override;
    Core::hresult ControllerNotificationCount(uint32_t& count) const override;
    Core::hresult LastControllerNotification(string& notification) const override;

private:
    void RecordShellNotification(const string& callsign, const string& event);
    void RecordControllerNotification(const string& notification);

    static string StateToString(PluginHost::IShell::state state);
    static string ReasonToString(PluginHost::IShell::reason reason);
    static string StateControlStateToString(Exchange::Controller::ILifeTime::state state);

private:
    PluginHost::IShell* _service;
    Exchange::Controller::ILifeTime* _controller;
    mutable Core::CriticalSection _adminLock;
    std::list<Core::OptionalType<string>> _shellCallsigns;
    std::list<Core::OptionalType<string>> _controllerCallsigns;
    uint32_t _shellNotificationCount;
    string _lastShellNotification;
    uint32_t _controllerNotificationCount;
    string _lastControllerNotification;
};

} // namespace Plugin
} // namespace Thunder