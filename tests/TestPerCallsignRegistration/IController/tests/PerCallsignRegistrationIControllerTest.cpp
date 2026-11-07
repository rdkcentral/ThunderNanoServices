#include "Module.h"

#include <qa_interfaces/IPerCallsignRegistrationIControllerTest.h>
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
            config.ClassName = PERCALLSIGNREGISTRATION_ICONTROLLER_TEST_CLASSNAME;
            config.Locator = PERCALLSIGNREGISTRATION_ICONTROLLER_TEST_LOCATOR;
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

        void EnsureResumed(PluginHost::IStateControl& control)
        {
            if (control.State() != PluginHost::IStateControl::RESUMED) {
                ASSERT_EQ(control.Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
            }
        }

    } // namespace

    class PerCallsignRegistrationIControllerTest : public ::testing::Test {
    protected:
        static ThunderTestRuntime _runtime;

        QualityAssurance::IPerCallsignRegistrationIControllerTest* _observer { nullptr };

        static void SetUpTestSuite()
        {
            const uint32_t result = _runtime.Initialize(
                {
                    CreatePluginConfig(ObserverCallsign),
                    CreatePluginConfig(PluginACallsign),
                    CreatePluginConfig(PluginBCallsign)
                },
                PERCALLSIGNREGISTRATION_ICONTROLLER_TEST_PLUGIN_PATH);

            ASSERT_EQ(result, Core::ERROR_NONE);
        }

        static void TearDownTestSuite()
        {
            _runtime.Deinitialize();
            Core::Singleton::Dispose();
        }

        void SetUp() override
        {
            _observer = _runtime.QueryInterfaceByCallsign<QualityAssurance::IPerCallsignRegistrationIControllerTest>(ObserverCallsign);

            ASSERT_NE(_observer, nullptr);

            auto pluginA = _runtime.GetShell(PluginACallsign);
            auto pluginB = _runtime.GetShell(PluginBCallsign);

            ASSERT_TRUE(pluginA.IsValid());
            ASSERT_TRUE(pluginB.IsValid());

            EnsureActivated(*pluginA);
            EnsureActivated(*pluginB);

            auto stateControlA = _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(PluginACallsign);
            ASSERT_NE(stateControlA, nullptr);
            EnsureResumed(*stateControlA);
            stateControlA->Release();

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

    ThunderTestRuntime PerCallsignRegistrationIControllerTest::_runtime;

    TEST_F(PerCallsignRegistrationIControllerTest, CallsignRegistrationSendsActivePluginSnapshot)
    {
        const Core::OptionalType<string> pluginA(PluginACallsign);

        uint32_t count = 0;
        string notification;

        ASSERT_EQ(_observer->Monitor(pluginA), Core::ERROR_NONE);
        ASSERT_EQ(_observer->NotificationCount(count), Core::ERROR_NONE);
        ASSERT_EQ(_observer->LastNotification(notification), Core::ERROR_NONE);

        EXPECT_EQ(count, 2u);
        EXPECT_EQ(notification, _T("StateControlStateChange:PluginA:RESUMED"));
    }

    TEST_F(PerCallsignRegistrationIControllerTest, DuplicateCallsignRegistrationIsRejected)
    {
        const Core::OptionalType<string> pluginA(PluginACallsign);

        ASSERT_EQ(_observer->Monitor(pluginA), Core::ERROR_NONE);

        EXPECT_EQ(_observer->Monitor(pluginA), Core::ERROR_ALREADY_CONNECTED);
    }

    TEST_F(PerCallsignRegistrationIControllerTest, CallsignRegistrationFiltersOtherPluginLifecycleChanges)
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
        EXPECT_EQ(notification, _T("StateChange:PluginA:DEACTIVATED:REQUESTED"));
    }

    TEST_F(PerCallsignRegistrationIControllerTest, UnregisterStopsCallsignNotifications)
    {
        const Core::OptionalType<string> pluginA(PluginACallsign);

        uint32_t count = 0;

        ASSERT_EQ(_observer->Monitor(pluginA), Core::ERROR_NONE);
        ASSERT_EQ(_observer->ClearNotifications(), Core::ERROR_NONE);
        ASSERT_EQ(_observer->StopMonitoring(pluginA), Core::ERROR_NONE);

        auto pluginAService = _runtime.GetShell(PluginACallsign);

        ASSERT_TRUE(pluginAService.IsValid());

        EnsureActivated(*pluginAService);

        ASSERT_EQ(pluginAService->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
        ASSERT_EQ(_observer->NotificationCount(count), Core::ERROR_NONE);

        EXPECT_EQ(count, 0u);
    }

    TEST_F(PerCallsignRegistrationIControllerTest, AllPluginRegistrationReceivesOtherPluginChanges)
    {
        Core::OptionalType<string> allPlugins;

        uint32_t count = 0;
        string notification;

        ASSERT_EQ(_observer->Monitor(allPlugins), Core::ERROR_NONE);

        /*
         * Registration for all plugins may produce the current-state
         * snapshot. We clear it so that only the subsequent state
         * transition is measured.
         */
        ASSERT_EQ(_observer->ClearNotifications(), Core::ERROR_NONE);

        auto pluginB = _runtime.GetShell(PluginBCallsign);

        ASSERT_TRUE(pluginB.IsValid());

        ASSERT_EQ(pluginB->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
        ASSERT_EQ(_observer->NotificationCount(count), Core::ERROR_NONE);
        ASSERT_EQ(_observer->LastNotification(notification), Core::ERROR_NONE);

        EXPECT_EQ(count, 1u);
        EXPECT_EQ(notification, _T("StateChange:PluginB:DEACTIVATED:REQUESTED"));
    }

    TEST_F(PerCallsignRegistrationIControllerTest, CallsignRegistrationFiltersStateControlChanges)
    {
        const Core::OptionalType<string> pluginA(PluginACallsign);
        uint32_t count = 0;
        string notification;

        ASSERT_EQ(_observer->Monitor(pluginA), Core::ERROR_NONE);
        ASSERT_EQ(_observer->ClearNotifications(), Core::ERROR_NONE);

        auto stateControlB = _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(PluginBCallsign);
        ASSERT_NE(stateControlB, nullptr);
        ASSERT_EQ(stateControlB->Request(PluginHost::IStateControl::RESUME), Core::ERROR_NONE);
        stateControlB->Release();

        ASSERT_EQ(_observer->NotificationCount(count), Core::ERROR_NONE);
        EXPECT_EQ(count, 0u);

        auto stateControlA = _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>(PluginACallsign);
        ASSERT_NE(stateControlA, nullptr);
        ASSERT_EQ(stateControlA->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
        stateControlA->Release();

        ASSERT_EQ(_observer->NotificationCount(count), Core::ERROR_NONE);
        ASSERT_EQ(_observer->LastNotification(notification), Core::ERROR_NONE);
        EXPECT_EQ(count, 1u);
        EXPECT_EQ(notification, _T("StateControlStateChange:PluginA:SUSPENDED"));
    }

} // namespace Tests
} // namespace TestCore
} // namespace Thunder