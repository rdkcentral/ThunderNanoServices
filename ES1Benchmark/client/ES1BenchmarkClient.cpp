/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Metrological
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

// ---------------------------------------------------------------------------
// ES1BenchmarkClient — standalone JSON-RPC measurement client
//
// Calls the ES1Benchmark plugin echo methods N times and reports:
//   avg µs | stddev µs | min µs | max µs | rtt µs
// for every (method, payload-size, connection-strategy) combination.
//
// Usage:
//   ES1BenchmarkClient [--host <ip>] [--port <port>]
//                      [--iterations <N>]
//                      [--string-sizes <s1,s2,...>]
//                      [--array-counts <n1,n2,...>]
//
// Connection strategies exercised:
//   persistent_ws  — one WebSocket connection shared across all iterations
//   oneshot        — a new connection is created for every single call
// ---------------------------------------------------------------------------

#include "Module.h"
#include "../ES1BenchmarkData.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <string>
#include <vector>

using namespace WPEFramework;

// ---------------------------------------------------------------------------
// JSONRPC::LinkType lives in WPEFramework::JSONRPC (via websocket/websocket.h)
// ---------------------------------------------------------------------------
static const char* CALLSIGN = "ES1Benchmark.1";

// ---------------------------------------------------------------------------
// Timing statistics
// ---------------------------------------------------------------------------
struct Stats {
    uint64_t avgUs   { 0 };
    uint64_t stddevUs{ 0 };
    uint64_t minUs   { 0 };
    uint64_t maxUs   { 0 };
    uint64_t rttUs   { 0 };   // same as avg for synchronous calls
};

static Stats ComputeStats(const std::vector<uint64_t>& samples)
{
    Stats s;
    if (samples.empty()) return s;

    s.minUs = *std::min_element(samples.begin(), samples.end());
    s.maxUs = *std::max_element(samples.begin(), samples.end());

    uint64_t sum = std::accumulate(samples.begin(), samples.end(), uint64_t(0));
    s.avgUs = sum / samples.size();
    s.rttUs = s.avgUs;

    double var = 0.0;
    for (auto t : samples) {
        double d = static_cast<double>(t) - static_cast<double>(s.avgUs);
        var += d * d;
    }
    s.stddevUs = static_cast<uint64_t>(std::sqrt(var / samples.size()));

    return s;
}

static void PrintHeader()
{
    printf("\n%-40s | %-18s | %7s | %9s | %7s | %7s | %7s\n",
        "Method", "Strategy", "Avg µs", "Stddev µs", "Min µs", "Max µs", "RTT µs");
    printf("%-40s-+-%-18s-+-%7s-+-%9s-+-%7s-+-%7s-+-%7s\n",
        "----------------------------------------",
        "------------------",
        "-------", "---------", "-------", "-------", "-------");
}

static void PrintRow(const char* method, const char* strategy, const Stats& s)
{
    printf("%-40s | %-18s | %7" PRIu64 " | %9" PRIu64 " | %7" PRIu64 " | %7" PRIu64 " | %7" PRIu64 "\n",
        method, strategy,
        s.avgUs, s.stddevUs, s.minUs, s.maxUs, s.rttUs);
}

// ---------------------------------------------------------------------------
// Run helper: persistent strategy
//   One JSONRPC::LinkType is created outside the loop.
//   Timing covers only the Invoke() call — connection setup is amortised.
// ---------------------------------------------------------------------------
template<typename IN, typename OUT>
static Stats RunPersistent(JSONRPC::LinkType<Core::JSON::IElement>& link,
                           const char* method,
                           const IN& params,
                           uint32_t iterations)
{
    // Warm-up call: avoids JIT / first-connect / page-fault noise in samples.
    {
        OUT warmup;
        link.Invoke<IN, OUT>(5000, _T(method), params, warmup);
    }

    std::vector<uint64_t> samples;
    samples.reserve(iterations);

    for (uint32_t i = 0; i < iterations; i++) {
        OUT response;
        uint64_t t0 = Core::Time::Now().Ticks();
        link.Invoke<IN, OUT>(5000, _T(method), params, response);
        uint64_t t1 = Core::Time::Now().Ticks();
        // Ticks() returns microseconds on Linux
        samples.push_back(t1 - t0);
    }

    return ComputeStats(samples);
}

