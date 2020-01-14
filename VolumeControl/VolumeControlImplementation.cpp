#include "VolumeControlImplementation.h"

#include "VolumeControlPlatform.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(VolumeControlImplementation, 1, 0);

    VolumeControlImplementation::VolumeControlImplementation()
        : _adminLock{}
        , _platform{std::move(VolumeControlPlatform::Create(
                                [this]() { NotifyVolumeChange(); },
                                [this]() { NotifyMutedChange(); }))}
    {
    }

    VolumeControlImplementation::~VolumeControlImplementation() = default;

    void VolumeControlImplementation::Register(Exchange::IVolumeControl::INotification* notification)
    {
        ASSERT(notification);
        _adminLock.Lock();
        notification->AddRef();
        _notifications.push_back(notification);
        _adminLock.Unlock();
    }

    void VolumeControlImplementation::Unregister(const Exchange::IVolumeControl::INotification* notification)
    {
        ASSERT(notification);
        _adminLock.Lock();
        auto item = std::find(_notifications.begin(), _notifications.end(), notification);
        ASSERT(item != _notifications.end());
        _notifications.erase(item);
        (*item)->Release();
        _adminLock.Unlock();
    }


    uint32_t VolumeControlImplementation::Muted(const bool muted)
    {
        return _platform->Muted(muted);
    }

    uint32_t VolumeControlImplementation::Muted(bool& muted) const
    {
        muted = _platform->Muted();
        return Core::ERROR_NONE;
    }

    uint32_t VolumeControlImplementation::Volume(const uint8_t volume)
    {
        return _platform->Volume(volume);
    }

    uint32_t VolumeControlImplementation::Volume(uint8_t& vol) const
    {
        vol = _platform->Volume();
        return Core::ERROR_NONE;
    }

    void VolumeControlImplementation::NotifyVolumeChange()
    {
        _adminLock.Lock();
        for (auto* notification : _notifications) {
            notification->Volume(_platform->Volume());
        }
        _adminLock.Unlock();
    }

    void VolumeControlImplementation::NotifyMutedChange()
    {
        _adminLock.Lock();
        for (auto* notification : _notifications) {
            notification->Muted(_platform->Muted());
        }
        _adminLock.Unlock();
    }

}  // namespace Plugin
}  // namespace WPEFramework
