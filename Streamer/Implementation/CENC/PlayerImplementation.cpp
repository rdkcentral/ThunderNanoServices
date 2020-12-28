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

#include "CENC.hpp"
#include "GstCallbacks.hpp"

#include <opencdm/open_cdm.h>
#include <sys/syscall.h>
#include <sys/types.h>

namespace WPEFramework {
namespace Player {
    namespace Implementation {

        namespace {

            PlayerPlatformRegistrationType<CENC, Exchange::IStream::streamtype::IP> Register(
                /*  Initialize */ [](const string& configuration) -> uint32_t {
                    config.FromString(configuration);
                    return (Core::ERROR_NONE);
                });

            CENC::CENC(const Exchange::IStream::streamtype streamType, const uint8_t index)
                : _index(index)
                , _speed(100)
                , _vecSpeeds()
                , _streamType(streamType)
                , _error(Core::ERROR_NONE)
                , _adminLock()
                , _data()
                , _bus(nullptr)
            {
                if(!gst_is_initialized())
                    gst_init(0, nullptr);
                
                uint16_t speedsLen = config.Speeds.Length();

                if ((config.Speeds.IsSet() == true) && (speedsLen != 0)) {
                    auto speedsIt(config.Speeds.Elements());
                    _vecSpeeds.reserve(speedsLen);

                    for (uint16_t index = 0; speedsIt.Next(); ++index) {
                        _vecSpeeds.push_back(speedsIt.Current().Value());
                    }
                } else {
                    _vecSpeeds = {
                        0, 25, 50, 75, 100, 125, 150, 175, 200
                    };
                }
            }

            // Core::Thread method
            // -------------------------------------------------------------
            uint32_t CENC::Worker()
            {
                g_main_loop_run(_data._mainLoop);
                return Core::infinite;
            }

            uint32_t CENC::Setup()
            {
                uint32_t result = Core::ERROR_NONE;

                _adminLock.Lock();
                result = SetupGstElements();
                _error = result;
                _adminLock.Unlock();

                return result;
            }

            uint32_t CENC::SetupGstElements()
            {
                _data._playbin = gst_element_factory_make("playbin", nullptr);
                _bus = gst_element_get_bus(_data._playbin);
                gst_bus_add_watch(_bus, (GstBusFunc)GstCallbacks::gstBusCallback, &_data);

                _data._mainLoop = g_main_loop_new(NULL, FALSE);
                if (_data._mainLoop == nullptr || _data._playbin == nullptr) {
                    TRACE(Trace::Error, (_T("Could not initialize the gstreamer pipeline")));
                    return Core::ERROR_OPENING_FAILED;
                }

                return Core::ERROR_NONE;
            }

            uint32_t CENC::Teardown()
            {
                DetachDecoder(_index);

                _adminLock.Lock();

                TeardownGstElements();
                this->Terminate();

                _adminLock.Unlock();
                
                return Core::ERROR_NONE;
            }

            uint32_t CENC::TeardownGstElements()
            {
                g_main_loop_quit(_data._mainLoop);
                g_main_loop_unref(_data._mainLoop);
                _data._mainLoop = nullptr;

                gst_object_unref(_bus);
                _bus = nullptr;

                gst_element_set_state(_data._playbin, GST_STATE_NULL);
                gst_object_unref(_data._playbin);
                _data._playbin = nullptr;
            }

            void CENC::Callback(ICallback* callback)
            {
                TRACE(Trace::Information, (_T("CENC callback setter is called, not implemented")));
            }

            string CENC::Metadata() const
            {
                TRACE(Trace::Information, (_T("CENC metadata is called, not implemented")));
                return string();
            }

            Exchange::IStream::streamtype CENC::Type() const
            {
                _adminLock.Lock();
                auto streamType = _streamType;
                _adminLock.Unlock();
                return streamType;
            }

            Exchange::IStream::drmtype CENC::DRM() const
            {
                TRACE(Trace::Information, (_T("CENC DRM getter called, not implemented")));
                return Exchange::IStream::drmtype::Unknown;
            }

            Exchange::IStream::state CENC::State() const
            {
                TRACE(Trace::Information, (_T("CENC state changes not implemented")));
                return Exchange::IStream::state::Prepared;
            }

            uint32_t CENC::Error() const
            {
                _adminLock.Lock();
                uint32_t error = _error;
                _adminLock.Unlock();
                return error;
            }

            uint8_t CENC::Index() const
            {
                _adminLock.Lock();
                uint8_t index = _index;
                _adminLock.Unlock();
                return index;
            }

