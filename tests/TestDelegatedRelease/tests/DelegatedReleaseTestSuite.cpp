#include "Module.h"

#include <gtest/gtest.h>
#include <qa_interfaces/ITestDelegatedRelease.h>
#include <test_support/ThunderTestRuntime.h>

namespace Thunder {
namespace Plugin {
namespace Tests {

    namespace {
        constexpr const char* Callsign = TESTDELEGATEDRELEASE_TEST_CALLSIGN;
        constexpr const char* SecondCallsign = "TestDelegatedRelease2";
        constexpr const char* ClassName = TESTDELEGATEDRELEASE_TEST_CLASSNAME;
        constexpr const char* Locator = TESTDELEGATEDRELEASE_TEST_LOCATOR;
        constexpr const char* PluginPath = TESTDELEGATEDRELEASE_TEST_PLUGIN_PATH;
        constexpr const char* ProxyStubPath = TESTDELEGATEDRELEASE_TEST_PROXYSTUB_PATH;
        constexpr uint32_t PingValue = 0x12345678;
        constexpr uint32_t WaitTime = 5000;

        class Peer : public QualityAssurance::ITestDelegatedReleasePeer {
        public:
            explicit Peer(Core::Event& destroyed)
                : _destroyed(destroyed)
            {
            }

            ~Peer() override
            {
                _destroyed.SetEvent();
            }

            Core::hresult Ping(uint32_t& value) override
            {
                value = PingValue;
                return Core::ERROR_NONE;
            }

            BEGIN_INTERFACE_MAP(Peer)
                INTERFACE_ENTRY(QualityAssurance::ITestDelegatedReleasePeer)
            END_INTERFACE_MAP

        private:
            Core::Event& _destroyed;
        };

        TestCore::ThunderTestRuntime::PluginConfig CreatePluginConfig(const char* callsign)
        {
            TestCore::ThunderTestRuntime::PluginConfig plugin;
            plugin.Callsign = callsign;
            plugin.ClassName = ClassName;
            plugin.Locator = Locator;
            plugin.StartMode = Plugin::Configuration::startmode::ACTIVATED;
            plugin.Root.Mode = Plugin::Config::RootConfig::ModeType::LOCAL;
            return plugin;
        }
    }

    class DelegatedReleaseTestSuite : public ::testing::Test {
    protected:
        void SetUp() override
        {
            const uint32_t result = _runtime.Initialize(
                { CreatePluginConfig(Callsign), CreatePluginConfig(SecondCallsign) },
                PluginPath,
                ProxyStubPath);
            ASSERT_EQ(result, Core::ERROR_NONE);
        }

        void TearDown() override
        {
            _runtime.Deinitialize();
        }

        TestCore::ThunderTestRuntime _runtime;
    };

    TEST_F(DelegatedReleaseTestSuite, PingOverCOMRPC)
    {
        auto* interface = _runtime.QueryInterfaceByCallsign<QualityAssurance::ITestDelegatedRelease>(Callsign);
        ASSERT_NE(interface, nullptr);

        uint32_t value = 0;
        EXPECT_EQ(interface->Ping(value), Core::ERROR_NONE);
        EXPECT_EQ(value, PingValue);

        interface->Release();
    }

