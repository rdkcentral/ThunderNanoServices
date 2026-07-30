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

// ---------------------------------------------------------------------------
// Shared JSON wire types used by both the ES1Benchmark plugin (server side)
// and the ES1BenchmarkClient executable (client side).
// Include this header in both without linking one against the other.
// ---------------------------------------------------------------------------

#include <core/core.h>

namespace WPEFramework {
namespace JsonData {
namespace ES1Benchmark {

    // -----------------------------------------------------------------------
    // echostring:  { "size": <uint32>, "value": <string> }
    //           -> { "echo": <string> }
    // -----------------------------------------------------------------------
    struct StringEchoParams : public Core::JSON::Container {
        StringEchoParams()
            : Core::JSON::Container()
            , Size(0)
            , Value()
        {
            Add(_T("size"),  &Size);
            Add(_T("value"), &Value);
        }
        StringEchoParams(const StringEchoParams&) = delete;
        StringEchoParams& operator=(const StringEchoParams&) = delete;

        Core::JSON::DecUInt32 Size;
        Core::JSON::String    Value;
    };

    struct StringEchoResult : public Core::JSON::Container {
        StringEchoResult()
            : Core::JSON::Container()
            , Echo()
        {
            Add(_T("echo"), &Echo);
        }
        StringEchoResult(const StringEchoResult&) = delete;
        StringEchoResult& operator=(const StringEchoResult&) = delete;

        Core::JSON::String Echo;
    };

    // -----------------------------------------------------------------------
    // echoarray:  { "count": <uint32>, "values": [<uint32>, ...] }
    //          -> { "echo":  [<uint32>, ...] }
    // -----------------------------------------------------------------------
    struct ArrayEchoParams : public Core::JSON::Container {
        ArrayEchoParams()
            : Core::JSON::Container()
            , Count(0)
            , Values()
        {
            Add(_T("count"),  &Count);
            Add(_T("values"), &Values);
        }
        ArrayEchoParams(const ArrayEchoParams&) = delete;
        ArrayEchoParams& operator=(const ArrayEchoParams&) = delete;

        Core::JSON::DecUInt32                       Count;
        Core::JSON::ArrayType<Core::JSON::DecUInt32> Values;
    };

    struct ArrayEchoResult : public Core::JSON::Container {
        ArrayEchoResult()
            : Core::JSON::Container()
            , Echo()
        {
            Add(_T("echo"), &Echo);
        }
        ArrayEchoResult(const ArrayEchoResult&) = delete;
        ArrayEchoResult& operator=(const ArrayEchoResult&) = delete;

        Core::JSON::ArrayType<Core::JSON::DecUInt32> Echo;
    };

} // namespace ES1Benchmark
} // namespace JsonData
} // namespace WPEFramework
