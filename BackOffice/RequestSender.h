#pragma once

#include "Module.h"
#include <iostream>

namespace WPEFramework {
namespace Plugin {
    class RequestSender {

    public:
        void Send(PluginHost::IShell::state state, const string& callsign)
        {
            std::cout << "CALLSIGN: " << callsign << " "
                      << "STATE: " << state << std::endl;
        }
    };
}
}