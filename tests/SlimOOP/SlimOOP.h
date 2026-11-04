#pragma once

#include "Module.h"

namespace Thunder {
namespace Plugin {

    class SlimOOP : public PluginHost::IPlugin {
    public:
        SlimOOP() = default;
        SlimOOP(const SlimOOP&) = delete;
        SlimOOP& operator=(const SlimOOP&) = delete;
        SlimOOP(SlimOOP&&) = delete;
        SlimOOP& operator=(SlimOOP&&) = delete;
        ~SlimOOP() override = default;

        BEGIN_INTERFACE_MAP(SlimOOP)
            INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        Core::IUnknown* _implementation = nullptr;
        uint32_t _connectionId = 0;
    };

}
}