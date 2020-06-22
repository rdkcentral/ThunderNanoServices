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

#include "Module.h"

#include "AudioCodec.h"
#include "AudioCodecSBC.h"

#include <sbc/sbc.h>

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(A2DP::AudioCodecSBC::qualityprofile)
    { A2DP::AudioCodecSBC::COMPATIBLE, _TXT("compatible") },
    { A2DP::AudioCodecSBC::LQ, _TXT("lq") },
    { A2DP::AudioCodecSBC::MQ, _TXT("mq") },
    { A2DP::AudioCodecSBC::HQ, _TXT("hq") },
    { A2DP::AudioCodecSBC::XQ, _TXT("xq") },
ENUM_CONVERSION_END(A2DP::AudioCodecSBC::qualityprofile);

ENUM_CONVERSION_BEGIN(A2DP::AudioCodecSBC::Config::channels)
    { A2DP::AudioCodecSBC::Config::MONO, _TXT("mono") },
    { A2DP::AudioCodecSBC::Config::STEREO, _TXT("stereo") },
ENUM_CONVERSION_END(A2DP::AudioCodecSBC::Config::channels);

namespace A2DP {

    /* virtual */ uint32_t AudioCodecSBC::Configure(const string& format)
    {
        uint32_t result = Core::ERROR_NONE;

        qualityprofile preferredProfile = _preferredProfile;
        Format::samplingfrequency frequency = Format::SF_INVALID;
        Format::channelmode channelMode = Format::CM_INVALID;
        uint8_t maxBitpool = _supported.MinBitpool();

        Config config;
        config.FromString(format);

        if (config.Profile.IsSet() == true) {
            // Profile requested in config, this will now be the target quality.
            if (config.Profile.Value() != COMPATIBLE) {
                preferredProfile = config.Profile.Value();
            } else {
                preferredProfile = LQ;
            }
        }

        if (config.SampleRate.IsSet() == true) {
            switch (config.SampleRate.Value()) {
            case 16000:
                frequency = Format::SF_16000_HZ;
                break;
            case 32000:
                frequency = Format::SF_32000_HZ;
                break;
            case 44100:
                frequency = Format::SF_44100_HZ;
                break;
            case 48000:
                frequency = Format::SF_48000_HZ;
                break;
            default:
                break;
            }

            frequency = static_cast<Format::samplingfrequency>(frequency & _supported.SamplingFrequency());
        }

        if (config.Channels.IsSet() == true) {
            switch (config.Channels.Value()) {
            case Config::MONO:
                channelMode = Format::CM_MONO;
                break;
            case Config::STEREO:
                // Joint-Stereo compresses better than regular stereo when the signal is concentrated
                // in the middle of the stereo image, what is quite typical.
                channelMode = Format::CM_JOINT_STEREO;
                break;
            }

            channelMode = static_cast<Format::channelmode>(channelMode & _supported.ChannelMode());
        }

        if ((channelMode != Format::CM_INVALID) && (frequency != Format::SF_INVALID)) {
            bool stereo = (channelMode != Format::CM_MONO);

            // Select bitpool and channel mode based on preferred format...
            // (Note that bitpools for sample rates of 16 and 32 kHz are not specified, hence will receive same values as 44,1 kHz.)
            if (preferredProfile == XQ) {
                if (_supported.MaxBitpool() >= 38) {
                    maxBitpool = 38;
                    if (stereo == true) {
                        if (_supported.MaxBitpool() >= 76) {
                            maxBitpool = 76;
                        } else {
                            // Max supported bitpool is too low for XQ joint-stereo, so try 38 on dual channel instead,
                            // what should give similar quality/bitrate.
                            channelMode = Format::CM_DUAL_CHANNEL;
                        }
                    }
                } else {
                    // XQ not supported, drop to the next best one...
                    preferredProfile = HQ;
                }
            }

            if (preferredProfile == HQ) {
                if (frequency == Format::SF_48000_HZ) {
                    maxBitpool = (stereo? 51 : 29);
                } else {
                    maxBitpool = (stereo? 53 : 31);
                }

                if (maxBitpool > _supported.MaxBitpool()) {
                    preferredProfile = MQ;
                }
            }

            if (preferredProfile == MQ) {
                if (frequency == Format::SF_48000_HZ) {
                    maxBitpool = (stereo? 33 : 18);
                } else {
                    maxBitpool = (stereo? 35 : 19);
                }

                if (maxBitpool > _supported.MaxBitpool()) {
                    preferredProfile = LQ;
                }
            }

            if (preferredProfile == LQ) {
                maxBitpool = (stereo? 29 : 15);

                if (maxBitpool > _supported.MaxBitpool()) {
                    preferredProfile = COMPATIBLE;
                }
            }

            if (preferredProfile == COMPATIBLE) {
                // Use whatever is the maximum supported bitpool, however low it is.
                maxBitpool = _supported.MaxBitpool();
            }

            _actuals.SamplingFrequency(frequency);
            _actuals.ChannelMode(channelMode);
            _actuals.MaxBitpool(maxBitpool);
            _profile = preferredProfile;

            DumpConfiguration();


            Bitpool(maxBitpool);
        } else {
            result = Core::ERROR_BAD_REQUEST;
            TRACE(Trace::Error, (_T("Unsuppored SBC paramters requested")));
        }

        return (result);
    }

