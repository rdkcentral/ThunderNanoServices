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
      uint32_t result = NxClient_SetAudioSettings(&audioSettings) != 0 ?
            Core::ERROR_GENERAL :
            Core::ERROR_NONE;

      return result;
  }

  bool Muted() const override
  {
      NxClient_AudioSettings audioSettings;
      NxClient_GetAudioSettings(&audioSettings);
      return audioSettings.muted;
  }

  uint32_t Volume(uint8_t volume) override
  {
      volume = std::max(volume, kMinVolume);
      volume = std::min(volume, kMaxVolume);
      int32_t toSet = volume;
      NxClient_AudioSettings audioSettings;
      NxClient_GetAudioSettings(&audioSettings);
      if (audioSettings.volumeType == NEXUS_AudioVolumeType_eDecibel) {
          toSet = VolumeControlPlatformNexus::ToNexusDb(volume);
      } else {
          toSet = VolumeControlPlatformNexus::ToNexusLinear(volume);
      }

      audioSettings.leftVolume = audioSettings.rightVolume = toSet;
      return NxClient_SetAudioSettings(&audioSettings) != 0 ?
            Core::ERROR_GENERAL :
            Core::ERROR_NONE;
  }

  uint8_t Volume() const override
  {
      NxClient_AudioSettings audioSettings;
      NxClient_GetAudioSettings(&audioSettings);
      ASSERT(audioSettings.leftVolume == audioSettings.rightVolume);
      auto result =  audioSettings.volumeType == NEXUS_AudioVolumeType_eDecibel ?
          VolumeControlPlatformNexus::FromNexusDb(audioSettings.leftVolume) :
          VolumeControlPlatformNexus::FromNexusLinear(audioSettings.leftVolume);
      return result;
  }

private:
  static int32_t ToNexus(int8_t vol, int32_t min, int32_t max)
  {
    return (vol * (max - min) / (kMaxVolume - kMinVolume)) + min;
  }

  static int32_t ToNexusLinear(int8_t vol)
  {
      return ToNexus(vol, NEXUS_AUDIO_VOLUME_LINEAR_MIN, NEXUS_AUDIO_VOLUME_LINEAR_NORMAL);
  }

  static int32_t ToNexusDb(int8_t vol)
  {
      return ToNexus(vol, NEXUS_AUDIO_VOLUME_DB_MIN, NEXUS_AUDIO_VOLUME_DB_NORMAL);
  }

  static int8_t FromNexus(int32_t vol, int32_t min, int32_t max)
  {
      return (vol - min) * (kMaxVolume - kMinVolume) / (max - min);
  }

  static int8_t FromNexusLinear(int32_t vol)
  {
      return FromNexus(vol, NEXUS_AUDIO_VOLUME_LINEAR_MIN, NEXUS_AUDIO_VOLUME_LINEAR_NORMAL);
  }

  static int8_t FromNexusDb(int32_t vol)
  {
    return FromNexus(vol, NEXUS_AUDIO_VOLUME_DB_MIN, NEXUS_AUDIO_VOLUME_DB_NORMAL);
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
