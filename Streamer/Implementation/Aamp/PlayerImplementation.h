#ifndef _PLAYER_IMPLEMENTATION_H
#define _PLAYER_IMPLEMENTATION_H

#include <plugins/plugins.h>
#include <tracing/tracing.h>
#include <interfaces/ITVControl.h>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <stdlib.h>
#include <signal.h>
#include <string>

typedef enum {
    GST_PLAY_FLAG_VIDEO = (1 << 0), // 0x001
    GST_PLAY_FLAG_AUDIO = (1 << 1), // 0x002
    GST_PLAY_FLAG_TEXT = (1 << 2), // 0x004
    GST_PLAY_FLAG_VIS = (1 << 3), // 0x008
    GST_PLAY_FLAG_SOFT_VOLUME = (1 << 4), // 0x010
    GST_PLAY_FLAG_NATIVE_AUDIO = (1 << 5), // 0x020
    GST_PLAY_FLAG_NATIVE_VIDEO = (1 << 6), // 0x040
    GST_PLAY_FLAG_DOWNLOAD = (1 << 7), // 0x080
    GST_PLAY_FLAG_BUFFERING = (1 << 8), // 0x100
    GST_PLAY_FLAG_DEINTERLACE = (1 << 9), // 0x200
    GST_PLAY_FLAG_SOFT_COLORBALANCE = (1 << 10) // 0x400
} GstPlayFlags;

namespace WPEFramework {

namespace Player {

namespace Implementation {

class PlayerPlatform: public Core::Thread {
private:
    PlayerPlatform() = delete;
    PlayerPlatform(const PlayerPlatform&) = delete;
    PlayerPlatform& operator= (const PlayerPlatform&) = delete;

public:
    PlayerPlatform(const Exchange::IStream::streamtype type, const uint8_t index, ICallback* callbacks)
        : _uri("")
        , _state(Exchange::IStream::NotAvailable)
        , _streamType(type) 
        , _drmType(Exchange::IStream::Unknown)
        , _speed(0)
        , _rate(0)
        , _absoluteTime(0)
        , _begin(0)
        , _end(~0)
        , _z(0)
        , _rectangle()
        , _callback(callbacks)
        , _mainLoop(nullptr)
        , _playbin(nullptr)
        , _videoSink(nullptr)
    {
        CreateMediaPipeline();
    }
    virtual ~PlayerPlatform() {
        Terminate();
        gst_object_unref(_playbin);
        _instances--;
    }

public:
    inline string Metadata() const {
        return string("{}");
    }
    inline Exchange::IStream::streamtype Type() const {
        return (Exchange::IStream::streamtype)_streamType;
    }
    inline Exchange::IStream::drmtype DRM() const {
        return (Exchange::IStream::drmtype)_drmType;
    }
    inline Exchange::IStream::state State() const {
        return (Exchange::IStream::state)_state;
    }
    inline uint32_t Load(const string& uri) {
        TRACE(Trace::Information, (string(__FUNCTION__)));
        TRACE(Trace::Information, (_T("uri = %s"), uri.c_str()));
        if (IsRunning() == true) {
            g_main_loop_quit(_mainLoop);
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
    void Speed(const int32_t speed);
    inline int32_t Speed() const {
        return _speed;
    }
    inline void Position(const uint64_t absoluteTime) {
        uint64_t time = GetPosition(absoluteTime);
        if (time) {
            if (IsValidPipelineState() == true) {
                gint64 cur = GST_CLOCK_TIME_NONE;
                if (!gst_element_query_position(_playbin, GST_FORMAT_TIME, &cur)) {
                    TRACE(Trace::Error, (_T("Query position failed")));
                } else {
                    TRACE(Trace::Information, (_T("Query position Success")));
                    if (time != ((cur/GST_SECOND) + _absoluteTime)) { //Avoid seek, if media already in the same position
                        cur = GST_SECOND * time;
                        if (!gst_element_seek(_playbin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, cur, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
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
    inline uint64_t Position() const {
        gint64 cur = GST_CLOCK_TIME_NONE;
        if (!gst_element_query_position(_playbin, GST_FORMAT_TIME, &cur)) {
            TRACE(Trace::Error, (_T("Query position failed")));
        } else {
            TRACE(Trace::Information, (_T("Query position Success")));
        }
        return ((cur/GST_SECOND) + _absoluteTime);
    }
    inline void TimeRange(uint64_t& begin, uint64_t& end) const {
        begin = _begin;
        end = _end;
    }
    inline const Rectangle& Window() const {
        return (_rectangle);
    }
    inline void Window (const Rectangle& rectangle) {
        _rectangle = rectangle;
        if (_videoSink != nullptr) {
            gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(_videoSink), _rectangle.X, _rectangle.Y, _rectangle.Width, _rectangle.Height);
        }
    }
    inline uint32_t Order() const {
        return (_z);
    }
    inline void Order(const uint32_t order) {
        _z = order;
    }
    inline void AttachDecoder(const uint8_t index) {
        //Auto decoders are used for the pipeline
    }
    inline void DetachDecoder(const uint8_t index) {
        Terminate(); //Calling Terminate since there is no decoder specified
    }
    inline void Terminate () {
        if (IsRunning() == true) {
            g_main_loop_quit(_mainLoop);
        }
 
        TRACE(Trace::Information, (string(__FUNCTION__)));
        Block();
        Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);

        gst_element_set_state(_playbin, GST_STATE_NULL);
    }

private: 
    virtual uint32_t Worker() override;

    void CreateMediaPipeline();
    bool IsValidPipelineState();
    bool SetRate(int rate, GstState state);
    inline string UriType(const string& uri) {
        if (uri.find_last_of(".") != std::string::npos)
            return uri.substr(uri.find_last_of(".") + 1);
        return "";
    }
    inline string ChangeSrcType(string& uri) {
        string prefix("aamp");
        if (uri.compare(0, prefix.size(), prefix))
            uri.replace(0, prefix.size(), prefix);
    }
    static gboolean HandleBusMessage(GstBus *bus, GstMessage *msg, void* arg);

    inline uint64_t GetPosition(uint64_t absoluteTime) {
        gint64 duration;
        if (!gst_element_query_duration (_playbin, GST_FORMAT_TIME, &duration)) {
            TRACE(Trace::Error, (_T("Unable to retrieve duration")));
        }

        return ((absoluteTime < duration) ? absoluteTime : duration);
    }

private:
    string _uri;

    Exchange::IStream::state _state;
    Exchange::IStream::drmtype _drmType;
    Exchange::IStream::streamtype _streamType;

    int32_t _speed;
    int32_t _rate;
    uint64_t _absoluteTime;
    uint64_t _begin;
    uint64_t _end;
    uint32_t _z;
    Rectangle _rectangle;

    ICallback* _callback;
    GMainLoop* _mainLoop;
    GstElement* _playbin;
    GstElement* _videoSink;

    static uint8_t _instances;
};

} } } // namespace WPEFramework::Player::Implementation

#endif // _PLAYER_IMPLEMENTATION_H
