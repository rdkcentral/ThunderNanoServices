#include <interfaces/json/JsonData_Streamer.h>
#include "Streamer.h"
#include "Module.h"

/*
    // Copy the code below to Streamer class definition

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_status(const JsonData::Streamer::StatusParamsInfo& params, JsonData::Streamer::StatusResultData& response);
        uint32_t endpoint_create(const JsonData::Streamer::CreateParamsData& params, Core::JSON::DecUInt8& response);
        uint32_t endpoint_destroy(const JsonData::Streamer::StatusParamsInfo& params);
        uint32_t endpoint_load(const JsonData::Streamer::LoadParamsData& params);
        uint32_t endpoint_attach(const JsonData::Streamer::StatusParamsInfo& params);
        uint32_t endpoint_detach(const JsonData::Streamer::StatusParamsInfo& params);
        uint32_t endpoint_speeds(const JsonData::Streamer::StatusParamsInfo& params, Core::JSON::ArrayType<Core::JSON::DecSInt32>& response);
        uint32_t set_speed(const JsonData::Streamer::SpeedParamsData& params);
        uint32_t get_speed(JsonData::Streamer::SpeedParamsData& params) const;
        uint32_t set_position(const JsonData::Streamer::PositionParamsData& params);
        uint32_t get_position(JsonData::Streamer::PositionParamsData& params) const;
        uint32_t set_window(const JsonData::Streamer::WindowParamsData& params);
        uint32_t get_window(JsonData::Streamer::WindowParamsData& params) const;
        uint32_t endpoint_streams(Core::JSON::ArrayType<Core::JSON::DecUInt32>& response);
        uint32_t endpoint_type(const JsonData::Streamer::StatusParamsInfo& params, JsonData::Streamer::TypeResultData& response);
        uint32_t endpoint_drm(const JsonData::Streamer::StatusParamsInfo& params, JsonData::Streamer::DrmResultInfo& response);
        uint32_t endpoint_state(const JsonData::Streamer::StatusParamsInfo& params, JsonData::Streamer::StateResultInfo& response);
        void event_statechange(const string& id, const JsonData::Streamer::StateType& state);
        void event_drmchange(const string& id, const JsonData::Streamer::DrmType& drm);
        void event_timeupdate(const string& id, const uint64_t& time);
*/

namespace WPEFramework {

/*
    // Copy the code below to json/Enumerations.cpp

    ENUM_CONVERSION_BEGIN(JsonData::Streamer::TypeType)
        { JsonData::Streamer::TypeType::STUBBED, _TXT("stubbed") },
        { JsonData::Streamer::TypeType::DVB, _TXT("dvb") },
        { JsonData::Streamer::TypeType::ATSC, _TXT("atsc") },
        { JsonData::Streamer::TypeType::VOD, _TXT("vod") },
    ENUM_CONVERSION_END(JsonData::Streamer::TypeType);

    ENUM_CONVERSION_BEGIN(JsonData::Streamer::StateType)
        { JsonData::Streamer::StateType::IDLE, _TXT("idle") },
        { JsonData::Streamer::StateType::LOADING, _TXT("loading") },
        { JsonData::Streamer::StateType::PREPARED, _TXT("prepared") },
        { JsonData::Streamer::StateType::PAUSED, _TXT("paused") },
        { JsonData::Streamer::StateType::PLAYING, _TXT("playing") },
        { JsonData::Streamer::StateType::ERROR, _TXT("error") },
    ENUM_CONVERSION_END(JsonData::Streamer::StateType);

    ENUM_CONVERSION_BEGIN(JsonData::Streamer::DrmType)
        { JsonData::Streamer::DrmType::UNKNOWN, _TXT("unknown") },
        { JsonData::Streamer::DrmType::CLEARKEY, _TXT("clearkey") },
        { JsonData::Streamer::DrmType::PLAYREADY, _TXT("playready") },
        { JsonData::Streamer::DrmType::WIDEVINE, _TXT("widevine") },
    ENUM_CONVERSION_END(JsonData::Streamer::DrmType);
*/

namespace Plugin {

    using namespace JsonData::Streamer;

    // Registration
    //

