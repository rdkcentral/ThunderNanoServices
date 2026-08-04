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

#include "ES1Benchmark.h"

namespace WPEFramework {
namespace Plugin {

    namespace {
        static Metadata<ES1Benchmark> metadata(
            // Version (Major, Minor, Patch)
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    const string ES1Benchmark::Initialize(PluginHost::IShell* /* service */)
    {
        Exchange::JES1Benchmark::Register(*this, this);
        return string();
    }

    void ES1Benchmark::Deinitialize(PluginHost::IShell* /* service */)
    {
        Exchange::JES1Benchmark::Unregister(*this);
    }

    string ES1Benchmark::Information() const
    {
        return string("ES1 JSON-RPC round-trip benchmark echo plugin");
    }

    uint32_t ES1Benchmark::EchoString(const string& value, string& echo)
    {
        echo = value;
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoArray(const std::vector<uint8_t>& values, std::vector<uint8_t>& echo)
    {
        echo = values;
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoUint32(const uint32_t value, uint32_t& echo)
    {
        echo = value;
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoUint64(const uint64_t value, uint64_t& echo)
    {
        echo = value;
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoBool(const bool value, bool& echo)
    {
        echo = value;
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoFloat(const float value, float& echo)
    {
        echo = value;
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoDouble(const double value, double& echo)
    {
        echo = value;
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
