/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
 
#include <atomic>

#include "../Module.h"
#include "../VolumeControlPlatform.h"

namespace Thunder {
namespace Plugin {

namespace {

class VolumeControlPlatformStub : public VolumeControlPlatform {
public:
    VolumeControlPlatformStub(VolumeChangedCallback&& volumeChanged, MutedChangedCallback&& mutedChanged)
        : _volumeChanged(std::move(volumeChanged))
        , _mutedChanged(std::move(mutedChanged))
        , _muted(false)
        , _volume(0)
    {
    }

    ~VolumeControlPlatformStub() override = default;

    uint32_t Muted(bool muted) override
    {
        _muted.store(muted);

        if (_mutedChanged) {
            _mutedChanged();
        }

        return (Core::ERROR_NONE);
    }

    bool Muted() const override
    {
        return (_muted.load());
    }

    uint32_t Volume(uint8_t volume) override
    {
        _volume.store(volume);

        if (_volumeChanged) {
            _volumeChanged();
        }

        return (Core::ERROR_NONE);
    }

    uint8_t Volume() const override
    {
        return (_volume.load());
    }

private:
    VolumeChangedCallback _volumeChanged;
    MutedChangedCallback _mutedChanged;
    std::atomic<bool> _muted;
    std::atomic<uint8_t> _volume;

};

}

// static
std::unique_ptr<VolumeControlPlatform> VolumeControlPlatform::Create(
    VolumeControlPlatform::VolumeChangedCallback&& volumeChanged,
    VolumeControlPlatform::MutedChangedCallback&& mutedChanged)
{
    return (std::unique_ptr<VolumeControlPlatform>{new VolumeControlPlatformStub(std::move(volumeChanged), std::move(mutedChanged))});
}

}  // namespace Plugin
}  // namespace Thunder
