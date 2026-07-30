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

#pragma once

#include "Module.h"
#include "ES1BenchmarkData.h"

namespace WPEFramework {
namespace Plugin {

    // -----------------------------------------------------------------------
    // ES1Benchmark — in-process (Local mode) echo plugin for JSON-RPC
    // round-trip benchmarking.
    //
    // Exposes echo endpoints for three parameter-type classes:
    //   String  : echostring
    //   Array   : echoarray
    //   Scalar  : echoint32, echoint64, echobool, echofloat, echodouble
    //
    // Each method receives a value and reflects it unchanged so the client
    // can measure the full JSON-RPC serialise → dispatch → deserialise cost.
    // -----------------------------------------------------------------------
    class ES1Benchmark : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        ES1Benchmark(const ES1Benchmark&) = delete;
        ES1Benchmark& operator=(const ES1Benchmark&) = delete;

        ES1Benchmark()
        {
            RegisterAll();
        }

        ~ES1Benchmark() override
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(ES1Benchmark)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        // IPlugin
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        // JSON-RPC registration
        void RegisterAll();
        void UnregisterAll();

        // ---- string ----
        uint32_t endpoint_echostring(
            const JsonData::ES1Benchmark::StringEchoParams& params,
            JsonData::ES1Benchmark::StringEchoResult& response);

        // ---- array ----
        uint32_t endpoint_echoarray(
            const JsonData::ES1Benchmark::ArrayEchoParams& params,
            JsonData::ES1Benchmark::ArrayEchoResult& response);

        // ---- scalars ----
        uint32_t endpoint_echoint32 (const Core::JSON::DecUInt32& params, Core::JSON::DecUInt32& response);
        uint32_t endpoint_echoint64 (const Core::JSON::DecUInt64& params, Core::JSON::DecUInt64& response);
        uint32_t endpoint_echobool  (const Core::JSON::Boolean&   params, Core::JSON::Boolean&   response);
        uint32_t endpoint_echofloat (const Core::JSON::Float&     params, Core::JSON::Float&     response);
        uint32_t endpoint_echodouble(const Core::JSON::Double&    params, Core::JSON::Double&    response);
    };

} // namespace Plugin
} // namespace WPEFramework