// ---------------------------------------------------------------------------
// Run helper: one-shot strategy
//   A brand-new JSONRPC::LinkType is created for every call, forcing a fresh
//   WebSocket connection each time.  Timing covers connection + Invoke.
// ---------------------------------------------------------------------------
template<typename IN, typename OUT>
static Stats RunOneShot(const char* method,
                        const IN& params,
                        uint32_t iterations)
{
    // Warm-up
    {
        JSONRPC::LinkType<Core::JSON::IElement> link(_T(CALLSIGN), _T("client.es1.warmup"));
        OUT warmup;
        link.Invoke<IN, OUT>(5000, _T(method), params, warmup);
    }

    std::vector<uint64_t> samples;
    samples.reserve(iterations);

    for (uint32_t i = 0; i < iterations; i++) {
        // New object = new WebSocket connection = simulates a cold caller.
        JSONRPC::LinkType<Core::JSON::IElement> link(_T(CALLSIGN), _T("client.es1.oneshot"));
        OUT response;
        uint64_t t0 = Core::Time::Now().Ticks();
        link.Invoke<IN, OUT>(5000, _T(method), params, response);
        uint64_t t1 = Core::Time::Now().Ticks();
        samples.push_back(t1 - t0);
    }

    return ComputeStats(samples);
}

// ---------------------------------------------------------------------------
// Per-type benchmark runners
// ---------------------------------------------------------------------------

static void BenchString(JSONRPC::LinkType<Core::JSON::IElement>& persistent,
                        uint32_t strSize,
                        uint32_t iterations)
{
    using namespace JsonData::ES1Benchmark;

    // Build a string payload of the requested size.
    StringEchoParams params;
    params.Size  = strSize;
    params.Value = string(strSize, 'x');

    char label[64];
    snprintf(label, sizeof(label), "echostring (size=%u)", strSize);

    PrintRow(label, "persistent_ws", RunPersistent<StringEchoParams, StringEchoResult>(
        persistent, "echostring", params, iterations));
    PrintRow(label, "oneshot",       RunOneShot<StringEchoParams, StringEchoResult>(
        "echostring", params, iterations));
}

static void BenchArray(JSONRPC::LinkType<Core::JSON::IElement>& persistent,
                       uint32_t count,
                       uint32_t iterations)
{
    using namespace JsonData::ES1Benchmark;

    ArrayEchoParams params;
    params.Count = count;
    for (uint32_t i = 0; i < count; i++) {
        params.Values.Add() = i;
    }

    char label[64];
    snprintf(label, sizeof(label), "echoarray  (count=%u)", count);

    PrintRow(label, "persistent_ws", RunPersistent<ArrayEchoParams, ArrayEchoResult>(
        persistent, "echoarray", params, iterations));
    PrintRow(label, "oneshot",       RunOneShot<ArrayEchoParams, ArrayEchoResult>(
        "echoarray", params, iterations));
}

static void BenchScalars(JSONRPC::LinkType<Core::JSON::IElement>& persistent,
                         uint32_t iterations)
{
    // int32
    {
        Core::JSON::DecUInt32 p; p = 42u;
        PrintRow("echoint32", "persistent_ws", RunPersistent<Core::JSON::DecUInt32, Core::JSON::DecUInt32>(
            persistent, "echoint32", p, iterations));
        PrintRow("echoint32", "oneshot",       RunOneShot<Core::JSON::DecUInt32, Core::JSON::DecUInt32>(
            "echoint32", p, iterations));
    }
    // int64
    {
        Core::JSON::DecUInt64 p; p = 123456789ULL;
        PrintRow("echoint64", "persistent_ws", RunPersistent<Core::JSON::DecUInt64, Core::JSON::DecUInt64>(
            persistent, "echoint64", p, iterations));
        PrintRow("echoint64", "oneshot",       RunOneShot<Core::JSON::DecUInt64, Core::JSON::DecUInt64>(
            "echoint64", p, iterations));
    }
    // bool
    {
        Core::JSON::Boolean p; p = true;
        PrintRow("echobool",  "persistent_ws", RunPersistent<Core::JSON::Boolean, Core::JSON::Boolean>(
            persistent, "echobool", p, iterations));
        PrintRow("echobool",  "oneshot",       RunOneShot<Core::JSON::Boolean, Core::JSON::Boolean>(
            "echobool", p, iterations));
    }
    // float
    {
        Core::JSON::Float p; p = 3.14f;
        PrintRow("echofloat", "persistent_ws", RunPersistent<Core::JSON::Float, Core::JSON::Float>(
            persistent, "echofloat", p, iterations));
        PrintRow("echofloat", "oneshot",       RunOneShot<Core::JSON::Float, Core::JSON::Float>(
            "echofloat", p, iterations));
    }
    // double
    {
        Core::JSON::Double p; p = 2.718281828;
        PrintRow("echodouble","persistent_ws", RunPersistent<Core::JSON::Double, Core::JSON::Double>(
            persistent, "echodouble", p, iterations));
        PrintRow("echodouble","oneshot",       RunOneShot<Core::JSON::Double, Core::JSON::Double>(
            "echodouble", p, iterations));
    }
}

