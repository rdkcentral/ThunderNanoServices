#pragma once

#include "Module.h"

#include <interfaces/IMath.h>
#include <interfaces/json/JMath.h>

namespace Thunder {
namespace Plugin {

    class TestSmartConsumer
        : public PluginHost::IPlugin
        , public PluginHost::JSONRPC
        , public Exchange::IMath
        , public RPC::PluginSmartInterfaceType<Exchange::IMath> {
    private:
        using SmartMath = RPC::PluginSmartInterfaceType<Exchange::IMath>;

        class Config : public Core::JSON::Container {
        public:
            Config();

            Core::JSON::String ProviderCallsign;
        };

    public:
        TestSmartConsumer();
        ~TestSmartConsumer() override;

        TestSmartConsumer(const TestSmartConsumer&) = delete;
        TestSmartConsumer& operator=(const TestSmartConsumer&) = delete;

        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        uint32_t Add(const uint16_t a, const uint16_t b, uint16_t& sum) const override;
        uint32_t Sub(const uint16_t a, const uint16_t b, uint16_t& sum) const override;

        BEGIN_INTERFACE_MAP(TestSmartConsumer)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        string _providerCallsign;
    };

} // namespace Plugin
} // namespace Thunder
