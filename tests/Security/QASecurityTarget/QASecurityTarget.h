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

namespace Thunder {
namespace Plugin {

class QASecurityTarget : public PluginHost::IPlugin
                          , public PluginHost::JSONRPC
                          , public PluginHost::IWeb {
private:
    QASecurityTarget(const QASecurityTarget&) = delete;
    QASecurityTarget& operator=(const QASecurityTarget&) = delete;

public:
    QASecurityTarget() = default;
    ~QASecurityTarget() override = default;

    BEGIN_INTERFACE_MAP(QASecurityTarget)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_ENTRY(PluginHost::IWeb)
    END_INTERFACE_MAP

public:
    const string Initialize(
        PluginHost::IShell* service) override;

    void Deinitialize(
        PluginHost::IShell* service) override;

    string Information() const override;

    void Inbound(
        Web::Request& request) override;

    Core::ProxyType<Web::Response> Process(
        const Web::Request& request) override;

private:
    void RegisterAll();
    void UnregisterAll();

    uint32_t endpoint_echo();
};

} // namespace Plugin
} // namespace Thunder