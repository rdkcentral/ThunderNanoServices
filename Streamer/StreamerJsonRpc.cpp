
#include <interfaces/json/JsonData_Streamer.h>
#include "Streamer.h"
#include "Module.h"

/*
    // Copy the code below to Streamer class definition

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_Ids(Core::JSON::ArrayType<Core::JSON::DecUInt32>& response);
        uint32_t endpoint_Type(const Core::JSON::DecUInt32& params, JsonData::Streamer::TypeResultData& response);
        uint32_t endpoint_DRM(const Core::JSON::DecUInt32& params, JsonData::Streamer::DRMResultData& response);
        uint32_t endpoint_State(const Core::JSON::DecUInt32& params, JsonData::Streamer::StateResultData& response);
        uint32_t endpoint_Metadata(const Core::JSON::DecUInt32& params, Core::JSON::String& response);
        uint32_t endpoint_GetSpeed(const Core::JSON::DecUInt32& params, Core::JSON::DecUInt32& response);
        uint32_t endpoint_GetPosition(const Core::JSON::DecUInt32& params, Core::JSON::DecUInt32& response);
        uint32_t endpoint_GetWindow(const Core::JSON::DecUInt32& params, JsonData::Streamer::GeometryInfo& response);
        uint32_t endpoint_Speed(const JsonData::Streamer::SpeedParamsData& params);
        uint32_t endpoint_Position(const JsonData::Streamer::PositionParamsData& params);
        uint32_t endpoint_Window(const JsonData::Streamer::WindowParamsData& params);
        uint32_t endpoint_Load(const JsonData::Streamer::LoadParamsData& params);
        uint32_t endpoint_Attach(const Core::JSON::DecUInt32& params);
        uint32_t endpoint_Detach(const Core::JSON::DecUInt32& params);
        uint32_t endpoint_CreateStream(const Core::JSON::DecUInt32& params, Core::JSON::DecUInt32& response);
        uint32_t endpoint_DeleteStream(const Core::JSON::DecUInt32& params);
*/

namespace WPEFramework {

/*
    // Copy the code below to json/Enumerations.cpp

    ENUM_CONVERSION_BEGIN(JsonData::Streamer::TypeResultData::StreamType)
        { JsonData::Streamer::TypeResultData::StreamType::STUBBED, _TXT("Stubbed") },
        { JsonData::Streamer::TypeResultData::StreamType::DVB, _TXT("DVB") },
        { JsonData::Streamer::TypeResultData::StreamType::VOD, _TXT("VOD") },
    ENUM_CONVERSION_END(JsonData::Streamer::TypeResultData::StreamType);

    ENUM_CONVERSION_BEGIN(JsonData::Streamer::DRMResultData::DrmType)
        { JsonData::Streamer::DRMResultData::DrmType::UNKNOWN, _TXT("UnKnown") },
        { JsonData::Streamer::DRMResultData::DrmType::CLEARKEY, _TXT("ClearKey") },
        { JsonData::Streamer::DRMResultData::DrmType::PLAYREADY, _TXT("PlayReady") },
        { JsonData::Streamer::DRMResultData::DrmType::WIDEVINE, _TXT("Widevine") },
    ENUM_CONVERSION_END(JsonData::Streamer::DRMResultData::DrmType);

    ENUM_CONVERSION_BEGIN(JsonData::Streamer::StateResultData::StateType)
        { JsonData::Streamer::StateResultData::StateType::IDLE, _TXT("Idle") },
        { JsonData::Streamer::StateResultData::StateType::LOADING, _TXT("Loading") },
        { JsonData::Streamer::StateResultData::StateType::PREPARED, _TXT("Prepared") },
        { JsonData::Streamer::StateResultData::StateType::PAUSED, _TXT("Paused") },
        { JsonData::Streamer::StateResultData::StateType::PLAYING, _TXT("Playing") },
        { JsonData::Streamer::StateResultData::StateType::ERROR, _TXT("Error") },
    ENUM_CONVERSION_END(JsonData::Streamer::StateResultData::StateType);
*/

namespace Plugin {

    using namespace JsonData::Streamer;

    // Registration
    //

