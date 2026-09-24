#include "Module.h"

#include <qa_interfaces/IPerCallsignRegistrationIShellTest.h>
#include <test_support/ThunderTestRuntime.h>

#include <gtest/gtest.h>

namespace Thunder {
namespace TestCore {
namespace Tests {

namespace {

constexpr const TCHAR* ObserverCallsign = _T("Observer");
constexpr const TCHAR* PluginACallsign = _T("PluginA");
constexpr const TCHAR* PluginBCallsign = _T("PluginB");

Plugin::Config CreatePluginConfig(const string& callsign)
{
    Plugin::Config config;
    config.Callsign = callsign;
    config.ClassName = PERCALLSIGNREGISTRATION_ISHELL_TEST_CLASSNAME;
    config.Locator = PERCALLSIGNREGISTRATION_ISHELL_TEST_LOCATOR;
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

} // namespace

class PerCallsignRegistrationIShellTest : public ::testing::Test {
protected:
    static ThunderTestRuntime _runtime;

    QualityAssurance::IPerCallsignRegistrationIShellTest* _observer { nullptr };

    static void SetUpTestSuite()
    {
        const uint32_t result = _runtime.Initialize(
            {
                CreatePluginConfig(ObserverCallsign),
                CreatePluginConfig(PluginACallsign),
                CreatePluginConfig(PluginBCallsign)
            },
            PERCALLSIGNREGISTRATION_ISHELL_TEST_PLUGIN_PATH);

        ASSERT_EQ(result, Core::ERROR_NONE);
    }

    static void TearDownTestSuite()
    {
        _runtime.Deinitialize();
        Core::Singleton::Dispose();
    }

    void SetUp() override
    {
        _observer = _runtime.QueryInterfaceByCallsign<QualityAssurance::IPerCallsignRegistrationIShellTest>(ObserverCallsign);
        ASSERT_NE(_observer, nullptr);

        auto pluginA = _runtime.GetShell(PluginACallsign);
        auto pluginB = _runtime.GetShell(PluginBCallsign);
        ASSERT_TRUE(pluginA.IsValid());
        ASSERT_TRUE(pluginB.IsValid());
        EnsureActivated(*pluginA);
        EnsureActivated(*pluginB);
        ASSERT_EQ(_observer->ClearNotifications(), Core::ERROR_NONE);
    }

    void TearDown() override
    {
        if (_observer != nullptr) {
            const Core::OptionalType<string> pluginA(PluginACallsign);
            const Core::OptionalType<string> pluginB(PluginBCallsign);
            Core::OptionalType<string> allPlugins;

            _observer->StopMonitoring(pluginA);
            _observer->StopMonitoring(pluginB);
            _observer->StopMonitoring(allPlugins);
            _observer->Release();
            _observer = nullptr;
        }
    }
};

ThunderTestRuntime PerCallsignRegistrationIShellTest::_runtime;

TEST_F(PerCallsignRegistrationIShellTest, CallsignRegistrationSendsActivePluginSnapshot)
{
    const Core::OptionalType<string> pluginA(PluginACallsign);
    uint32_t count = 0;
    string notification;

    ASSERT_EQ(_observer->Monitor(pluginA), Core::ERROR_NONE);
    ASSERT_EQ(_observer->NotificationCount(count), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastNotification(notification), Core::ERROR_NONE);

    EXPECT_EQ(count, 1u);
    EXPECT_EQ(notification, _T("Activated:PluginA"));
}

TEST_F(PerCallsignRegistrationIShellTest, CallsignRegistrationFiltersOtherPluginLifecycleChanges)
{
    const Core::OptionalType<string> pluginA(PluginACallsign);
    uint32_t count = 0;
    string notification;

    ASSERT_EQ(_observer->Monitor(pluginA), Core::ERROR_NONE);
    ASSERT_EQ(_observer->ClearNotifications(), Core::ERROR_NONE);

    auto pluginB = _runtime.GetShell(PluginBCallsign);
    ASSERT_TRUE(pluginB.IsValid());
    ASSERT_EQ(pluginB->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);

    ASSERT_EQ(_observer->NotificationCount(count), Core::ERROR_NONE);
    EXPECT_EQ(count, 0u);

    auto pluginAService = _runtime.GetShell(PluginACallsign);
    ASSERT_TRUE(pluginAService.IsValid());
    ASSERT_EQ(pluginAService->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);

    ASSERT_EQ(_observer->NotificationCount(count), Core::ERROR_NONE);
    ASSERT_EQ(_observer->LastNotification(notification), Core::ERROR_NONE);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(notification, _T("Deactivated:PluginA"));
}

} // namespace Tests
} // namespace TestCore
} // namespace Thunder