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

/**
 * @file JSONRPCThrottleTestSuite.cpp
 *
 * Stage 1 sanity + concurrency-counter test for the JSONRPCThrottle plugin.
 *
 * IMPORTANT: everything in this file goes through ThunderTestRuntime's
 * in-process JSON-RPC link — a direct dispatcher path. It does NOT exercise
 * Thunder's channel_throttle / plugin-throttle queue, which only sits in
 * front of the real HTTP/WebSocket transport. The "concurrency" tests below
 * verify the PLUGIN's own atomic counters behave correctly under genuine
 * OS-thread concurrency (multiple std::threads hitting the plugin at once);
 * they say nothing about whether Thunder would have queued or rejected any
 * of those calls. Actual throttle verification is Stage 2 (external
 * HTTP/WebSocket bombardment client).
 */

#include "Module.h"
#include <test_support/ThunderTestRuntime.h>

#include <gtest/gtest.h>

#include <atomic>
#include <cctype>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

namespace Thunder {
namespace TestCore {
namespace Tests {

namespace {

    // Minimal scraper for the flat, non-nested JSON that JSONRPCThrottle's
    // Statistics container produces, e.g. {"totalCalls":3,"activeCalls":0,...}.
    // Not a general JSON parser — only good for unsigned integer fields at
    // the top level of this specific response shape.
    uint64_t ExtractNumber(const string& json, const string& key)
    {
        const string needle = "\"" + key + "\":";
        const auto pos = json.find(needle);
        if (pos == string::npos) {
            return 0;
        }
        auto start = pos + needle.size();
        auto end = start;
        while (end < json.size() && std::isdigit(static_cast<unsigned char>(json[end]))) {
            ++end;
        }
        if (end == start) {
            return 0;
        }
        return static_cast<uint64_t>(std::stoull(json.substr(start, end - start)));
    }

    constexpr uint16_t HTTP_PORT = 19091;
    constexpr uint8_t CHANNEL_THROTTLE = 2;
    constexpr uint8_t PLUGIN_THROTTLE = 4;

} // namespace

class JSONRPCThrottleTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        ThunderTestRuntime::PluginConfig cfg;

        cfg.Callsign  = JSONRPCTHROTTLE_TEST_CALLSIGN;
        cfg.ClassName = JSONRPCTHROTTLE_TEST_CLASSNAME;
        cfg.Locator   = JSONRPCTHROTTLE_TEST_LOCATOR;
        cfg.Resumed   = false;

        // Thunder's per-plugin JSON-RPC throttle.
        cfg.Throttle = PLUGIN_THROTTLE;

        const uint32_t result =
            _runtime.Initialize(
                { cfg },
                JSONRPCTHROTTLE_TEST_PLUGIN_PATH,
                "",
                HTTP_PORT,
                CHANNEL_THROTTLE);

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
        _link = _runtime.CreateJSONRPCLink(JSONRPCTHROTTLE_TEST_CALLSIGN);
        ASSERT_TRUE(_link.IsValid());

        // Every test starts from a clean counter state so assertions don't
        // depend on execution order / what earlier tests left behind.
        string response;
        ASSERT_EQ(_link->Invoke("reset", "{}", response), Core::ERROR_NONE);
    }

    uint64_t ReadStat(const string& key)
    {
        string response;
        EXPECT_EQ(_link->Invoke("statistics", "{}", response), Core::ERROR_NONE);
        return ExtractNumber(response, key);
    }

        uint32_t ResetStatistics()
    {
        string response;

        return _link->Invoke(
            _T("reset"),
            _T("{}"),
            response);
    }

