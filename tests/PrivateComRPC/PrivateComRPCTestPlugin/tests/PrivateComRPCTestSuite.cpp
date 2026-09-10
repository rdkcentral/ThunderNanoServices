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

#include <test_support/ThunderTestRuntime.h>

#include <gtest/gtest.h>
#include <qa_interfaces/IPrivateComRPC.h>

#include <chrono>

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

        string CreatePayload(const uint32_t size)
        {
            static constexpr char Hex[] = "0123456789ABCDEF";

            string payload;
            payload.reserve(size);

            for (uint32_t index = 0; index < size; ++index) {
                payload += Hex[index % (sizeof(Hex) - 1)];
            }

            return payload;
        }

    } // namespace

    class PrivateComRPCTestSuite : public ::testing::Test {
    protected:
        static ThunderTestRuntime _runtime;

        QualityAssurance::IPrivateComRPC* _privateComRPC { nullptr };

        static void SetUpTestSuite()
        {
            ThunderTestRuntime::PluginConfig cfg;

            cfg.Callsign = PRIVATECOMRPCTEST_TEST_CALLSIGN;
            cfg.ClassName = PRIVATECOMRPCTEST_TEST_CLASSNAME;
            cfg.Locator = PRIVATECOMRPCTEST_TEST_LOCATOR;
            cfg.Resumed = false;
            cfg.Root.Mode = Plugin::Config::RootConfig::ModeType::LOCAL;

            const uint32_t result = _runtime.Initialize({ cfg }, PRIVATECOMRPCTEST_TEST_PLUGIN_PATH);

            ASSERT_EQ(result, Core::ERROR_NONE)
                << "ThunderTestRuntime failed to initialise (error "
                << result << ")";
        }

        static void TearDownTestSuite()
        {
            _runtime.Deinitialize();
        }

        void SetUp() override
        {
            _privateComRPC = _runtime.QueryInterfaceByCallsign<QualityAssurance::IPrivateComRPC>(PRIVATECOMRPCTEST_TEST_CALLSIGN);

            ASSERT_NE(_privateComRPC, nullptr)
                << "IPrivateComRPC must be accessible via QueryInterfaceByCallsign";
        }

        void TearDown() override
        {
            if (_privateComRPC != nullptr) {
                _privateComRPC->Release();
                _privateComRPC = nullptr;
            }
        }
    };

    ThunderTestRuntime PrivateComRPCTestSuite::_runtime;

    TEST_F(PrivateComRPCTestSuite, InterfaceIsAvailable)
    {
        EXPECT_NE(_privateComRPC, nullptr);
    }

    TEST_F(PrivateComRPCTestSuite, SmallPayloadIsProcessed)
    {
        const string payload(_T("0123456789ABCDEF"));

        string response;

        const Core::hresult result = _privateComRPC->ProcessLargeData(payload, 0, true, response);

        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(response, payload);
    }

    TEST_F(PrivateComRPCTestSuite, LargePayloadIsProcessed)
    {
        constexpr uint32_t PayloadSize = 60 * 1024;

        const string payload = CreatePayload(PayloadSize);

        string response;

        const Core::hresult result = _privateComRPC->ProcessLargeData(payload, 0, true, response);

        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(response.size(), payload.size());
        EXPECT_EQ(response, payload);
    }

    TEST_F(PrivateComRPCTestSuite, DelayedLargePayloadIsProcessed)
    {
        constexpr uint32_t PayloadSize = 60 * 1024;
        constexpr uint32_t DelayMs = 2000;

        const string payload = CreatePayload(PayloadSize);

        string response;

        const auto start = std::chrono::steady_clock::now();

        const Core::hresult result = _privateComRPC->ProcessLargeData(payload, DelayMs, true, response);

        const auto end = std::chrono::steady_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(response, payload);

        EXPECT_GE(elapsed.count(), static_cast<int64_t>(DelayMs));
    }

    TEST_F(PrivateComRPCTestSuite, PayloadCanBeProcessedWithoutEcho)
    {
        const string payload = CreatePayload(1024);

        string response;

        const Core::hresult result = _privateComRPC->ProcessLargeData(payload, 0, false, response);

        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_TRUE(response.empty());
    }

} // namespace Tests
} // namespace TestCore
} // namespace Thunder