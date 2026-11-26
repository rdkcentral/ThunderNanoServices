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

#include "Module.h"

#include <plugins/plugins.h>

#include <mutex>

namespace Thunder {
namespace Plugin {

class JSONRPCThrottle : public PluginHost::IPlugin,
                        public PluginHost::JSONRPC {
public:
    JSONRPCThrottle(const JSONRPCThrottle&) = delete;
    JSONRPCThrottle& operator=(const JSONRPCThrottle&) = delete;

    JSONRPCThrottle();
    ~JSONRPCThrottle() override = default;

    BEGIN_INTERFACE_MAP(JSONRPCThrottle)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
    END_INTERFACE_MAP

public:
    // IPlugin
    const string Initialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED) override;
    void Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED) override;
    string Information() const override;

private:
    class DelayParams : public Core::JSON::Container {
    public:
        DelayParams()
            : Core::JSON::Container()
            , Milliseconds(1000)
        {
            Add(_T("milliseconds"), &Milliseconds);
        }

        Core::JSON::DecUInt32 Milliseconds;
    };

    class Statistics : public Core::JSON::Container {
    public:
        Statistics()
            : Core::JSON::Container()
            , TotalCalls(0)
            , ActiveCalls(0)
            , MaximumConcurrentCalls(0)
        {
            Add(_T("totalCalls"), &TotalCalls);
            Add(_T("activeCalls"), &ActiveCalls);
            Add(_T("maximumConcurrentCalls"), &MaximumConcurrentCalls);
        }

        Core::JSON::DecUInt64 TotalCalls;
        Core::JSON::DecUInt64 ActiveCalls;
        Core::JSON::DecUInt64 MaximumConcurrentCalls;
    };

private:
    uint32_t Delay(
        const Core::JSONRPC::Context& context VARIABLE_IS_NOT_USED,
        const DelayParams& params);

    uint32_t Fast(
        const Core::JSONRPC::Context& context VARIABLE_IS_NOT_USED);

    uint32_t GetStatistics(
        const Core::JSONRPC::Context& context VARIABLE_IS_NOT_USED,
        Statistics& response);

    uint32_t Reset(
        const Core::JSONRPC::Context& context VARIABLE_IS_NOT_USED);

    struct Counters {
        uint64_t Total;
        uint64_t Active;
        uint64_t Maximum;
    };

    void Enter();
    void Leave();
    Counters Snapshot() const;

private:
    // All counters change together under _lock, so Reset() can verify that no
    // measured call is active and clear the counters as one atomic step. The
    // lock is held only for the counter update, never while a call is delayed.
    mutable std::mutex _lock;
    uint64_t _totalCalls;
    uint64_t _activeCalls;
    uint64_t _maximumConcurrentCalls;
};

} // namespace Plugin
} // namespace Thunder