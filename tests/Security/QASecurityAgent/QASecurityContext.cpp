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

#include "QASecurityContext.h"

namespace Thunder {
namespace Plugin {

QASecurityContext::QASecurityContext(
    const string& token,
    const Policy& policy)
    : _token(token)
    , _policy(policy)
    , _pathCalls(0)
    , _httpCalls(0)
    , _jsonRpcCalls(0)
{
}

bool QASecurityContext::Allowed(const string& path VARIABLE_IS_NOT_USED) const
{
    ++_pathCalls;

    return (_policy.path == Decision::ALLOW);
}

bool QASecurityContext::Allowed(
    const Web::Request& request VARIABLE_IS_NOT_USED) const
{
    ++_httpCalls;

    return (_policy.http == Decision::ALLOW);
}

bool QASecurityContext::Allowed(
    const Core::JSONRPC::Message& message VARIABLE_IS_NOT_USED) const
{
    ++_jsonRpcCalls;

    return (_policy.jsonrpc == Decision::ALLOW);
}

string QASecurityContext::Token() const
{
    return _token;
}

uint32_t QASecurityContext::PathCalls() const
{
    return _pathCalls.load();
}

uint32_t QASecurityContext::HttpCalls() const
{
    return _httpCalls.load();
}

uint32_t QASecurityContext::JsonRpcCalls() const
{
    return _jsonRpcCalls.load();
}

} // namespace Plugin
} // namespace Thunder
