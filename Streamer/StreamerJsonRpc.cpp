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
 
#include <interfaces/json/JsonData_Streamer.h>
#include "Streamer.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    const int MAX_STREAMS = 255;

    using namespace JsonData::Streamer;

    // Registration
    //

    void Streamer::RegisterAll()
    {
        Register<CreateParamsData,Core::JSON::DecUInt8>(_T("create"), &Streamer::endpoint_create, this);
        Register<IdInfo,void>(_T("destroy"), &Streamer::endpoint_destroy, this);
        Register<LoadParamsData,void>(_T("load"), &Streamer::endpoint_load, this);
        Register<IdInfo,void>(_T("attach"), &Streamer::endpoint_attach, this);
        Register<IdInfo,void>(_T("detach"), &Streamer::endpoint_detach, this);
        Property<Core::JSON::DecSInt32>(_T("speed"), &Streamer::get_speed, &Streamer::set_speed, this);
        Property<Core::JSON::DecUInt64>(_T("position"), &Streamer::get_position, &Streamer::set_position, this);
        Property<WindowData>(_T("window"), &Streamer::get_window, &Streamer::set_window, this);
        Property<Core::JSON::ArrayType<Core::JSON::DecSInt32>>(_T("speeds"), &Streamer::get_speeds, nullptr, this);
        Property<Core::JSON::ArrayType<Core::JSON::DecUInt32>>(_T("streams"), &Streamer::get_streams, nullptr, this);
        Property<Core::JSON::EnumType<StreamType>>(_T("type"), &Streamer::get_type, nullptr, this);
        Property<Core::JSON::EnumType<DrmType>>(_T("drm"), &Streamer::get_drm, nullptr, this);
        Property<Core::JSON::EnumType<StateType>>(_T("state"), &Streamer::get_state, nullptr, this);
        Property<Core::JSON::String>(_T("metadata"), &Streamer::get_metadata, nullptr, this);
        Property<Core::JSON::DecUInt32>(_T("error"), &Streamer::get_error, nullptr, this);
        Property<Core::JSON::ArrayType<StreamelementData>>(_T("elements"), &Streamer::get_elements, nullptr, this);
    }

    void Streamer::UnregisterAll()
    {
        Unregister(_T("detach"));
        Unregister(_T("attach"));
        Unregister(_T("load"));
        Unregister(_T("destroy"));
        Unregister(_T("create"));
        Unregister(_T("elements"));
        Unregister(_T("error"));
        Unregister(_T("metadata"));
        Unregister(_T("state"));
        Unregister(_T("drm"));
        Unregister(_T("type"));
        Unregister(_T("streams"));
        Unregister(_T("speeds"));
        Unregister(_T("window"));
        Unregister(_T("position"));
        Unregister(_T("speed"));
    }

    // API implementation
    //

    // Method: create - Creates a stream instance
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_BAD_REQUEST: Invalid stream type given
    //  - ERROR_UNAVAILABLE: Fronted of the selected stream type is not available
    uint32_t Streamer::endpoint_create(const CreateParamsData& params, Core::JSON::DecUInt8& response)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (params.Type.IsSet()) {
            const StreamType& streamType = params.Type.Value();

            uint8_t id = 0;
            for (; id < _streams.size(); ++id) {
                Streams::iterator stream = _streams.find(id);
                if (stream == _streams.end()) {
                    break;
                }
            }

            if (id < MAX_STREAMS) {
                Core::EnumerateType<JsonData::Streamer::StreamType> type(streamType);
                Exchange::IStream* stream = _player->CreateStream(static_cast<const WPEFramework::Exchange::IStream::streamtype>(type.Value()));

                if (stream != nullptr) {
                    _streams.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(*this, id, stream));
                    response = id;
                    result = Core::ERROR_NONE;
                }
            }
        }
        else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return result;
    }

    // Method: destroy - Destroys a stream instance
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_INPROGRESS: Stream is in use
    uint32_t Streamer::endpoint_destroy(const IdInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(id);
            if (control != _controls.end()) {
                control->second->Callback(nullptr);
                uint32_t relResult = control->second->Release();
                if (relResult == Core::ERROR_DESTRUCTION_SUCCEEDED) {
                    _controls.erase(control);
                } else {
                    result = Core::ERROR_INPROGRESS;
                }
            }

            if (result == Core::ERROR_NONE) {
                stream->second->Callback(nullptr);
                stream->second->Release();
                _streams.erase(stream);
            }
        }
        else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Method: load - Loads a source into a stream
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_INCORRECT_URL: Invalid location given
    //  - ERROR_GENERAL: Undefined loading error
    //  - ERROR_ILLEGAL_STATE: Stream is not in a valid state
    uint32_t Streamer::endpoint_load(const LoadParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();
        const string& location = params.Location.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            if (((stream->second->State() == Exchange::IStream::state::Idle)
                    || (stream->second->State() == Exchange::IStream::state::Prepared)
                    || (stream->second->State() == Exchange::IStream::state::Error))
                    && (_controls.find(id) == _controls.end())) {
                result = stream->second->Load(location);
                if ((result != Core::ERROR_NONE) && (result != Core::ERROR_INCORRECT_URL)) {
                    result = Core::ERROR_GENERAL;
                }
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

   // Method: attach - Attaches a decoder to the streamer
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_INPROGRESS: Decoder already attached
    //  - ERROR_ILLEGAL_STATE: Stream is not in a valid state
    //  - ERROR_UNAVAILABLE: No free decoders available
    uint32_t Streamer::endpoint_attach(const IdInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            if ((stream->second->State() == Exchange::IStream::state::Prepared) && (_controls.find(id) == _controls.end())) {
                Exchange::IStream::IControl* control = stream->second->Control();
                if (control != nullptr) {
                    _controls.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(*this, id, control));
                } else {
                    result = Core::ERROR_UNAVAILABLE;
                }
            } else if ((stream->second->State() == Exchange::IStream::state::Controlled) || (stream->second->State() == Exchange::IStream::state::Prepared)) {
                result = Core::ERROR_INPROGRESS;
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Method: detach - Detaches a decoder from the streamer
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_ILLEGAL_STATE: Stream is not in a valid state or decoder not attached
    //  - ERROR_INPROGRESS: Decoder is in use
    uint32_t Streamer::endpoint_detach(const IdInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint8_t& id = params.Id.Value();

        Streams::iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(id);
            if (control != _controls.end()) {
                control->second->Callback(nullptr);
                if (control->second->Release() == Core::ERROR_DESTRUCTION_SUCCEEDED) {
                    _controls.erase(id);
                } else {
                    result = Core::ERROR_INPROGRESS;
                }
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Property: speed - Playback speed
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_ILLEGAL_STATE: Player is not in a valid state or decoder not attached
    uint32_t Streamer::get_speed(const string& index, Core::JSON::DecSInt32& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::const_iterator control = _controls.find(id);
            if (control != _controls.end()) {
                response = control->second->Speed();
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Property: speed - Playback speed
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_ILLEGAL_STATE: Player is not in a valid state or decoder not attached
    uint32_t Streamer::set_speed(const string& index, const Core::JSON::DecSInt32& param)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());
        const uint32_t& speed = param.Value();

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

    // Property: position - Stream position
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_ILLEGAL_STATE: Player is not in a valid state or decoder not attached
    uint32_t Streamer::get_position(const string& index, Core::JSON::DecUInt64& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::const_iterator control = _controls.find(id);
            if (control != _controls.end()) {
                response = control->second->Position();
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        }  else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }


    // Property: position - Stream position
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_BAD_REQUEST: Invalid position given
    //  - ERROR_ILLEGAL_STATE: Player is not in a valid state or decoder not attached
    uint32_t Streamer::set_position(const string& index, const Core::JSON::DecUInt64& param)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());
        const uint64_t& position = param.Value();

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

    // Property: window - Stream playback window
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_ILLEGAL_STATE: Player is not in a valid state or decoder not attached
    uint32_t Streamer::get_window(const string& index, WindowData& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::const_iterator control = _controls.find(id);
            if (control != _controls.end()) {
                Exchange::IStream::IControl::IGeometry* geometry = control->second->Geometry();
                ASSERT(geometry != nullptr);
                response.X = geometry->X();
                response.Y = geometry->Y();
                response.Width = geometry->Width();
                response.Height = geometry->Height();
                geometry->Release();
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        }  else {
            result = Core::ERROR_UNKNOWN_KEY;
        }
        return result;
    }

    // Property: window - Stream playback window
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_ILLEGAL_STATE: Player is not in a valid state or decoder not attached
    uint32_t Streamer::set_window(const string& index, const WindowData& param)
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::iterator control = _controls.find(id);
            if (control != _controls.end()) {
                Exchange::IStream::IControl::IGeometry* geometry;
                geometry = Core::Service<Player::Implementation::Geometry>::Create<Player::Implementation::Geometry>(param.X.Value(), param.Y.Value(), control->second->Geometry()->Z(), param.Width.Value(), param.Height.Value());
                control->second->Geometry(geometry);
                geometry->Release();
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Property: speeds - Speeds supported by the stream player
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::get_speeds(const string& index, Core::JSON::ArrayType<Core::JSON::DecSInt32>& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            Controls::const_iterator control = _controls.find(id);
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

    // Property: streams - All created stream instance IDs
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Streamer::get_streams(Core::JSON::ArrayType<Core::JSON::DecUInt32>& response) const
    {
        Streams::const_iterator stream = _streams.begin();
        while (stream != _streams.end()) {
            Core::JSON::DecUInt32 id;
            id = stream->first;
            response.Add(id);
            stream++;
        }

        return Core::ERROR_NONE;
    }

    // Property: type - Type of a stream
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::get_type(const string& index, Core::JSON::EnumType<JsonData::Streamer::StreamType>& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            response = static_cast<StreamType>(stream->second->Type());
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Property: drm - DRM type associated with a stream
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::get_drm(const string& index, Core::JSON::EnumType<JsonData::Streamer::DrmType>& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            response = static_cast<DrmType>(stream->second->DRM());
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Property: state - Current state of a stream
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::get_state(const string& index, Core::JSON::EnumType<JsonData::Streamer::StateType>& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            response = static_cast<StateType>(stream->second->State());
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

   // Property: metadata - Metadata associated with the stream
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::get_metadata(const string& index, Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            response = stream->second->Metadata();
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Property: error - Most recent error code
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    uint32_t Streamer::get_error(const string& index, Core::JSON::DecUInt32& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::const_iterator stream = _streams.find(id);
        if (stream != _streams.end()) {
            response = stream->second->Error();
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Property: elements - Stream elements
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown stream ID given
    //  - ERROR_UNAVAILABLE: Elements retrieval not supported
    uint32_t Streamer::get_elements(const string& index, Core::JSON::ArrayType<StreamelementData>& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        const uint32_t& id = atoi(index.c_str());

        Streams::iterator stream = const_cast<Streamer*>(this)->_streams.find(id);
        if (stream != _streams.end()) {
            Exchange::IStream::IElement::IIterator* iter = stream->second->Elements();
            if (iter != nullptr) {
                while (iter->Next()) {
                    Exchange::IStream::IElement* element = iter->Current();
                    if (element != nullptr) {
                        StreamelementData streamElement;
                        switch(element->Type())
                        {
                            case Exchange::IStream::IElement::type::Audio:
                                streamElement.Type = StreamelementData::ElementType::AUDIO;
                                break;
                            case Exchange::IStream::IElement::type::Video:
                                streamElement.Type = StreamelementData::ElementType::VIDEO;
                                break;
                            case Exchange::IStream::IElement::type::Subtitles:
                                streamElement.Type = StreamelementData::ElementType::SUBTITLES;
                                break;
                            case Exchange::IStream::IElement::type::Teletext:
                                streamElement.Type = StreamelementData::ElementType::TELETEXT;
                                break;
                            default:
                                streamElement.Type = StreamelementData::ElementType::DATA;
                                break;
                        }
                        response.Add(streamElement);
                        element->Release();
                    }
                }

                iter->Release();
            }
            else {
                result = Core::ERROR_UNAVAILABLE;
            }
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
        }

        return result;
    }

    // Event: statechange - Notifies of stream state change
    void Streamer::event_statechange(const string& id, const StateType& state)
    {
        StatechangeParamsData params;
        params.State = state;

        Notify(_T("statechange"), params, [&](const string& designator) -> bool {
            const string designator_id = designator.substr(0, designator.find('.'));
            return (id == designator_id);
        });
    }

    // Event: timeupdate - Notifies of stream position change
    void Streamer::event_timeupdate(const string& id, const uint64_t& time)
    {
        TimeupdateParamsData params;
        params.Time = time;

        Notify(_T("timeupdate"), params, [&](const string& designator) -> bool {
            const string designator_id = designator.substr(0, designator.find('.'));
            return (id == designator_id);
        });
    }

    // Event: stream - Notifies of a custom stream incident
    void Streamer::event_stream(const string& id, const uint32_t& code)
    {
        StreamParamsInfo params;
        params.Code = code;

        Notify(_T("stream"), params, [&](const string& designator) -> bool {
            const string designator_id = designator.substr(0, designator.find('.'));
            return (id == designator_id);
        });
    }

    // Event: player - Notifies of a custom player incident
    void Streamer::event_player(const string& id, const uint32_t& code)
    {
        StreamParamsInfo params;
        params.Code = code;

        Notify(_T("player"), params, [&](const string& designator) -> bool {
            const string designator_id = designator.substr(0, designator.find('.'));
            return (id == designator_id);
        });
    }

    // Event: drm - Notifies of a custom DRM-related incident
    void Streamer::event_drm(const string& id, const uint32_t& code)
    {
        StreamParamsInfo params;
        params.Code = code;

        Notify(_T("drm"), params, [&](const string& designator) -> bool {
            const string designator_id = designator.substr(0, designator.find('.'));
            return (id == designator_id);
        });
    }

} // namespace Plugin

}
