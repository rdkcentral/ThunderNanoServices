#include "../Module.h"
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
            std::list<AudioCodec>::const_iterator codec = std::next(_codecs.begin(), _index);

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
            std::list<VideoCodec>::const_iterator codec = std::next(_codecs.begin(), _index);

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
