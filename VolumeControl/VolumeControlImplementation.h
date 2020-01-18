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
