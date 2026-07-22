
#include "JsonRpcExample.h"
#include <iostream>

namespace Thunder {
namespace Plugin {
    namespace {
    static Metadata<JsonRpcExample> metadata(
        1, 0, 0,
        {},
        {},
        {}
    );
}


const string JsonRpcExample::Initialize(PluginHost::IShell*)
{
    std::cout << "JsonRpcExample Initialized" << std::endl;
    return string();
}

void JsonRpcExample::Deinitialize(PluginHost::IShell*)
{
    std::cout << "JsonRpcExample Deinitialized" << std::endl;
}

string JsonRpcExample::Information() const
{
    return "JSONRPC base-only plugin for memory measurement";
}

}
}

