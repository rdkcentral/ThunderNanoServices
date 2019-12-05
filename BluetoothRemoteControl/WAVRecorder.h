#pragma once

#include "Module.h"

namespace WPEFramework {

namespace WAV {

class EXTERNAL Recorder {
public:
    enum codec {
        PCM = 1,
        ADPCM 
    };

public:
    Recorder(const Recorder&) = delete;
    Recorder& operator= (const Recorder&) = delete;

    Recorder() 
        : _file() {
    }
    ~Recorder() {
    }

public:
    bool IsOpen() const {
        return (_file.IsOpen());
    }
    uint32_t Open(const string& fileName, const codec type, const uint8_t channels, const uint32_t sampleRate, const uint8_t bitsPerSample) {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        printf ("Opened file for: codec: %d, channels: %d, sampleRate: %d, bitsPerSample: %d\n", type, channels, sampleRate, bitsPerSample);

        _file = Core::File(fileName, false);

        if (_file.Create() == true) {

            ASSERT (bitsPerSample >= 1);

            _file.Write(reinterpret_cast<const uint8_t*>(_T("RIFF")), 4);
            _file.Write(reinterpret_cast<const uint8_t*>(_T("    ")), 4);
            _file.Write(reinterpret_cast<const uint8_t*>(_T("WAVE")), 4);
            _file.Write(reinterpret_cast<const uint8_t*>(_T("fmt ")), 4);
            Store<uint32_t>(16);         /* SubChunk1Size is 16 */
            Store<uint16_t>(type);   
            Store<uint16_t>(channels);   
            Store<uint32_t>(sampleRate);   
            Store<uint32_t>(sampleRate * ( bitsPerSample / 8));   
            Store<uint16_t>(bitsPerSample / 8);   
            Store<uint16_t>(bitsPerSample);   
            _file.Write(reinterpret_cast<const uint8_t*>(_T("data")), 4);
            _file.Write(reinterpret_cast<const uint8_t*>(_T("    ")), 4);
            _fileSize = 0;
            result = Core::ERROR_NONE;
        }
        return (result);
    }
    void Close() {
        if (_file.IsOpen() == true) {
            _file.Position(false, 4);
            Store<uint32_t>(_fileSize + 36);
            _file.Position(false, 40);
            Store<uint32_t>(_fileSize);
            _file.Close();
        }
    }
    void Write (const uint16_t length, const uint8_t data[]) {

        _file.Write(data, length);
        _fileSize += length;
    }

private:
    template<typename TYPE>
    void Store(const TYPE value) {
        TYPE store = value;
        for (uint8_t index = 0; index < sizeof(TYPE); index++) {
            uint8_t byte = (store & 0xFF);
            _file.Write(&byte, 1);
            store = (store >> 8);
        }
    }

private:
    Core::File _file;
    uint32_t _fileSize;
};
 
} } // namespace WPEFramework::WAV
