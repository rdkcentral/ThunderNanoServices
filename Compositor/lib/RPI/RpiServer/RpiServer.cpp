#include "RpiServer.h"

namespace WPEFramework {
namespace Rpi {

Platform::Platform(const string& callsign, IStateChange* stateChanges,
        IClient* clientChanges, const std::string& configuration) {
    fprintf(stderr, "RpiServer::: Platform::Platform\n");
}

Platform::~Platform() {
    fprintf(stderr, "RpiServer::: Platform::~Platform\n");
}
}
} // WPEFramework
