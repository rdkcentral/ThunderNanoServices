#include "../Module.h"

#include <gtest/gtest.h>

#include <interfaces/IVolumeControl.h>
#include <test_support/ThunderTestRuntime.h>

namespace Thunder {
namespace Plugin {
namespace Tests {

    namespace {
        constexpr const char* Callsign = VOLUMECONTROL_TEST_CALLSIGN;
        constexpr const char* ClassName = VOLUMECONTROL_TEST_CLASSNAME;
        constexpr const char* Locator = VOLUMECONTROL_TEST_LOCATOR;
        constexpr const char* PluginPath = VOLUMECONTROL_TEST_PLUGIN_PATH;
    }

    class VolumeControlTest : public ::testing::Test {
    protected:
        static TestCore::ThunderTestRuntime _runtime;

        static void SetUpTestSuite()
        {
            TestCore::ThunderTestRuntime::PluginConfig plugin;
            plugin.Callsign = Callsign;
            plugin.ClassName = ClassName;
            plugin.Locator = Locator;
            plugin.StartMode = PluginHost::IShell::startmode::ACTIVATED;

            std::vector<TestCore::ThunderTestRuntime::PluginConfig> plugins{ plugin };

            const uint32_t result = _runtime.Initialize(plugins, PluginPath);
            ASSERT_EQ(result, Core::ERROR_NONE) << "Failed to initialize Thunder runtime for " << Callsign;
        }

        static void TearDownTestSuite()
        {
            _runtime.Deinitialize();
        }
    };

    TestCore::ThunderTestRuntime VolumeControlTest::_runtime;

    TEST_F(VolumeControlTest, VersionsAvailable)
    {
        string response;
        const uint32_t result = _runtime.Invoke("VolumeControl.versions", "{}", response);

        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_NE(response.find("JVolumeControl"), string::npos);
    }

    TEST_F(VolumeControlTest, ExistsReportsRegisteredProperties)
    {
        string mutedResponse;
        const uint32_t mutedResult = _runtime.Invoke("VolumeControl.exists", R"({"method":"muted"})", mutedResponse);

        EXPECT_EQ(mutedResult, Core::ERROR_NONE);
        EXPECT_EQ(mutedResponse, "true");

        string volumeResponse;
        const uint32_t volumeResult = _runtime.Invoke("VolumeControl.exists", R"({"method":"volume"})", volumeResponse);

        EXPECT_EQ(volumeResult, Core::ERROR_NONE);
        EXPECT_EQ(volumeResponse, "true");
    }

    TEST_F(VolumeControlTest, JsonRpcLinkCanAccessVersions)
    {
        auto link = _runtime.CreateJSONRPCLink(Callsign);
        ASSERT_TRUE(link.IsValid());

        string response;
        const uint32_t result = link->Invoke("versions", "{}", response);

        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_NE(response.find("JVolumeControl"), string::npos);
    }

    TEST_F(VolumeControlTest, JsonRpcPropertyGettersReturnStubDefaults)
    {
        string mutedResponse;
        const uint32_t mutedResult = _runtime.Invoke("VolumeControl.muted", string(), mutedResponse);

        EXPECT_EQ(mutedResult, Core::ERROR_NONE);
        EXPECT_EQ(mutedResponse, "false");

        string volumeResponse;
        const uint32_t volumeResult = _runtime.Invoke("VolumeControl.volume", string(), volumeResponse);

        EXPECT_EQ(volumeResult, Core::ERROR_NONE);
        EXPECT_EQ(volumeResponse, "0");
    }

    TEST_F(VolumeControlTest, ComRpcInterfaceIsAvailable)
    {
        auto* volumeControl = _runtime.QueryInterfaceByCallsign<Exchange::IVolumeControl>(Callsign);

        ASSERT_NE(volumeControl, nullptr);

        const Exchange::IVolumeControl& reader = *volumeControl;

        bool muted = true;
        uint8_t volume = 255;

        EXPECT_EQ(reader.Muted(muted), Core::ERROR_NONE);
        EXPECT_FALSE(muted);

        EXPECT_EQ(reader.Volume(volume), Core::ERROR_NONE);
        EXPECT_EQ(volume, 0);

        volumeControl->Release();
    }

} // namespace Tests
} // namespace Plugin
} // namespace Thunder