#include "Module.h"

#include <test_support/ThunderTestRuntime.h>

#include <gtest/gtest.h>
#include <qa_interfaces/ISmartConsumer.h>

#include <chrono>
#include <thread>

namespace Thunder {
namespace TestCore {
namespace Tests {

    namespace {

        struct ScopedSingletonDispose : public ::testing::Environment {
            void TearDown() override
            {
                Core::Singleton::Dispose();
            }
        };

        const ::testing::Environment* const kGlobalEnv = ::testing::AddGlobalTestEnvironment(new ScopedSingletonDispose());

        constexpr std::chrono::milliseconds kTimeout { 2000 };
        constexpr std::chrono::milliseconds kPollInterval { 10 };

        template <typename PREDICATE>
        bool WaitUntil(PREDICATE&& predicate)
        {
            const auto deadline = std::chrono::steady_clock::now() + kTimeout;

            while (std::chrono::steady_clock::now() < deadline) {
                if (predicate()) {
                    return true;
                }

                std::this_thread::sleep_for(kPollInterval);
            }

            return predicate();
        }

        static ThunderTestRuntime::PluginConfig ProviderConfig(const Plugin::Configuration::startmode startMode = Plugin::Configuration::startmode::DEACTIVATED)
        {
            ThunderTestRuntime::PluginConfig cfg;
            cfg.Callsign = TEST_SMART_PROVIDER_CALLSIGN;
            cfg.ClassName = TEST_SMART_PROVIDER_CLASSNAME;
            cfg.Locator = TEST_SMART_PROVIDER_LOCATOR;
            cfg.StartMode = startMode;
            return cfg;
        }

        static ThunderTestRuntime::PluginConfig ConsumerConfig(const Plugin::Configuration::startmode startMode = Plugin::Configuration::startmode::ACTIVATED)
        {
            ThunderTestRuntime::PluginConfig cfg;
            cfg.Callsign = TEST_SMART_CONSUMER_CALLSIGN;
            cfg.ClassName = TEST_SMART_CONSUMER_CLASSNAME;
            cfg.Locator = TEST_SMART_CONSUMER_LOCATOR;
            cfg.StartMode = startMode;
            return cfg;
        }

        bool CalculateReturns(QualityAssurance::ISmartConsumer* consumer, const uint32_t expected, const uint16_t expectedAddResult = 0, const uint16_t expectedSubResult = 0)
        {
            uint16_t addResult = 0;
            uint16_t subResult = 0;

            const uint32_t result = consumer->Calculate(7, 5, addResult, subResult);

            if (result != expected) {
                return false;
            }

            if (expected == Core::ERROR_NONE) {
                return (addResult == expectedAddResult && subResult == expectedSubResult);
            }

            return true;
        }

    } // namespace

    class PluginSmartInterfaceTestSuite : public ::testing::Test {
    protected:
        static ThunderTestRuntime _runtime;

        QualityAssurance::ISmartConsumer* _consumer { nullptr };
        Core::ProxyType<PluginHost::IShell> _providerShell;

        static void SetUpTestSuite()
        {
            const uint32_t result = _runtime.Initialize(
                {
                    ProviderConfig(Plugin::Configuration::startmode::DEACTIVATED),
                    ConsumerConfig(Plugin::Configuration::startmode::ACTIVATED)
                },
                TEST_SMART_PLUGIN_PATH);

            ASSERT_EQ(result, Core::ERROR_NONE)
                << "ThunderTestRuntime failed to initialise";
        }

        static void TearDownTestSuite()
        {
            _runtime.Deinitialize();
        }

        void SetUp() override
        {
            _consumer = _runtime.QueryInterfaceByCallsign<QualityAssurance::ISmartConsumer>(TEST_SMART_CONSUMER_CALLSIGN);

            ASSERT_NE(_consumer, nullptr)
                << "The active consumer must expose ISmartConsumer";

            _providerShell = _runtime.GetShell(TEST_SMART_PROVIDER_CALLSIGN);

            ASSERT_TRUE(_providerShell.IsValid());

            EnsureProviderDeactivated();
        }

        void TearDown() override
        {
            if (_consumer != nullptr) {
                _consumer->Release();
                _consumer = nullptr;
            }
        }

