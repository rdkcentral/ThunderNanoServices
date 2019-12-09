
#include "DisplayInfo.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::DisplayInfo;

    // Registration
    //

    void DisplayInfo::RegisterAll()
    {
        Property<SysteminfoData>(_T("systeminfo"), &DisplayInfo::get_systeminfo, nullptr, this);
        Property<Core::JSON::ArrayType<AddressesData>>(_T("addresses"), &DisplayInfo::get_addresses, nullptr, this);
        Property<SocketinfoData>(_T("socketinfo"), &DisplayInfo::get_socketinfo, nullptr, this);
    }

    void DisplayInfo::UnregisterAll()
    {
        Unregister(_T("socketinfo"));
        Unregister(_T("addresses"));
        Unregister(_T("systeminfo"));
    }

    // API implementation
    //

    // Property: systeminfo - System general information
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t DisplayInfo::get_systeminfo(SysteminfoData& response) const
    {
        SysInfo(response);
        return Core::ERROR_NONE;
    }

    // Property: addresses - Network interface addresses
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t DisplayInfo::get_addresses(Core::JSON::ArrayType<AddressesData>& response) const
    {
        AddressInfo(response);
        return Core::ERROR_NONE;
    }

    // Property: socketinfo - Socket information
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t DisplayInfo::get_socketinfo(SocketinfoData& response) const
    {
        SocketPortInfo(response);
        return Core::ERROR_NONE;
    }

} // namespace Plugin

}

