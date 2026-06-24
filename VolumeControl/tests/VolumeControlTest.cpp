/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../Module.h"

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>

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

        class NotificationSink : public Exchange::IVolumeControl::INotification {
        public:
            NotificationSink() = default;
            ~NotificationSink() override = default;

            NotificationSink(const NotificationSink&) = delete;
            NotificationSink& operator=(const NotificationSink&) = delete;

            void Volume(const uint8_t volume) override
            {
                _lastVolume = volume;
                _volumeCount++;
            }

            void Muted(const bool muted) override
            {
                _lastMuted = muted;
                _mutedCount++;
            }

            void Reset()
            {
                _lastMuted = false;
                _lastVolume = 0;
                _mutedCount = 0;
                _volumeCount = 0;
            }

            bool LastMuted() const
            {
                return (_lastMuted);
            }

            uint8_t LastVolume() const
            {
                return (_lastVolume);
            }

            uint32_t MutedCount() const
            {
                return (_mutedCount);
            }

            uint32_t VolumeCount() const
            {
                return (_volumeCount);
            }

            BEGIN_INTERFACE_MAP(NotificationSink)
                INTERFACE_ENTRY(Exchange::IVolumeControl::INotification)
            END_INTERFACE_MAP

        private:
            bool _lastMuted { false };
            uint8_t _lastVolume { 0 };
            uint32_t _mutedCount { 0 };
            uint32_t _volumeCount { 0 };
        };
    }

    class VolumeControlTest : public ::testing::Test {
    protected:
        static TestCore::ThunderTestRuntime _runtime;

        void SetUp() override
        {
            auto* volumeControl = Interface();
            ASSERT_NE(volumeControl, nullptr);

            EXPECT_EQ(volumeControl->Muted(false), Core::ERROR_NONE);
            EXPECT_EQ(volumeControl->Volume(static_cast<uint8_t>(0)), Core::ERROR_NONE);

            volumeControl->Release();
        }

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

        static Exchange::IVolumeControl* Interface()
        {
            return (_runtime.QueryInterfaceByCallsign<Exchange::IVolumeControl>(Callsign));
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

    TEST_F(VolumeControlTest, JsonRpcMutedSetThenGetMatches)
    {
        string response;
        EXPECT_EQ(_runtime.Invoke("VolumeControl.muted", "true", response), Core::ERROR_NONE);

        string mutedResponse;
        EXPECT_EQ(_runtime.Invoke("VolumeControl.muted", string(), mutedResponse), Core::ERROR_NONE);
        EXPECT_EQ(mutedResponse, "true");

        EXPECT_EQ(_runtime.Invoke("VolumeControl.muted", "false", response), Core::ERROR_NONE);
        EXPECT_EQ(_runtime.Invoke("VolumeControl.muted", string(), mutedResponse), Core::ERROR_NONE);
        EXPECT_EQ(mutedResponse, "false");
    }

    TEST_F(VolumeControlTest, JsonRpcVolumeSetThenGetMatches)
    {
        string response;
        EXPECT_EQ(_runtime.Invoke("VolumeControl.volume", "27", response), Core::ERROR_NONE);

        string volumeResponse;
        EXPECT_EQ(_runtime.Invoke("VolumeControl.volume", string(), volumeResponse), Core::ERROR_NONE);
        EXPECT_EQ(volumeResponse, "27");

        EXPECT_EQ(_runtime.Invoke("VolumeControl.volume", "81", response), Core::ERROR_NONE);
        EXPECT_EQ(_runtime.Invoke("VolumeControl.volume", string(), volumeResponse), Core::ERROR_NONE);
        EXPECT_EQ(volumeResponse, "81");
    }

    TEST_F(VolumeControlTest, ComRpcInterfaceIsAvailable)
    {
        auto* volumeControl = Interface();
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

    TEST_F(VolumeControlTest, ComRpcMutedSetThenGetMatches)
    {
        auto* volumeControl = Interface();
        ASSERT_NE(volumeControl, nullptr);

        const Exchange::IVolumeControl& reader = *volumeControl;

        EXPECT_EQ(volumeControl->Muted(true), Core::ERROR_NONE);

        bool muted = false;
        EXPECT_EQ(reader.Muted(muted), Core::ERROR_NONE);
        EXPECT_TRUE(muted);

        EXPECT_EQ(volumeControl->Muted(false), Core::ERROR_NONE);
        EXPECT_EQ(reader.Muted(muted), Core::ERROR_NONE);
        EXPECT_FALSE(muted);

        volumeControl->Release();
    }

    TEST_F(VolumeControlTest, ComRpcVolumeSetThenGetMatches)
    {
        auto* volumeControl = Interface();
        ASSERT_NE(volumeControl, nullptr);

        const Exchange::IVolumeControl& reader = *volumeControl;

        EXPECT_EQ(volumeControl->Volume(static_cast<uint8_t>(33)), Core::ERROR_NONE);

        uint8_t volume = 0;
        EXPECT_EQ(reader.Volume(volume), Core::ERROR_NONE);
        EXPECT_EQ(volume, 33);

        EXPECT_EQ(volumeControl->Volume(static_cast<uint8_t>(99)), Core::ERROR_NONE);
        EXPECT_EQ(reader.Volume(volume), Core::ERROR_NONE);
        EXPECT_EQ(volume, 99);

        volumeControl->Release();
    }

    TEST_F(VolumeControlTest, ComRpcVolumeNotificationStopsAfterUnregister)
    {
        auto* volumeControl = Interface();
        ASSERT_NE(volumeControl, nullptr);

        Core::SinkType<NotificationSink> sink;
        sink.Reset();

        volumeControl->Register(&sink);
        EXPECT_EQ(volumeControl->Volume(static_cast<uint8_t>(44)), Core::ERROR_NONE);

        EXPECT_EQ(sink.VolumeCount(), 1u);
        EXPECT_EQ(sink.LastVolume(), 44);

        volumeControl->Unregister(&sink);
        EXPECT_EQ(volumeControl->Volume(static_cast<uint8_t>(12)), Core::ERROR_NONE);

        EXPECT_EQ(sink.VolumeCount(), 1u);
        EXPECT_EQ(sink.LastVolume(), 44);

        volumeControl->Release();
    }

    TEST_F(VolumeControlTest, ComRpcMutedNotificationStopsAfterUnregister)
    {
        auto* volumeControl = Interface();
        ASSERT_NE(volumeControl, nullptr);

        Core::SinkType<NotificationSink> sink;
        sink.Reset();

        volumeControl->Register(&sink);
        EXPECT_EQ(volumeControl->Muted(true), Core::ERROR_NONE);

        EXPECT_EQ(sink.MutedCount(), 1u);
        EXPECT_TRUE(sink.LastMuted());

        volumeControl->Unregister(&sink);
        EXPECT_EQ(volumeControl->Muted(false), Core::ERROR_NONE);

        EXPECT_EQ(sink.MutedCount(), 1u);
        EXPECT_TRUE(sink.LastMuted());

        volumeControl->Release();
    }

    TEST_F(VolumeControlTest, JsonRpcVolumeNotificationStopsAfterUnsubscribe)
    {
        auto link = _runtime.CreateJSONRPCLink(Callsign);
        ASSERT_TRUE(link.IsValid());

        std::mutex mutex;
        std::condition_variable condition;
        uint32_t count = 0;
        string lastParams;

        const uint32_t subscribeResult = link->Subscribe("volume",
            [&](const string& /* designator */, const string& /* index */, const string& params) {
                std::lock_guard<std::mutex> lock(mutex);
                lastParams = params;
                count++;
                condition.notify_one();
            });
        ASSERT_EQ(subscribeResult, Core::ERROR_NONE);

        string response;
        EXPECT_EQ(_runtime.Invoke("VolumeControl.volume", "44", response), Core::ERROR_NONE);

        {
            std::unique_lock<std::mutex> lock(mutex);
            EXPECT_TRUE(condition.wait_for(lock, std::chrono::seconds(2), [&] { return (count >= 1); }));
            EXPECT_EQ(count, 1u);
            EXPECT_NE(lastParams.find("44"), string::npos);
        }

        EXPECT_EQ(link->Unsubscribe("volume"), Core::ERROR_NONE);

        EXPECT_EQ(_runtime.Invoke("VolumeControl.volume", "12", response), Core::ERROR_NONE);

        {
            std::unique_lock<std::mutex> lock(mutex);
            EXPECT_FALSE(condition.wait_for(lock, std::chrono::milliseconds(500), [&] { return (count >= 2); }));
            EXPECT_EQ(count, 1u);
        }
    }

    TEST_F(VolumeControlTest, JsonRpcMutedNotificationStopsAfterUnsubscribe)
    {
        auto link = _runtime.CreateJSONRPCLink(Callsign);
        ASSERT_TRUE(link.IsValid());

        std::mutex mutex;
        std::condition_variable condition;
        uint32_t count = 0;
        string lastParams;

        const uint32_t subscribeResult = link->Subscribe("muted",
            [&](const string& /* designator */, const string& /* index */, const string& params) {
                std::lock_guard<std::mutex> lock(mutex);
                lastParams = params;
                count++;
                condition.notify_one();
            });
        ASSERT_EQ(subscribeResult, Core::ERROR_NONE);

        string response;
        EXPECT_EQ(_runtime.Invoke("VolumeControl.muted", "true", response), Core::ERROR_NONE);

        {
            std::unique_lock<std::mutex> lock(mutex);
            EXPECT_TRUE(condition.wait_for(lock, std::chrono::seconds(2), [&] { return (count >= 1); }));
            EXPECT_EQ(count, 1u);
            EXPECT_NE(lastParams.find("true"), string::npos);
        }

        EXPECT_EQ(link->Unsubscribe("muted"), Core::ERROR_NONE);

        EXPECT_EQ(_runtime.Invoke("VolumeControl.muted", "false", response), Core::ERROR_NONE);

        {
            std::unique_lock<std::mutex> lock(mutex);
            EXPECT_FALSE(condition.wait_for(lock, std::chrono::milliseconds(500), [&] { return (count >= 2); }));
            EXPECT_EQ(count, 1u);
        }
    }

} // namespace Tests
} // namespace Plugin
} // namespace Thunder