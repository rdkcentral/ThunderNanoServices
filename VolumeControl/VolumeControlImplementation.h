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

#include <memory>

#include "Module.h"
#include <interfaces/IVolumeControl.h>

namespace WPEFramework {
namespace Plugin {

    class VolumeControlPlatform;

    class VolumeControlImplementation : public Exchange::IVolumeControl {
    public:
        VolumeControlImplementation(const VolumeControlImplementation&) = delete;
        VolumeControlImplementation& operator=(const VolumeControlImplementation&) = delete;

        VolumeControlImplementation();
        ~VolumeControlImplementation() override;

        BEGIN_INTERFACE_MAP(VolumeControlImplementation)
            INTERFACE_ENTRY(Exchange::IVolumeControl)
        END_INTERFACE_MAP

        //   IVolumControl methods
        void Register(Exchange::IVolumeControl::INotification* observer) override;
        void Unregister(const Exchange::IVolumeControl::INotification* observer) override;
        uint32_t Muted(const bool muted) override;
        uint32_t Muted(bool& muted) const override;
        uint32_t Volume(const uint8_t volume) override;
        uint32_t Volume(uint8_t& volume) const override;

    private:
        void NotifyMutedChange();
        void NotifyVolumeChange();

        Core::CriticalSection _adminLock;
        std::vector<Exchange::IVolumeControl::INotification*> _notifications;

        std::unique_ptr<VolumeControlPlatform> _platform;
    };

}  // namespace Plugin
}  // namespace WPEFramework
