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

#pragma once

#include "Module.h"

namespace WPEFramework {

namespace A2DP {

    class AudioCodecSBC : public IAudioCodec {
    public:
        static constexpr uint8_t CODEC_TYPE = 0x00; // SBC

    public:
        enum qualityprofile {
            COMPATIBLE,
            LQ,
            MQ,
            HQ,
            XQ
        };

        class Config : public Core::JSON::Container {
        public:
            enum channels {
                STEREO,
                MONO
            };

        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Profile(COMPATIBLE)
                , SampleRate(44100)
                , Channels(STEREO)
            {
                Add(_T("profile"), &Profile);
                Add(_T("samplerate"), &SampleRate);
                Add(_T("channels"), &Channels);
            }
            ~Config() = default;

        public:
            Core::JSON::EnumType<qualityprofile> Profile;
            Core::JSON::DecUInt32 SampleRate;
            Core::JSON::EnumType<channels> Channels;
        }; // class Config

    private:
        static constexpr uint8_t MIN_BITPOOL = 2;
        static constexpr uint8_t MAX_BITPOOL = 250;

        class Format {
        public:
            enum samplingfrequency : uint8_t {
                SF_INVALID      = 0,
                SF_48000_HZ     = 1, // mandatory for sink
                SF_44100_HZ     = 2, // mandatory for sink
                SF_32000_HZ     = 4,
                SF_16000_HZ     = 8
            };

            enum channelmode : uint8_t  {
                CM_INVALID      = 0,
                CM_JOINT_STEREO = 1, // all mandatory for sink
                CM_STEREO       = 2,
                CM_DUAL_CHANNEL = 4,
                CM_MONO         = 8
            };

            enum blocklength : uint8_t  {
                BL_INVALID      = 0,
                BL_16           = 1,  // all mandatory for sink
                BL_12           = 2,
                BL_8            = 4,
                BL_4            = 8,
            };

            enum subbands : uint8_t  {
                SB_INVALID      = 0,
                SB_8            = 1, // all mandatory for sink
                SB_4            = 2,
            };

            enum allocationmethod : uint8_t {
                AM_INVALID      = 0,
                AM_LOUDNESS     = 1, // all mandatory for sink
                AM_SNR          = 2,
            };

        public:
            Format()
                : _samplingFrequency(SF_44100_HZ)
                , _channelMode(CM_JOINT_STEREO)
                , _blockLength(BL_16)
                , _subBands(SB_8)
                , _allocationMethod(AM_LOUDNESS)
                , _minBitpool(MIN_BITPOOL)
                , _maxBitpool(MIN_BITPOOL) // not an error
            {
            }
            ~Format() = default;
            Format(const Format&) = default;
            Format& operator=(const Format&) = default;

        public:
            void Serialize(Bluetooth::Buffer& config) const
            {
                uint8_t octet;
                uint8_t scratchpad[16];
                Bluetooth::DataRecord data(scratchpad, sizeof(scratchpad));

                data.Push(IAudioCodec::MEDIA_TYPE);
                data.Push(CODEC_TYPE);

                octet = ((static_cast<uint8_t>(_samplingFrequency) << 4) | static_cast<uint8_t>(_channelMode));
                data.Push(octet);

                octet = ((static_cast<uint8_t>(_blockLength) << 4) | (static_cast<uint8_t>(_subBands) << 2) | static_cast<uint8_t>(_allocationMethod));
                data.Push(octet);

                data.Push(_minBitpool);
                data.Push(_maxBitpool);

                data.Export(config);
            }
            void Deserialize(const Bluetooth::Buffer& config)
            {
                Bluetooth::DataRecord data(config);

                uint8_t octet{};
                data.Pop(octet); // AUDIO, already checked
                ASSERT(octet == IAudioCodec::MEDIA_TYPE);
                data.Pop(octet); // SBC, already checked
                ASSERT(octet == CODEC_TYPE);

                data.Pop(octet);
                _samplingFrequency = (octet >> 4);
                _channelMode = (octet & 0xFF);

                data.Pop(octet);
                _blockLength = (octet >> 4);
                _subBands = ((octet >> 2) & 0x3);
                _allocationMethod = (octet & 0x3);

                data.Pop(_minBitpool);
                data.Pop(_maxBitpool);
            }

