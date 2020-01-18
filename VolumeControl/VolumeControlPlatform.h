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
