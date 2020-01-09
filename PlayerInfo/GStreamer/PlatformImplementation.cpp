#include "../Module.h"
#include <interfaces/IPlayerInfo.h>

#include <gst/gst.h>

namespace WPEFramework {
namespace Plugin {

class PlayerInfoImplementation : public Exchange::IPlayerProperties {

private:
    class AudioIteratorImplementation : public Exchange::IPlayerProperties::IAudioIterator {
    public:
        AudioIteratorImplementation() = delete;
        AudioIteratorImplementation(const AudioIteratorImplementation&) = delete;
        AudioIteratorImplementation& operator= (const AudioIteratorImplementation&) = delete;

        AudioIteratorImplementation(const std::list<AudioCodec>& codecs)
            : _index(0)
            , _codecs(codecs)
        {
        }
        virtual ~AudioIteratorImplementation()
        {
            _codecs.clear();
        }

    public:
        bool IsValid() const override
        {
            return ((_index != 0) && (_index <= _codecs.size()));
        }
        bool Next() override
        {
            if (_index == 0) {
                _index = 1;
            } else if (_index <= _codecs.size()) {
                _index++;
            }
            return (IsValid());
        }
        void Reset() override
        {
            _index = 0;
        }
        AudioCodec Codec() const
        {
            ASSERT(IsValid() == true);
            std::list<AudioCodec>::const_iterator codec = std::next(_codecs.begin(), _index - 1);
            ASSERT(*codec != AudioCodec::UNDEFINED);

            return *codec;
        }

        BEGIN_INTERFACE_MAP(AudioIteratorIImplementation)
        INTERFACE_ENTRY(Exchange::IPlayerProperties::IAudioIterator)
        END_INTERFACE_MAP

    private:
        uint16_t _index;
        std::list<AudioCodec> _codecs;
    };

    class VideoIteratorImplementation : public Exchange::IPlayerProperties::IVideoIterator {
    public:
        VideoIteratorImplementation() = delete;
        VideoIteratorImplementation(const VideoIteratorImplementation&) = delete;
        VideoIteratorImplementation& operator= (const VideoIteratorImplementation&) = delete;

        VideoIteratorImplementation(const std::list<VideoCodec>& codecs)
            : _index(0)
            , _codecs(codecs)
        {
        }
        virtual ~VideoIteratorImplementation()
        {
            _codecs.clear();
        }

    public:
        bool IsValid() const override
        {
            return ((_index != 0) && (_index <= _codecs.size()));
        }
        bool Next() override
        {
            if (_index == 0) {
                _index = 1;
            } else if (_index <= _codecs.size()) {
                _index++;
            }
            return (IsValid());
        }
        void Reset() override
        {
            _index = 0;
        }
        VideoCodec Codec() const
        {
            ASSERT(IsValid() == true);
            std::list<VideoCodec>::const_iterator codec = std::next(_codecs.begin(), _index - 1);

            ASSERT(*codec != VideoCodec::UNDEFINED);

            return *codec;
        }

        BEGIN_INTERFACE_MAP(VideoIteratorIImplementation)
        INTERFACE_ENTRY(Exchange::IPlayerProperties::IVideoIterator)
        END_INTERFACE_MAP

    private:
        uint16_t _index;
        std::list<VideoCodec> _codecs;
    };

    typedef std::map<const string, const Exchange::IPlayerProperties::IAudioIterator::AudioCodec> AudioCaps;
    typedef std::map<const string, const Exchange::IPlayerProperties::IVideoIterator::VideoCodec> VideoCaps;

public:
    PlayerInfoImplementation() {
        gst_init(0, nullptr);
        UpdateAudioCodecInfo();
        UpdateVideoCodecInfo();
    }

