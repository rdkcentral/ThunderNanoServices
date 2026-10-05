#include "Module.h"

#include <qa_interfaces/IPerCallsignRegistrationTest.h>
#include <test_support/ThunderTestRuntime.h>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace Thunder {
namespace TestCore {
namespace Tests {

namespace {

constexpr const TCHAR* ObserverCallsign = _T("Observer");
constexpr const TCHAR* PluginACallsign = _T("PluginA");
constexpr const TCHAR* PluginBCallsign = _T("PluginB");
constexpr const TCHAR* StateControlACallsign = _T("StateControlA");
constexpr const TCHAR* StateControlBCallsign = _T("StateControlB");

Plugin::Config CreatePluginConfig(const string& callsign)
{
    Plugin::Config config;
    config.Callsign = callsign;
    config.ClassName = PERCALLSIGNREGISTRATION_TEST_CLASSNAME;
    config.Locator = PERCALLSIGNREGISTRATION_TEST_LOCATOR;
    config.StartMode = Plugin::Configuration::startmode::ACTIVATED;
    config.Resumed = false;
    return config;
}

Plugin::Config CreateStateControlConfig(const string& callsign)
{
    Plugin::Config config;
    config.Callsign = callsign;
    config.ClassName = PERCALLSIGNREGISTRATION_STATECONTROL_CLASSNAME;
    config.Locator = PERCALLSIGNREGISTRATION_STATECONTROL_LOCATOR;
    config.StartMode = Plugin::Configuration::startmode::ACTIVATED;
    config.Resumed = false;
    return config;
}

void EnsureActivated(PluginHost::IShell& shell)
{
    if (shell.State() != PluginHost::IShell::ACTIVATED) {
        ASSERT_EQ(shell.Activate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
    }
}

bool EnsureResumed(PluginHost::IStateControl& stateControl)
{
    if (stateControl.State() != PluginHost::IStateControl::RESUMED) {
        if (stateControl.Request(PluginHost::IStateControl::RESUME) != Core::ERROR_NONE) {
            return false;
        }

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while ((stateControl.State() != PluginHost::IStateControl::RESUMED) &&
               (std::chrono::steady_clock::now() < deadline)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    return (stateControl.State() == PluginHost::IStateControl::RESUMED);
}

bool EnsureState(
    PluginHost::IStateControl& stateControl,
    const PluginHost::IStateControl::state expected)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while ((stateControl.State() != expected) &&
           (std::chrono::steady_clock::now() < deadline)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return (stateControl.State() == expected);
}

bool WaitForControllerCount(
    QualityAssurance::IPerCallsignRegistrationTest& observer,
    const uint32_t expected)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    uint32_t count = 0;

    while (std::chrono::steady_clock::now() < deadline) {
        if ((observer.ControllerNotificationCount(count) == Core::ERROR_NONE) &&
            (count >= expected)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return ((observer.ControllerNotificationCount(count) == Core::ERROR_NONE) &&
            (count >= expected));
}

} // namespace

class PerCallsignRegistrationTest : public ::testing::Test {
protected:
    static ThunderTestRuntime _runtime;

    QualityAssurance::IPerCallsignRegistrationTest* _observer { nullptr };

    static void SetUpTestSuite()
    {
        const uint32_t result = _runtime.Initialize(
            {
                CreatePluginConfig(ObserverCallsign),
                CreatePluginConfig(PluginACallsign),
                CreatePluginConfig(PluginBCallsign),
                CreateStateControlConfig(StateControlACallsign),
                CreateStateControlConfig(StateControlBCallsign)
            },
            PERCALLSIGNREGISTRATION_TEST_PLUGIN_PATH);

        ASSERT_EQ(result, Core::ERROR_NONE);
    }

    static void TearDownTestSuite()
    {
        _runtime.Deinitialize();
        Core::Singleton::Dispose();
    }

    void SetUp() override
    {
        _observer = _runtime.QueryInterfaceByCallsign<QualityAssurance::IPerCallsignRegistrationTest>(ObserverCallsign);
        ASSERT_NE(_observer, nullptr);

        auto pluginA = _runtime.GetShell(PluginACallsign);
        auto pluginB = _runtime.GetShell(PluginBCallsign);
        ASSERT_TRUE(pluginA.IsValid());
        ASSERT_TRUE(pluginB.IsValid());
        EnsureActivated(*pluginA);
        EnsureActivated(*pluginB);

        auto stateControlA =
            _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(StateControlACallsign);
        auto stateControlB =
            _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(StateControlBCallsign);
        ASSERT_NE(stateControlA, nullptr);
        ASSERT_NE(stateControlB, nullptr);
        ASSERT_TRUE(EnsureResumed(*stateControlA));
        ASSERT_TRUE(EnsureResumed(*stateControlB));
        stateControlA->Release();
        stateControlB->Release();

        ASSERT_EQ(_observer->ClearShellNotifications(), Core::ERROR_NONE);
        ASSERT_EQ(_observer->ClearControllerNotifications(), Core::ERROR_NONE);
    }

    void TearDown() override
    {
        if (_observer != nullptr) {
            const Core::OptionalType<string> pluginA(PluginACallsign);
            const Core::OptionalType<string> pluginB(PluginBCallsign);
            const Core::OptionalType<string> stateControlA(StateControlACallsign);
            const Core::OptionalType<string> stateControlB(StateControlBCallsign);
            Core::OptionalType<string> allPlugins;

            _observer->StopMonitoringShell(pluginA);
            _observer->StopMonitoringShell(pluginB);
            _observer->StopMonitoringShell(allPlugins);
            _observer->StopMonitoringController(pluginA);
            _observer->StopMonitoringController(pluginB);
            _observer->StopMonitoringController(stateControlA);
            _observer->StopMonitoringController(stateControlB);
            _observer->StopMonitoringController(allPlugins);

            _observer->Release();
            _observer = nullptr;
        }
    }
};

ThunderTestRuntime PerCallsignRegistrationTest::_runtime;

TEST_F(PerCallsignRegistrationTest, ShellRegistrationFiltersByCallsign)
{
    const Core::OptionalType<string> pluginA(PluginACallsign);
    uint32_t count = 0;
    string notification;

    ASSERT_EQ(_observer->MonitorShell(pluginA), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ShellNotificationCount(count), Core::ERROR_NONE);
    EXPECT_EQ(count, 1u);
    ASSERT_EQ(_observer->LastShellNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(notification, _T("Activated:PluginA"));

    ASSERT_EQ(_observer->ClearShellNotifications(), Core::ERROR_NONE);

    auto pluginBService = _runtime.GetShell(PluginBCallsign);
    ASSERT_TRUE(pluginBService.IsValid());
    ASSERT_EQ(pluginBService->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ShellNotificationCount(count), Core::ERROR_NONE);
    EXPECT_EQ(count, 0u);

    auto pluginAService = _runtime.GetShell(PluginACallsign);
    ASSERT_TRUE(pluginAService.IsValid());
    ASSERT_EQ(pluginAService->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);

    ASSERT_EQ(_observer->ShellNotificationCount(count), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastShellNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(notification, _T("Deactivated:PluginA"));
}

TEST_F(PerCallsignRegistrationTest, ControllerRegistrationFiltersByCallsign)
{
    const Core::OptionalType<string> pluginA(PluginACallsign);
    uint32_t count = 0;
    string notification;

    ASSERT_EQ(_observer->MonitorController(pluginA), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ControllerNotificationCount(count), Core::ERROR_NONE);
    EXPECT_EQ(count, 1u);
    ASSERT_EQ(_observer->LastControllerNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(notification, _T("StateChange:PluginA:ACTIVATED:REQUESTED"));

    ASSERT_EQ(_observer->ClearControllerNotifications(), Core::ERROR_NONE);

    auto pluginBService = _runtime.GetShell(PluginBCallsign);
    ASSERT_TRUE(pluginBService.IsValid());
    ASSERT_EQ(pluginBService->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ControllerNotificationCount(count), Core::ERROR_NONE);
    EXPECT_EQ(count, 0u);

    auto pluginAService = _runtime.GetShell(PluginACallsign);
    ASSERT_TRUE(pluginAService.IsValid());
    ASSERT_EQ(pluginAService->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);

    ASSERT_EQ(_observer->ControllerNotificationCount(count), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastControllerNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(notification, _T("StateChange:PluginA:DEACTIVATED:REQUESTED"));
}

TEST_F(PerCallsignRegistrationTest, ShellApiRejectsDuplicateAndMissingUnregister)
{
    const Core::OptionalType<string> pluginA(PluginACallsign);

    ASSERT_EQ(_observer->MonitorShell(pluginA), Core::ERROR_NONE);
    EXPECT_EQ(_observer->MonitorShell(pluginA), Core::ERROR_ALREADY_CONNECTED);
    EXPECT_EQ(_observer->StopMonitoringShell(pluginA), Core::ERROR_NONE);
    EXPECT_EQ(_observer->StopMonitoringShell(pluginA), Core::ERROR_NOT_EXIST);
}

TEST_F(PerCallsignRegistrationTest, ControllerApiRejectsDuplicateAndMissingUnregister)
{
    const Core::OptionalType<string> pluginA(PluginACallsign);

    ASSERT_EQ(_observer->MonitorController(pluginA), Core::ERROR_NONE);
    EXPECT_EQ(_observer->MonitorController(pluginA), Core::ERROR_ALREADY_CONNECTED);
    EXPECT_EQ(_observer->StopMonitoringController(pluginA), Core::ERROR_NONE);
    EXPECT_EQ(_observer->StopMonitoringController(pluginA), Core::ERROR_NOT_EXIST);
}

TEST_F(PerCallsignRegistrationTest, ClearMethodsResetTheirOwnResults)
{
    const Core::OptionalType<string> pluginA(PluginACallsign);
    uint32_t shellCount = 0;
    uint32_t controllerCount = 0;
    string notification;

    ASSERT_EQ(_observer->MonitorShell(pluginA), Core::ERROR_NONE);
    ASSERT_EQ(_observer->MonitorController(pluginA), Core::ERROR_NONE);

    ASSERT_EQ(_observer->ShellNotificationCount(shellCount), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ControllerNotificationCount(controllerCount), Core::ERROR_NONE);
    EXPECT_EQ(shellCount, 1u);
    EXPECT_EQ(controllerCount, 1u);

    ASSERT_EQ(_observer->ClearShellNotifications(), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ShellNotificationCount(shellCount), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastShellNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(shellCount, 0u);
    EXPECT_TRUE(notification.empty());

    ASSERT_EQ(_observer->ControllerNotificationCount(controllerCount), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastControllerNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(controllerCount, 1u);
    EXPECT_EQ(notification, _T("StateChange:PluginA:ACTIVATED:REQUESTED"));

    ASSERT_EQ(_observer->ClearControllerNotifications(), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ControllerNotificationCount(controllerCount), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastControllerNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(controllerCount, 0u);
    EXPECT_TRUE(notification.empty());
}

TEST_F(PerCallsignRegistrationTest, GlobalRegistrationsReceiveOtherCallsigns)
{
    Core::OptionalType<string> allPlugins;
    uint32_t shellCount = 0;
    uint32_t controllerCount = 0;
    string notification;

    ASSERT_EQ(_observer->MonitorShell(allPlugins), Core::ERROR_NONE);
    ASSERT_EQ(_observer->MonitorController(allPlugins), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ClearShellNotifications(), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ClearControllerNotifications(), Core::ERROR_NONE);

    auto pluginBService = _runtime.GetShell(PluginBCallsign);
    ASSERT_TRUE(pluginBService.IsValid());
    ASSERT_EQ(pluginBService->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);

    ASSERT_EQ(_observer->ShellNotificationCount(shellCount), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastShellNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(shellCount, 1u);
    EXPECT_EQ(notification, _T("Deactivated:PluginB"));

    ASSERT_EQ(_observer->ControllerNotificationCount(controllerCount), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastControllerNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(controllerCount, 1u);
    EXPECT_EQ(notification, _T("StateChange:PluginB:DEACTIVATED:REQUESTED"));
}

TEST_F(PerCallsignRegistrationTest, ControllerRegistrationFiltersStateControlChanges)
{
    const Core::OptionalType<string> stateControlASelection(StateControlACallsign);
    uint32_t count = 0;
    string notification;

    ASSERT_EQ(_observer->MonitorController(stateControlASelection), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ControllerNotificationCount(count), Core::ERROR_NONE);
    EXPECT_EQ(count, 2u);
    ASSERT_EQ(_observer->LastControllerNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(notification, _T("StateControlStateChange:StateControlA:RESUMED"));

    ASSERT_EQ(_observer->ClearControllerNotifications(), Core::ERROR_NONE);

    auto stateControlB =
        _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(StateControlBCallsign);
    ASSERT_NE(stateControlB, nullptr);
    ASSERT_EQ(
        stateControlB->Request(PluginHost::IStateControl::SUSPEND),
        Core::ERROR_NONE);
    ASSERT_TRUE(EnsureState(*stateControlB, PluginHost::IStateControl::SUSPENDED));
    stateControlB->Release();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(_observer->ControllerNotificationCount(count), Core::ERROR_NONE);
    EXPECT_EQ(count, 0u);

    auto stateControlA =
        _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(StateControlACallsign);
    ASSERT_NE(stateControlA, nullptr);
    ASSERT_EQ(
        stateControlA->Request(PluginHost::IStateControl::SUSPEND),
        Core::ERROR_NONE);
    ASSERT_TRUE(EnsureState(*stateControlA, PluginHost::IStateControl::SUSPENDED));
    stateControlA->Release();

    ASSERT_TRUE(WaitForControllerCount(*_observer, 1u));
    ASSERT_EQ(_observer->ControllerNotificationCount(count), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastControllerNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(notification, _T("StateControlStateChange:StateControlA:SUSPENDED"));

    stateControlA =
        _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(StateControlACallsign);
    ASSERT_NE(stateControlA, nullptr);
    ASSERT_EQ(
        stateControlA->Request(PluginHost::IStateControl::RESUME),
        Core::ERROR_NONE);
    ASSERT_TRUE(EnsureState(*stateControlA, PluginHost::IStateControl::RESUMED));
    stateControlA->Release();

    ASSERT_TRUE(WaitForControllerCount(*_observer, 2u));
    ASSERT_EQ(_observer->ControllerNotificationCount(count), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastControllerNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(count, 2u);
    EXPECT_EQ(notification, _T("StateControlStateChange:StateControlA:RESUMED"));
}

TEST_F(PerCallsignRegistrationTest, ControllerUnregisterStopsStateControlChanges)
{
    const Core::OptionalType<string> stateControlASelection(StateControlACallsign);
    uint32_t count = 0;

    ASSERT_EQ(_observer->MonitorController(stateControlASelection), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ClearControllerNotifications(), Core::ERROR_NONE);
    ASSERT_EQ(_observer->StopMonitoringController(stateControlASelection), Core::ERROR_NONE);

    auto stateControlA =
        _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(StateControlACallsign);
    ASSERT_NE(stateControlA, nullptr);
    ASSERT_EQ(
        stateControlA->Request(PluginHost::IStateControl::SUSPEND),
        Core::ERROR_NONE);
    ASSERT_TRUE(EnsureState(*stateControlA, PluginHost::IStateControl::SUSPENDED));
    stateControlA->Release();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(_observer->ControllerNotificationCount(count), Core::ERROR_NONE);
    EXPECT_EQ(count, 0u);
}

TEST_F(PerCallsignRegistrationTest, GlobalControllerRegistrationReceivesStateControlChanges)
{
    Core::OptionalType<string> allPlugins;
    uint32_t count = 0;
    string notification;

    ASSERT_EQ(_observer->MonitorController(allPlugins), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ClearControllerNotifications(), Core::ERROR_NONE);

    auto stateControlB =
        _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(StateControlBCallsign);
    ASSERT_NE(stateControlB, nullptr);
    ASSERT_EQ(
        stateControlB->Request(PluginHost::IStateControl::SUSPEND),
        Core::ERROR_NONE);
    ASSERT_TRUE(EnsureState(*stateControlB, PluginHost::IStateControl::SUSPENDED));
    stateControlB->Release();

    ASSERT_TRUE(WaitForControllerCount(*_observer, 1u));
    ASSERT_EQ(_observer->LastControllerNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(notification, _T("StateControlStateChange:StateControlB:SUSPENDED"));

    auto stateControlA =
        _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(StateControlACallsign);
    ASSERT_NE(stateControlA, nullptr);
    ASSERT_EQ(
        stateControlA->Request(PluginHost::IStateControl::SUSPEND),
        Core::ERROR_NONE);
    ASSERT_TRUE(EnsureState(*stateControlA, PluginHost::IStateControl::SUSPENDED));
    stateControlA->Release();

    ASSERT_TRUE(WaitForControllerCount(*_observer, 2u));
    ASSERT_EQ(_observer->ControllerNotificationCount(count), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastControllerNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(count, 2u);
    EXPECT_EQ(notification, _T("StateControlStateChange:StateControlA:SUSPENDED"));
}

} // namespace Tests
} // namespace TestCore
} // namespace Thunder