    void Streamer::RegisterAll()
    {
        Register<StatusParamsInfo,StatusResultData>(_T("status"), &Streamer::endpoint_status, this);
        Register<CreateParamsData,Core::JSON::DecUInt8>(_T("create"), &Streamer::endpoint_create, this);
        Register<StatusParamsInfo,void>(_T("destroy"), &Streamer::endpoint_destroy, this);
        Register<LoadParamsData,void>(_T("load"), &Streamer::endpoint_load, this);
        Register<StatusParamsInfo,void>(_T("attach"), &Streamer::endpoint_attach, this);
        Register<StatusParamsInfo,void>(_T("detach"), &Streamer::endpoint_detach, this);
        Register<StatusParamsInfo,Core::JSON::ArrayType<Core::JSON::DecSInt32>>(_T("speeds"), &Streamer::endpoint_speeds, this);
        Register<void,Core::JSON::ArrayType<Core::JSON::DecUInt32>>(_T("streams"), &Streamer::endpoint_streams, this);
        Register<StatusParamsInfo,TypeResultData>(_T("type"), &Streamer::endpoint_type, this);
        Register<StatusParamsInfo,DrmResultInfo>(_T("drm"), &Streamer::endpoint_drm, this);
        Register<StatusParamsInfo,StateResultInfo>(_T("state"), &Streamer::endpoint_state, this);

        Property<SpeedParamsData>(_T("speed"), &Streamer::get_speed, &Streamer::set_speed, this);
        Property<PositionParamsData>(_T("position"), &Streamer::get_position, &Streamer::set_position, this);
        Property<WindowParamsData>(_T("window"), &Streamer::get_window, &Streamer::set_window, this);
    }

    void Streamer::UnregisterAll()
    {
        Unregister(_T("state"));
        Unregister(_T("drm"));
        Unregister(_T("type"));
        Unregister(_T("streams"));
        Unregister(_T("window"));
        Unregister(_T("position"));
        Unregister(_T("speed"));
        Unregister(_T("speeds"));
        Unregister(_T("detach"));
        Unregister(_T("attach"));
        Unregister(_T("load"));
        Unregister(_T("destroy"));
        Unregister(_T("create"));
        Unregister(_T("status"));
    }

    // API implementation
    //

    // Retrieves the status of a stream.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::endpoint_status(const StatusParamsInfo& params, StatusResultData& response)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            response.Type = static_cast<TypeType>(stream->second->Type());
            response.Drm = static_cast<DrmType>(stream->second->DRM());
            response.State = static_cast<StateType>(stream->second->State());
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Creates a stream instance.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_BAD_REQUEST: Invalid stream type given
    //  - ERROR_UNAVAILABLE: Streamer instance is not available
    uint32_t Streamer::endpoint_create(const CreateParamsData& params, Core::JSON::DecUInt8& response)
    {
        uint32_t result = Core::ERROR_NONE;
        const TypeType& streamType = params.Type.Value();

        Core::EnumerateType<JsonData::Streamer::TypeType> type(streamType);
        if (type.IsSet()) {

            Exchange::IStream* stream = _player->CreateStream(static_cast<const WPEFramework::Exchange::IStream::streamtype>(type.Value()));
            if (stream != nullptr) {

                uint8_t id = 0;
                for (; id < _streams.size(); ++id) {
                    Streams::iterator stream = _streams.find(id);
                    if (stream == _streams.end()) {
                        break;
                    }
                }

                _streams.emplace(std::piecewise_construct,
                std::forward_as_tuple(id),
                std::forward_as_tuple(*this, id, stream));

                response = id;
            } else {
                result = Core::ERROR_UNAVAILABLE;
            }
        } else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return result;
    }

