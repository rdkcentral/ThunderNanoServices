#include "Module.h"

#include <test_support/ThunderTestRuntime.h>

#include <gtest/gtest.h>
#include <interfaces/IMath.h>

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

const ::testing::Environment* const kGlobalEnv =
    ::testing::AddGlobalTestEnvironment(new ScopedSingletonDispose());

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

static ThunderTestRuntime::PluginConfig ProviderConfig()
{
    ThunderTestRuntime::PluginConfig cfg;
    cfg.Callsign = TEST_SMART_PROVIDER_CALLSIGN;
    cfg.ClassName = TEST_SMART_PROVIDER_CLASSNAME;
    cfg.Locator = TEST_SMART_PROVIDER_LOCATOR;
    cfg.StartMode = Thunder::Plugin::Configuration::startmode::DEACTIVATED;
    return cfg;
}

static ThunderTestRuntime::PluginConfig ConsumerConfig()
{
    ThunderTestRuntime::PluginConfig cfg;
    cfg.Callsign = TEST_SMART_CONSUMER_CALLSIGN;
    cfg.ClassName = TEST_SMART_CONSUMER_CLASSNAME;
    cfg.Locator = TEST_SMART_CONSUMER_LOCATOR;
    cfg.StartMode = Thunder::Plugin::Configuration::startmode::ACTIVATED;
    return cfg;
}

bool AddReturns(Exchange::IMath* math, const uint32_t expected)
{
    uint16_t sum = 0;
    return (math->Add(7, 5, sum) == expected);
}

} // namespace

class PluginSmartInterfaceTestSuite : public ::testing::Test {
protected:
    static ThunderTestRuntime _runtime;

    Exchange::IMath* _consumer { nullptr };
    Core::ProxyType<PluginHost::IShell> _providerShell;

    static void SetUpTestSuite()
    {
        const uint32_t result = _runtime.Initialize(
            {
                ProviderConfig(),
                ConsumerConfig()
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
        _consumer = _runtime.QueryInterfaceByCallsign<Exchange::IMath>(
            TEST_SMART_CONSUMER_CALLSIGN);
        ASSERT_NE(_consumer, nullptr)
            << "The active consumer must expose IMath";

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
            return AddReturns(_consumer, Core::ERROR_NONE);
        })) << "Consumer did not acquire the provider interface";
    }

    void EnsureProviderDeactivated()
    {
        if (_providerShell->State() != PluginHost::IShell::DEACTIVATED) {
            ASSERT_EQ(_providerShell->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
        }
        ASSERT_TRUE(WaitUntil([this] {
            return AddReturns(_consumer, Core::ERROR_UNAVAILABLE);
        })) << "Consumer retained a usable provider interface after deactivation";
    }
};

ThunderTestRuntime PluginSmartInterfaceTestSuite::_runtime;

TEST_F(PluginSmartInterfaceTestSuite, ConsumerIsUnavailableWhileProviderIsInactive)
{
    uint16_t sum = 0;
    EXPECT_EQ(_consumer->Add(7, 5, sum), Core::ERROR_UNAVAILABLE);
}

TEST_F(PluginSmartInterfaceTestSuite, ConsumerForwardsCallsAfterProviderActivation)
{
    EnsureProviderActive();

    uint16_t sum = 0;
    EXPECT_EQ(_consumer->Add(7, 5, sum), Core::ERROR_NONE);
    EXPECT_EQ(sum, 12);
}

TEST_F(PluginSmartInterfaceTestSuite, ConsumerDropsInterfaceAfterProviderDeactivation)
{
    EnsureProviderActive();
    EnsureProviderDeactivated();

    uint16_t sum = 0;
    EXPECT_EQ(_consumer->Add(7, 5, sum), Core::ERROR_UNAVAILABLE);
}

TEST_F(PluginSmartInterfaceTestSuite, ConsumerReacquiresInterfaceAfterProviderReactivation)
{
    EnsureProviderActive();
    EnsureProviderDeactivated();
    EnsureProviderActive();

    uint16_t sum = 0;
    EXPECT_EQ(_consumer->Add(8, 3, sum), Core::ERROR_NONE);
    EXPECT_EQ(sum, 11);
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
    Exchange::IMath* _consumer { nullptr };

    static void SetUpTestSuite()
    {
        const uint32_t result = _runtime.Initialize(
            {
                ProviderConfig(),
                ConsumerConfig()
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
        _providerShell =
            _runtime.GetShell(TEST_SMART_PROVIDER_CALLSIGN);
        ASSERT_TRUE(_providerShell.IsValid());

        _consumerShell =
            _runtime.GetShell(TEST_SMART_CONSUMER_CALLSIGN);
        ASSERT_TRUE(_consumerShell.IsValid());

        // Provider must already be active before Consumer starts.
        ASSERT_EQ(
            _providerShell->Activate(PluginHost::IShell::REQUESTED),
            Core::ERROR_NONE);

        ASSERT_TRUE(WaitUntil([this] {
            return _providerShell->State() ==
                PluginHost::IShell::ACTIVATED;
        }));

        // Now start the Consumer.
        ASSERT_EQ(
            _consumerShell->Activate(PluginHost::IShell::REQUESTED),
            Core::ERROR_NONE);

        _consumer =
            _runtime.QueryInterfaceByCallsign<Exchange::IMath>(
                TEST_SMART_CONSUMER_CALLSIGN);

        ASSERT_NE(_consumer, nullptr);
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
        return AddReturns(_consumer, Core::ERROR_NONE);
    }));

    uint16_t sum = 0;
    EXPECT_EQ(_consumer->Add(4, 9, sum), Core::ERROR_NONE);
    EXPECT_EQ(sum, 13);
}

} // namespace Tests
} // namespace TestCore
} // namespace Thunder
