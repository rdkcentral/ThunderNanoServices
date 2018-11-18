
#include "PlayerImplementation.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Exchange::IStream::state)

    { Exchange::IStream::state::Paused, _TXT("Paused") },
    { Exchange::IStream::state::Playing, _TXT("Playing") },
    { Exchange::IStream::state::NotAvailable, _TXT("NotAvailable") },

ENUM_CONVERSION_END(Exchange::IStream::state);

namespace Player {

namespace Implementation {

uint8_t PlayerPlatform::_instances = 0;
gboolean PlayerPlatform::HandleBusMessage(GstBus *bus, GstMessage *msg, void* arg)
{
    GError *error;
    gchar *info;
    PlayerPlatform* player = (PlayerPlatform*) arg;

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
        g_main_loop_quit(player->_mainLoop);
        break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
        GstState old, now, pending;
        gst_message_parse_state_changed(msg, &old, &now, &pending);
        if (memcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)), "playbin", 7) == 0) {
            GST_FIXME( "**PLAYBINTEST: element %s state change : %s -> %s . pending state %s\n", GST_ELEMENT_NAME(GST_MESSAGE_SRC(msg)),
                gst_element_state_get_name(old), gst_element_state_get_name(now), gst_element_state_get_name(pending) );
        }
        if (now == GST_STATE_PLAYING) {
            if ((memcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)), "glimagesink", strlen("glimagesink")) == 0) ||
                (memcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)), "brcmvideosink", strlen("brcmvideosink")) == 0) ||
                (memcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)), "video-sink", strlen("video-sink") == 0))) {
                if (player->_videoSink == nullptr) {
                    player->_videoSink = GST_ELEMENT(GST_MESSAGE_SRC(msg));
                    gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(player->_videoSink), player->_rectangle.X, player->_rectangle.Y, player->_rectangle.Width, player->_rectangle.Height);
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

void PlayerPlatform::CreateMediaPipeline()
{
    gst_init(nullptr, nullptr);
    string playbinName = "playbin" + std::to_string(_instances);
    _playbin = gst_element_factory_make("playbin", playbinName.c_str());
    if (_playbin) {
        _instances++;
#ifdef USE_WESTEROS
        GstElement* vidsink = gst_element_factory_make("westerossink", NULL);
        g_object_set(_playbin, "video-sink", vidsink, NULL);
#endif
    } else {
        TRACE(Trace::Error, (_T("Error in player creation")));
        _state = Exchange::IStream::state::Error;
    }
}

bool PlayerPlatform::IsValidPipelineState()
{
    GstState state;
    GstState newState;
    bool pipelineState = false;

    GstStateChangeReturn stateResult = gst_element_get_state(_playbin, &state, &newState, 0);
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

bool PlayerPlatform::SetRate(int rate, GstState state)
{
    bool status = false;
    if (rate != _rate) {
        if ((state == GST_STATE_PLAYING) || (state == GST_STATE_PAUSED)) {
            _rate = rate;
            gint64 cur = GST_CLOCK_TIME_NONE;
            if (!gst_element_query_position(_playbin, GST_FORMAT_TIME, &cur)) {
                TRACE(Trace::Error, (_T("Query position failed")));
            } else {
                uint64_t time = GetPosition((cur/GST_SECOND) + _absoluteTime);
                gboolean ret = 0;
                if (rate > 0) {
                    ret = gst_element_seek(_playbin, rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
                } else {
                    ret = gst_element_seek(_playbin, rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE, GST_SEEK_TYPE_SET, time);
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

uint32_t PlayerPlatform::Worker()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    g_object_set(_playbin, "uri", _uri.c_str(), nullptr);

    gint flags;
    g_object_get(_playbin, "flags", &flags, nullptr);
    flags |= 0x03 | GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO | GST_PLAY_FLAG_NATIVE_AUDIO | GST_PLAY_FLAG_NATIVE_VIDEO;
    g_object_set(_playbin, "flags", flags, nullptr);

    GstBus* bus = gst_element_get_bus(_playbin);
    gst_bus_add_watch(bus, (GstBusFunc) PlayerPlatform::HandleBusMessage, this);

    _state = Exchange::IStream::state::Paused;
    if (_callback != nullptr) {
        _callback->StateChange(_state);
    }
    TRACE(Trace::Information, (string(__FUNCTION__)));
    _mainLoop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(_mainLoop);

    TRACE(Trace::Information, (string(__FUNCTION__)));
    g_main_loop_unref(_mainLoop);
    gst_element_set_state(_playbin, GST_STATE_NULL);
    g_object_set(_playbin, "uri", "", nullptr);
    gst_object_unref(bus);

    _speed = 0;
    _rate = 1.0;
    _absoluteTime = 0;
    _videoSink = 0;
    TRACE(Trace::Information, (string(__FUNCTION__)));
    return WPEFramework::Core::infinite;
}

void PlayerPlatform::Speed(const int32_t speed) {
    TRACE(Trace::Information, (string(__FUNCTION__)));
    TRACE(Trace::Information, (_T("speed = %d"), speed));

    _speed = speed;

    GstState gstState;
    GstState pending;

    gst_element_get_state(_playbin, &gstState, &pending, 0);

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
        if (SetRate(rate, gstState) != true) {
            newState = Exchange::IStream::state::Error;
        }
    } else {
        newGstState = GST_STATE_PAUSED;
        newState = Exchange::IStream::state::Paused;
    }
    if (newGstState != gstState) {
        GstStateChangeReturn ret = gst_element_set_state(_playbin, newGstState);
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

} } }  //namespace WPEFramework::Player::Implementation
