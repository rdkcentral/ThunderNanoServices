/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the License);
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

#include "Administrator.h"

using namespace WPEFramework;

namespace {

class ADPCM : public Decoders::IDecoder {
private:
    const uint8_t  WindowSize = 32;

    struct __attribute__((packed)) Header {
        uint8_t seq;
        uint8_t step;
        uint16_t pred;
        uint8_t compression;
    };

    struct __attribute__((packed)) Preamble {
        // Conforms to MS IMA ADPCM preamble
        uint16_t pred; // little-endian
        uint8_t step;
        uint8_t pad;
    };

public:
    static constexpr Exchange::IVoiceProducer::IProfile::codec DecoderType = Exchange::IVoiceProducer::IProfile::codec::ADPCM;
    static const TCHAR*                                        Name;

public:
    ADPCM() = delete;
    ADPCM(const ADPCM&) = delete;
    ADPCM& operator= (const ADPCM&) = delete;

    ADPCM(const string&) {
    }
    ~ADPCM() {
    }

public:
    uint32_t Frames() const override {
        return (_frames);
    }
    uint32_t Dropped() const override {
        return (_dropped);
    }
    void Reset() override {
        _frames = ~0;
        _dropped = ~0;
    }
    uint16_t Decode (const uint16_t lengthIn, const uint8_t dataIn[], const uint16_t lengthOut, uint8_t dataOut[]) override
    {

        uint16_t result = 0;

	if (lengthIn == 5) {
            const Header* hdr = reinterpret_cast<const Header*>(dataIn);
            _seq = hdr->seq;
            _stepIdx = hdr->step;
            _pred = btohs(hdr->pred);
            _compression = hdr->compression;

            if (_dropped != static_cast<uint32_t>(~0)) {
                // Is it a next frame, see if we dropped frames..
                if (_seq > _nextFrame) {
                    _dropped += _seq - _nextFrame;
                }
                else if (_seq < _nextFrame) {
                    _dropped += _seq + (WindowSize - _nextFrame);
                }
                _frames++;
            }
            else {
                _frames = 0;
            }
            _nextFrame = (_seq + 1) % WindowSize;
        }
        else if (lengthIn == 1) {
            // footer nothing to do
        }
        else if (_frames != static_cast<uint32_t>(~0)) {
            ASSERT (lengthOut >= sizeof(Preamble));

            Preamble *preamble = reinterpret_cast<Preamble*>(dataOut);
            preamble->step = _stepIdx;
            preamble->pred = _pred;
            preamble->pad = 0;

            if (_dropped == static_cast<uint32_t>(~0)) {
                _dropped = 0;
            }

            result = std::min(static_cast<uint16_t>(lengthOut - sizeof(Preamble)), lengthIn);

            // Add the incoming buffer with the preamble built from the header notification
            ::memcpy(&(dataOut[sizeof(Preamble)]), dataIn, result);

            result = sizeof(Preamble) + lengthIn;
        }

        return (result);
    }
    private:
        uint8_t  _seq;
        uint8_t  _stepIdx;
        uint16_t _pred;
        uint8_t  _compression;
        uint8_t  _nextFrame;
        uint32_t _frames;
        uint32_t _dropped;
};

static Decoders::DecoderFactory<ADPCM> _adpcmFactory;
/* static */ const TCHAR* ADPCM::Name = _T("Tech4Home");

class PCM : public Decoders::IDecoder {
private:
    const uint8_t  WindowSize = 32;

public:
    static constexpr Exchange::IVoiceProducer::IProfile::codec DecoderType = Exchange::IVoiceProducer::IProfile::codec::PCM;
    static const TCHAR*                                        Name;

public:
    PCM() = delete;
    PCM(const PCM&) = delete;
    PCM& operator= (const PCM&) = delete;

