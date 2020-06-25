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

#include "Dolby.h"
#include "audio_if.h"
#include "hardware/audio.h"

using namespace WPEFramework::Exchange;

/**
 * @brief Resource wrapper for audio_hw_device.
 * 
 */
class AudioDevice
{
public:
    AudioDevice()
        : _audioDev(nullptr), _error(0), _initialized(false)
    {
        _error = audio_hw_load_interface(&_audioDev);
        if (_error == 0)
        {
            _error = _audioDev->init_check(_audioDev);
            _initialized = (_error == 0);
        }
    }

    ~AudioDevice()
    {
        if (IsInitialized())
        {
            audio_hw_unload_interface(_audioDev);
        }
    }

    AudioDevice(const AudioDevice &) = delete;
    AudioDevice &operator=(const AudioDevice &) = delete;

    int Error() const { return _error; }
    bool IsInitialized() const { return _initialized; }

    void HdmiFormat(int hdmiMode)
    {
        std::string command("hdmi_format=");
        command += std::to_string(hdmiMode);
        _error = _audioDev->set_parameters(_audioDev, command.c_str());
    }

    int HdmiFormat() const
    {
        char *value = _audioDev->get_parameters(_audioDev, "hdmi_format");
        int code = '0' - value[0];
        delete value;

        return code;
    }

private:
    audio_hw_device *_audioDev;
    int _error;
    bool _initialized;
};

uint32_t
set_audio_output_type(const Dolby::IOutput::Type type)
{
    uint32_t result = WPEFramework::Core::ERROR_NONE;
    AudioDevice audioDev;
    if (audioDev.IsInitialized())
    {
        audioDev.HdmiFormat(static_cast<int>(type));
        int error = audioDev.Error();

        if (error != 0)
        {
            TRACE_L1("Cannot set the HDMI format <%d>", error);
            result = WPEFramework::Core::ERROR_GENERAL;
        }
    }
    else
    {
        TRACE_L1("Audio device not initialized");
        result = WPEFramework::Core::ERROR_ILLEGAL_STATE;
    }

    return result;
}

Dolby::IOutput::Type toEnum(int code, uint32_t &error)
{
    Dolby::IOutput::Type result;
    switch (code)
    {
    case Dolby::IOutput::Type::AUTO:
    {
        result = Dolby::IOutput::Type::AUTO;
        error = WPEFramework::Core::ERROR_NONE;
        break;
    }
    case Dolby::IOutput::Type::ATMOS_PASS_THROUGH:
    {
        result = Dolby::IOutput::Type::ATMOS_PASS_THROUGH;
        error = WPEFramework::Core::ERROR_NONE;
    }
    case Dolby::IOutput::Type::DIGITAL_PASS_THROUGH:
    {
        result = Dolby::IOutput::Type::DIGITAL_PASS_THROUGH;
        error = WPEFramework::Core::ERROR_NONE;
    }
    case Dolby::IOutput::Type::DIGITAL_PCM:
    {
        result = Dolby::IOutput::Type::DIGITAL_PCM;
        error = WPEFramework::Core::ERROR_NONE;
    }
    default:
    {
        result = Dolby::IOutput::Type::AUTO;
        error = WPEFramework::Core::ERROR_GENERAL;
        TRACE_L1("Could not map the provided dolby output: %d to Dolby::IOutput::Type enumeration.");
    }
    }

    return result;
}

WPEFramework::Exchange::Dolby::IOutput::Type
get_audio_output_type(uint32_t &error)
{
    Dolby::IOutput::Type result = Dolby::IOutput::Type::AUTO;

    AudioDevice audioDev;

    if (audioDev.IsInitialized())
    {
        int audioType = audioDev.HdmiFormat();
        result = toEnum(audioType, error);
    }
    else
    {
        TRACE_L1("Audio device not initialized");
        error = WPEFramework::Core::ERROR_ILLEGAL_STATE;
    }

    return result;
}
