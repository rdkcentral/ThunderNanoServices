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

#include "../VolumeControlPlatform.h"
#include "../Module.h"
#include "audio_if.h"

namespace WPEFramework {
namespace Plugin {

    namespace {

        constexpr uint8_t kMinVolume = 0;
        constexpr uint8_t kMaxVolume = 100;

        class VolumeControlPlatformAMLogic : public VolumeControlPlatform {
        public:
            VolumeControlPlatformAMLogic(
                VolumeControlPlatform::VolumeChangedCallback&& volumeChanged,
                VolumeControlPlatform::MutedChangedCallback&& mutedChanged)
                : _volumeChanged(std::move(volumeChanged))
                , _muteChanged(std::move(mutedChanged))
                , _device(nullptr)
            {
                audio_hw_load_interface(&_device);
                if(_device != nullptr && _device->init_check(_device) == 0){
                    SYSLOG(Logging::Startup, (_T("Initialized audio_client interface.")));
                } else {
                    SYSLOG(Logging::Startup, (_T("Failed to initialize audio_client interface.")));
                    _device = nullptr;
                }
            }

            ~VolumeControlPlatformAMLogic() override
            {
                if (_device != nullptr) {
                    audio_hw_unload_interface(_device);
                }
            }

            uint32_t Muted(bool muted) override
            {
                uint32_t errorCode = Core::ERROR_NONE;

                if (_device != nullptr) {
                    int error = _device->set_master_mute(_device, muted);
                    if (error != 0) {
                        TRACE_L1("Could not set master to mute.");
                        errorCode = Core::ERROR_GENERAL;    
                    } else {
                        // _muteChanged();
                    }
                } else {
                    errorCode = Core::ERROR_GENERAL;
                }
                return errorCode;
            }

            bool Muted() const override
            {
                bool mute = false;
                if (_device != nullptr) {
                    _device->get_master_mute(_device, &mute);
                } else {
                    TRACE_L1("Cannot query mute property. Device uninitialized.");
                }
                return mute;
            }

            uint32_t Volume(uint8_t volume) override
            {
                uint32_t errorCode = Core::ERROR_NONE;

                // From volume level to loudness gain db
                auto gain = (33.22 * log10(static_cast<double>(volume) / (kMaxVolume - kMinVolume)));
                // from db to loudness gain ratio factor
                auto scaledVolume = std::pow(10, (static_cast<double>(gain) / 20.0));

                if (_device != nullptr) {
                    int error = _device->set_master_volume(_device, scaledVolume);
                    if (error != 0) {
                        fprintf(stderr, "\nCould not set master volume <%d>", error);
                        errorCode = Core::ERROR_GENERAL;
                    } else {
                        _volumeChanged();
                    }
                } else {
                    errorCode = Core::ERROR_GENERAL;
                }
                return errorCode;
            }

            uint8_t Volume() const override
            {
                float volume = .0;
                if (_device != nullptr) {
                    _device->get_master_volume(_device, &volume);
                } else {
                    fprintf(stderr, "\nCannot query volume. Device uninitialized.");
                }
                return static_cast<uint8_t>(volume * 100);
            }

        private:
            VolumeControlPlatform::VolumeChangedCallback _volumeChanged;
            VolumeControlPlatform::MutedChangedCallback&& _muteChanged;

            audio_hw_device_t* _device;
        };

    }

    // static
    std::unique_ptr<VolumeControlPlatform> VolumeControlPlatform::Create(
        VolumeControlPlatform::VolumeChangedCallback&& volumeChanged,
        VolumeControlPlatform::MutedChangedCallback&& mutedChanged)
    {
        return std::unique_ptr<VolumeControlPlatform>(new VolumeControlPlatformAMLogic(std::move(volumeChanged), std::move(mutedChanged)));
    }

} // namespace Plugin
} // namespace WPEFramework
