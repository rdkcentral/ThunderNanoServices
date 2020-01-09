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
