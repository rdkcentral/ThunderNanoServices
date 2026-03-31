#include "PluginMemoryExample.h"
#include "core/Portability.h"
#include <iostream>

namespace Thunder {
namespace Plugin {
    namespace {

        static Metadata<PluginMemoryExample> metadata(
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

    const string PluginMemoryExample::Initialize(PluginHost::IShell*)
    {
        std::cout<<"PluginMemory has been executed"<<"\n";
        return string();
    }
    void PluginMemoryExample::Deinitialize(PluginHost::IShell*)
    {
        std::cout << "PluginMemory has been deinitialized" << "\n";
    }

    string PluginMemoryExample::Information() const
    {
        return "Plugin used to measure  .so size";
    }
}
}