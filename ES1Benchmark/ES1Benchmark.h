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
#include <interfaces/IES1Benchmark.h>
#include <interfaces/json/JES1Benchmark.h>

namespace WPEFramework {
namespace Plugin {

    class ES1Benchmark : public PluginHost::IPlugin
                       , public PluginHost::JSONRPC
                       , public Exchange::IES1Benchmark {
    public:
        ES1Benchmark(const ES1Benchmark&) = delete;
        ES1Benchmark& operator=(const ES1Benchmark&) = delete;

        ES1Benchmark() = default;
        ~ES1Benchmark() override = default;

        BEGIN_INTERFACE_MAP(ES1Benchmark)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IES1Benchmark)
        END_INTERFACE_MAP

        // IPlugin
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        // IES1Benchmark
        uint32_t EchoString(const string& value, string& echo, uint64_t& ts2, uint64_t& ts3) override;
        uint32_t EchoArray(const std::vector<uint8_t>& values, std::vector<uint8_t>& echo, uint64_t& ts2, uint64_t& ts3) override;
        uint32_t EchoMixedArray(const std::vector<Exchange::IES1Benchmark::MixedElement>& elements, std::vector<Exchange::IES1Benchmark::MixedElement>& echo, uint64_t& ts2, uint64_t& ts3) override;
        uint32_t EchoNestedObjects(const std::vector<Exchange::IES1Benchmark::NestedObject>& objects, std::vector<Exchange::IES1Benchmark::NestedObject>& echo, uint64_t& ts2, uint64_t& ts3) override;
        uint32_t EchoUint32(const uint32_t value, uint32_t& echo, uint64_t& ts2, uint64_t& ts3) override;
        uint32_t EchoUint64(const uint64_t value, uint64_t& echo, uint64_t& ts2, uint64_t& ts3) override;
        uint32_t EchoBool(const bool value, bool& echo, uint64_t& ts2, uint64_t& ts3) override;
        uint32_t EchoFloat(const float value, float& echo, uint64_t& ts2, uint64_t& ts3) override;
        uint32_t EchoDouble(const double value, double& echo, uint64_t& ts2, uint64_t& ts3) override;
    };

} // namespace Plugin
} // namespace WPEFramework
