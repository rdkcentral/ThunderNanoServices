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
 
#include "Module.h"
#include "Compositor.h"
#include <interfaces/json/JsonData_Compositor.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::Compositor;

    // Registration
    //

    void Compositor::RegisterAll()
    {
        Register<PutontopParamsInfo,void>(_T("putontop"), &Compositor::endpoint_putontop, this);
        Register<PutontopParamsInfo,void>(_T("select"), &Compositor::endpoint_select, this);
        Register<PutbelowParamsData,void>(_T("putbelow"), &Compositor::endpoint_putbelow, this);
        Property<Core::JSON::EnumType<ResolutionType>>(_T("resolution"), &Compositor::get_resolution, &Compositor::set_resolution, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("zorder"), &Compositor::get_zorder, nullptr, this);
        Property<GeometryData>(_T("geometry"), &Compositor::get_geometry, &Compositor::set_geometry, this);
        Property<Core::JSON::EnumType<VisiblityType>>(_T("visiblity"), nullptr, &Compositor::set_visiblity, this);
        Property<Core::JSON::DecUInt8>(_T("opacity"), nullptr, &Compositor::set_opacity, this);

        // Deprecated call, not documented, to be removed if the ThunderUI is adapted!!!
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("clients"), &Compositor::get_zorder, nullptr, this);
    }

    void Compositor::UnregisterAll()
    {
        // Deprecated call, not documented, to be removed if the ThunderUI is adapted!!!
        Unregister(_T("clients"));  

        Unregister(_T("opacity"));
        Unregister(_T("visiblity"));
        Unregister(_T("geometry"));
        Unregister(_T("zorder"));
        Unregister(_T("resolution"));
        Unregister(_T("putbelow"));
        Unregister(_T("select"));
        Unregister(_T("putontop"));
    }

    // API implementation
    //

    // Method: putontop - Puts client surface on top in z-order
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Client not found
    uint32_t Compositor::endpoint_putontop(const PutontopParamsInfo& params)
    {
        const string& client = params.Client.Value();

        return ToTop(client);
    }

    // Method: select the surface that should receive the inputs.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Client not found
    uint32_t Compositor::endpoint_select(const PutontopParamsInfo& params)
    {
        const string& client = params.Client.Value();

        return Select(client);
    }


    // Method: putbelow - Puts client surface below another surface
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Client(s) not found
    uint32_t Compositor::endpoint_putbelow(const PutbelowParamsData& params)
    {
        const string& client = params.Client.Value();
        const string& relative = params.Relative.Value();

        return PutBefore(relative, client);
    }

    // Property: resolution - Screen resolution
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown resolution
    uint32_t Compositor::get_resolution(Core::JSON::EnumType<ResolutionType>& response) const
    {
        switch (Resolution()) {
            case Exchange::IComposition::ScreenResolution_480i:
                response = ResolutionType::E480I;
                break;
            case Exchange::IComposition::ScreenResolution_480p:
                response = ResolutionType::E480P;
                break;
            case Exchange::IComposition::ScreenResolution_720p:
                response = ResolutionType::E720P60;
                break;
            case Exchange::IComposition::ScreenResolution_720p50Hz:
                response = ResolutionType::E720P50;
                break;
            case Exchange::IComposition::ScreenResolution_1080p24Hz:
                response = ResolutionType::E1080P24;
                break;
            case Exchange::IComposition::ScreenResolution_1080i50Hz:
                response = ResolutionType::E1080I50;
                break;
            case Exchange::IComposition::ScreenResolution_1080p50Hz:
                response = ResolutionType::E1080P50;
                break;
            case Exchange::IComposition::ScreenResolution_1080p60Hz:
                response = ResolutionType::E1080P60;
                break;
            case Exchange::IComposition::ScreenResolution_2160p50Hz:
                response = ResolutionType::E2160P50;
                break;
            case Exchange::IComposition::ScreenResolution_2160p60Hz:
                response = ResolutionType::E2160P60;
                break;
            default:
                response = ResolutionType::UNKNOWN;
        }

        return Core::ERROR_NONE;
    }

    // Property: resolution - Screen resolution
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown resolution
    //  - ERROR_UNAVAILABLE: Set resultion is not supported
    //  - ERROR_GENERAL: Failed to set resultion
    uint32_t Compositor::set_resolution(const Core::JSON::EnumType<ResolutionType>& param)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        Exchange::IComposition::ScreenResolution resolution = Exchange::IComposition::ScreenResolution_Unknown;

        if (param != ResolutionType::UNKNOWN) {
            switch(param) {
            case ResolutionType::E480I:
                resolution = Exchange::IComposition::ScreenResolution_480i;
                break;
            case ResolutionType::E480P:
                resolution = Exchange::IComposition::ScreenResolution_480p;
                break;
            case ResolutionType::E720P60:
                resolution = Exchange::IComposition::ScreenResolution_720p;
                break;
            case ResolutionType::E720P50:
                resolution = Exchange::IComposition::ScreenResolution_720p50Hz;
                break;
            case ResolutionType::E1080P24:
                resolution = Exchange::IComposition::ScreenResolution_1080p24Hz;
                break;
            case ResolutionType::E1080I50:
                resolution = Exchange::IComposition::ScreenResolution_1080i50Hz;
                break;
            case ResolutionType::E1080P50:
                resolution = Exchange::IComposition::ScreenResolution_1080p50Hz;
                break;
            case ResolutionType::E1080P60:
                resolution = Exchange::IComposition::ScreenResolution_1080p60Hz;
                break;
            case ResolutionType::E2160P50:
                resolution = Exchange::IComposition::ScreenResolution_2160p50Hz;
                break;
            case ResolutionType::E2160P60:
                resolution = Exchange::IComposition::ScreenResolution_2160p60Hz;
                break;
            default:
                resolution = Exchange::IComposition::ScreenResolution_Unknown;
            }

            result = Resolution(resolution);
        }

        return result;
    }

    // Property: clients - List of compositor clients
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Compositor::get_zorder(Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        ZOrder(response);

        return Core::ERROR_NONE;
    }

    // Property: geometry - Client surface geometry
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Client not found
    uint32_t Compositor::get_geometry(const string& index, GeometryData& response) const
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_clients.find(index) != _clients.end()) {
            Exchange::IComposition::Rectangle rectangle = Geometry(index);
            response.X = rectangle.x;
            response.Y = rectangle.y;
            response.Width = rectangle.width;
            response.Height = rectangle.height;
            result = Core::ERROR_NONE;
        }

        return result;
    }

    // Property: geometry - Client surface geometry
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Client not found
    uint32_t Compositor::set_geometry(const string& index, const GeometryData& param)
    {
        Exchange::IComposition::Rectangle rectangle = Exchange::IComposition::Rectangle();
        rectangle.x = param.X.Value();
        rectangle.y = param.Y.Value();
        rectangle.width = param.Width.Value();
        rectangle.height = param.Height.Value();

        return Geometry(index, rectangle);
    }

    // Property: visiblity - Client surface visibility
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Client not found
    uint32_t Compositor::set_visiblity(const string& index, const Core::JSON::EnumType<VisiblityType>& param)
    {
        return Visible(index, (param.Value() == VisiblityType::VISIBLE));
    }

    // Property: opacity - Client surface opacity
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Client not found
    uint32_t Compositor::set_opacity(const string& index, const Core::JSON::DecUInt8& param)
    {
        return Opacity(index, param.Value());
    }

} // namespace Plugin

}
