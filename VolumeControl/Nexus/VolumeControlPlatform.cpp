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

#include <nexus_config.h>
#include <nexus_audio_types.h>
#include <nexus_platform.h>
#include <nexus_simple_audio_decoder.h>
#include <nxclient.h>

namespace WPEFramework {
namespace Plugin {

namespace {

constexpr uint8_t kMinVolume = 0;
constexpr uint8_t kMaxVolume = 100;

class VolumeControlPlatformNexus : public VolumeControlPlatform {
public:

  VolumeControlPlatformNexus()
  {
      NxClient_JoinSettings joinSettings;
      NxClient_GetDefaultJoinSettings(&joinSettings);
      snprintf(joinSettings.name, NXCLIENT_MAX_NAME, "%s", "wpevolumecontrol");
      NxClient_Join(&joinSettings);
      NxClient_UnregisterAcknowledgeStandby(NxClient_RegisterAcknowledgeStandby());
  }

  ~VolumeControlPlatformNexus() override
  {
      NxClient_Uninit();
  }

  uint32_t Muted(bool muted) override
  {
      NxClient_AudioSettings audioSettings;
      NxClient_GetAudioSettings(&audioSettings);
      audioSettings.muted = muted;
      int result = NxClient_SetAudioSettings(&audioSettings); 

      TRACE(Trace::Information, (_T("Hardware set muted: %d [%d]"), audioSettings.muted, result));
      return (result != 0 ? Core::ERROR_GENERAL : Core::ERROR_NONE);
  }

  bool Muted() const override
  {
      NxClient_AudioSettings audioSettings;
      NxClient_GetAudioSettings(&audioSettings);
      TRACE(Trace::Information, (_T("Hardware get muted: %d"), audioSettings.muted));
      return audioSettings.muted;
  }

  uint32_t Volume(uint8_t volume) override
  {
      const TCHAR* type = _T("Decibel");
      volume = std::max(volume, kMinVolume);
      volume = std::min(volume, kMaxVolume);
      int32_t toSet = volume;
      NxClient_AudioSettings audioSettings;
      NxClient_GetAudioSettings(&audioSettings);
      if (audioSettings.volumeType == NEXUS_AudioVolumeType_eDecibel) {
          toSet = VolumeControlPlatformNexus::ToNexusDb(volume);
      } else {
          toSet = VolumeControlPlatformNexus::ToNexusLinear(volume);
          type = _T("Linear");
      }

      audioSettings.leftVolume = audioSettings.rightVolume = toSet;

      int result = NxClient_SetAudioSettings(&audioSettings);

      TRACE(Trace::Information, (_T("Hardware set volume (%s): %d [%d]"), type, toSet, result));

      return (result != 0 ? Core::ERROR_GENERAL : Core::ERROR_NONE);
  }

  uint8_t Volume() const override
  {
      uint8_t result;

      NxClient_AudioSettings audioSettings;
      NxClient_GetAudioSettings(&audioSettings);
      ASSERT(audioSettings.leftVolume == audioSettings.rightVolume);
      result = audioSettings.volumeType == NEXUS_AudioVolumeType_eDecibel ?
          VolumeControlPlatformNexus::FromNexusDb(audioSettings.leftVolume) :
          VolumeControlPlatformNexus::FromNexusLinear(audioSettings.leftVolume);

      TRACE(Trace::Information, (_T("Hardware get volume: %d"), result));

      return (result);
  }

private:
  static int32_t ToNexusLinear(uint8_t vol)
  {
      return std::round(static_cast<double>((vol * (NEXUS_AUDIO_VOLUME_LINEAR_NORMAL - NEXUS_AUDIO_VOLUME_LINEAR_MIN)) /
          (kMaxVolume - kMinVolume)) + NEXUS_AUDIO_VOLUME_LINEAR_MIN);
  }

  static uint8_t FromNexusLinear(int32_t vol)
  {
      return  std::round(static_cast<double>((vol - NEXUS_AUDIO_VOLUME_LINEAR_MIN) * (kMaxVolume - kMinVolume)) /
          (NEXUS_AUDIO_VOLUME_LINEAR_NORMAL - NEXUS_AUDIO_VOLUME_LINEAR_MIN));
  }

  static int32_t ToNexusDb(uint8_t vol)
  {
      int32_t result = 0;
      auto gain = 2000.0 * log10(static_cast<double>(vol) / (kMaxVolume - kMinVolume));
      if (std::isinf(gain)) {
          result = NEXUS_AUDIO_VOLUME_DB_MIN;
      } else if (gain == 0) {
          result = NEXUS_AUDIO_VOLUME_DB_NORMAL;
      } else {
          result = NEXUS_AUDIO_VOLUME_DB_NORMAL + std::floor(gain);
      }

      return std::max(NEXUS_AUDIO_VOLUME_DB_MIN, result);
  }

  static uint8_t FromNexusDb(int32_t vol)
  {
      auto gain = NEXUS_AUDIO_VOLUME_DB_NORMAL - vol;
      auto factor = std::pow(10, (static_cast<double>(gain) / 2000.0));
      return std::round((kMaxVolume - kMinVolume) / factor);
  }
};

}

// static
std::unique_ptr<VolumeControlPlatform> VolumeControlPlatform::Create(
    VolumeControlPlatform::VolumeChangedCallback&& volumeChanged,
    VolumeControlPlatform::MutedChangedCallback&& mutedChanged)
{
    return std::unique_ptr<VolumeControlPlatform>{new VolumeControlPlatformNexus};
}

}  // namespace Plugin
}  // namespace WPEFramework