    uint32_t GetStatistics(
        uint64_t& totalCalls,
        uint64_t& activeCalls,
        uint64_t& maximumConcurrentCalls)
    {
        string response;

        const uint32_t result =
            _link->Invoke(
                _T("statistics"),
                _T("{}"),
                response);

        if (result != Core::ERROR_NONE) {
            return result;
        }

        Core::JSON::Container statistics;

        Core::JSON::DecUInt64 total;
        Core::JSON::DecUInt64 active;
        Core::JSON::DecUInt64 maximum;

        statistics.Add(
            _T("totalCalls"),
            &total);

        statistics.Add(
            _T("activeCalls"),
            &active);

        statistics.Add(
            _T("maximumConcurrentCalls"),
            &maximum);

        if (!statistics.FromString(response)) {
            return Core::ERROR_GENERAL;
        }

        totalCalls = total.Value();
        activeCalls = active.Value();
        maximumConcurrentCalls = maximum.Value();

        return Core::ERROR_NONE;
    }

    void SendDelayRequest(
    const uint32_t delayMilliseconds)
    {
        const int socketFd = socket(
            AF_INET,
            SOCK_STREAM,
            0);

        ASSERT_GE(socketFd, 0);

        sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_port = htons(HTTP_PORT);

        ASSERT_EQ(
            inet_pton(
                AF_INET,
                "127.0.0.1",
                &address.sin_addr),
            1);

        ASSERT_EQ(
            connect(
                socketFd,
                reinterpret_cast<sockaddr*>(&address),
                sizeof(address)),
            0);

        const std::string body =
            std::string(
                "{\"jsonrpc\":\"2.0\","
                "\"id\":1,"
                "\"method\":\"delay\","
                "\"params\":{\"milliseconds\":")
            + std::to_string(delayMilliseconds)
            + "}}";

        std::cout
            << "[HTTP] thread=" << std::this_thread::get_id()
            << " connecting to 127.0.0.1:" << HTTP_PORT
            << std::endl;

        const std::string request =
            "POST /jsonrpc/JSONRPCThrottle HTTP/1.1\r\n"
            "Host: 127.0.0.1\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: "
            + std::to_string(body.size())
            + "\r\n"
            "Connection: close\r\n"
            "\r\n"
            + body;

        size_t sent = 0;

        std::cout
            << "[HTTP] thread=" << std::this_thread::get_id()
            << " SEND delay(" << delayMilliseconds << ")"
            << std::endl;
        while (sent < request.size()) {
            const ssize_t result =
                send(
                    socketFd,
                    request.data() + sent,
                    request.size() - sent,
                    0);

            ASSERT_GT(result, 0);

            sent += static_cast<size_t>(result);
        }

        char buffer[4096];

        while (true) {
            const ssize_t received =
                recv(
                    socketFd,
                    buffer,
                    sizeof(buffer),
                    0);

            if (received <= 0) {
                break;
            }
        }

        std::cout
                << "[HTTP] thread=" << std::this_thread::get_id()
                << " RESPONSE received"
                << std::endl;

        close(socketFd);
    }

    void SendConcurrentDelayRequests(
    const uint32_t requestCount,
    const uint32_t delayMilliseconds)
    {
        std::atomic<bool> start{false};

        std::vector<std::thread> workers;
        workers.reserve(requestCount);

        for (uint32_t i = 0; i < requestCount; ++i) {
            workers.emplace_back(
                [&start, delayMilliseconds, this]() {
                    while (!start.load(
                        std::memory_order_acquire)) {
                        std::this_thread::yield();
                    }

                    SendDelayRequest(delayMilliseconds);
                });
        }

        start.store(
            true,
            std::memory_order_release);

        for (auto& worker : workers) {
            worker.join();
        }
    }

