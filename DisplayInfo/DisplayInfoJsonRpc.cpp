
#include "DisplayInfo.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::DisplayInfo;

    // Registration
    //

    void DisplayInfo::RegisterAll()
    {
        Property<DisplayinfoData>(_T("displayinfo"), &DisplayInfo::get_displayinfo, nullptr, this);
    }

    void DisplayInfo::UnregisterAll()
    {
        Unregister(_T("displayinfo"));
    }

    // API implementation
    //

    // Property: systeminfo - System general information
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t DisplayInfo::get_displayinfo(DisplayinfoData& response) const
    {
        Info(response);
        return Core::ERROR_NONE;
    }

} // namespace Plugin

}

