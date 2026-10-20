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

#include "QASecurityTargetAlt.h"

namespace Thunder {
namespace Plugin {

namespace {

static Metadata<QASecurityTargetAlt> metadata(
    // Version
    1, 0, 0,
    // Preconditions
    { PluginHost::ISubSystem::SECURITY },
    // Terminations
    {},
    // Controls
    {}
);

} // namespace


void QASecurityTargetAlt::RegisterAll()
{
    Register<void, void>(
        _T("echo"),
        &QASecurityTargetAlt::endpoint_echo,
        this);
}


void QASecurityTargetAlt::UnregisterAll()
{
    Unregister(_T("echo"));
}


uint32_t QASecurityTargetAlt::endpoint_echo()
{
    return Core::ERROR_NONE;
}


const string QASecurityTargetAlt::Initialize(
    PluginHost::IShell* service VARIABLE_IS_NOT_USED)
{
    ASSERT(service != nullptr);

    RegisterAll();

    return {};
}


void QASecurityTargetAlt::Deinitialize(
    PluginHost::IShell* service VARIABLE_IS_NOT_USED)
{
    ASSERT(service != nullptr);

    UnregisterAll();
}


string QASecurityTargetAlt::Information() const
{
    return _T("QA Security Target");
}

void QASecurityTargetAlt::Inbound(
    Web::Request& request VARIABLE_IS_NOT_USED)
{
    // No special processing is required.
}

Core::ProxyType<Web::Response> QASecurityTargetAlt::Process(
    const Web::Request& request)
{
    Core::ProxyType<Web::Response> response =
        Core::ProxyType<Web::Response>::Create();

    if (request.Path == "/Service/QASecurityTargetAlt/http") {
        response->ErrorCode = Web::STATUS_OK;
    } else {
        response->ErrorCode = Web::STATUS_NOT_FOUND;
    }

    return response;
}

} // namespace Plugin
} // namespace Thunder
