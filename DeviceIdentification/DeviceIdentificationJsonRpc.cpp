
#include "DeviceIdentification.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::DeviceIdentification;

    // Registration
    //

    void DeviceIdentification::RegisterAll()
    {
        Property<DeviceidentificationData>(_T("deviceidentification"), &DeviceIdentification::get_deviceidentification, nullptr, this);
    }

    void DeviceIdentification::UnregisterAll()
    {
        Unregister(_T("deviceidentification"));
    }

    // API implementation
    //

    // Property: deviceidentification - Device paltform specific information
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t DeviceIdentification::get_deviceidentification(DeviceidentificationData& response) const
    {
        // response = ...

        return Core::ERROR_NONE;
    }

} // namespace Plugin

}

