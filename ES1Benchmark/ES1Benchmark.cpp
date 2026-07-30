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

    // -----------------------------------------------------------------------
    // IPlugin
    // -----------------------------------------------------------------------

    const string ES1Benchmark::Initialize(PluginHost::IShell* /* service */)
    {
        // Nothing to initialise for a pure echo plugin.
        return string();
    }

    void ES1Benchmark::Deinitialize(PluginHost::IShell* /* service */)
    {
    }

    string ES1Benchmark::Information() const
    {
        return string("ES1 JSON-RPC round-trip benchmark echo plugin");
    }

    // -----------------------------------------------------------------------
    // JSON-RPC registration
    // -----------------------------------------------------------------------

    void ES1Benchmark::RegisterAll()
    {
        using namespace JsonData::ES1Benchmark;

        Register<StringEchoParams, StringEchoResult>(
            _T("echostring"),  &ES1Benchmark::endpoint_echostring,  this);

        Register<ArrayEchoParams, ArrayEchoResult>(
            _T("echoarray"),   &ES1Benchmark::endpoint_echoarray,   this);

        Register<Core::JSON::DecUInt32, Core::JSON::DecUInt32>(
            _T("echoint32"),   &ES1Benchmark::endpoint_echoint32,   this);

        Register<Core::JSON::DecUInt64, Core::JSON::DecUInt64>(
            _T("echoint64"),   &ES1Benchmark::endpoint_echoint64,   this);

        Register<Core::JSON::Boolean, Core::JSON::Boolean>(
            _T("echobool"),    &ES1Benchmark::endpoint_echobool,    this);

        Register<Core::JSON::Float, Core::JSON::Float>(
            _T("echofloat"),   &ES1Benchmark::endpoint_echofloat,   this);

        Register<Core::JSON::Double, Core::JSON::Double>(
            _T("echodouble"),  &ES1Benchmark::endpoint_echodouble,  this);
    }

    void ES1Benchmark::UnregisterAll()
    {
        Unregister(_T("echostring"));
        Unregister(_T("echoarray"));
        Unregister(_T("echoint32"));
        Unregister(_T("echoint64"));
        Unregister(_T("echobool"));
        Unregister(_T("echofloat"));
        Unregister(_T("echodouble"));
    }

    // -----------------------------------------------------------------------
    // String echo
    // Method: echostring
    // Params: { "size": <uint32>, "value": "<string>" }
    // Result: { "echo":  "<string>" }
    // -----------------------------------------------------------------------
    uint32_t ES1Benchmark::endpoint_echostring(
        const JsonData::ES1Benchmark::StringEchoParams& params,
        JsonData::ES1Benchmark::StringEchoResult& response)
    {
        response.Echo = params.Value.Value();
        return Core::ERROR_NONE;
    }

    // -----------------------------------------------------------------------
    // Array echo
    // Method: echoarray
    // Params: { "count": <uint32>, "values": [<uint32>, ...] }
    // Result: { "echo":  [<uint32>, ...] }
    // -----------------------------------------------------------------------
    uint32_t ES1Benchmark::endpoint_echoarray(
        const JsonData::ES1Benchmark::ArrayEchoParams& params,
        JsonData::ES1Benchmark::ArrayEchoResult& response)
    {
        auto iter = params.Values.Elements();
        while (iter.Next()) {
            response.Echo.Add() = iter.Current().Value();
        }
        return Core::ERROR_NONE;
    }

    // -----------------------------------------------------------------------
    // Scalar echoes
    // -----------------------------------------------------------------------

    // Method: echoint32  — params/result: <uint32>
    uint32_t ES1Benchmark::endpoint_echoint32(
        const Core::JSON::DecUInt32& params, Core::JSON::DecUInt32& response)
    {
        response = params.Value();
        return Core::ERROR_NONE;
    }

    // Method: echoint64  — params/result: <uint64>
    uint32_t ES1Benchmark::endpoint_echoint64(
        const Core::JSON::DecUInt64& params, Core::JSON::DecUInt64& response)
    {
        response = params.Value();
        return Core::ERROR_NONE;
    }

    // Method: echobool   — params/result: <bool>
    uint32_t ES1Benchmark::endpoint_echobool(
        const Core::JSON::Boolean& params, Core::JSON::Boolean& response)
    {
        response = params.Value();
        return Core::ERROR_NONE;
    }

    // Method: echofloat  — params/result: <float>
    uint32_t ES1Benchmark::endpoint_echofloat(
        const Core::JSON::Float& params, Core::JSON::Float& response)
    {
        response = params.Value();
        return Core::ERROR_NONE;
    }

    // Method: echodouble — params/result: <double>
    uint32_t ES1Benchmark::endpoint_echodouble(
        const Core::JSON::Double& params, Core::JSON::Double& response)
    {
        response = params.Value();
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
