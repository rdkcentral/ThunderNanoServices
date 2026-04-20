#pragma once

#include "Module.h"

#include <qa_interfaces/ITestPlugin.h>
#include <qa_interfaces/json/JTestPlugin.h>

namespace Thunder {
namespace Plugin {

    class TestPlugin : public PluginHost::IPlugin,
                        public PluginHost::JSONRPC,
                        public QualityAssurance::ITestPlugin {
    public:
        TestPlugin() = default;
        ~TestPlugin() override = default;

        TestPlugin(const TestPlugin&) = delete;
        TestPlugin& operator=(const TestPlugin&) = delete;

        // IPlugin
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override { return {}; }

        // ITestPlugin
        uint32_t Echo(const string& input, string& output) override;
        uint32_t Greet(const string& name, string& message) override;

        BEGIN_INTERFACE_MAP(TestPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(QualityAssurance::ITestPlugin)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service = nullptr;
    };

} // namespace Plugin
} // namespace Thunder