    /* virtual */ void AudioCodecSBC::Configuration(string& currentFormat) const
    {
        Config config;

        switch (_actuals.SamplingFrequency()) {
        case Format::SF_48000_HZ:
            config.SampleRate = 48000;
            break;
        case Format::SF_44100_HZ:
            config.SampleRate = 44100;
            break;
        case Format::SF_32000_HZ:
            config.SampleRate = 32000;
            break;
        case Format::SF_16000_HZ:
            config.SampleRate = 16000;
            break;
        default:
            ASSERT(false && "Invalid sampling frequency configured");
            break;
        }

        switch (_actuals.ChannelMode()) {
        case Format::CM_MONO:
            config.Channels = Config::MONO;
            break;
        case Format::CM_DUAL_CHANNEL:
        case Format::CM_STEREO:
        case Format::CM_JOINT_STEREO:
            config.Channels = Config::STEREO;
            break;
        default:
            ASSERT(false && "Invalid channel mode configured");
            break;
        }

        config.Profile = _profile;

        config.ToString(currentFormat);
    }

    /* virtual */ uint32_t AudioCodecSBC::Encode(const uint32_t inBufferSize, const uint8_t inBuffer[],
                                                 uint32_t& outSize, uint8_t outBuffer[]) const
    {
        ASSERT(_inFrameSize != 0);
        ASSERT(_outFrameSize != 0);

        ASSERT(inBuffer != nullptr);
        ASSERT(outBuffer != nullptr);

        uint32_t consumed = 0;
        uint32_t produced = sizeof(SBCHeader);
        uint16_t count = 0;

        if ((_inFrameSize != 0) && (_outFrameSize != 0)) {

            const uint8_t MAX_FRAMES = 15; // only a four bit number holds the number of frames in a packet

            uint16_t frames = (inBufferSize / _inFrameSize);
            uint32_t available = (outSize - produced);

            ASSERT(outSize >= sizeof(SBCHeader));

            if (frames > MAX_FRAMES) {
                frames = MAX_FRAMES;
            }

            while ((frames-- > 0)
                    && (inBufferSize >= (consumed + _inFrameSize))
                    && (available >= _outFrameSize)) {

                ssize_t written = 0;
                ssize_t read = ::sbc_encode(static_cast<::sbc_t*>(_sbcHandle),
                                            (inBuffer + consumed), _inFrameSize,
                                            (outBuffer + produced), available,
                                            &written);

                if (read < 0) {
                    TRACE_L1("Failed to encode an SBC frame!");
                    break;
                } else {
                    consumed += read;
                    available -= written;
                    produced += written;
                    count++;
                }
            }
        }

        if (count > 0) {
            SBCHeader* header = reinterpret_cast<SBCHeader*>(outBuffer);
            header->frameCount = (count & 0xF);
            outSize = produced;
        } else {
            outSize = 0;
        }

        return (consumed);
    }

    /* virtual */ uint32_t AudioCodecSBC::Decode(const uint32_t inBufferSize, const uint8_t inBuffer[],
                                                 uint32_t& outBufferSize, uint8_t outBuffer[]) const
    {
        ASSERT(false && "SBC decode not implemented"); // TODO...?
        return (0);
    }

    void AudioCodecSBC::SBCInitialize()
    {
        ASSERT(_sbcHandle == nullptr);

        _sbcHandle = static_cast<void*>(new ::sbc_t);
        ASSERT(_sbcHandle != nullptr);

        ::sbc_init(static_cast<::sbc_t*>(_sbcHandle), 0L);
    }

    void AudioCodecSBC::SBCDeinitialize()
    {
        if (_sbcHandle != nullptr) {
            ::sbc_finish(static_cast<::sbc_t*>(_sbcHandle));
            delete static_cast<::sbc_t*>(_sbcHandle);
            _sbcHandle = nullptr;
        }
    }