// ---------------------------------------------------------------------------
// Argument parsing
// ---------------------------------------------------------------------------
static std::vector<uint32_t> ParseCSV(const char* str)
{
    std::vector<uint32_t> result;
    char* copy = strdup(str);
    char* tok  = strtok(copy, ",");
    while (tok != nullptr) {
        result.push_back(static_cast<uint32_t>(atoi(tok)));
        tok = strtok(nullptr, ",");
    }
    free(copy);
    return result;
}

static void ShowUsage(const char* prog)
{
    printf("Usage: %s [options]\n"
           "  --host <ip>              Device address (default: 127.0.0.1)\n"
           "  --port <port>            JSON-RPC port  (default: 9998)\n"
           "  --iterations <N>         Calls per method per size (default: 100)\n"
           "  --string-sizes <csv>     String sizes in bytes     (default: 64,2048,20480)\n"
           "  --array-counts <csv>     Array element counts      (default: 10,512,1024)\n",
           prog);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
    const char*  host       = "127.0.0.1";
    uint16_t     port       = 9998;
    uint32_t     iterations = 100;
    const char*  strSizes   = "64,2048,20480";
    const char*  arrCounts  = "10,512,1024";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host")         == 0 && i + 1 < argc) { host       = argv[++i]; }
        else if (strcmp(argv[i], "--port")    == 0 && i + 1 < argc) { port       = static_cast<uint16_t>(atoi(argv[++i])); }
        else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) { iterations = static_cast<uint32_t>(atoi(argv[++i])); }
        else if (strcmp(argv[i], "--string-sizes") == 0 && i + 1 < argc) { strSizes = argv[++i]; }
        else if (strcmp(argv[i], "--array-counts") == 0 && i + 1 < argc) { arrCounts = argv[++i]; }
        else if (strcmp(argv[i], "--help")    == 0 || strcmp(argv[i], "-h") == 0) {
            ShowUsage(argv[0]);
            return 0;
        }
    }

    // Set THUNDER_ACCESS so all JSONRPC::LinkType objects connect to the right host.
    char access[128];
    snprintf(access, sizeof(access), "%s:%u", host, port);
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), access);

    printf("ES1Benchmark client\n");
    printf("  Target     : %s\n", access);
    printf("  Iterations : %u\n", iterations);
    printf("  String sizes: %s bytes\n", strSizes);
    printf("  Array counts: %s elements\n\n", arrCounts);

    // Open a single persistent connection for the persistent_ws strategy.
    JSONRPC::LinkType<Core::JSON::IElement> persistent(_T(CALLSIGN), _T("client.es1.persistent"));

    std::vector<uint32_t> sizes  = ParseCSV(strSizes);
    std::vector<uint32_t> counts = ParseCSV(arrCounts);

    PrintHeader();

    // ---- String payloads ----
    for (uint32_t sz : sizes) {
        BenchString(persistent, sz, iterations);
    }

    // ---- Array payloads ----
    for (uint32_t cnt : counts) {
        BenchArray(persistent, cnt, iterations);
    }

    // ---- Scalar types ----
    BenchScalars(persistent, iterations);

    printf("\nDone.\n");

    Core::Singleton::Dispose();
    return 0;
}
