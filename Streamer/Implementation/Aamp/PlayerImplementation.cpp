
#include "PlayerImplementation.h"
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Exchange::IStream::state)

    { Exchange::IStream::state::Paused, _TXT("Paused") },
    { Exchange::IStream::state::Playing, _TXT("Playing") },
    { Exchange::IStream::state::Idle, _TXT("Idle") },

    ENUM_CONVERSION_END(Exchange::IStream::state);

namespace Player {

    namespace Implementation {
        class GstWrapper {
        private:
            GstWrapper(const GstWrapper&) = delete;
            GstWrapper& operator=(const GstWrapper&) = delete;

        public:
            GstWrapper()
            : _mainLoop(nullptr)
            , _playbin(nullptr)
            , _videoSink(nullptr)
            {
            }
            virtual ~GstWrapper()
            {
                gst_object_unref(_playbin);
                _playbin = nullptr;
            }

            bool SetRate(int rate, int& currentRate, uint64_t _absoluteTime, GstState state);
            static gboolean HandleBusMessage(GstBus* bus, GstMessage* msg, void* arg);

            uint64_t GetPosition(uint64_t absoluteTime) const {
                gint64 duration;
                if (!gst_element_query_duration(_playbin, GST_FORMAT_TIME, &duration)) {
                    TRACE(Trace::Error, (_T("Unable to retrieve duration")));
                }
                return ((absoluteTime < static_cast<uint64_t>(duration)) ? absoluteTime : duration);
            }
            inline GMainLoop* MainLoop() const { return _mainLoop; }
            inline void MainLoop(GMainLoop* mainLoop) { _mainLoop = mainLoop; }
            inline GstElement* Playbin() const { return _playbin; }
            inline void Playbin(GstElement* playbin) { _playbin = playbin; }
            inline GstElement* VideoSink() const { return _videoSink; }
            inline void VideoSink(GstElement* videoSink) { _videoSink = videoSink; }

        private:
            GMainLoop* _mainLoop;
            GstElement* _playbin;
            GstElement* _videoSink;
        };

