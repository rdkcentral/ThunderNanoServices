#pragma once

#include "Module.h"
#include "qa_interfaces/IPerCallsignRegistrationIControllerTest.h"

#include <list>
#include <plugins/IStateControl.h>
#include <plugins/IController.h>

namespace Thunder {
namespace Plugin {

    class TestPerCallsignRegistrationIController : public PluginHost::IPlugin
                                                   , public PluginHost::JSONRPC
                                                   , public PluginHost::IStateControl
                                                   , public Exchange::Controller::ILifeTime::INotification
                                                   , public QualityAssurance::IPerCallsignRegistrationIControllerTest {
    public:
        TestPerCallsignRegistrationIController();
        ~TestPerCallsignRegistrationIController() override;

        TestPerCallsignRegistrationIController(const TestPerCallsignRegistrationIController&) = delete;
        TestPerCallsignRegistrationIController& operator=(const TestPerCallsignRegistrationIController&) = delete;

        BEGIN_INTERFACE_MAP(TestPerCallsignRegistrationIController)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IStateControl)
            INTERFACE_ENTRY(Exchange::Controller::ILifeTime::INotification)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(QualityAssurance::IPerCallsignRegistrationIControllerTest)
        END_INTERFACE_MAP

        // IPlugin
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        // IStateControl
        Core::hresult Configure(PluginHost::IShell* framework) override;
        PluginHost::IStateControl::state State() const override;
        Core::hresult Request(const PluginHost::IStateControl::command command) override;
        void Register(PluginHost::IStateControl::INotification* notification) override;
        void Unregister(PluginHost::IStateControl::INotification* notification) override;

        // IController::ILifeTime::INotification
        void StateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason) override;
        void StateControlStateChange(const string& callsign, const Exchange::Controller::ILifeTime::state& state) override;

        // IPerCallsignRegistrationIControllerTest
        Core::hresult Monitor(const Core::OptionalType<string>& callsign) override;
        Core::hresult StopMonitoring(const Core::OptionalType<string>& callsign) override;
        Core::hresult ClearNotifications() override;
        Core::hresult NotificationCount(uint32_t& count) const override;
        Core::hresult LastNotification(string& notification) const override;

    private:
        void RecordNotification(const string& notification);

        static string StateToString(const PluginHost::IShell::state state);
        static string ReasonToString(const PluginHost::IShell::reason reason);
        static string StateControlStateToString(const Exchange::Controller::ILifeTime::state state);

    private:
        PluginHost::IShell* _service;
        Exchange::Controller::ILifeTime* _controller;
        mutable Core::CriticalSection _adminLock;
        std::list<Core::OptionalType<string>> _monitoredCallsigns;
        std::list<PluginHost::IStateControl::INotification*> _stateObservers;
        PluginHost::IStateControl::state _state;
        uint32_t _notificationCount;
        string _lastNotification;
    };

} // namespace Plugin
} // namespace Thunder