    void AudioCodecSBC::SBCConfigure()
    {
        ASSERT(_sbcHandle != nullptr);
        ::sbc_reinit(static_cast<::sbc_t*>(_sbcHandle), 0L);

        uint32_t rate;
        uint32_t blocks;
        uint32_t bands;

        ::sbc_t* sbc = static_cast<::sbc_t*>(_sbcHandle);

        sbc->bitpool = _bitpool;
        sbc->allocation = (_actuals.AllocationMethod() == Format::AM_LOUDNESS? SBC_AM_LOUDNESS : SBC_AM_SNR);

        switch (_actuals.SubBands()) {
        default:
        case Format::SB_8:
            sbc->subbands = SBC_SB_8;
            bands = 8;
            break;
        case Format::SB_4:
            sbc->subbands = SBC_SB_4;
            bands = 4;
            break;
        }

        switch (_actuals.SamplingFrequency()) {
        case Format::SF_48000_HZ:
            sbc->frequency = SBC_FREQ_48000;
            rate = 48000;
            break;
        default:
        case Format::SF_44100_HZ:
            sbc->frequency = SBC_FREQ_44100;
            rate = 44100;
            break;
        case Format::SF_32000_HZ:
            sbc->frequency = SBC_FREQ_32000;
            rate = 32000;
            break;
        case Format::SF_16000_HZ:
            sbc->frequency = SBC_FREQ_16000;
            rate = 16000;
            break;
        }

        switch (_actuals.BlockLength()) {
        default:
        case Format::BL_16:
            sbc->blocks = SBC_BLK_16;
            blocks = 16;
            break;
        case Format::BL_12:
            sbc->blocks = SBC_BLK_12;
            blocks = 12;
            break;
        case Format::BL_8:
            sbc->blocks = SBC_BLK_8;
            blocks = 8;
            break;
        case Format::BL_4:
            sbc->blocks = SBC_BLK_4;
            blocks = 4;
            break;
        }

        switch (_actuals.ChannelMode()) {
        default:
        case Format::CM_JOINT_STEREO:
            sbc->mode = SBC_MODE_JOINT_STEREO;
            break;
        case Format::CM_STEREO:
            sbc->mode = SBC_MODE_STEREO;
            break;
        case Format::CM_DUAL_CHANNEL:
            sbc->mode = SBC_MODE_DUAL_CHANNEL;
            break;
        case Format::CM_MONO:
            sbc->mode = SBC_MODE_MONO;
            break;
        }

        _frameDuration = ::sbc_get_frame_duration(sbc); /* microseconds */
        _inFrameSize = ::sbc_get_codesize(sbc); /* bytes */
        _outFrameSize = ::sbc_get_frame_length(sbc); /* bytes */

        _bitRate = ((8L * _outFrameSize * rate) / (bands * blocks)); /* bits per second */
        _channels = (sbc->mode == SBC_MODE_MONO? 1 : 2);
        _sampleRate = rate;

    }

    void AudioCodecSBC::DumpConfiguration() const
    {
        #define ELEM(name, val, prop) (_T("  [  %d] " name " [  %d]"), !!(_supported.val() & Format::prop), !!(_actuals.val() & Format::prop))
        TRACE(Trace::Information, (_T("SBC configuration:")));
        TRACE(Trace::Information, ELEM("Sampling frequency - 16 kHz     ", SamplingFrequency, SF_16000_HZ));
        TRACE(Trace::Information, ELEM("Sampling frequency - 32 kHz     ", SamplingFrequency, SF_32000_HZ));
        TRACE(Trace::Information, ELEM("Sampling frequency - 44.1 kHz   ", SamplingFrequency, SF_44100_HZ));
        TRACE(Trace::Information, ELEM("Sampling frequency - 48 kHz     ", SamplingFrequency, SF_48000_HZ));
        TRACE(Trace::Information, ELEM("Channel mode - Mono             ", ChannelMode, CM_MONO));
        TRACE(Trace::Information, ELEM("Channel mode - Stereo           ", ChannelMode, CM_STEREO));
        TRACE(Trace::Information, ELEM("Channel mode - Dual Channel     ", ChannelMode, CM_DUAL_CHANNEL));
        TRACE(Trace::Information, ELEM("Channel mode - Joint Stereo     ", ChannelMode, CM_JOINT_STEREO));
        TRACE(Trace::Information, ELEM("Block length - 4                ", BlockLength, BL_4));
        TRACE(Trace::Information, ELEM("Block length - 8                ", BlockLength, BL_8));
        TRACE(Trace::Information, ELEM("Block length - 12               ", BlockLength, BL_12));
        TRACE(Trace::Information, ELEM("Block length - 16               ", BlockLength, BL_16));
        TRACE(Trace::Information, ELEM("Frequency sub-bands - 4         ", SubBands, SB_4));
        TRACE(Trace::Information, ELEM("Frequency sub-bands - 8         ", SubBands, SB_8));
        TRACE(Trace::Information, ELEM("Bit allocation method - SNR     ", AllocationMethod, AM_SNR));
        TRACE(Trace::Information, ELEM("Bit allocation method - Loudness", AllocationMethod, AM_LOUDNESS));
        TRACE(Trace::Information, (_T("  [%3d] Minimal bitpool value            [%3d]"), _supported.MinBitpool(), _actuals.MinBitpool()));
        TRACE(Trace::Information, (_T("  [%3d] Maximal bitpool value            [%3d]"), _supported.MaxBitpool(), _actuals.MaxBitpool()));
        #undef ELEM
    }

    void AudioCodecSBC::DumpBitrateConfiguration() const
    {
        Core::EnumerateType<qualityprofile> profile(_profile);
        TRACE(Trace::Information, (_T("Quality profile: %s"), (profile.IsSet() == true? profile.Data() : "(error)")));
        TRACE(Trace::Information, (_T("Bitpool value: %d"), _bitpool));
        TRACE(Trace::Information, (_T("Bitrate: %d bps"), _bitRate));
        TRACE(Trace::Information, (_T("Frame size: in %d bytes, out %d bytes (%d us)"), _inFrameSize, _outFrameSize, _frameDuration));
    }

} // namespace A2DP

}