        void EnsureProviderActive()
        {
            if (_providerShell->State() != PluginHost::IShell::ACTIVATED) {
                ASSERT_EQ(_providerShell->Activate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
            }

            ASSERT_TRUE(WaitUntil([this] {
                return CalculateReturns(_consumer, Core::ERROR_NONE, 12, 2);
            }))
                << "Consumer did not acquire the provider interface";
        }

        void EnsureProviderDeactivated()
        {
            if (_providerShell->State() != PluginHost::IShell::DEACTIVATED) {
                ASSERT_EQ(_providerShell->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
            }

            ASSERT_TRUE(WaitUntil([this] {
                return CalculateReturns(_consumer, Core::ERROR_UNAVAILABLE);
            }))
                << "Consumer retained a usable provider interface after deactivation";
        }
    };

    ThunderTestRuntime PluginSmartInterfaceTestSuite::_runtime;

    TEST_F(PluginSmartInterfaceTestSuite, ConsumerIsUnavailableWhileProviderIsInactive)
    {
        uint16_t addResult = 0;
        uint16_t subResult = 0;

        EXPECT_EQ(_consumer->Calculate(7, 5, addResult, subResult), Core::ERROR_UNAVAILABLE);
    }

    TEST_F(PluginSmartInterfaceTestSuite, ConsumerForwardsCallsAfterProviderActivation)
    {
        EnsureProviderActive();

        uint16_t addResult = 0;
        uint16_t subResult = 0;

        EXPECT_EQ(_consumer->Calculate(7, 5, addResult, subResult), Core::ERROR_NONE);

        EXPECT_EQ(addResult, 12);
        EXPECT_EQ(subResult, 2);
    }

    TEST_F(PluginSmartInterfaceTestSuite, ConsumerDropsInterfaceAfterProviderDeactivation)
    {
        EnsureProviderActive();
        EnsureProviderDeactivated();

        uint16_t addResult = 0;
        uint16_t subResult = 0;

        EXPECT_EQ(_consumer->Calculate(7, 5, addResult, subResult), Core::ERROR_UNAVAILABLE);
    }

    TEST_F(PluginSmartInterfaceTestSuite, ConsumerReacquiresInterfaceAfterProviderReactivation)
    {
        EnsureProviderActive();
        EnsureProviderDeactivated();
        EnsureProviderActive();

        uint16_t addResult = 0;
        uint16_t subResult = 0;

        EXPECT_EQ(_consumer->Calculate(8, 3, addResult, subResult), Core::ERROR_NONE);

        EXPECT_EQ(addResult, 11);
        EXPECT_EQ(subResult, 5);
    }

    TEST_F(PluginSmartInterfaceTestSuite, RepeatedProviderLifecycleDoesNotLeaveAStaleInterface)
    {
        for (uint8_t iteration = 0; iteration < 25; ++iteration) {
            EnsureProviderActive();
            EnsureProviderDeactivated();
        }
    }

    class ProviderAlreadyActiveFixture : public ::testing::Test {
    protected:
        static ThunderTestRuntime _runtime;

        Core::ProxyType<PluginHost::IShell> _providerShell;
        Core::ProxyType<PluginHost::IShell> _consumerShell;
        QualityAssurance::ISmartConsumer* _consumer { nullptr };

        static void SetUpTestSuite()
        {
            const uint32_t result = _runtime.Initialize(
                {
                    ProviderConfig(Plugin::Configuration::startmode::ACTIVATED),
                    ConsumerConfig(Plugin::Configuration::startmode::DEACTIVATED)
                },
                TEST_SMART_PLUGIN_PATH);

            ASSERT_EQ(result, Core::ERROR_NONE)
                << "ThunderTestRuntime failed to initialise";
        }

        static void TearDownTestSuite()
        {
            _runtime.Deinitialize();
        }

        void SetUp() override
        {
            _providerShell = _runtime.GetShell(TEST_SMART_PROVIDER_CALLSIGN);

            ASSERT_TRUE(_providerShell.IsValid());

            _consumerShell = _runtime.GetShell(TEST_SMART_CONSUMER_CALLSIGN);

            ASSERT_TRUE(_consumerShell.IsValid());

            // The Provider is already active before the Consumer starts.
            ASSERT_TRUE(WaitUntil([this] {
                return _providerShell->State() == PluginHost::IShell::ACTIVATED;
            }))
                << "Provider did not become active";

            // Start the Consumer after the Provider is already active.
            ASSERT_EQ(_consumerShell->Activate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);

            _consumer = _runtime.QueryInterfaceByCallsign<QualityAssurance::ISmartConsumer>(TEST_SMART_CONSUMER_CALLSIGN);

            ASSERT_NE(_consumer, nullptr)
                << "The active consumer must expose ISmartConsumer";
        }

        void TearDown() override
        {
            if (_consumer != nullptr) {
                _consumer->Release();
                _consumer = nullptr;
            }
        }
    };

    ThunderTestRuntime ProviderAlreadyActiveFixture::_runtime;

    TEST_F(ProviderAlreadyActiveFixture, ConsumerFindsProviderThatWasAlreadyActive)
    {
        ASSERT_TRUE(WaitUntil([this] {
            return CalculateReturns(_consumer, Core::ERROR_NONE, 12, 2);
        }))
            << "Consumer did not acquire the already-active provider interface";

        uint16_t addResult = 0;
        uint16_t subResult = 0;

        EXPECT_EQ(_consumer->Calculate(9, 4, addResult, subResult), Core::ERROR_NONE);

        EXPECT_EQ(addResult, 13);
        EXPECT_EQ(subResult, 5);
    }

} // namespace Tests
} // namespace TestCore
} // namespace Thunder