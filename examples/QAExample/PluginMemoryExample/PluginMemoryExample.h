#pragma once

#include "Module.h"
#include "core/Portability.h"
#include "core/Services.h"

namespace Thunder {
namespace Plugin {

    class PluginMemoryExample : public PluginHost::IPlugin {
    public:
        PluginMemoryExample() = default;
        ~PluginMemoryExample() override = default;

        BEGIN_INTERFACE_MAP(PluginMemoryExample)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        // IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
    };

} // namespace Plugin
} // namespace Thunder
