#pragma once

#include "Module.h"

#include <interfaces/IMath.h>

namespace Thunder {
namespace Plugin {

    class TestSmartProvider : public PluginHost::IPlugin, public Exchange::IMath {
    public:
        TestSmartProvider() = default;
        ~TestSmartProvider() override = default;

        TestSmartProvider(const TestSmartProvider&) = delete;
        TestSmartProvider& operator=(const TestSmartProvider&) = delete;

        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        uint32_t Add(const uint16_t a, const uint16_t b, uint16_t& sum) const override;
        uint32_t Sub(const uint16_t a, const uint16_t b, uint16_t& sum) const override;

        BEGIN_INTERFACE_MAP(TestSmartProvider)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP
    };

} // namespace Plugin
} // namespace Thunder