        public:
            uint8_t SamplingFrequency() const
            {
                return (_samplingFrequency);
            }
            uint8_t ChannelMode() const
            {
                return (_channelMode);
            }
            uint8_t BlockLength() const
            {
                return (_blockLength);
            }
            uint8_t SubBands() const
            {
                return (_subBands);
            }
            uint8_t AllocationMethod() const
            {
                return (_allocationMethod);
            }
            uint8_t MinBitpool() const
            {
                return (_minBitpool);
            }
            uint8_t MaxBitpool() const
            {
                return (_maxBitpool);
            }

        public:
            void SamplingFrequency(const samplingfrequency sf)
            {
                _samplingFrequency = sf;
            }
            void ChannelMode(const channelmode cm)
            {
                _channelMode = cm;
            }
            void BlockLength(const blocklength bl)
            {
                _blockLength = bl;
            }
            void SubBands(const subbands sb)
            {
                _subBands = sb;
            }
            void AllocationMethod(const allocationmethod am)
            {
                _allocationMethod = am;
            }
            void MinBitpool(const uint8_t value)
            {
                _minBitpool = (value < MIN_BITPOOL? MIN_BITPOOL : value);
            }
            void MaxBitpool(const uint8_t value)
            {
                _maxBitpool = (value > MAX_BITPOOL? MAX_BITPOOL : value);
            }

        private:
            uint8_t _samplingFrequency;
            uint8_t _channelMode;
            uint8_t _blockLength;
            uint8_t _subBands;
            uint8_t _allocationMethod;
            uint8_t _minBitpool;
            uint8_t _maxBitpool;
        }; // class Format

    public:
        AudioCodecSBC(const Bluetooth::Buffer& config)
            : _lock()
            , _supported()
            , _actuals()
            , _preferredProfile(HQ) // TODO: Perhaps this should come from a config file...?
            , _profile(COMPATIBLE)
            , _sbcHandle(nullptr)
            , _preferredBitpool(0)
            , _bitpool(0)
            , _bitRate(0)
            , _sampleRate(0)
            , _channels(0)
            , _inFrameSize(0)
            , _outFrameSize(0)
            , _frameDuration(0)
        {
            _supported.Deserialize(config);
            SBCInitialize();
        }
        ~AudioCodecSBC() override
        {
            SBCDeinitialize();
        }

    public:
        IAudioCodec::type Type() const override
        {
            return (IAudioCodec::SBC);
        }
        uint32_t BitRate() const override
        {
            return (_bitRate);
        }
        uint32_t ClockRate() const override
        {
            return (_sampleRate);
        }
        uint8_t Channels() const override
        {
            return (_channels);
        }

    public:
        uint32_t Configure(const string& format) override;

        void Configuration(string& format) const override;

        void SerializeConfiguration(Bluetooth::Buffer& output) const override;

        uint32_t QOS(const int8_t policy);

        uint32_t Encode(const uint32_t inBufferSize, const uint8_t inBuffer[],
                        uint32_t& outBufferSize, uint8_t outBuffer[]) const override;

        uint32_t Decode(const uint32_t inBufferSize, const uint8_t inBuffer[],
                        uint32_t& outBufferSize, uint8_t outBuffer[]) const override;

    private:
        void Bitpool(uint8_t value);

    private:
        void DumpConfiguration() const;
        void DumpBitrateConfiguration() const;

    private:
        void SBCInitialize();
        void SBCDeinitialize();
        void SBCConfigure();

    private:
        mutable Core::CriticalSection _lock;
        Format _supported;
        Format _actuals;
        qualityprofile _preferredProfile;
        qualityprofile _profile;
        void* _sbcHandle;
        uint8_t _preferredBitpool;
        uint8_t _bitpool;
        uint32_t _bitRate;
        uint32_t _sampleRate;
        uint8_t _channels;
        uint32_t _inFrameSize;
        uint32_t _outFrameSize;
        uint32_t _frameDuration;

    private:
        struct SBCHeader {
            uint8_t frameCount;
        } __attribute__((packed));
    }; // class AudioCodecSBC

} // namespace A2DP

}
