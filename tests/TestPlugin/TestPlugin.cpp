#include "Module.h"
#include "TestPlugin.h"

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<TestPlugin> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    const string TestPlugin::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        _service = service;

        QualityAssurance::JTestPlugin::Register(*this, this);

        return {};
    }

    void TestPlugin::Deinitialize(PluginHost::IShell* /* service */)
    {
        QualityAssurance::JTestPlugin::Unregister(*this);
        _service = nullptr;
    }

    uint32_t TestPlugin::Echo(const string& input, string& output)
    {
        output = input;
        return Core::ERROR_NONE;
    }

    uint32_t TestPlugin::Greet(const string& name, string& message)
    {
        if (name.empty()) {
            message = "Hello, World!";
        } else {
            message = "Hello, " + name + "!";
        }
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace Thunder