        bool GstWrapper::SetRate(int newRate, int& currentRate, uint64_t absoluteTime, GstState state)
        {
            bool status = false;
            if (newRate != currentRate) {
                if ((state == GST_STATE_PLAYING) || (state == GST_STATE_PAUSED)) {
                    currentRate = newRate;
                    gint64 cur = GST_CLOCK_TIME_NONE;
                    if (!gst_element_query_position(_playbin, GST_FORMAT_TIME, &cur)) {
                        TRACE(Trace::Error, (_T("Query position failed")));
                    } else {
                        uint64_t time = GetPosition((cur / GST_SECOND) + absoluteTime);
                        gboolean ret = 0;
                        if (newRate > 0) {
                            ret = gst_element_seek(_playbin, newRate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
                        } else {
                            ret = gst_element_seek(_playbin, newRate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE, GST_SEEK_TYPE_SET, time);
                        }
                        if (!ret) {
                            TRACE(Trace::Error, (_T("Seek failed")));
                        } else {
                            TRACE(Trace::Information, (_T("Seek Success")));
                            status = true;
                        }
                    }
                } else {
                    TRACE(Trace::Error, (_T("Invalid player state")));
                }
            } else {
                TRACE(Trace::Error, (_T("Ignoring rate, since it is already in the same")));
                status = true;
            }
            return status;
        }


        gboolean GstWrapper::HandleBusMessage(GstBus* bus, GstMessage* msg, void* arg)
        {
            GError* error;
            gchar* info;
            PlayerPlatform* player = (PlayerPlatform*)arg;

            switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_EOS: {
                g_print("EOS\n");
                break;
            }
            case GST_MESSAGE_ERROR: {
                gst_message_parse_error(msg, &error, &info);
                g_printerr("**PLAYBINTEST: Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), error->message);
                g_printerr("**PLAYBINTEST: Debugging information: %s\n", info ? info : "none");
                g_free(info);
                g_main_loop_quit(player->GetGstWrapper()->MainLoop());
                break;
            }
            case GST_MESSAGE_STATE_CHANGED: {
                GstState old, now, pending;
                gst_message_parse_state_changed(msg, &old, &now, &pending);
                if (memcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)), "playbin", 7) == 0) {
                    GST_FIXME("**PLAYBINTEST: element %s state change : %s -> %s . pending state %s\n", GST_ELEMENT_NAME(GST_MESSAGE_SRC(msg)),
                        gst_element_state_get_name(old), gst_element_state_get_name(now), gst_element_state_get_name(pending));
                }
                if (now == GST_STATE_PLAYING) {
                    if ((memcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)), "glimagesink", strlen("glimagesink")) == 0) || (memcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)), "brcmvideosink", strlen("brcmvideosink")) == 0) || (memcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)), "video-sink", strlen("video-sink") == 0))) {
                        if (player->GetGstWrapper()->VideoSink() == nullptr) {
                            GstElement* videoSink = GST_ELEMENT(GST_MESSAGE_SRC(msg));
                            player->GetGstWrapper()->VideoSink(videoSink);
                            gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(videoSink), player->Window().X, player->Window().Y, player->Window().Width, player->Window().Height);
                        }
                    }
                }
                break;
            }
            case GST_MESSAGE_BUFFERING: {
                gint percent;
                gst_message_parse_buffering(msg, &percent);
                g_print("**PLAYBINTEST: eBuffering %d\n", percent);
                break;
            }
            case GST_MESSAGE_TAG:
                break;
            default:
                g_print("Bus msg type: %s\n", gst_message_type_get_name(msg->type));
                break;
            }
            return TRUE;
        }


        uint8_t PlayerPlatform::_instances = 0;

        PlayerPlatform::PlayerPlatform(const Exchange::IStream::streamtype type, const uint8_t index, ICallback* callbacks)
            : _uri("")
            , _state(Exchange::IStream::Idle)
            , _drmType(Exchange::IStream::Unknown)
            , _streamType(type)
            , _speed(0)
            , _rate(0)
            , _absoluteTime(0)
            , _begin(0)
            , _end(~0)
            , _z(0)
            , _rectangle()
            , _gstWrapper(nullptr)
            , _callback(callbacks)
        {
            _gstWrapper = new GstWrapper();

            CreateMediaPipeline();
        }

        PlayerPlatform::~PlayerPlatform()
        {
            Terminate();
            _instances--;
            if (_gstWrapper) {
                delete _gstWrapper;
                _gstWrapper = nullptr;
            }
        }

        void PlayerPlatform::Window(const Rectangle& rectangle)
        {
            _rectangle = rectangle;
            if (GetGstWrapper()->VideoSink() != nullptr) {
                gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(GetGstWrapper()->VideoSink()), _rectangle.X, _rectangle.Y, _rectangle.Width, _rectangle.Height);
            }
        }

        uint32_t PlayerPlatform::Load(const string& uri)
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            TRACE(Trace::Information, (_T("uri = %s"), uri.c_str()));
            if (IsRunning() == true) {
                g_main_loop_quit(GetGstWrapper()->MainLoop());
            }
            TRACE(Trace::Information, (_T("uri = %s"), uri.c_str()));
            Block();
            Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
            if (Core::Thread::State() == Thread::BLOCKED) {
                TRACE(Trace::Information, (_T("uri = %s"), uri.c_str()));

                if (uri.empty() == false) {
                    TRACE(Trace::Information, (_T("uri = %s"), uri.c_str()));
                    string uriType = UriType(uri);
                    if ((uriType == "m3u8") || (uriType == "mpd")) {
                        TRACE(Trace::Information, (_T("URI type is %s"), uriType.c_str()));
                        _uri = uri;
                        _rate = 1.0; //Normal playback
                        ChangeSrcType(_uri);
                        TRACE(Trace::Information, (_T("URI = %s"), _uri.c_str()));
                        Run();
                    } else {
                        _state = Exchange::IStream::state::Error;
                        TRACE(Trace::Error, (_T("URI is not dash/hls")));
                    }
                } else {
                    TRACE(Trace::Error, (_T("URI is not provided")));
                }
            }
            return 0;
        }

        uint64_t PlayerPlatform::Position() const
        {
            gint64 cur = GST_CLOCK_TIME_NONE;
            GstElement* playbin = GetGstWrapper()->Playbin();
            if (!gst_element_query_position(playbin, GST_FORMAT_TIME, &cur)) {
                TRACE(Trace::Error, (_T("Query position failed")));
            } else {
                TRACE(Trace::Information, (_T("Query position Success")));
            }
            return ((cur / GST_SECOND) + _absoluteTime);
        }

        void PlayerPlatform::Position(const uint64_t absoluteTime)
        {
            uint64_t time = GetGstWrapper()->GetPosition(absoluteTime);
            if (time) {
                if (IsValidPipelineState() == true) {
                    gint64 cur = GST_CLOCK_TIME_NONE;
                    if (!gst_element_query_position(GetGstWrapper()->Playbin(), GST_FORMAT_TIME, &cur)) {
                        TRACE(Trace::Error, (_T("Query position failed")));
                    } else {
                        TRACE(Trace::Information, (_T("Query position Success")));
                        if (time != ((cur / GST_SECOND) + _absoluteTime)) { //Avoid seek, if media already in the same position
                            cur = GST_SECOND * time;
                            if (!gst_element_seek(GetGstWrapper()->Playbin(), 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, cur, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
                                TRACE(Trace::Error, (_T("Seek failed")));
                            } else {
                                TRACE(Trace::Information, (_T("Seek Success")));
                                _absoluteTime = time;
                            }
                        }
                    }
                }
            }
        }

        void PlayerPlatform::Terminate()
        {
            if (IsRunning() == true) {
                g_main_loop_quit(GetGstWrapper()->MainLoop());
            }

            TRACE(Trace::Information, (string(__FUNCTION__)));
            Block();
            Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);

            gst_element_set_state(GetGstWrapper()->Playbin(), GST_STATE_NULL);
        }

        void PlayerPlatform::CreateMediaPipeline()
        {
            gst_init(nullptr, nullptr);
            string playbinName = "playbin" + std::to_string(_instances);

            GstElement* playbin = gst_element_factory_make("playbin", playbinName.c_str());
            if (playbin) {
                _instances++;
#ifdef USE_WESTEROS
                GstElement* vidsink = gst_element_factory_make("westerossink", NULL);
                g_object_set(playbin, "video-sink", vidsink, NULL);
#endif
                GetGstWrapper()->Playbin(playbin);
                _state = Exchange::IStream::state::Prepared;
            } else {
                TRACE(Trace::Error, (_T("Error in player creation")));
                _state = Exchange::IStream::state::Error;
            }
            if (_callback != nullptr) {
                _callback->StateChange(_state);
            }
        }

        bool PlayerPlatform::IsValidPipelineState()
        {
            GstState state;
            GstState newState;
            bool pipelineState = false;

            GstStateChangeReturn stateResult = gst_element_get_state(GetGstWrapper()->Playbin(), &state, &newState, 0);
            if (stateResult == GST_STATE_CHANGE_FAILURE || stateResult == GST_STATE_CHANGE_NO_PREROLL) {
                TRACE(Trace::Error, (_T("Cannot seek, current state change is not valid: %s"), gst_element_state_change_return_get_name(stateResult)));
            } else {
                if ((stateResult == GST_STATE_CHANGE_ASYNC) || (state < GST_STATE_PAUSED)) {
                    TRACE(Trace::Error, (_T("Invalid state: %s"), gst_element_state_change_return_get_name(stateResult)));
                } else {
                    pipelineState = true;
                }
            }
            return pipelineState;
        }

        uint32_t PlayerPlatform::Worker()
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            g_object_set(GetGstWrapper()->Playbin(), "uri", _uri.c_str(), nullptr);

            gint flags;
            g_object_get(GetGstWrapper()->Playbin(), "flags", &flags, nullptr);
            flags |= 0x03 | GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO | GST_PLAY_FLAG_NATIVE_AUDIO | GST_PLAY_FLAG_NATIVE_VIDEO;
            g_object_set(GetGstWrapper()->Playbin(), "flags", flags, nullptr);

            GstBus* bus = gst_element_get_bus(GetGstWrapper()->Playbin());
            gst_bus_add_watch(bus, (GstBusFunc)GstWrapper::HandleBusMessage, this);

            _state = Exchange::IStream::state::Paused;
            if (_callback != nullptr) {
                _callback->StateChange(_state);
            }
            TRACE(Trace::Information, (string(__FUNCTION__)));
            GMainLoop* mainLoop = g_main_loop_new(NULL, FALSE);
            GetGstWrapper()->MainLoop(mainLoop);
            g_main_loop_run(mainLoop);

            TRACE(Trace::Information, (string(__FUNCTION__)));
            g_main_loop_unref(mainLoop);
            gst_element_set_state(GetGstWrapper()->Playbin(), GST_STATE_NULL);
            g_object_set(GetGstWrapper()->Playbin(), "uri", "", nullptr);
            gst_object_unref(bus);

            _speed = 0;
            _rate = 1.0;
            _absoluteTime = 0;
            GetGstWrapper()->VideoSink(nullptr);
            TRACE(Trace::Information, (string(__FUNCTION__)));
            return WPEFramework::Core::infinite;
        }

        void PlayerPlatform::Speed(const int32_t speed)
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            TRACE(Trace::Information, (_T("speed = %d"), speed));

            _speed = speed;

            GstState gstState;
            GstState pending;

            gst_element_get_state(GetGstWrapper()->Playbin(), &gstState, &pending, 0);

            GstState newGstState;
            Exchange::IStream::state newState;
            if (speed != 0) {
                int rate = 1;
                if ((speed <= 4) && (speed >= -4)) { //limiting the rate to get a working version
                    rate = speed;
                    newGstState = gstState; //No state change is required
                    newState = _state;
                } else {
                    newGstState = GST_STATE_PLAYING;
                    newState = Exchange::IStream::state::Playing;
                }
                if (GetGstWrapper()->SetRate(rate, _rate, _absoluteTime, gstState) != true) {
                    newState = Exchange::IStream::state::Error;
                }
            } else {
                newGstState = GST_STATE_PAUSED;
                newState = Exchange::IStream::state::Paused;
            }
            if (newGstState != gstState) {
                GstStateChangeReturn ret = gst_element_set_state(GetGstWrapper()->Playbin(), newGstState);
                if (ret == GST_STATE_CHANGE_FAILURE) {
                    TRACE(Trace::Error, (_T("Player state change to %s is failed"), Core::EnumerateType<Exchange::IStream::state>(newState).Data()));
                    newState = Exchange::IStream::state::Error;
                } else {
                    TRACE(Trace::Information, (_T("Player successfully changed to %s"), Core::EnumerateType<Exchange::IStream::state>(newState).Data()));
                }
            }
            if (_state != newState) {
                _state = newState;
                if (_callback != nullptr) {
                    _callback->StateChange(_state);
                }
            }
        }
    }
}
} //namespace WPEFramework::Player::Implementation
