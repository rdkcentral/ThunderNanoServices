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

namespace A2DP {

    /* virtual */ void AudioCodecSBC::Configure(const IAudioCodec::Format& preferredFormat)
    {
        qualityprofile preferredProfile = _preferredProfile;

        // Start with sane values.
        Info::samplingfrequency frequency = Info::SF_44100_HZ;
        uint8_t maxBitpool = _supported.MinBitpool();

        // Joint-Stereo packs better than plain stereo and offers same quality.
        Info::channelmode channelMode = ((preferredFormat.channels == 1)? Info::CM_MONO : Info::CM_JOINT_STEREO);
        bool stereo = (preferredFormat.channels == 2);

        // Find a best-matched sampling frequency...
        if (_supportedSamplingFreqs.empty() == false) {
            if (preferredFormat.samplingFrequency <= _supportedSamplingFreqs.front().first) {
                frequency = _supportedSamplingFreqs.front().second;
            } else if (preferredFormat.samplingFrequency >= _supportedSamplingFreqs.back().first) {
                frequency = _supportedSamplingFreqs.back().second;
            } else {
                for (uint8_t i = 0; i <= ((_supportedSamplingFreqs.size() - 1) - 1); i++ ) {
                    if ((preferredFormat.samplingFrequency >= _supportedSamplingFreqs[i].first)
                            && (preferredFormat.samplingFrequency < _supportedSamplingFreqs[i + 1].first)) {
                        frequency = _supportedSamplingFreqs[i].second;
                    }
                }
            }
        }

        // Select bitpool and channel mode based on preferred format...
        // (Note that bitpools for sample rates of 16 and 32 kHz are not specified and will receive same values as 44,1 kHz.)
        if (preferredProfile == XQ) {
            if (_supported.MaxBitpool() >= 38) {
                maxBitpool = 38;
                if (stereo == true) {
                    if (_supported.MaxBitpool() >= 76) {
                        maxBitpool = 76;
                    } else {
                        // Max supported bitpool is too low for XQ joint-stereo, so try 38 on dual channel instead,
                        // what should give roughly same quality/bitrate.
                        channelMode = Info::CM_DUAL_CHANNEL;
                    }
                }
            } else {
                preferredProfile = HQ;
            }
        }

        if (preferredProfile == HQ) {
            if (frequency == Info::SF_48000_HZ) {
                maxBitpool = (stereo? 51 : 29);
            } else {
                maxBitpool = (stereo? 53 : 31);
            }

            if (maxBitpool > _supported.MaxBitpool()) {
                preferredProfile = MQ;
            }
        }

        if (preferredProfile == MQ) {
            if (frequency == Info::SF_48000_HZ) {
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
            maxBitpool = _supported.MaxBitpool();
        }

        _actuals.SamplingFrequency(frequency);
        _actuals.ChannelMode(channelMode);
        _actuals.MaxBitpool(maxBitpool);

        _profile = preferredProfile;

        // Configure the encoder
        SBCConfigure();

        DumpConfiguration();
    }

    /* virtual */ void AudioCodecSBC::Configuration(IAudioCodec::Format& currentFormat) const
    {
        switch (_actuals.SamplingFrequency()) {
        case Info::SF_48000_HZ:
            currentFormat.samplingFrequency = 48000;
            break;
        case Info::SF_44100_HZ:
            currentFormat.samplingFrequency = 44100;
            break;
        case Info::SF_32000_HZ:
            currentFormat.samplingFrequency = 32000;
            break;
        case Info::SF_16000_HZ:
            currentFormat.samplingFrequency = 16000;
            break;
        default:
            currentFormat.samplingFrequency = 0;
            ASSERT(false && "Invalid sampling frequency configured");
            break;
        }

        switch (_actuals.ChannelMode()) {
        case Info::CM_MONO:
            currentFormat.channels = 1;
            break;
        case Info::CM_DUAL_CHANNEL:
        case Info::CM_STEREO:
        case Info::CM_JOINT_STEREO:
            currentFormat.channels = 2;
            break;
        default:
            currentFormat.channels = 0;
            ASSERT(false && "Invalid channel mode configured");
            break;
        }
    }

    /* virtual */ uint32_t AudioCodecSBC::Encode(const uint32_t inBufferSize, const uint8_t inBuffer[],
                                                 uint32_t& outSize, uint8_t outBuffer[]) const
    {
        const uint8_t MAX_FRAMES = 15;

        uint16_t frames = (inBufferSize / _inFrameSize);
        uint32_t produced = sizeof(SBCHeader);
        uint32_t available = (outSize - produced);
        uint32_t consumed = 0;
        uint16_t count = 0;

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
                TRACE(Trace::Error, (_T("Failed to SBC-encode frame!!!")));
                break;
            } else {
                consumed += read;
                available -= written;
                produced += written;
                count++;
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

        _bitpool = _actuals.MaxBitpool(); // Let's start with maximum we have negotiated
        sbc->bitpool = _bitpool;

        sbc->allocation = (_actuals.AllocationMethod() == Info::AM_LOUDNESS? SBC_AM_LOUDNESS : SBC_AM_SNR);

        switch (_actuals.SubBands()) {
        default:
        case Info::SB_8:
            sbc->subbands = SBC_SB_8;
            bands = 8;
            break;
        case Info::SB_4:
            sbc->subbands = SBC_SB_4;
            bands = 4;
            break;
        }

        switch (_actuals.SamplingFrequency()) {
        case Info::SF_48000_HZ:
            sbc->frequency = SBC_FREQ_48000;
            rate = 48000;
            break;
        default:
        case Info::SF_44100_HZ:
            sbc->frequency = SBC_FREQ_44100;
            rate = 44100;
            break;
        case Info::SF_32000_HZ:
            sbc->frequency = SBC_FREQ_32000;
            rate = 32000;
            break;
        case Info::SF_16000_HZ:
            sbc->frequency = SBC_FREQ_16000;
            rate = 16000;
            break;
        }

        switch (_actuals.BlockLength()) {
        default:
        case Info::BL_16:
            sbc->blocks = SBC_BLK_16;
            blocks = 16;
            break;
        case Info::BL_12:
            sbc->blocks = SBC_BLK_12;
            blocks = 12;
            break;
        case Info::BL_8:
            sbc->blocks = SBC_BLK_8;
            blocks = 8;
            break;
        case Info::BL_4:
            sbc->blocks = SBC_BLK_4;
            blocks = 4;
            break;
        }

        switch (_actuals.ChannelMode()) {
        default:
        case Info::CM_JOINT_STEREO:
            sbc->mode = SBC_MODE_JOINT_STEREO;
            break;
        case Info::CM_STEREO:
            sbc->mode = SBC_MODE_STEREO;
            break;
        case Info::CM_DUAL_CHANNEL:
            sbc->mode = SBC_MODE_DUAL_CHANNEL;
            break;
        case Info::CM_MONO:
            sbc->mode = SBC_MODE_MONO;
            break;
        }

        _frameDuration = ::sbc_get_frame_duration(sbc); /* microseconds */
        _inFrameSize = ::sbc_get_codesize(sbc); /* bytes */
        _outFrameSize = ::sbc_get_frame_length(sbc); /* bytes */

        _bitRate = ((8L * _outFrameSize * rate) / (bands * blocks)); /* bits per second */
    }

    void AudioCodecSBC::DumpConfiguration() const
    {
        #define ELEM(name, val, prop) (_T("  [  %d] " name " [  %d]"), !!(_supported.val() & Info::prop), !!(_actuals.val() & Info::prop))
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

        static const char* profileStr[] = { "SBC compatibility mode", "SBC-LQ", "SBC-MQ", "SBC-HQ", "SBC-XQ" };
        TRACE(Trace::Information, (_T("Quality profile: %s"), profileStr[_profile]));
        TRACE(Trace::Information, (_T("Bitpool value: %d"), _bitpool));
        TRACE(Trace::Information, (_T("Bitrate: %d bps"), _bitRate));
        TRACE(Trace::Information, (_T("Frame size: in %d bytes, out %d bytes (%d us)"), _inFrameSize, _outFrameSize, _frameDuration));
    }

} // namespace A2DP

}
