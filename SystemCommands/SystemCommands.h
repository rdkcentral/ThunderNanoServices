#pragma once

#include "Module.h"
#include <interfaces/ISystemCommands.h>

#include "SystemCommandsImplementation.h"

namespace WPEFramework {
namespace Plugin {

    class SystemCommands : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::ISystemCommands {
    public:
        SystemCommands() = default;
        ~SystemCommands() override = default;

        BEGIN_INTERFACE_MAP(SystemCommands)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(Exchange::ISystemCommands)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IWeb methods
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        // SystemCommands methods
        uint32_t USBReset(const std::string& device) override;

    private:
        SystemCommands(const SystemCommands&) = delete;
        SystemCommands& operator=(const SystemCommands&) = delete;

        SystemCommandsImplementation _implemetation;

        uint8_t _skipURL = 0;
    };

} // namespace Plugin
} // namespace WPEFramework
