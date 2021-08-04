#include "DataContainer.h"

namespace WPEFramework {
    ENUM_CONVERSION_BEGIN(DataContainer::CommandType)

        { DataContainer::ExecuteShell, _TXT("ExecuteShell") },
        { DataContainer::WiFiSettings, _TXT("WiFiSettings") },
        { DataContainer::FanControl, _TXT("FanControl") },
        { DataContainer::PlayerControl, _TXT("PlayerControl") },

    ENUM_CONVERSION_END(DataContainer::CommandType)
}
