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
 
#pragma once

#include <functional>
#include <memory>

namespace WPEFramework {
namespace Plugin {

    class VolumeControlPlatform {
    public:
        using VolumeChangedCallback = std::function<void()>;
        using MutedChangedCallback = std::function<void()>;

        static std::unique_ptr<VolumeControlPlatform> Create(
            VolumeChangedCallback&& volumeChanged,
            MutedChangedCallback&& mutedChanged);

        virtual ~VolumeControlPlatform() {}

        virtual uint32_t Muted(bool muted) = 0;
        virtual bool Muted() const = 0;
        virtual uint32_t Volume(uint8_t volume) = 0;
        virtual uint8_t Volume() const = 0;
    };

}  // namespace Plugin
}  // namespace WPEFramework
