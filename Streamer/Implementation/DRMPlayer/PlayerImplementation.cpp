#include "DRMPlayer.h"
#include "GstCallbacks.h"

#include <opencdm/open_cdm.h>
#include <sys/syscall.h>
#include <sys/types.h>

namespace WPEFramework {
namespace Player {
    namespace Implementation {

        namespace {

            PlayerPlatformRegistrationType<DRMPlayer, Exchange::IStream::streamtype::IP> Register(
                /*  Initialize */ [](const string& configuration) -> uint32_t {
                    config.FromString(configuration);
                    return (Core::ERROR_NONE);
                });

            DRMPlayer::DRMPlayer(const Exchange::IStream::streamtype streamType, const uint8_t index)
                : _index(index)
                , _speed(100)
                , _vecSpeeds()
                , _streamType(streamType)
                , _error(Core::ERROR_NONE)
                , _adminLock()
                , _data()
                , _bus(nullptr)
            {
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

            // The g_main_loop_run function is a blocking one.
            // It's role is to handle incoming messages on the GstBus.
            // Due to this, it will only be called once, on the first playback startup.
            // It's exit is tied to the call of DetachDecoder (and indirectly Teardown) method.
            uint32_t DRMPlayer::Worker()
            {
                g_main_loop_run(_data._mainLoop);
                return (Core::infinite);
            }

            uint32_t DRMPlayer::Setup()
            {
                uint32_t result = Core::ERROR_NONE;

                _adminLock.Lock();
                result = SetupGstElements();
                _error = result;
                _adminLock.Unlock();

                return result;
            }

            uint32_t DRMPlayer::SetupGstElements()
            {
                gst_plugin_register_static(GST_VERSION_MAJOR,
                    GST_VERSION_MINOR,
                    "ocdmdecrypt",
                    "description",
                    GstCallbacks::plugin_init, "1.0", "LGPL", "package", "package-origin", "website");

                _data._playbin = gst_element_factory_make("playbin", nullptr);
                _bus = gst_element_get_bus(_data._playbin);
                gst_bus_add_watch(_bus, (GstBusFunc)GstCallbacks::gstBusCallback, &_data);

                _data._mainLoop = g_main_loop_new(NULL, FALSE);
                if (_data._mainLoop == nullptr || _data._playbin == nullptr) {
                    TRACE_L1("Could not initialize the gstreamer pipeline");
                    return Core::ERROR_OPENING_FAILED;
                }

                return Core::ERROR_NONE;
            }

            uint32_t DRMPlayer::Teardown()
            {
                DetachDecoder(_index);

                _adminLock.Lock();

                TeardownGstElements();
                this->Terminate();

                _adminLock.Unlock();
                return Core::ERROR_NONE;
            }

            uint32_t DRMPlayer::TeardownGstElements()
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

            void DRMPlayer::Callback(ICallback* callback)
            {
                TRACE_L1("DRMPlayer callback setter is called, not implemented");
            }

            string DRMPlayer::Metadata() const
            {
                TRACE_L1("DRMPlayer metadata is called, not implemented");
                return string();
            }

            Exchange::IStream::streamtype DRMPlayer::Type() const
            {
                _adminLock.Lock();
                auto streamType = _streamType;
                _adminLock.Unlock();
                return streamType;
            }

            Exchange::IStream::drmtype DRMPlayer::DRM() const
            {
                TRACE_L1("DRMPlayer DRM getter called, not implemented");
                return Exchange::IStream::drmtype::Unknown;
            }

            Exchange::IStream::state DRMPlayer::State() const
            {
                TRACE_L1("DRMPlayer state changes not implemented");
                return Exchange::IStream::state::Prepared;
            }

            uint32_t DRMPlayer::Error() const
            {
                _adminLock.Lock();
                uint32_t error = _error;
                _adminLock.Unlock();
                return error;
            }

            uint8_t DRMPlayer::Index() const
            {
                _adminLock.Lock();
                uint8_t index = _index;
                _adminLock.Unlock();
                return index;
            }

            uint32_t DRMPlayer::Load(const string& uri)
            {
                _adminLock.Lock();

                initializeOcdm();
                gst_element_set_state(_data._playbin, GstState::GST_STATE_NULL);
                g_object_set(_data._playbin, "uri", uri.c_str(), NULL);

                _adminLock.Unlock();
                return Error();
            }

            void DRMPlayer::initializeOcdm()
            {
            }

            uint32_t DRMPlayer::AttachDecoder(const uint8_t index VARIABLE_IS_NOT_USED)
            {
                _adminLock.Lock();

                gst_element_set_state(_data._playbin, GstState::GST_STATE_PLAYING);
                if (!this->IsRunning())
                    this->Run();

                _adminLock.Unlock();
                return (Core::ERROR_NONE);
            }

            uint32_t DRMPlayer::DetachDecoder(const uint8_t index VARIABLE_IS_NOT_USED)
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
            uint32_t DRMPlayer::Speed(const int32_t speed)
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

            void DRMPlayer::SendSeekEvent()
            {
                gint64 position;
                gst_element_query_position(_data._playbin, GST_FORMAT_TIME, &position);

                GstEvent* event = CreateSeekEvent(position);
                gst_element_send_event(_data._playbin, event);
            }

            GstEvent* DRMPlayer::CreateSeekEvent(const uint64_t position)
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

            int32_t DRMPlayer::Speed() const
            {
                _adminLock.Lock();
                uint8_t speed = _speed;
                _adminLock.Unlock();
                return speed;
            }

            const std::vector<int32_t>& DRMPlayer::Speeds() const
            {
                _adminLock.Lock();
                auto speeds = _vecSpeeds;
                _adminLock.Unlock();
                return speeds;
            }

            void DRMPlayer::Position(const uint64_t absoluteTime)
            {
                gint64 duration;
                _adminLock.Lock();

                gst_element_query_duration(_data._playbin, GST_FORMAT_TIME, &duration);
                if (absoluteTime < static_cast<uint64_t>(duration)) {
                    gst_element_send_event(_data._playbin, CreateSeekEvent(absoluteTime));
                    _adminLock.Unlock();
                } else {
                    _adminLock.Unlock();
                    TRACE_L1("Absolute time cannot be bigger than the file duration");
                }
            }

            uint64_t DRMPlayer::Position() const
            {
                _adminLock.Lock();
                gint64 position;
                gst_element_query_position(_data._playbin, GST_FORMAT_TIME, &position);
                _adminLock.Unlock();
                return static_cast<uint64_t>(position);
            }

            void DRMPlayer::TimeRange(uint64_t& begin, uint64_t& end) const
            {
                TRACE_L1("DRMPlayer time range not supported");
            }

            const Rectangle& DRMPlayer::Window() const
            {
                TRACE_L1("DRMPlayer window shape not supported");
                return Rectangle();
            }

            void DRMPlayer::Window(const Rectangle& rectangle)
            {
                TRACE_L1("DRMPlayer window shape not supported");
            }

            uint32_t DRMPlayer::Order() const
            {
                TRACE_L1("DRMPlayer window order not supported");
                return 0;
            }

            void DRMPlayer::Order(const uint32_t order)
            {
                TRACE_L1("DRMPlayer window order not supported.");
            }

            const std::list<ElementaryStream>& DRMPlayer::Elements() const
            {
                TRACE_L1("DRMPlayer elementary streams not supported");
                return {};
            }
        }
    }
}
}