            uint32_t CENC::Load(const string& uri)
            {
                _adminLock.Lock();

                initializeOcdm();
                gst_element_set_state(_data._playbin, GstState::GST_STATE_NULL);
                g_object_set(_data._playbin, "uri", uri.c_str(), NULL);

                _adminLock.Unlock();
                return Error();
            }

            void CENC::initializeOcdm()
            {
            }

            uint32_t CENC::AttachDecoder(const uint8_t index VARIABLE_IS_NOT_USED)
            {
                _adminLock.Lock();

                gst_element_set_state(_data._playbin, GstState::GST_STATE_PLAYING);
                if (!this->IsRunning())
                    this->Run();

                _adminLock.Unlock();
                return (Core::ERROR_NONE);
            }

            uint32_t CENC::DetachDecoder(const uint8_t index VARIABLE_IS_NOT_USED)
            {
                _adminLock.Lock();

                
                gst_element_set_state(_data._playbin, GstState::GST_STATE_NULL);
                this->Stop();

                _adminLock.Unlock();
                return (Core::ERROR_NONE);
            }

            // There are two courses of action, based on the input speed:
            //      - For speed == 0 || speed == 100:
            //          The pipeline state should be changed (GST_STATE_PLAYING, GST_STATE_PAUSED).
            //      - For speed != 0 && speed != 100:
            //          A seek event, with modified playback rates,
            //          has to be issued to the pipeline element.
            uint32_t CENC::Speed(const int32_t speed)
            {
                _adminLock.Lock();

                _speed = speed;
                uint32_t result = Core::ERROR_NONE;

                if (speed == 0) {
                    gst_element_set_state(_data._playbin, GST_STATE_PAUSED);
                } else if (std::find(_vecSpeeds.begin(), _vecSpeeds.end(), speed) != _vecSpeeds.end()) {
                    gst_element_set_state(_data._playbin, GST_STATE_PLAYING);
                    SendSeekEvent();
                } else {
                    result = Core::ERROR_UNAVAILABLE;
                }

                _adminLock.Unlock();
                return result;
            }

            void CENC::SendSeekEvent()
            {
                gint64 position;
                gst_element_query_position(_data._playbin, GST_FORMAT_TIME, &position);

                GstEvent* event = CreateSeekEvent(position);
                gst_element_send_event(_data._playbin, event);
            }

            GstEvent* CENC::CreateSeekEvent(const uint64_t position)
            {
                gdouble newRate = gdouble(gdouble(_speed) / 100);
                return gst_event_new_seek(newRate,
                    GST_FORMAT_TIME,
                    GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                    GST_SEEK_TYPE_SET,
                    position,
                    GST_SEEK_TYPE_END,
                    0);
            }

            int32_t CENC::Speed() const
            {
                _adminLock.Lock();
                uint8_t speed = _speed;
                _adminLock.Unlock();
                return speed;
            }

            const std::vector<int32_t>& CENC::Speeds() const
            {
                _adminLock.Lock();
                auto speeds = _vecSpeeds;
                _adminLock.Unlock();
                return speeds;
            }

            void CENC::Position(const uint64_t absoluteTime)
            {
                gint64 duration;
                _adminLock.Lock();

                gst_element_query_duration(_data._playbin, GST_FORMAT_TIME, &duration);
                if (absoluteTime < static_cast<uint64_t>(duration)) {
                    gst_element_send_event(_data._playbin, CreateSeekEvent(absoluteTime));
                    _adminLock.Unlock();
                } else {
                    _adminLock.Unlock();
                    TRACE(Trace::Warning, (_T("Absolute time cannot be bigger than the file duration")));
                }
            }

            uint64_t CENC::Position() const
            {
                _adminLock.Lock();
                gint64 position;
                gst_element_query_position(_data._playbin, GST_FORMAT_TIME, &position);
                _adminLock.Unlock();
                return static_cast<uint64_t>(position);
            }

            void CENC::TimeRange(uint64_t& begin, uint64_t& end) const
            {
                TRACE(Trace::Information, (_T("CENC time range not supported")));
            }

            const Rectangle& CENC::Window() const
            {
                TRACE(Trace::Information, (_T("CENC window shape not supported")));
                return Rectangle();
            }

            void CENC::Window(const Rectangle& rectangle)
            {
                TRACE(Trace::Information, (_T("CENC window shape not supported")));
            }

            uint32_t CENC::Order() const
            {
                TRACE(Trace::Information, (_T("CENC window order not supported")));
                return 0;
            }

            void CENC::Order(const uint32_t order)
            {
                TRACE(Trace::Information, (_T("CENC window order not supported.")));
            }

            const std::list<ElementaryStream>& CENC::Elements() const
            {
                TRACE(Trace::Information, (_T("CENC elementary streams not supported")));
                return {};
            }
        }
    }
}
}
