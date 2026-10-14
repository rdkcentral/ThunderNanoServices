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
#include <plugins/ISubSystem.h>

#include "QASecurityContext.h"

namespace Thunder {
namespace Plugin {

enum class TokenPolicy : uint8_t {
    DENY,
    PATH_ALLOW,
    HTTP_ALLOW,
    JSONRPC_ALLOW,
    ALL_ALLOW
};

class QASecurityAgent : public PluginHost::IPlugin
                      , public PluginHost::IAuthenticate {
private:
    class Notification : public PluginHost::IPlugin::INotification {
    public:
        Notification() = default;
        ~Notification() override = default;

        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

        void Activated(
            const string& callsign,
            PluginHost::IShell* plugin) override;

        void Deactivated(
            const string& callsign,
            PluginHost::IShell* plugin) override;

        void Unavailable(
            const string& callsign,
            PluginHost::IShell* plugin) override;

        BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP
    };

private:
    QASecurityAgent(const QASecurityAgent&) = delete;
    QASecurityAgent& operator=(const QASecurityAgent&) = delete;

public:
    QASecurityAgent() = default;
    ~QASecurityAgent() override = default;

    BEGIN_INTERFACE_MAP(QASecurityAgent)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IAuthenticate)
    END_INTERFACE_MAP

public:
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

    Core::hresult CreateToken(
        const uint16_t length,
        const uint8_t buffer[],
        string& token) override;

    PluginHost::ISecurity* Officer(
        const string& token) override;

    static void SetPolicy(
        const QASecurityContext::Policy& policy);

private:
    static bool DecodeToken(
        const string& token,
        TokenPolicy& policy);

    static bool EncodeToken(
        const TokenPolicy policy,
        string& token);

    static QASecurityContext::Policy _policy;

    Core::Sink<Notification> _notification;
};

} // namespace Plugin
} // namespace Thunder