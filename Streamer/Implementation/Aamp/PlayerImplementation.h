#ifndef _PLAYER_IMPLEMENTATION_H
#define _PLAYER_IMPLEMENTATION_H

#include "Module.h"
#include <interfaces/ITVControl.h>
#include <plugins/plugins.h>
#include <tracing/tracing.h>

#include <signal.h>
#include <stdlib.h>
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
        class GstWrapper;
        class PlayerPlatform : public Core::Thread {
        private:
            PlayerPlatform() = delete;
            PlayerPlatform(const PlayerPlatform&) = delete;
            PlayerPlatform& operator=(const PlayerPlatform&) = delete;

        public:
            PlayerPlatform(const Exchange::IStream::streamtype type, const uint8_t index, ICallback* callbacks);
            virtual ~PlayerPlatform();

        public:
            inline string Metadata() const
            {
                return string("{}");
            }
            inline Exchange::IStream::streamtype Type() const
            {
                return (Exchange::IStream::streamtype)_streamType;
            }
            inline Exchange::IStream::drmtype DRM() const
            {
                return (Exchange::IStream::drmtype)_drmType;
            }
            inline Exchange::IStream::state State() const
            {
                return (Exchange::IStream::state)_state;
            }
            uint32_t Load(const string& uri);
            void Speed(const int32_t speed);
            inline int32_t Speed() const
            {
                return _speed;
            }
            void Position(const uint64_t absoluteTime);
            uint64_t Position() const;
            inline void TimeRange(uint64_t& begin, uint64_t& end) const
            {
                begin = _begin;
                end = _end;
            }
            inline const Rectangle& Window() const
            {
                return (_rectangle);
            }
            void Window(const Rectangle& rectangle);
            inline uint32_t Order() const
            {
                return (_z);
            }
            inline void Order(const uint32_t order)
            {
                _z = order;
            }
            inline void AttachDecoder(const uint8_t index)
            {
                //Auto decoders are used for the pipeline
            }
            inline void DetachDecoder(const uint8_t index)
            {
                Terminate(); //Calling Terminate since there is no decoder specified
            }
            void Terminate();
            GstWrapper* GetGstWrapper() const { return _gstWrapper; }

        private:
            virtual uint32_t Worker() override;

            void CreateMediaPipeline();
            bool IsValidPipelineState();
            inline string UriType(const string& uri)
            {
                if (uri.find_last_of(".") != std::string::npos)
                    return uri.substr(uri.find_last_of(".") + 1);
                return "";
            }
            inline void ChangeSrcType(string& uri)
            {
                string prefix("aamp");
                if (uri.compare(0, prefix.size(), prefix))
                    uri.replace(0, prefix.size(), prefix);
            }

            inline uint64_t GetPosition(uint64_t absoluteTime);

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

            GstWrapper* _gstWrapper;
            ICallback* _callback;
            static uint8_t _instances;
        };
    }
}
} // namespace WPEFramework::Player::Implementation

#endif // _PLAYER_IMPLEMENTATION_H
