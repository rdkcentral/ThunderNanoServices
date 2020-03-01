#include "Containers.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(Containers, 1, 0);

    const string Containers::Initialize(PluginHost::IShell* service) 
    {
        return (string());
    }

    void Containers::Deinitialize(PluginHost::IShell* service) 
    {
        
    }

    string Containers::Information() const 
    {
        return (string());
    }
}
}