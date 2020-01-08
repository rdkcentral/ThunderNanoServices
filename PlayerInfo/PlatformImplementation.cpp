#include "Module.h"
#include <interfaces/IPlayerInfo.h>

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
        {
        }
        virtual ~AudioIteratorImplementation()
        {
        }

    public:
        bool IsValid() const override
        {
            return false;
        }
        bool Next() override
        {
            return false;
        }
        void Reset() override
        {
        }
        AudioCodec Codec() const
        {
            AudioCodec codec;
            return codec;
        }

        BEGIN_INTERFACE_MAP(AudioIteratorIImplementation)
        INTERFACE_ENTRY(Exchange::IPlayerProperties::IAudioIterator)
        END_INTERFACE_MAP
    };

    class VideoIteratorImplementation : public Exchange::IPlayerProperties::IVideoIterator {
    public:
        VideoIteratorImplementation() = delete;
        VideoIteratorImplementation(const VideoIteratorImplementation&) = delete;
        VideoIteratorImplementation& operator= (const VideoIteratorImplementation&) = delete;

        VideoIteratorImplementation(const std::list<VideoCodec>& codecs)
        {
        }
        virtual ~VideoIteratorImplementation()
        {
        }

    public:
        bool IsValid() const override
        {
            return false;
        }
        bool Next() override
        {
            return false;
        }
        void Reset() override
        {
        }
        VideoCodec Codec() const
        {
            VideoCodec codec;
            return codec;
        }

        BEGIN_INTERFACE_MAP(VideoIteratorIImplementation)
        INTERFACE_ENTRY(Exchange::IPlayerProperties::IVideoIterator)
        END_INTERFACE_MAP

    };

public:
    PlayerInfoImplementation() {

        UpdateAudioCodecInfo();
        UpdateVideoCodecInfo();
    }

    PlayerInfoImplementation(const PlayerInfoImplementation&) = delete;
    PlayerInfoImplementation& operator= (const PlayerInfoImplementation&) = delete;
    virtual ~PlayerInfoImplementation()
    {
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
    void UpdateAudioCodecInfo()
    {
    }
    void UpdateVideoCodecInfo()
    {
    }

private:
    std::list<Exchange::IPlayerProperties::IAudioIterator::AudioCodec> _audioCodecs;
    std::list<Exchange::IPlayerProperties::IVideoIterator::VideoCodec> _videoCodecs;
};

    SERVICE_REGISTRATION(PlayerInfoImplementation, 1, 0);
}
}
