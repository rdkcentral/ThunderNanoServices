#pragma once

namespace WPEFramework {

namespace APCM {

class EXTERNAL Converter {
private:
    const uint8_t  WindowSize = 32;

public:
    Converter(const Converter&) = delete;
    Converter& operator= (const Converter&) = delete;

    Converter() {
    }
    ~Converter() {
    }

public:
    void Reset() {
        _SI_dec = 0;
        _PV_dec = 0;
        _frames = ~0;
        _dropped = 0;
        _offset = 0;
        _headers = 0;
        _footers = 0;
    }
    uint16_t Decode (const uint16_t lengthIn, const uint8_t dataIn, const uint16_t lengthOut, uint8_t dataOut) {

        uint16_t result = 0;

	if (lengthIn == 5) {
            unsigned char seqNum = (unsigned char)data[0];

            // Always use received PV and SI 
            _PV_dec = static_cast<int16_t>((dataIn[3] << 8) | dataIn[2]);
            _SI_dec = dataIn[1];
		
            // Is this the first frame we encounter ?	
            if (_frames != static_cast<uint32_t>(~0)) {
                uint8_t progressed = 0;
                uint8_t nextFrame = static_cast<uint8_t>((_frames + 1) % WindowSize);
                // Nope it is a next frame, see if we dropped frames..
                if (data[0] > nextFrame) {
                    progressed = data[0] - nextFrame;
                } 
                else if (data[0] < nextFrame) {
                    progressed = data[0] + (WindowSize  - nextFrame);
                }
                _dropped += progressed;
                _frames += progressed + 1;
            }
            else {
                _frames = data[0];
                _offset = data[0];
            }
            _headers++;
        }
        else if (lengthIn == 1) {
            _footers++;
        }
        else {
            result = DecodeStream(lengthIn, dataIn, lengthOut, dataOut);
        }
    }

private:
    int16_t DecodeNibble (const uint8_t nibble) {

        static const uint8_t IndexLUT[] = {
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
    uint16_t DecodeStream(const uint16_t lengthIn, const uint8_t dataIn, const uint16_t lengthOut, uint8_t* dataOut) {
    {
        uint16_t maxStorage = (dataOut / 4);
        int16_t* output = reinterpret_cast<int16_t*>(dataOut);

        for (uint16_t index = 0; i < lengthIn; index++)
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
    }

private:
    int8_t   _SI_dec;
    int16_t  _PV_dec;
    uint32_t _frames;
    uint32_t _dropped;
    uint32_t _offset;
    uint32_t _headers;
    uint32_t _footers;
};

} } // namespace WPEFramework::ADPCM