    // Destroys a stream instance.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::endpoint_destroy(const StatusParamsInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Controls::iterator control = _controls.find(id);
        if (control != _controls.end()) {
            control->second->Release();
            _controls.erase(control);
        }
        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            stream->second->Release();
            _streams.erase(id);
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Loads a source into a stream.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_BAD_REQUEST: Invalid location given
    //  - ERROR_ILLEGAL_STATE: Player is not in a valid state
    //  - ERROR_UNAVAILABLE: Player instance is not available
    uint32_t Streamer::endpoint_load(const LoadParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();
        const string& location = params.Location.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            result = stream->second->Load(location);
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Attaches a decoder to the streamer.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_INPROGRESS: Decoder already attached
    //  - ERROR_ILLEGAL_STATE: Stream not prepared
    uint32_t Streamer::endpoint_attach(const StatusParamsInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {

            if (stream->second->State() == Exchange::IStream::Prepared) {
                Exchange::IStream::IControl* control = stream->second->Control();
                if (control != nullptr) {
                    _controls.insert(std::make_pair(id, control));
                }
            } else if (stream->second->State() == Exchange::IStream::Playing) {
                result = Core::ERROR_INPROGRESS;
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Detaches a decoder from the streamer.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_ILLEGAL_STATE: Decoder not attached to the stream
    //  - ERROR_INPROGRESS: Decoder is in use
    uint32_t Streamer::endpoint_detach(const StatusParamsInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            if (stream->second->State() == Exchange::IStream::Prepared) {
                Controls::iterator control = _controls.find(id);
                if (control != _controls.end()) {
                    if (control->second->Release() == Core::ERROR_DESTRUCTION_SUCCEEDED) {
                        _controls.erase(id);
                    } else {
                        result = Core::ERROR_INPROGRESS;
                    }
                } else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Retrieves the speeds supported by the player.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_ILLEGAL_STATE: Decoder not attached to the stream
    uint32_t Streamer::endpoint_speeds(const StatusParamsInfo& params, Core::JSON::ArrayType<Core::JSON::DecSInt32>& response)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {

            Controls::iterator control = _controls.find(id);
            if (control != _controls.end()) {
                RPC::IValueIterator* iterator = control->second->Speeds();
                if (iterator != nullptr) {

                    uint32_t currentElement;

                    while (iterator->Next(currentElement) == true) {
                        Core::JSON::DecSInt32& element(response.Add());
                        element = currentElement;
                    }
                    iterator->Release();
                }
            } else {

                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Retrieves current speed of the stream.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::get_speed(SpeedParamsData& params) const
    {
        uint32_t result = Core::ERROR_NONE;

        const uint32_t& id = params.Id.Value();

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::const_iterator control = _controls.find(id);
            if (control != _controls.end()) {
                params.Speed = control->second->Speed();
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }


    // Sets stream speed.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_BAD_REQUEST: Invalid speed given
    //  - ERROR_ILLEGAL_STATE: Player is not in a valid state or decoder not attached
    //  - ERROR_UNAVAILABLE: Player instance is not available
    uint32_t Streamer::set_speed(const SpeedParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();
        const uint32_t& speed = params.Speed.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(id);
            if (control != _controls.end()) {
               control->second->Speed(speed);
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Retrieves current position of the stream.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::get_position(PositionParamsData& params) const
    {
        uint32_t result = Core::ERROR_NONE;

        const uint32_t& id = params.Id.Value();

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::const_iterator control = _controls.find(id);
            if (control != _controls.end()) {
                params.Position = control->second->Position();
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        }  else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }


    // Sets stream position.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_BAD_REQUEST: Invalid position given
    //  - ERROR_ILLEGAL_STATE: Player is not in a valid state or decoder not attached
    uint32_t Streamer::set_position(const PositionParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();
        const uint64_t& position = params.Position.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(id);
            if (control != _controls.end()) {
                control->second->Position(position);
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Retrieves current window of the stream.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::get_window(WindowParamsData& params) const
    {
        uint32_t result = Core::ERROR_NONE;

        const uint32_t& id = params.Id.Value();

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::const_iterator control = _controls.find(id);
            if (control != _controls.end()) {
                Exchange::IStream::IControl::IGeometry* geometry = control->second->Geometry();
                params.Window.X = geometry->X();
                params.Window.Y = geometry->Y();
                params.Window.Width = geometry->Width();
                params.Window.Height = geometry->Height();
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        }  else {
            result = Core::ERROR_UNKNOWN_KEY;
        }
        return result;
    }

    // Sets stream playback window.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_BAD_REQUEST: Invalid window geometry given
    //  - ERROR_ILLEGAL_STATE: Player is not in a valid state or decoder not attached
    uint32_t Streamer::set_window(const WindowParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();
        // params.Window ...

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(id);
            if (control != _controls.end()) {
                Exchange::IStream::IControl::IGeometry* geometry;
                geometry = Core::Service<Player::Implementation::Geometry>::Create<Player::Implementation::Geometry>(params.Window.X, params.Window.Y, control->second->Geometry()->Z(), params.Window.Width, params.Window.Height);
                control->second->Geometry(geometry);
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Retrieves all created stream instance IDs.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::endpoint_streams(Core::JSON::ArrayType<Core::JSON::DecUInt32>& response)
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

    // Retrieves the streame type - DVB, ATSC or VOD.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::endpoint_type(const StatusParamsInfo& params, TypeResultData& response)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            response.Stream = static_cast<TypeType>(stream->second->Type());
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Retrieves the DRM Type attached with stream.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::endpoint_drm(const StatusParamsInfo& params, DrmResultInfo& response)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            response.Drm = static_cast<DrmType>(stream->second->DRM());
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Retrieves the current state of Player.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::endpoint_state(const StatusParamsInfo& params, StateResultInfo& response)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            response.State = static_cast<StateType>(stream->second->State());
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Notifies of stream state change.
    void Streamer::event_statechange(const string& id, const StateType& state)
    {
        StateResultInfo params;
        params.State = state;

        Notify(_T("statechange"), params, [&](const string& designator) -> bool {
            const string designator_id = designator.substr(0, designator.find('.'));
            return (id == designator_id);
        });
    }

    // Notifies of stream DRM system change.
    void Streamer::event_drmchange(const string& id, const DrmType& drm)
    {
        DrmResultInfo params;
        params.Drm = drm;

        Notify(_T("drmchange"), params, [&](const string& designator) -> bool {
            const string designator_id = designator.substr(0, designator.find('.'));
            return (id == designator_id);
        });
    }

    // Event fired to indicate the position in the stream.
    void Streamer::event_timeupdate(const string& id, const uint64_t& time)
    {
        TimeupdateParamsData params;
        params.Time = time;

        Notify(_T("timeupdate"), params, [&](const string& designator) -> bool {
            const string designator_id = designator.substr(0, designator.find('.'));
            return (id == designator_id);
        });
    }

} // namespace Plugin

}
