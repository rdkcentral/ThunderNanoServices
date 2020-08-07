/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PlayerInfo.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::PlayerInfo;

    // Registration
    //

    void PlayerInfo::RegisterAll()
    {
        Property<CodecsData>(_T("playerinfo"), &PlayerInfo::get_playerinfo, nullptr, this);
        Property<Core::JSON::EnumType<DolbyType>>(_T("dolbymode"), &PlayerInfo::get_dolbymode, &PlayerInfo::set_dolbymode, this);
    }

    void PlayerInfo::UnregisterAll()
    {
        Unregister(_T("playerinfo"));
        Unregister(_T("dolbymode"));
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

    uint32_t PlayerInfo::get_dolbymode(Core::JSON::EnumType<DolbyType>& response) const
    {
        uint32_t error = Core::ERROR_NONE;
        if(_dolbyOut != nullptr){

            Exchange::Dolby::IOutput::Type outputType = Exchange::Dolby::IOutput::Type::DIGITAL_PCM;
            error = const_cast<const Exchange::Dolby::IOutput*>(_dolbyOut)->Mode(outputType);

            response = static_cast<DolbyType>(outputType);
        } else {
            error = Core::ERROR_UNAVAILABLE;
        }
        
        return error;
    }
    
    uint32_t PlayerInfo::set_dolbymode(const Core::JSON::EnumType<DolbyType>& params)
    {
        uint32_t error = Core::ERROR_NONE;
        
        if(!params.IsSet()){
            error = Core::ERROR_GENERAL;
        }
        else if(_dolbyOut == nullptr){
            error = Core::ERROR_UNAVAILABLE;
        } else {
            error = _dolbyOut->Mode(static_cast<Exchange::Dolby::IOutput::Type>(params.Value()));
        }
        
        return error;
    }

} // namespace Plugin

}