    TEST_F(DelegatedReleaseTestSuite, ClosingChannelReleasesRetainedPeer)
    {
        Core::Event destroyed(false, true);
        auto* peer = Core::Service<Peer>::Create<QualityAssurance::ITestDelegatedReleasePeer>(destroyed);
        auto* interface = _runtime.QueryInterfaceByCallsign<QualityAssurance::ITestDelegatedRelease>(Callsign);
        ASSERT_NE(peer, nullptr);
        ASSERT_NE(interface, nullptr);

        ASSERT_EQ(interface->HoldPeer(peer), Core::ERROR_NONE);
        interface->Release();
        EXPECT_NE(peer->Release(), Core::ERROR_DESTRUCTION_SUCCEEDED);
        EXPECT_EQ(destroyed.Lock(100), Core::ERROR_TIMEDOUT);

        Core::ProxyType<PluginHost::IShell> shell = _runtime.GetShell(Callsign);
        ASSERT_TRUE(shell.IsValid());
        ASSERT_EQ(shell->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
        EXPECT_EQ(destroyed.Lock(WaitTime), Core::ERROR_NONE);
    }

    TEST_F(DelegatedReleaseTestSuite, ClosingChannelReleasesMultiplePeerReferences)
    {
        Core::Event firstDestroyed(false, true);
        Core::Event secondDestroyed(false, true);
        Core::Event thirdDestroyed(false, true);
        auto* firstPeer = Core::Service<Peer>::Create<QualityAssurance::ITestDelegatedReleasePeer>(firstDestroyed);
        auto* secondPeer = Core::Service<Peer>::Create<QualityAssurance::ITestDelegatedReleasePeer>(secondDestroyed);
        auto* thirdPeer = Core::Service<Peer>::Create<QualityAssurance::ITestDelegatedReleasePeer>(thirdDestroyed);
        auto* interface = _runtime.QueryInterfaceByCallsign<QualityAssurance::ITestDelegatedRelease>(Callsign);
        ASSERT_NE(firstPeer, nullptr);
        ASSERT_NE(secondPeer, nullptr);
        ASSERT_NE(thirdPeer, nullptr);
        ASSERT_NE(interface, nullptr);

        ASSERT_EQ(interface->HoldPeer(firstPeer), Core::ERROR_NONE);
        ASSERT_EQ(interface->HoldPeer(secondPeer), Core::ERROR_NONE);
        ASSERT_EQ(interface->HoldPeer(thirdPeer), Core::ERROR_NONE);
        interface->Release();
        EXPECT_NE(firstPeer->Release(), Core::ERROR_DESTRUCTION_SUCCEEDED);
        EXPECT_NE(secondPeer->Release(), Core::ERROR_DESTRUCTION_SUCCEEDED);
        EXPECT_NE(thirdPeer->Release(), Core::ERROR_DESTRUCTION_SUCCEEDED);

        Core::ProxyType<PluginHost::IShell> shell = _runtime.GetShell(Callsign);
        ASSERT_TRUE(shell.IsValid());
        ASSERT_EQ(shell->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
        EXPECT_EQ(firstDestroyed.Lock(WaitTime), Core::ERROR_NONE);
        EXPECT_EQ(secondDestroyed.Lock(WaitTime), Core::ERROR_NONE);
        EXPECT_EQ(thirdDestroyed.Lock(WaitTime), Core::ERROR_NONE);
    }

    TEST_F(DelegatedReleaseTestSuite, ClosingOneChannelDoesNotReleaseAnotherChannelsPeer)
    {
        Core::Event firstDestroyed(false, true);
        Core::Event secondDestroyed(false, true);
        auto* firstPeer = Core::Service<Peer>::Create<QualityAssurance::ITestDelegatedReleasePeer>(firstDestroyed);
        auto* secondPeer = Core::Service<Peer>::Create<QualityAssurance::ITestDelegatedReleasePeer>(secondDestroyed);
        auto* firstInterface = _runtime.QueryInterfaceByCallsign<QualityAssurance::ITestDelegatedRelease>(Callsign);
        auto* secondInterface = _runtime.QueryInterfaceByCallsign<QualityAssurance::ITestDelegatedRelease>(SecondCallsign);
        ASSERT_NE(firstPeer, nullptr);
        ASSERT_NE(secondPeer, nullptr);
        ASSERT_NE(firstInterface, nullptr);
        ASSERT_NE(secondInterface, nullptr);

        ASSERT_EQ(firstInterface->HoldPeer(firstPeer), Core::ERROR_NONE);
        ASSERT_EQ(secondInterface->HoldPeer(secondPeer), Core::ERROR_NONE);
        firstInterface->Release();
        secondInterface->Release();
        EXPECT_NE(firstPeer->Release(), Core::ERROR_DESTRUCTION_SUCCEEDED);
        EXPECT_NE(secondPeer->Release(), Core::ERROR_DESTRUCTION_SUCCEEDED);

        Core::ProxyType<PluginHost::IShell> firstShell = _runtime.GetShell(Callsign);
        ASSERT_TRUE(firstShell.IsValid());
        ASSERT_EQ(firstShell->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
        EXPECT_EQ(firstDestroyed.Lock(WaitTime), Core::ERROR_NONE);
        EXPECT_EQ(secondDestroyed.Lock(100), Core::ERROR_TIMEDOUT);

        Core::ProxyType<PluginHost::IShell> secondShell = _runtime.GetShell(SecondCallsign);
        ASSERT_TRUE(secondShell.IsValid());
        ASSERT_EQ(secondShell->Deactivate(PluginHost::IShell::REQUESTED), Core::ERROR_NONE);
        EXPECT_EQ(secondDestroyed.Lock(WaitTime), Core::ERROR_NONE);
    }

} // namespace Tests
} // namespace Plugin
} // namespace Thunder