    void Streamer::RegisterAll()
    {
        Register<void,Core::JSON::ArrayType<Core::JSON::DecUInt32>>(_T("Ids"), &Streamer::endpoint_Ids, this);
        Register<Core::JSON::DecUInt32,TypeResultData>(_T("Type"), &Streamer::endpoint_Type, this);
        Register<Core::JSON::DecUInt32,DRMResultData>(_T("DRM"), &Streamer::endpoint_DRM, this);
        Register<Core::JSON::DecUInt32,StateResultData>(_T("State"), &Streamer::endpoint_State, this);
        Register<Core::JSON::DecUInt32,Core::JSON::String>(_T("Metadata"), &Streamer::endpoint_Metadata, this);
        Register<Core::JSON::DecUInt32,Core::JSON::DecUInt32>(_T("GetSpeed"), &Streamer::endpoint_GetSpeed, this);
        Register<Core::JSON::DecUInt32,Core::JSON::DecUInt32>(_T("GetPosition"), &Streamer::endpoint_GetPosition, this);
        Register<Core::JSON::DecUInt32,GeometryInfo>(_T("GetWindow"), &Streamer::endpoint_GetWindow, this);
        Register<SpeedParamsData,void>(_T("Speed"), &Streamer::endpoint_Speed, this);
        Register<PositionParamsData,void>(_T("Position"), &Streamer::endpoint_Position, this);
        Register<WindowParamsData,void>(_T("Window"), &Streamer::endpoint_Window, this);
        Register<LoadParamsData,void>(_T("Load"), &Streamer::endpoint_Load, this);
        Register<Core::JSON::DecUInt32,void>(_T("Attach"), &Streamer::endpoint_Attach, this);
        Register<Core::JSON::DecUInt32,void>(_T("Detach"), &Streamer::endpoint_Detach, this);
        Register<Core::JSON::DecUInt32,Core::JSON::DecUInt32>(_T("CreateStream"), &Streamer::endpoint_CreateStream, this);
        Register<Core::JSON::DecUInt32,void>(_T("DeleteStream"), &Streamer::endpoint_DeleteStream, this);
    }

    void Streamer::UnregisterAll()
    {
        Unregister(_T("DeleteStream"));
        Unregister(_T("CreateStream"));
        Unregister(_T("Detach"));
        Unregister(_T("Attach"));
        Unregister(_T("Load"));
        Unregister(_T("Window"));
        Unregister(_T("Position"));
        Unregister(_T("GetSpeed"));
        Unregister(_T("GetWindow"));
        Unregister(_T("GetPosition"));
        Unregister(_T("Speed"));
        Unregister(_T("Metadata"));
        Unregister(_T("State"));
        Unregister(_T("DRM"));
        Unregister(_T("Type"));
        Unregister(_T("Ids"));
    }

    // API implementation
    //

    // Returns the player numbers in use.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_Ids(Core::JSON::ArrayType<Core::JSON::DecUInt32>& response)
    {
        uint32_t result = Core::ERROR_NONE;
        Streams::iterator stream = _streams.begin();
        while (stream != _streams.end()) {
            Core::JSON::DecUInt32 id;
            id = stream->first;
            response.Add(id);
            stream++;
        }
        return result;
    }

    // Returns the streame type - DVB or VOD.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_Type(const Core::JSON::DecUInt32& params, TypeResultData& response)
    {
        uint32_t result = Core::ERROR_NONE;

        uint8_t position = Core::NumberType<uint8_t>(params);
        Streams::iterator stream = _streams.find(position);
        if (stream != _streams.end()) {
            response.Stream = static_cast<TypeResultData::StreamType>(stream->second->Type());
        }
        return result;
    }

    // Returns the DRM Type attached with stream.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_DRM(const Core::JSON::DecUInt32& params, DRMResultData& response)
    {
        uint32_t result = Core::ERROR_NONE;

        uint8_t position = Core::NumberType<uint8_t>(params);
        Streams::iterator stream = _streams.find(position);
        if (stream != _streams.end()) {
            response.Drm = static_cast<DRMResultData::DrmType>(stream->second->DRM());
        }

        return result;
    }

    // Returns the current state of Player.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_State(const Core::JSON::DecUInt32& params, StateResultData& response)
    {
        uint32_t result = Core::ERROR_NONE;

        uint8_t position = Core::NumberType<uint8_t>(params);
        Streams::iterator stream = _streams.find(position);
        if (stream != _streams.end()) {
            response.State = static_cast<StateResultData::StateType>(stream->second->State());
        }

        return result;
    }

    // Return stream metadata.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_Metadata(const Core::JSON::DecUInt32& params, Core::JSON::String& response)
    {
        uint32_t result = Core::ERROR_NONE;

        // TODO...

        return result;
    }

    // Return current speed of the stream.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_GetSpeed(const Core::JSON::DecUInt32& params, Core::JSON::DecUInt32& response)
    {
        uint32_t result = Core::ERROR_NONE;

        const uint32_t& index = params.Value();

        Streams::iterator stream = _streams.find(index);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(index);
            if (control != _controls.end()) {
                response = control->second->Speed();
            }
        }

        return result;
    }

    // Return current position of the stream.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_GetPosition(const Core::JSON::DecUInt32& params, Core::JSON::DecUInt32& response)
    {
        uint32_t result = Core::ERROR_NONE;

        const uint32_t& index = params.Value();

        Streams::iterator stream = _streams.find(index);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(index);
            if (control != _controls.end()) {
                response = control->second->Position();
            }
        }

