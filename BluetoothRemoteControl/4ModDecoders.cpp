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
    struct __attribute__((packed)) Preamble {
        // Conforms to MS IMA ADPCM preamble
        uint16_t pred; // little-endian
        uint8_t step;
        uint8_t pad;
    };

public:
    static constexpr Exchange::IVoiceProducer::IProfile::codec DecoderType = Exchange::IVoiceProducer::IProfile::codec::ADPCM;
    static const TCHAR*                                    Name;

public:
    ADPCM() = delete;
    ADPCM(const ADPCM&) = delete;
    ADPCM& operator= (const ADPCM&) = delete;

    ADPCM(const string&)
        : _stepIdx(0)
        , _pred(0)
        , _frames(0)
        , _dropped(0)
        , _frame(0) {
    }
    ~ADPCM() {
    }

public:
    const uint8_t* Data() const {
        return (_package);
    }
    uint16_t Length() const {
        return (static_cast<uint16_t>(sizeof(_package)));
    }
    uint8_t StepIndex() const {
        return (_stepIdx);
    }
    uint16_t Predicted() const {
        return (_pred);
    }
    uint32_t Frames() const override {
        return _frames;
    }
    uint32_t Dropped() const override {
        return (_dropped);
    }
    void Reset() override {
        _frames = 0;
        _dropped = 0;
    }
    uint16_t Decode (const uint16_t lengthIn, const uint8_t dataIn[], const uint16_t lengthOut, uint8_t dataOut[]) override
    {
        uint16_t result = 0;

        if (AddFrame(lengthIn, dataIn) == true) {

            ASSERT (lengthOut > sizeof(Preamble));

            Preamble *preamble = reinterpret_cast<Preamble*>(dataOut);
            preamble->step = _stepIdx;
            preamble->pred = _pred;
            preamble->pad = 0;

            result = std::min(static_cast<uint16_t>(lengthOut - sizeof(Preamble)), static_cast<uint16_t>(sizeof(_package)));

            // Add the incoming buffer with the preamble build from the header notification
            ::memcpy(&(dataOut[sizeof(Preamble)]), _package, result);

            result += sizeof(Preamble);
        }

        return (result);
    }

protected:
    bool AddFrame(const uint16_t lengthIn, const uint8_t dataIn[]) {

        bool sendFrame = false;

        _frames++;

	if (lengthIn >= 20) {
            if ((_frame == 0) || (_frame >= 6)) {
                _pred    = (dataIn[0] << 8) | dataIn[1];
                _stepIdx = dataIn[2];
                _offset  = (lengthIn - 3);

                if (_frame >= 6) {
                    _frame   = 0;
                    _dropped += 1;
                }

                ::memcpy(_package, &(dataIn[3]), _offset);
            }
            else if ((lengthIn + _offset) < sizeof(_package)) { 
                ::memcpy(&(_package[_offset]), dataIn, lengthIn);
                _offset += lengthIn;
            }
            _frame++;
        }
        else {
            // Seems we have a full frame
            uint16_t copyLength = std::min(lengthIn, static_cast<uint16_t>(sizeof(_package) - _offset));

            // Last frame should fill up the package...
            if (copyLength == lengthIn) {

                ::memcpy(&(_package[_offset]), dataIn, copyLength);
                _offset += lengthIn;
            }

            if (_offset < sizeof(_package)) {
                // Looks like we lost some frames along the way, Reset and retry for the next..
                _dropped += 1;
            }
            else {
                sendFrame = true;
            }

            _frame = 0;
            _offset = 0;
        }
 
        return ( sendFrame );
    }

private:
    uint8_t  _stepIdx;
    uint16_t _pred;
    uint32_t _frames;
    uint32_t _dropped;

    uint8_t  _frame;
    uint8_t  _offset;
    uint8_t  _package[128];
};

static Decoders::DecoderFactory<ADPCM> _adpcmFactory;
/* static */ const TCHAR* ADPCM::Name = _T("4MOD Technology");

class PCM : public ADPCM {
public:
    static constexpr Exchange::IVoiceProducer::IProfile::codec DecoderType = Exchange::IVoiceProducer::IProfile::codec::PCM;

public:
    PCM() = delete;
    PCM(const PCM&) = delete;
    PCM& operator= (const PCM&) = delete;

    PCM(const string& config) : ADPCM(config) {
    }
    ~PCM() {
    }

public:
    uint16_t Decode (const uint16_t lengthIn, const uint8_t dataIn[], const uint16_t lengthOut, uint8_t dataOut[]) override {
        uint16_t result = 0;

        if (ADPCM::AddFrame(lengthIn, dataIn) == true) {

            _PV_dec = ADPCM::Predicted();
            _SI_dec = ADPCM::StepIndex();

            result = DecodeStream(ADPCM::Length(), ADPCM::Data(), lengthOut, dataOut);
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
};

static Decoders::DecoderFactory<PCM> _pcmFactory;

} // namespace 
