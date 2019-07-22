
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
        Register<TotopParamsInfo,void>(_T("totop"), &Compositor::endpoint_totop, this);
        Register<PutbelowParamsData,void>(_T("putbelow"), &Compositor::endpoint_putbelow, this);
        Register<TotopParamsInfo,void>(_T("kill"), &Compositor::endpoint_kill, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("clients"), &Compositor::get_clients, nullptr, this);
        Property<Core::JSON::EnumType<ResolutionType>>(_T("resolution"), &Compositor::get_resolution, &Compositor::set_resolution, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("zorder"), &Compositor::get_zorder, nullptr, this);
        Property<GeometryData>(_T("geometry"), &Compositor::get_geometry, &Compositor::set_geometry, this);
        Property<Core::JSON::Boolean>(_T("visible"), nullptr, &Compositor::set_visible, this);
        Property<Core::JSON::DecUInt32>(_T("opacity"), nullptr, &Compositor::set_opacity, this);
    }

    void Compositor::UnregisterAll()
    {
        Unregister(_T("kill"));
        Unregister(_T("putbelow"));
        Unregister(_T("totop"));
        Unregister(_T("opacity"));
        Unregister(_T("visible"));
        Unregister(_T("geometry"));
        Unregister(_T("zorder"));
        Unregister(_T("resolution"));
        Unregister(_T("clients"));
    }

    // API implementation
    //

    // Method: totop - Put client on top
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_FIRST_RESOURCE_NOT_FOUND: Client not found
    //  - ERROR_GENERAL: Compositor couldn't put service on top
    uint32_t Compositor::endpoint_totop(const TotopParamsInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& callsign = params.Callsign.Value();

        result = ToTop(callsign);

         if (result != Core::ERROR_FIRST_RESOURCE_NOT_FOUND && result != Core::ERROR_NONE) {
            TRACE_L1("Failed to put surface of client %s\n on top\n", callsign.c_str());
            result = Core::ERROR_GENERAL;
         }

        return result;
    }

    // Method: putbelow - Slide <below> just behind <above>
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_FIRST_RESOURCE_NOT_FOUND: At least one of clients not found
    //  - ERROR_GENERAL: Compositor failed to put service on top
    uint32_t Compositor::endpoint_putbelow(const PutbelowParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& below = params.Below.Value();
        const string& above = params.Above.Value();

        result = PutBelow(above, below);

        if (result != Core::ERROR_FIRST_RESOURCE_NOT_FOUND && result != Core::ERROR_NONE) {
            TRACE_L1("Failed to put surface of client %s\n below %s\n", below.c_str(), above.c_str());
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

    // Method: kill - Use this method to kill the client
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_FIRST_RESOURCE_NOT_FOUND: Client not found
    uint32_t Compositor::endpoint_kill(const TotopParamsInfo& params)
    {
        const string& callsign = params.Callsign.Value();

        return Kill(callsign);
    }

    // Property: clients - List of compositor clients
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Compositor::get_clients(Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        Clients(response);

        return Core::ERROR_NONE;
    }

    // Property: resolution - Output resolution
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Compositor::get_resolution(Core::JSON::EnumType<ResolutionType>& response) const
    {
        switch (Resolution()) {
            case Exchange::IComposition::ScreenResolution_480i:
                response = ResolutionType::SCREENRESOLUTION_480I;
                break;
            case Exchange::IComposition::ScreenResolution_480p:
                response = ResolutionType::SCREENRESOLUTION_480P;
                break;
            case Exchange::IComposition::ScreenResolution_720p:
                response = ResolutionType::SCREENRESOLUTION_720P;
                break;
            case Exchange::IComposition::ScreenResolution_720p50Hz:
                response = ResolutionType::SCREENRESOLUTION_720P50HZ;
                break;
            case Exchange::IComposition::ScreenResolution_1080p24Hz:
                response = ResolutionType::SCREENRESOLUTION_1080P24HZ;
                break;
            case Exchange::IComposition::ScreenResolution_1080i50Hz:
                response = ResolutionType::SCREENRESOLUTION_1080I50HZ;
                break;
            case Exchange::IComposition::ScreenResolution_1080p50Hz:
                response = ResolutionType::SCREENRESOLUTION_1080P50HZ;
                break;
            case Exchange::IComposition::ScreenResolution_1080p60Hz:
                response = ResolutionType::SCREENRESOLUTION_1080P60HZ;
                break;
            case Exchange::IComposition::ScreenResolution_2160p50Hz:
                response = ResolutionType::SCREENRESOLUTION_2160P50HZ;
                break;
            case Exchange::IComposition::ScreenResolution_2160p60Hz:
                response = ResolutionType::SCREENRESOLUTION_2160P60HZ;
                break;
            default:
                response = ResolutionType::SCREENRESOLUTION_UNKNOWN;
        }

        return Core::ERROR_NONE;
    }

    // Property: resolution - Output resolution
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_TABLE: Unknown resolution
    uint32_t Compositor::set_resolution(const Core::JSON::EnumType<ResolutionType>& param)
    {
       uint32_t result = Core::ERROR_NONE;

        switch(param) {
            case ResolutionType::SCREENRESOLUTION_480I:
                Resolution(Exchange::IComposition::ScreenResolution_480i);
                break;
            case ResolutionType::SCREENRESOLUTION_480P:
                Resolution(Exchange::IComposition::ScreenResolution_480p);
                break;
            case ResolutionType::SCREENRESOLUTION_720P:
                Resolution(Exchange::IComposition::ScreenResolution_720p);
                break;
            case ResolutionType::SCREENRESOLUTION_720P50HZ:
                Resolution(Exchange::IComposition::ScreenResolution_720p50Hz);
                break;
            case ResolutionType::SCREENRESOLUTION_1080P24HZ:
                Resolution(Exchange::IComposition::ScreenResolution_1080p24Hz);
                break;
            case ResolutionType::SCREENRESOLUTION_1080I50HZ:
                Resolution(Exchange::IComposition::ScreenResolution_1080i50Hz);
                break;
            case ResolutionType::SCREENRESOLUTION_1080P50HZ:
                Resolution(Exchange::IComposition::ScreenResolution_1080p50Hz);
                break;
            case ResolutionType::SCREENRESOLUTION_1080P60HZ:
                Resolution(Exchange::IComposition::ScreenResolution_1080p60Hz);
                break;
            case ResolutionType::SCREENRESOLUTION_2160P50HZ:
                Resolution(Exchange::IComposition::ScreenResolution_2160p50Hz);
                break;
            case ResolutionType::SCREENRESOLUTION_2160P60HZ:
                Resolution(Exchange::IComposition::ScreenResolution_2160p60Hz);
                break;
            default:
                result = Core::ERROR_UNKNOWN_TABLE;
        }

        return result;
    }

    // Property: zorder - List of compositor clients sorted by z-order
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Compositor::get_zorder(Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        ZOrder(response);

        return Core::ERROR_NONE;
    }

    // Property: geometry - Property describing nanoservice window on screen
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_FIRST_RESOURCE_NOT_FOUND: Callsign not found
    uint32_t Compositor::get_geometry(const string& index, GeometryData& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        if (index.empty() == false) {
            Exchange::IComposition::Rectangle rectangle = Geometry(index);

            if ((rectangle.x == 0) && (rectangle.y == 0) && (rectangle.width == 0) && (rectangle.height == 0)) {
                result = Core::ERROR_FIRST_RESOURCE_NOT_FOUND;
            } else {
                response.X = rectangle.x;
                response.Y = rectangle.y;
                response.Width = rectangle.width;
                response.Height = rectangle.height;
            }
        } else {
            result = Core::ERROR_FIRST_RESOURCE_NOT_FOUND;
        }

        return result;
    }

    // Property: geometry - Property describing nanoservice window on screen
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_FIRST_RESOURCE_NOT_FOUND: Callsign not found
    //  - ERROR_GENERAL: Failed to set/get geometry
    uint32_t Compositor::set_geometry(const string& index, const GeometryData& param)
    {
        uint32_t result = Core::ERROR_NONE;
        Exchange::IComposition::Rectangle rectangle = Exchange::IComposition::Rectangle();

        rectangle.x = param.X.Value();
        rectangle.y = param.Y.Value();
        rectangle.width = param.Width.Value();
        rectangle.height = param.Height.Value();

        result = Geometry(index, rectangle);

        if (result != Core::ERROR_FIRST_RESOURCE_NOT_FOUND && result != Core::ERROR_NONE) {
            TRACE_L1("Failed to set surface geometry for client %s\n", index.c_str());
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

    // Property: visible - Determines if client's surface is visible
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_FIRST_RESOURCE_NOT_FOUND: Callsign not found
    uint32_t Compositor::set_visible(const string& index, const Core::JSON::Boolean& param)
    {
        return Visible(index, param.Value());;
    }

    // Property: opacity - Opacity of client
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_FIRST_RESOURCE_NOT_FOUND: Callsign not found
    uint32_t Compositor::set_opacity(const string& index, const Core::JSON::DecUInt32& param)
    {
        return Opacity(index, param.Value());
    }

} // namespace Plugin

}

