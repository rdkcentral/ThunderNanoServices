#pragma once

#include "Module.h"
#include <plugins/plugins.h>

namespace Thunder {
namespace Plugin {

class JsonRpcExample : public PluginHost::IPlugin, public PluginHost::JSONRPC {
public:
    JsonRpcExample() = default;
    ~JsonRpcExample() override = default;

    BEGIN_INTERFACE_MAP(JsonRpcExample)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
    END_INTERFACE_MAP

    const string Initialize(PluginHost::IShell*) override;
    void Deinitialize(PluginHost::IShell*) override;
    string Information() const override;
};

}
}
