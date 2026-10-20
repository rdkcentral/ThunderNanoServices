#pragma once

#include "Module.h"

#include <plugins/IPlugin.h>

namespace Thunder {
namespace Plugin {

class QASecurityTargetAlt : public PluginHost::IPlugin
                          , public PluginHost::JSONRPC
                          , public PluginHost::IWeb {
private:
    QASecurityTargetAlt(const QASecurityTargetAlt&) = delete;
    QASecurityTargetAlt& operator=(const QASecurityTargetAlt&) = delete;

public:
    QASecurityTargetAlt() = default;
    ~QASecurityTargetAlt() override = default;

    BEGIN_INTERFACE_MAP(QASecurityTargetAlt)
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