        return result;
    }

    // Return geometry value of the screen.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_GetWindow(const Core::JSON::DecUInt32& params, GeometryInfo& response)
    {
        uint32_t result = Core::ERROR_NONE;

        const uint32_t& index = params.Value();

        Streams::iterator stream = _streams.find(index);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(index);
            if (control != _controls.end()) {
                Exchange::IStream::IControl::IGeometry* geometry = control->second->Geometry();
                response.X = geometry->X();
                response.Y = geometry->Y();
                response.Width = geometry->Width();
               response.Height = geometry->Height();
            }
        }

        return result;
    }

    // Set speed of the stream.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: Player is invalid state
    //  - ERROR_UNAVAILABLE: Player is not available
    //  - ERROR_GENERAL: Generic error
    uint32_t Streamer::endpoint_Speed(const SpeedParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& index = params.Index.Value();
        const uint32_t& speed = params.Speed.Value();

        Streams::iterator stream = _streams.find(index);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(index);
            if (control != _controls.end()) {
                control->second->Speed(speed);
            }
        }

        return result;
    }

    // Set position of the stream.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: Player is invalid state
    //  - ERROR_UNAVAILABLE: Player instance not avaialbe
    uint32_t Streamer::endpoint_Position(const PositionParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& index = params.Index.Value();
        const uint32_t& position = params.Position.Value();

        Streams::iterator stream = _streams.find(index);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(index);
            if (control != _controls.end()) {
                control->second->Position(position);
            }
        }

        return result;
    }

    // Set geometry value of the screen.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_Window(const WindowParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& index = params.Index.Value();

        Streams::iterator stream = _streams.find(index);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(index);
            if (control != _controls.end()) {
                Exchange::IStream::IControl::IGeometry* geometry;
                geometry = Core::Service<Player::Implementation::Geometry>::Create<Player::Implementation::Geometry>(params.Geometry.X, params.Geometry.Y, control->second->Geometry()->Z(), params.Geometry.Width, params.Geometry.Height);
                control->second->Geometry(geometry);
            }
        }

        return result;
    }

    // Load the URL given in the body onto this stream.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: Player is invalid state
    //  - ERROR_UNAVAILABLE: Player is not available
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    uint32_t Streamer::endpoint_Load(const LoadParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& index = params.Index.Value();
        const string& url = params.Url.Value();
        
        Streams::iterator stream = _streams.find(index);
        if (stream != _streams.end()) {
            result = stream->second->Load(url);
        }
        return result;
    }

    // Attach a decoder to the primer of stream <Number>.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: Player is invalid state
    //  - ERROR_UNAVAILABLE: Player is not available
    uint32_t Streamer::endpoint_Attach(const Core::JSON::DecUInt32& params)
    {
        uint32_t result = Core::ERROR_NONE;

        const uint32_t& index = params.Value();

        Streams::iterator stream = _streams.find(index);
        if (stream != _streams.end()) {
            if (stream->second->State() == Exchange::IStream::Prepared) {
                Exchange::IStream::IControl* control = stream->second->Control();
                if (control != nullptr) {
                    _controls.insert(std::make_pair(index, control));
                }
            } else if (stream->second->State() == Exchange::IStream::Playing) {
                result = Core::ERROR_ILLEGAL_STATE; //FIXME errors
            } else {
                result = Core::ERROR_INCOMPLETE_CONFIG;
            }
        }
        return result;
    }

    // Detach a decoder to the primer of stream <Number>.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_ILLEGAL_STATE: Player is invalid state
    //  - ERROR_UNAVAILABLE: Player is not available
    uint32_t Streamer::endpoint_Detach(const Core::JSON::DecUInt32& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& index = params.Value();

        Streams::iterator stream = _streams.find(index);
        if (stream != _streams.end()) { 
            if (stream->second->State() == Exchange::IStream::Prepared) {
                Controls::iterator control = _controls.find(index);
                if (control != _controls.end()) {
                    if (control->second->Release() == Core::ERROR_DESTRUCTION_SUCCEEDED) {
                        _controls.erase(index);
                    }
                } else {
                    result = Core::ERROR_ILLEGAL_STATE; //FIXME errors
                }
            }
        }
        return result;
    }

    // Create an instance of a stream of type <Type>, Body return the stream index for reference in the other calls.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_CreateStream(const Core::JSON::DecUInt32& params, Core::JSON::DecUInt32& response)
    {
        uint32_t result = Core::ERROR_NONE;
        Core::EnumerateType<Exchange::IStream::streamtype> type(params.Value());
        Exchange::IStream* stream = _player->CreateStream(type.Value());
        if (stream != nullptr) {

            uint8_t position = 0;
            for (; position < _streams.size(); ++position) {
                Streams::iterator stream = _streams.find(position);
                if (stream == _streams.end()) {
                    break;
                }
            }

            _streams.emplace(std::piecewise_construct,
            std::forward_as_tuple(position),
            std::forward_as_tuple(*this, position, stream));

            response = position;
        }
        return result;
    }

    // Delete the given streamer instance.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_DeleteStream(const Core::JSON::DecUInt32& params)
    {
        uint32_t result = Core::ERROR_NONE;

        uint8_t position = params.Value();
        Controls::iterator control = _controls.find(position);
        if (control != _controls.end()) {
            control->second->Release();
            _controls.erase(control);
        }
        Streams::iterator stream = _streams.find(position);
        if (stream != _streams.end()) {
            stream->second->Release();
            _streams.erase(position);
        }

        return result;
    }

} // namespace Plugin

}

