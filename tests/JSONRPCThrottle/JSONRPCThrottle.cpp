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
        PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        _totalCalls = 0;
        _activeCalls = 0;
        _maximumConcurrentCalls = 0;

        return {};
    }

    void JSONRPCThrottle::Deinitialize(
        PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
    }

    string JSONRPCThrottle::Information() const
    {
        return _T("JSON-RPC concurrency throttling test plugin");
    }

    void JSONRPCThrottle::Enter()
    {
        const uint64_t active =
            _activeCalls.fetch_add(1) + 1;

        _totalCalls.fetch_add(1);

        uint64_t maximum =
            _maximumConcurrentCalls.load();

        while (
            active > maximum &&
            !_maximumConcurrentCalls.compare_exchange_weak(
                maximum,
                active
            )
        ) {
        }
    }

    void JSONRPCThrottle::Leave()
    {
        _activeCalls.fetch_sub(1);
    }

    uint32_t JSONRPCThrottle::Delay(
        const Core::JSONRPC::Context& /*context*/,
        const DelayParams& params)
    {
        std::cout
            << "[PLUGIN] ENTER thread="
            << std::this_thread::get_id()
            << std::endl;
        Enter();

        std::cout
            << "[PLUGIN] ACTIVE="
            << _activeCalls
            << " MAX="
            << _maximumConcurrentCalls
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
        const Core::JSONRPC::Context& /*context*/)
    {
        Enter();
        Leave();

        return Core::ERROR_NONE;
    }

    uint32_t JSONRPCThrottle::GetStatistics(
        const Core::JSONRPC::Context& /*context*/,
        Statistics& response)
    {
        response.TotalCalls =
            _totalCalls.load();

        response.ActiveCalls =
            _activeCalls.load();

        response.MaximumConcurrentCalls =
            _maximumConcurrentCalls.load();

        return Core::ERROR_NONE;
    }

    uint32_t JSONRPCThrottle::Reset(
        const Core::JSONRPC::Context& /*context*/)
    {
        _totalCalls = 0;
        _activeCalls = 0;
        _maximumConcurrentCalls = 0;

        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace Thunder