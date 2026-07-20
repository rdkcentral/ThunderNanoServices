/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2026 Metrological
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

#include "Module.h"

#include <gtest/gtest.h>
#include <test_support/ThunderTestRuntime.h>

namespace Thunder {
namespace Plugin {
namespace Tests {

    namespace {
        constexpr const char* Callsign = TESTWHOLEPLUGINOUTOFPROCESS_TEST_CALLSIGN;
        constexpr const char* ClassName = TESTWHOLEPLUGINOUTOFPROCESS_TEST_CLASSNAME;
        constexpr const char* Locator = TESTWHOLEPLUGINOUTOFPROCESS_TEST_LOCATOR;
        constexpr const char* PluginPath = TESTWHOLEPLUGINOUTOFPROCESS_TEST_PLUGIN_PATH;

        TestCore::ThunderTestRuntime::PluginConfig CreatePluginConfig(const bool wholePluginOutOfProcess)
        {
            TestCore::ThunderTestRuntime::PluginConfig plugin;
            plugin.Callsign = Callsign;
            plugin.ClassName = ClassName;
            plugin.Locator = Locator;
            plugin.StartMode = PluginHost::IShell::startmode::ACTIVATED;

            if (wholePluginOutOfProcess == true) {
                plugin.Root.Mode = Plugin::Config::RootConfig::ModeType::LOCAL;
            }

            return plugin;
        }

        void ExpectConfigurationResult(TestCore::ThunderTestRuntime& runtime, PluginHost::IShell* shell, const uint32_t expectedConfigureResult)
        {
            Exchange::IConfiguration* configuration = runtime.QueryInterfaceByCallsign<Exchange::IConfiguration>(Callsign);
            ASSERT_NE(configuration, nullptr) << "Expected TestWholePluginOutOfProcess to expose IConfiguration";
            EXPECT_EQ(configuration->Configure(shell), expectedConfigureResult);
            configuration->Release();
        }

        void RunActivationScenario(const bool wholePluginOutOfProcess)
        {
            SCOPED_TRACE(wholePluginOutOfProcess == true ? "whole plugin out-of-process" : "in-process");

            TestCore::ThunderTestRuntime runtime;
            const uint32_t result = runtime.Initialize({ CreatePluginConfig(wholePluginOutOfProcess) }, PluginPath);
            ASSERT_EQ(result, Core::ERROR_NONE) << "Failed to initialize Thunder runtime for " << Callsign;

            Core::ProxyType<PluginHost::IShell> shell = runtime.GetShell(Callsign);
            ASSERT_TRUE(shell.IsValid()) << "Expected TestWholePluginOutOfProcess shell to be registered";
            ExpectConfigurationResult(runtime, shell.operator->(), wholePluginOutOfProcess == true ? Core::ERROR_NOT_SUPPORTED : Core::ERROR_NONE);

            runtime.Deinitialize();
        }
    }

    TEST(WholePluginOutOfProcess, ActivatesInProcessAndWholePluginOutOfProcess)
    {
        RunActivationScenario(false);
        RunActivationScenario(true);
    }

} // namespace Tests
} // namespace Plugin
} // namespace Thunder