#include "Commands.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN( Plugin::Command::PluginControl::Config::state)

    { Plugin::Command::PluginControl::Config::state::ACTIVATE, _TXT("activate") },
    { Plugin::Command::PluginControl::Config::state::DEACTIVATE, _TXT("deactivate") },

ENUM_CONVERSION_END( Plugin::Command::PluginControl::Config::state);

}
