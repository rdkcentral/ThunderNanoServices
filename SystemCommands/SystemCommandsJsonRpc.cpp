
// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#include <interfaces/json/JsonData_SystemCommands.h>
#include "SystemCommands.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::SystemCommands;

    // Registration
    //

    void SystemCommands::RegisterAll()
    {
        Register<UsbresetParamsData,void>(_T("usbreset"), &SystemCommands::endpoint_usbreset, this);
    }

    void SystemCommands::UnregisterAll()
    {
        Unregister(_T("usbreset"));
    }

    // API implementation
    //

    // Resets a USB device.
    uint32_t SystemCommands::endpoint_usbreset(const UsbresetParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if(params.Device.IsSet() == true) {
            result = USBReset(params.Device.Value());
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        return result;
    }

} // namespace Plugin

} // namespace WPEFramework