    static ThunderTestRuntime _runtime;
    decltype(_runtime.CreateJSONRPCLink(JSONRPCTHROTTLE_TEST_CALLSIGN)) _link;
};



ThunderTestRuntime JSONRPCThrottleTest::_runtime;

// ==========================================================================
// 0. Basic sanity (unchanged from before)
// ==========================================================================

TEST_F(JSONRPCThrottleTest, Fast)
{
    string response;
    EXPECT_EQ(_link->Invoke("fast", "{}", response), Core::ERROR_NONE);
}

TEST_F(JSONRPCThrottleTest, Delay)
{
    string response;
    EXPECT_EQ(_link->Invoke("delay", "{\"milliseconds\":100}", response), Core::ERROR_NONE);
}

TEST_F(JSONRPCThrottleTest, Statistics)
{
    string response;
    EXPECT_EQ(_link->Invoke("statistics", "{}", response), Core::ERROR_NONE);
    EXPECT_FALSE(response.empty());
}

TEST_F(JSONRPCThrottleTest, Reset)
{
    string response;
    EXPECT_EQ(_link->Invoke("reset", "{}", response), Core::ERROR_NONE);
}

// ==========================================================================
// 1. Counter correctness (single caller)
// ==========================================================================

TEST_F(JSONRPCThrottleTest, ResetZeroesAllCounters)
{
    // SetUp already reset; confirm the observable state agrees.
    EXPECT_EQ(ReadStat("totalCalls"), 0u);
    EXPECT_EQ(ReadStat("activeCalls"), 0u);
    EXPECT_EQ(ReadStat("maximumConcurrentCalls"), 0u);
}

TEST_F(JSONRPCThrottleTest, StatisticsAloneDoesNotIncrementCounters)
{
    // statistics() must be a pure observer — it doesn't call Enter()/Leave().
    ReadStat("totalCalls");
    ReadStat("totalCalls");
    ReadStat("totalCalls");

    EXPECT_EQ(ReadStat("totalCalls"), 0u)
        << "Calling statistics() repeatedly must not itself count as activity";
}

TEST_F(JSONRPCThrottleTest, SequentialDelayCallsAccumulateTotalCalls)
{
    constexpr int kCalls = 5;
    string response;

    for (int i = 0; i < kCalls; ++i) {
        ASSERT_EQ(_link->Invoke("delay", "{\"milliseconds\":20}", response), Core::ERROR_NONE);
    }

    EXPECT_EQ(ReadStat("totalCalls"), static_cast<uint64_t>(kCalls));
    EXPECT_EQ(ReadStat("activeCalls"), 0u);
    // Calls were issued one at a time on the same thread — no overlap possible.
    EXPECT_EQ(ReadStat("maximumConcurrentCalls"), 1u);
}

TEST_F(JSONRPCThrottleTest, FastAndDelayShareTheSameCounters)
{
    string response;
    ASSERT_EQ(_link->Invoke("fast", "{}", response), Core::ERROR_NONE);
    ASSERT_EQ(_link->Invoke("delay", "{\"milliseconds\":10}", response), Core::ERROR_NONE);
    ASSERT_EQ(_link->Invoke("fast", "{}", response), Core::ERROR_NONE);

    EXPECT_EQ(ReadStat("totalCalls"), 3u);
}

TEST_F(JSONRPCThrottleTest, DelayHonoursRequestedDuration)
{
    string response;
    const auto start = std::chrono::steady_clock::now();

    ASSERT_EQ(_link->Invoke("delay", "{\"milliseconds\":200}", response), Core::ERROR_NONE);

    const auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_GE(elapsed, std::chrono::milliseconds(190))
        << "delay() returned noticeably earlier than the requested 200ms";
}

// ==========================================================================
// 2. Counter correctness under genuine concurrent access
//
// NOTE (repeat of the file-level caveat): concurrency here comes from this
// TEST launching multiple OS threads against the plugin, each with its own
// JSONRPCLink. This validates JSONRPCThrottle::Enter()/Leave() are correct
// under real concurrent access — it does NOT validate Thunder's throttle
// queue, which these in-process links never touch.
// ==========================================================================

TEST_F(JSONRPCThrottleTest, ConcurrentDelayCallsAreCountedCorrectly)
{
    constexpr int kThreadCount = 8;
    constexpr int kDelayMs = 300;

    std::atomic<int> ready{ 0 };
    std::atomic<bool> go{ false };
    std::vector<std::thread> workers;
    std::vector<uint32_t> results(kThreadCount, static_cast<uint32_t>(Core::ERROR_GENERAL));

    for (int i = 0; i < kThreadCount; ++i) {
        workers.emplace_back([this, i, &ready, &go, &results]() {
            auto link = _runtime.CreateJSONRPCLink(JSONRPCTHROTTLE_TEST_CALLSIGN);
            if (!link.IsValid()) {
                return;
            }

            ++ready;
            while (!go.load()) {
                std::this_thread::yield();
            }

            string response;
            results[i] = link->Invoke(
                "delay",
                "{\"milliseconds\":" + std::to_string(kDelayMs) + "}",
                response);
        });
    }

    // Let every worker obtain its link and reach the starting line before
    // releasing them, so the calls actually overlap rather than trickling in.
    while (ready.load() < kThreadCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    go = true;

    for (auto& t : workers) {
        t.join();
    }

    for (uint32_t r : results) {
        EXPECT_EQ(r, Core::ERROR_NONE);
    }

    const uint64_t maxConcurrent = ReadStat("maximumConcurrentCalls");
    const uint64_t totalCalls    = ReadStat("totalCalls");
    const uint64_t activeCalls   = ReadStat("activeCalls");

    EXPECT_EQ(totalCalls, static_cast<uint64_t>(kThreadCount));
    EXPECT_EQ(activeCalls, 0u)
        << "All workers joined, so every in-flight call must have completed";
    EXPECT_GT(maxConcurrent, 1u)
        << "Expected genuine overlap across " << kThreadCount
        << " threads; got observed max concurrency of only " << maxConcurrent
        << " — either the threads didn't actually overlap, or Enter()/Leave() "
           "has a bug";
    EXPECT_LE(maxConcurrent, static_cast<uint64_t>(kThreadCount))
        << "Observed concurrency can't exceed the number of callers";
}

TEST_F(JSONRPCThrottleTest, ResetDuringNoActivityLeavesCountersAtZero)
{
    // Sequence a batch of calls, reset, then confirm nothing "leaks" across
    // the reset boundary into the next measurement window.
    string response;
    for (int i = 0; i < 3; ++i) {
        ASSERT_EQ(_link->Invoke("fast", "{}", response), Core::ERROR_NONE);
    }
    ASSERT_GT(ReadStat("totalCalls"), 0u);

    ASSERT_EQ(_link->Invoke("reset", "{}", response), Core::ERROR_NONE);

    EXPECT_EQ(ReadStat("totalCalls"), 0u);
    EXPECT_EQ(ReadStat("activeCalls"), 0u);
    EXPECT_EQ(ReadStat("maximumConcurrentCalls"), 0u);
}

TEST_F(JSONRPCThrottleTest, HTTPThrottleFour)
{
    ASSERT_EQ(
        ResetStatistics(),
        Core::ERROR_NONE);

    const auto start =
        std::chrono::steady_clock::now();

    SendConcurrentDelayRequests(
        20,
        2000);

    const auto elapsed =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);

    uint64_t totalCalls = 0;
    uint64_t activeCalls = 0;
    uint64_t maximumConcurrentCalls = 0;

    ASSERT_EQ(
        GetStatistics(
            totalCalls,
            activeCalls,
            maximumConcurrentCalls),
        Core::ERROR_NONE);

    EXPECT_EQ(totalCalls, 20);
    EXPECT_EQ(activeCalls, 0);

    // This is the important assertion.
    EXPECT_LE(
        maximumConcurrentCalls,
        4);

    // 20 requests / 4 slots = 5 batches.
    // Each request takes 2 seconds.
    // Expected duration is roughly 10 seconds.
    EXPECT_GE(
        elapsed.count(),
        7000);
}

} // namespace Tests
} // namespace TestCore
} // namespace Thunder