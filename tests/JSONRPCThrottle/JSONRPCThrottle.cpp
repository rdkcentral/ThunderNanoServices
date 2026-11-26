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

#include "JSONRPCThrottle.h"

#include <chrono>
#include <thread>

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<JSONRPCThrottle> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );

    } // namespace

    SERVICE_REGISTRATION(
        JSONRPCThrottle,
        1,
        0
    )

    JSONRPCThrottle::JSONRPCThrottle()
        : _totalCalls(0)
        , _activeCalls(0)
        , _maximumConcurrentCalls(0)
    {
        Register<DelayParams, void>(
            _T("delay"),
            &JSONRPCThrottle::Delay,
            this
        );

        Register<void, void>(
            _T("fast"),
            &JSONRPCThrottle::Fast,
            this
        );

        Register<void, Statistics>(
            _T("statistics"),
            &JSONRPCThrottle::GetStatistics,
            this
        );

        Register<void, void>(
            _T("reset"),
            &JSONRPCThrottle::Reset,
            this
        );
    }

    const string JSONRPCThrottle::Initialize(
        PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ASSERT(service != nullptr);

        {
            std::lock_guard<std::mutex> guard(_lock);
            _totalCalls = 0;
            _activeCalls = 0;
            _maximumConcurrentCalls = 0;
        }

        return {};
    }

    void JSONRPCThrottle::Deinitialize(
        PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ASSERT(service != nullptr);
    }

    string JSONRPCThrottle::Information() const
    {
        return _T("JSON-RPC concurrency throttling test plugin");
    }

    void JSONRPCThrottle::Enter()
    {
        std::lock_guard<std::mutex> guard(_lock);

        ++_activeCalls;
        ++_totalCalls;

        if (_activeCalls > _maximumConcurrentCalls) {
            _maximumConcurrentCalls = _activeCalls;
        }
    }

    void JSONRPCThrottle::Leave()
    {
        std::lock_guard<std::mutex> guard(_lock);

        ASSERT(_activeCalls > 0);
        --_activeCalls;
    }

    JSONRPCThrottle::Counters JSONRPCThrottle::Snapshot() const
    {
        std::lock_guard<std::mutex> guard(_lock);

        return { _totalCalls, _activeCalls, _maximumConcurrentCalls };
    }

    uint32_t JSONRPCThrottle::Delay(
        const Core::JSONRPC::Context& context VARIABLE_IS_NOT_USED,
        const DelayParams& params)
    {
        std::cout
            << "[PLUGIN] ENTER thread="
            << std::this_thread::get_id()
            << std::endl;
        Enter();

        const Counters counters = Snapshot();

        std::cout
            << "[PLUGIN] ACTIVE="
            << counters.Active
            << " MAX="
            << counters.Maximum
            << " thread="
            << std::this_thread::get_id()
            << std::endl;

        std::this_thread::sleep_for(
            std::chrono::milliseconds(
                params.Milliseconds.Value()
            )
        );

        std::cout
            << "[PLUGIN] EXIT thread="
            << std::this_thread::get_id()
            << std::endl;

        Leave();

        return Core::ERROR_NONE;
    }

    uint32_t JSONRPCThrottle::Fast(
        const Core::JSONRPC::Context& context VARIABLE_IS_NOT_USED)
    {
        Enter();
        Leave();

        return Core::ERROR_NONE;
    }

    uint32_t JSONRPCThrottle::GetStatistics(
        const Core::JSONRPC::Context& context VARIABLE_IS_NOT_USED,
        Statistics& response)
    {
        const Counters counters = Snapshot();

        response.TotalCalls = counters.Total;
        response.ActiveCalls = counters.Active;
        response.MaximumConcurrentCalls = counters.Maximum;

        return Core::ERROR_NONE;
    }

    uint32_t JSONRPCThrottle::Reset(
        const Core::JSONRPC::Context& context VARIABLE_IS_NOT_USED)
    {
        std::lock_guard<std::mutex> guard(_lock);

        // Clearing while a call is in flight would corrupt the statistics and
        // make that call's Leave() wrap _activeCalls below zero.
        if (_activeCalls != 0) {
            return Core::ERROR_INPROGRESS;
        }

        _totalCalls = 0;
        _maximumConcurrentCalls = 0;

        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace Thunder