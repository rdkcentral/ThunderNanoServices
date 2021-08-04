#include "DataContainer.h"

ENUM_CONVERSION_BEGIN(WPEFramework::DataContainer::CommandType)

    { WPEFramework::DataContainer::ExecuteShell, _TXT("ExecuteShell") },
    { WPEFramework::DataContainer::WiFiSettings, _TXT("WiFiSettings") },
    { WPEFramework::DataContainer::FanControl, _TXT("FanControl") },
    { WPEFramework::DataContainer::PlayerControl, _TXT("PlayerControl") },

    ENUM_CONVERSION_END(WPEFramework::DataContainer::CommandType)
