#pragma once

#include "Module.h"

#include <qa_interfaces/ITestPlugin.h>
#include <qa_interfaces/json/JTestPlugin.h>
#include <algorithm>
#include <vector>

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
        Core::hresult Echo(const string& input, string& output) override;
        Core::hresult Greet(const string& name, string& message) override;
        Core::hresult Register(ITestPlugin::INotification* notification) override;
        Core::hresult Unregister(ITestPlugin::INotification* notification) override;

        BEGIN_INTERFACE_MAP(TestPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(QualityAssurance::ITestPlugin)
        END_INTERFACE_MAP

    private:
        void NotifyGreeting(const string& message);

        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service = nullptr;
        std::vector<QualityAssurance::ITestPlugin::INotification*> _notifications;
    };

} // namespace Plugin
} // namespace Thunder
