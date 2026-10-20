/*
 * If not stated otherwise in this file or this component's license file the
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

#include <plugins/IPlugin.h>

#include <atomic>

namespace Thunder {
namespace Plugin {

class QASecurityContext : public PluginHost::ISecurity {
public:
    enum class Decision : uint8_t {
        DENY = 0,
        ALLOW = 1
    };

    struct Policy {
        Decision path;
        Decision http;
        Decision jsonrpc;

        Policy()
            : path(Decision::DENY)
            , http(Decision::DENY)
            , jsonrpc(Decision::DENY)
        {
        }
    };

public:
    QASecurityContext(
        const string& token,
        const Policy& policy);

    QASecurityContext(const QASecurityContext&) = delete;
    QASecurityContext& operator=(const QASecurityContext&) = delete;

    ~QASecurityContext() override = default;

    BEGIN_INTERFACE_MAP(QASecurityContext)
        INTERFACE_ENTRY(PluginHost::ISecurity)
    END_INTERFACE_MAP

public:
    bool Allowed(const string& path) const override;

    bool Allowed(const Web::Request& request) const override;

    bool Allowed(
        const Core::JSONRPC::Message& message) const override;

    string Token() const override;

    uint32_t AddRef() const override
    {
        return (Core::ERROR_NONE);
    }

    uint32_t Release() const override
    {
        return (Core::ERROR_NONE);
    }

    uint32_t PathCalls() const;
    uint32_t HttpCalls() const;
    uint32_t JsonRpcCalls() const;

private:
    string _token;
    Policy _policy;

    mutable std::atomic<uint32_t> _pathCalls;
    mutable std::atomic<uint32_t> _httpCalls;
    mutable std::atomic<uint32_t> _jsonRpcCalls;
};

} // namespace Plugin
} // namespace Thunder