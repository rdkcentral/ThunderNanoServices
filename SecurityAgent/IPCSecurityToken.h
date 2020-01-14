#pragma once

#include <core/core.h>

using namespace WPEFramework;

namespace IPC {

namespace SecurityAgent {

    typedef Core::IPCMessageType<10, ::IPC::Buffer, IPC::Buffer> TokenData;
}

} // namespace IPC::SecurityAgent
