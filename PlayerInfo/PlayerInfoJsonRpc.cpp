#include "PlayerInfo.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::PlayerInfo;

    // Registration
    //

    void PlayerInfo::RegisterAll()
    {
        Property<CodecsData>(_T("playerinfo"), &PlayerInfo::get_playerinfo, nullptr, this);
    }

    void PlayerInfo::UnregisterAll()
    {
        Unregister(_T("playerinfo"));
    }

    // API implementation
    //

    // Property: playerinfo - Player general information
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PlayerInfo::get_playerinfo(CodecsData& response) const
    {
        Info(response);

        return Core::ERROR_NONE;
    }

} // namespace Plugin

}

