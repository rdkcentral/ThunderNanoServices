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

#include <atomic>

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
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
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
        const Core::JSONRPC::Context& context,
        const DelayParams& params);

    uint32_t Fast(
        const Core::JSONRPC::Context& context);

    uint32_t GetStatistics(
        const Core::JSONRPC::Context& context,
        Statistics& response);

    uint32_t Reset(
        const Core::JSONRPC::Context& context);

    void Enter();
    void Leave();

private:
    std::atomic<uint64_t> _totalCalls;
    std::atomic<uint64_t> _activeCalls;
    std::atomic<uint64_t> _maximumConcurrentCalls;
};

} // namespace Plugin
} // namespace Thunder