    PCM(const string&) {
    }
    ~PCM() {
    }

public:
    uint32_t Frames() const override {
        return (_frames);
    }
    uint32_t Dropped() const override {
        return (_dropped);
    }
    void Reset() override {
        _PV_dec = 0;
        _SI_dec = 0;
        _frames = ~0;
        _dropped = ~0;
    }
    uint16_t Decode (const uint16_t lengthIn, const uint8_t dataIn[], const uint16_t lengthOut, uint8_t dataOut[]) override {

        uint16_t result = 0;

	if (lengthIn == 5) {
            unsigned char seqNum = (unsigned char)dataIn[0];

            // Always use received PV and SI
            _PV_dec = static_cast<int16_t>((dataIn[3] << 8) | dataIn[2]);
            _SI_dec = dataIn[1];

            // Is this the first frame we encounter ?
            if (_dropped != static_cast<uint32_t>(~0)) {
                // Is it a next frame, see if we dropped frames..
                if (dataIn[0] > _nextFrame) {
                    _dropped += dataIn[0] - _nextFrame;
                }
                else if (seqNum < _nextFrame) {
                    _dropped += dataIn[0] + (WindowSize - _nextFrame);
                }
                _frames++;
            }
            else {
                _frames = 0;
            }
            _nextFrame = (dataIn[0] + 1) % WindowSize;
        }
        else if (lengthIn == 1) {
            // This is a footer, so what :-)
        }
        else if (_frames != static_cast<uint32_t>(~0)) {

            if (_dropped == static_cast<uint32_t>(~0)) {
                _dropped = 0;
            }

            result = DecodeStream(lengthIn, dataIn, lengthOut, dataOut);
        }
        return (result);
    }

private:
    int16_t DecodeNibble (const uint8_t nibble) {

        static const int8_t IndexLUT[] = {
            -1, -1, -1, -1, 2, 4, 6, 8,
            -1, -1, -1, -1, 2, 4, 6, 8
        };

        static const uint16_t StepSizeLUT[] = {
            7,     8,     9,     10,    11,    12,    13,    14,
            16,    17,    19,    21,    23,    25,    28,    31,
            34,    37,    41,    45,    50,    55,    60,    66,
            73,    80,    88,    97,    107,   118,   130,   143,
            157,   173,   190,   209,   230,   253,   279,   307,
            337,   371,   408,   449,   494,   544,   598,   658,
            724,   796,   876,   963,   1060,  1166,  1282,  1411,
            1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
            3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
            7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
            15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
            32767
        };

        uint16_t step = StepSizeLUT[_SI_dec];
        uint16_t cum_diff = step >> 3;

        _SI_dec += IndexLUT[nibble];

        if (_SI_dec < 0) {
            _SI_dec = 0;
        }
        else if (_SI_dec > 88) {
            _SI_dec = 88;
        }

        if ((nibble & 4) != 0) {
            cum_diff += step;
        }
        if ((nibble & 2) != 0) {
            cum_diff += step >> 1;
        }
        if ((nibble & 1) != 0) {
            cum_diff += step >> 2;
        }
        if ((nibble & 8) != 0) {
            if (_PV_dec < (-32767 + cum_diff)) {
                _PV_dec = -32767;
            }
            else {
                _PV_dec -= cum_diff;
            }
        }
        else {
            if (_PV_dec > (0x7fff - cum_diff)) {
                _PV_dec = 0x7fff;
            }
            else {
                _PV_dec += cum_diff;
	    }
        }
        return (_PV_dec);
    }
    uint16_t DecodeStream(const uint16_t lengthIn, const uint8_t dataIn[], const uint16_t lengthOut, uint8_t dataOut[])
    {
        // TODO: check lengthOut
        uint16_t maxStorage = (lengthOut / 4);
        int16_t* output = reinterpret_cast<int16_t*>(dataOut);

        for (uint16_t index = 0; index < lengthIn; index++)
        {
            uint8_t byte = dataIn[index];

            int16_t dec1 = DecodeNibble(byte & 0xF);
            int16_t dec2 = DecodeNibble((byte >> 4) & 0xF);

            if (maxStorage >= 2) {

                // Add the new bytes
                *output++ = dec1;
                *output++ = dec2;
                maxStorage -= 2;
            }
            else if (maxStorage >= 1) {
                *output++ = dec1;
                maxStorage -= 1;
            }
   	}
        return (lengthIn * 4);
    }

private:
    int16_t  _PV_dec;
    int8_t   _SI_dec;
    uint8_t  _nextFrame;
    uint32_t _frames;
    uint32_t _dropped;
};

static Decoders::DecoderFactory<PCM> _pcmFactory;
/* static */ const TCHAR* PCM::Name = _T("Tech4Home");

} // namespace 
