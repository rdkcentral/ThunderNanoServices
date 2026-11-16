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

#pragma once

#include <open_telemetry/open_telemetry.h>

#include <chrono>
#include <cstdio>
#include <memory>
#include <string>

namespace Thunder {
namespace Telemetry {

    // Wraps OpenTelemetry::Span and prints to the console how long it took from
    // calling StartSpan() until End() completed, including the cost of both calls.
    class TimedSpan {
    private:
        using Clock = std::chrono::steady_clock;

    public:
        TimedSpan() = delete;
        TimedSpan(const TimedSpan&) = delete;
        TimedSpan& operator=(const TimedSpan&) = delete;
        TimedSpan(TimedSpan&&) = delete;
        TimedSpan& operator=(TimedSpan&&) = delete;

        explicit TimedSpan(const char name[])
            : _name(name)
            , _begin(Clock::now())
            , _span(OpenTelemetry::Instance().StartSpan(name))
            , _started(Clock::now())
            , _ended(false)
        {
        }

        ~TimedSpan()
        {
            End();
        }

        OpenTelemetry::Span* operator->()
        {
            return _span.get();
        }

        OpenTelemetry::Span& operator*()
        {
            return *_span;
        }

        void End()
        {
            // Only measure/print the first End(); the destructor calls End() again.
            if ((_span != nullptr) && (_ended == false)) {
                _ended = true;
                const Clock::time_point ending = Clock::now();
                // OpenTelemetry::Span::End() is not idempotent and ~Span() always calls it, so
                // release the only reference to end the span exactly once.
                _span->End();
                const Clock::time_point ended = Clock::now();

                fprintf(stderr, "[SpanTiming] %s: total %lld us (StartSpan %lld us, body %lld us, End %lld us)\n",
                    _name.c_str(),
                    Microseconds(_begin, ended),
                    Microseconds(_begin, _started),
                    Microseconds(_started, ending),
                    Microseconds(ending, ended));
            }
        }

    private:
        static long long Microseconds(const Clock::time_point& from, const Clock::time_point& to)
        {
            return static_cast<long long>(std::chrono::duration_cast<std::chrono::microseconds>(to - from).count());
        }

    private:
        const std::string _name;
        const Clock::time_point _begin;
        std::shared_ptr<OpenTelemetry::Span> _span;
        const Clock::time_point _started;
        bool _ended;
    };

} // namespace Telemetry
} // namespace Thunder
