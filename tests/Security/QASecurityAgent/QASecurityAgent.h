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