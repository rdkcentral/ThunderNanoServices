#include "RemoteAdministrator.h"

namespace WPEFramework {
namespace Remotes {

/* static */ RemoteAdministrator& RemoteAdministrator::Instance() {
    static RemoteAdministrator singleton;
    return (singleton);
}

}
}
