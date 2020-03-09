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
 
#include "../Module.h"
#include "../VolumeControlPlatform.h"

namespace WPEFramework {
namespace Plugin {

namespace {

class VolumeControlPlatformStub : public VolumeControlPlatform {
public:
  ~VolumeControlPlatformStub() override = default;

  uint32_t Muted(bool muted) override
  {
      return Core::ERROR_NONE;
  }

  bool Muted() const override
  {
      return false;
  }

  uint32_t Volume(uint8_t volume) override
  {
      return Core::ERROR_NONE;
  }

  uint8_t Volume() const override
  {
      return 0;
  }

};

}

// static
std::unique_ptr<VolumeControlPlatform> VolumeControlPlatform::Create(
    VolumeControlPlatform::VolumeChangedCallback&& volumeChanged,
    VolumeControlPlatform::MutedChangedCallback&& mutedChanged)
{
    return std::unique_ptr<VolumeControlPlatform>{new VolumeControlPlatformStub};
}

}  // namespace Plugin
}  // namespace WPEFramework