    PlayerInfoImplementation(const PlayerInfoImplementation&) = delete;
    PlayerInfoImplementation& operator= (const PlayerInfoImplementation&) = delete;
    virtual ~PlayerInfoImplementation()
    {
        _audioCodecs.clear();
        _videoCodecs.clear();
    }

public:
    Exchange::IPlayerProperties::IAudioIterator* AudioCodec() const override
    {
        return (Core::Service<AudioIteratorImplementation>::Create<Exchange::IPlayerProperties::IAudioIterator>(_audioCodecs));
    }
    Exchange::IPlayerProperties::IVideoIterator* VideoCodec() const override
    {
        return (Core::Service<VideoIteratorImplementation>::Create<Exchange::IPlayerProperties::IVideoIterator>(_videoCodecs));
    }

   BEGIN_INTERFACE_MAP(PlayerInfoImplementation)
        INTERFACE_ENTRY(Exchange::IPlayerProperties)
   END_INTERFACE_MAP

private:

    inline bool GstRegistryHasElementForMediaType(GList* elementFactories, const string& capsString)
    {
        GstCaps* caps = gst_caps_from_string(capsString.c_str());
        GList* candidates = gst_element_factory_list_filter(elementFactories, caps, GST_PAD_SINK, false);
        bool result = candidates;

        gst_plugin_feature_list_free(candidates);
        gst_caps_unref(caps);
        return result;
    }

    void UpdateAudioCodecInfo()
    {
        AudioCaps audioCaps = {
            {"audio/mpeg, mpegversion=(int)1", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_MPEG1},
            {"audio/mpeg, mpegversion=(int)2", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_MPEG2},
            {"audio/mpeg, mpegversion=(int)4", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_MPEG4},
            {"audio/mpeg, mpegversion=(int)1, layer=(int)[1, 3]", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_MPEG3},
            {"audio/mpeg, mpegversion=(int){2, 4}", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_AAC},
            {"audio/x-ac3", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_AC3},
            {"audio/x-eac3", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_AC3_PLUS},
            {"audio/x-opus", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_OPUS},
            {"audio/x-dts", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_DTS},
            {"audio/x-vorbis", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_VORBIS_OGG},
            {"audio/x-wav", Exchange::IPlayerProperties::IAudioIterator::AudioCodec::AUDIO_WAV},
        };

        GList* audioDecoderFactories = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_PARSER | GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO, GST_RANK_MARGINAL);
        for (auto index: audioCaps) {

            if (GstRegistryHasElementForMediaType(audioDecoderFactories, index.first) == true) {
                _audioCodecs.push_back(index.second);
            }
        }
    }
    void UpdateVideoCodecInfo()
    {
        VideoCaps videoCaps = {
            {"video/x-h263", Exchange::IPlayerProperties::IVideoIterator::VideoCodec::VIDEO_H263},
            {"video/x-h264, profile=(string)high", Exchange::IPlayerProperties::IVideoIterator::VideoCodec::VIDEO_H264},
            {"video/x-h265", Exchange::IPlayerProperties::IVideoIterator::VideoCodec::VIDEO_H265},
            {"video/mpeg, mpegversion=(int){1,2}, systemstream=(boolean)false", Exchange::IPlayerProperties::IVideoIterator::VideoCodec::VIDEO_MPEG},
            {"video/x-vp8", Exchange::IPlayerProperties::IVideoIterator::VideoCodec::VIDEO_VP8},
            {"video/x-vp9", Exchange::IPlayerProperties::IVideoIterator::VideoCodec::VIDEO_VP9},
            {"video/x-vp10", Exchange::IPlayerProperties::IVideoIterator::VideoCodec::VIDEO_VP10}
        };

        GList* videoDecoderFactories = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_PARSER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO, GST_RANK_MARGINAL);
        for (auto index: videoCaps) {

            if (GstRegistryHasElementForMediaType(videoDecoderFactories, index.first) == true) {
                _videoCodecs.push_back(index.second);
            }
        }
    }

private:
    std::list<Exchange::IPlayerProperties::IAudioIterator::AudioCodec> _audioCodecs;
    std::list<Exchange::IPlayerProperties::IVideoIterator::VideoCodec> _videoCodecs;
};

    SERVICE_REGISTRATION(PlayerInfoImplementation, 1, 0);
}
}
