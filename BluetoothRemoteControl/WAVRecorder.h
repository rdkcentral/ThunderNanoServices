#pragma once

#include "Module.h"

namespace WPEFramework {

namespace WAV {

class EXTERNAL Recorder {
public:
    enum codec {
        PCM = 1
    };

public:
    Recorder() = delete;
    Recorder(const Recorder&) = delete;
    Recorder& operator= (const Recorder&) = delete;

    Recorder(const codec type, const uint8_t channels, const uint32_t sampleRate) 
        : _file()
        , _type(type)
        , _channels(channels)
        , _sampleRate(sampleRate) 
        , _bytesPerSample(2) {
    }
    ~Recorder() {
    }

public:
    uint32_t Open(const string& fileName) {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _file = Core::File(fileName, false);

        if (_file.Create() == true) {
            _file.Write(_T("RIFF"), 4);
            _file.Write(_T("    "), 4);
            _file.Write(_T("WAVE"), 4);
            _file.Write(_T("fmt "), 4);
            _file.Store<uin32_t>(_bytesPerSample * 8);         /* SubChunk1Size is 16 */
            _file.Store<uin16_t>(_type);   
            _file.Store<uin16_t>(_channels);   
            _file.Store<uin32_t>(_samplerate);   
            _file.Store<uin32_t>(_sampleRate * _channels * _bytesPerSample);   
            _file.Store<uin16_t>(_channels * _bytesPerSample);   
            _file.Store<uin16_t>(_bytesPerSample * 8);   
            _file.Write(_T("data"), 4);
            _file.Write(_T("    "), 4);
            result = Core::ERROR_NONE;
        }
        return (result);
    }
    void Close() {
        if (_file.IsValid() == true) {
            _file.Position(false, 4);
            _file.Store<uint32>(_file.Size() - 8);
            _file.Position(false, 40);
            _file.Store<uint32>((_file.Size() - 44) / _bytesPerSample);
            _file.Close();
        }
    }
    void Write (const uint16_t samples, const int16_t samples[]) {

        for (uint16_t index = 0 ; index < samples; index++) {
            Store<int16_t>(samples[index]);
        }
    }

private:
    template<typename TYPE>
    void Store(const TYPE value) {
        TYPE store = value;
        for (uint8_t index = 0; index < sizeof(TYPE); index++) {
            uint8_t byte = (value & 0xFF);
            _file.Write(&byte, 1);
            value = (value >> 8);
        }
    }

private:
    Core::File _file;
    uint16_t   _type;
    uint16_t   _channels;
    uint32_t   _sampleRate;
    uint8_t    _bytesPerSample;
};
 

} } // namespace WPEFramework::WAV
