#include "../DeviceProperties.h"
#include <nexus_config.h>
#include <nexus_platform.h>
#include <nxclient.h>

namespace WPEFramework {
namespace Device {
namespace Implementation {

class NexusPlatform : public Plugin::IDeviceProperties, public Plugin::IGraphicsProperties, Plugin::IConnectionProperties, public Core::IReferenceCounted {
public:
    NexusPlatform()
    : _refCount(0)
    , _adminLock()
    , _totalGpuRam(0) {

        NEXUS_Error rc = NxClient_Join(NULL);
        ASSERT(!rc);
        NEXUS_Platform_GetConfiguration(&_platformConfig);
        UpdateTotalGpuRam(_totalGpuRam);
    }

    NexusPlatform(const NexusPlatform&) = delete;
    NexusPlatform& operator= (const NexusPlatform&) = delete;
    virtual ~NexusPlatform()
    {
        NxClient_Uninit();
    }

public:
    void AddRef() const override
    {
        Core::InterlockedIncrement(_refCount);
    }
    uint32_t Release() const override
    {
        Core::InterlockedDecrement(_refcount);
        return (Core::ERROR_NONE);
    }

    // Device Propertirs interface
    const std::string Chipset() const override
    {
        return string();
    }
    const std::string FirmwareVersion() const override
    {
        return string();
    }
    Core::ProxyType<IGraphicsProperties>  GraphicsInstance() override
    {
        return static_cast<Core::ProxyType<Plugin::IGraphicsProperties>>(*this);
    }
    Core::ProxyType<IConnectionProperties>  ConnectionInstance() override
    {
        return static_cast<Core::ProxyType<Plugin::IConnectionProperties>>(*this);
    }

    // Graphics Properties interface
    uint64_t TotalGpuRam() const override
    {
        return _totalGpuRam;
    }
    uint64_t FreeGpuRam() const override
    {
        uint64_t freeRam = 0;
        NEXUS_MemoryStatus status;

        NEXUS_Error rc = NEXUS_UNKNOWN;
#if NEXUS_MEMC0_GRAPHICS_HEAP
        if (_platformConfig.heap[NEXUS_MEMC0_GRAPHICS_HEAP]) {
            rc = NEXUS_Heap_GetStatus(_platformConfig.heap[NEXUS_MEMC0_GRAPHICS_HEAP], &status);
            if (rc == NEXUS_SUCCESS) {
                freeRam = static_cast<uint64_t>(status.free);
            }
        }
#endif
#if NEXUS_MEMC1_GRAPHICS_HEAP
        if (_platformConfig.heap[NEXUS_MEMC1_GRAPHICS_HEAP]) {
            rc = NEXUS_Heap_GetStatus(_platformConfig.heap[NEXUS_MEMC1_GRAPHICS_HEAP], &status);
            if (rc == NEXUS_SUCCESS) {
                freeRam += static_cast<uint64_t>(status.free);
            }
        }
#endif
#if NEXUS_MEMC2_GRAPHICS_HEAP
        if (_platformConfig.heap[NEXUS_MEMC2_GRAPHICS_HEAP]) {
            rc = NEXUS_Heap_GetStatus(_platformConfig.heap[NEXUS_MEMC2_GRAPHICS_HEAP], &status);
            if (rc == NEXUS_SUCCESS) {
                freeRam  += static_cast<uint64_t>(status.free);
            }
        }
#endif
        return (freeRam);
    }

    // Connection Properties interface
    uint32_t Register(INotification* notification) override
    {
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());

        _observers.push_back(notification);
        notification->AddRef();

        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }
    uint32_t Unregister(INotification* notification) override
    {
        _adminLock.Lock();

        std::list<IConnectionProperties::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _observers.end());

        if (index != _observers.end()) {
            (*index)->Release();
            _observers.erase(index);
        }

        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }

    bool IsAudioPassThrough () const override
    {
        return false;
    }
    bool Connected() const override
    {
        return false;
    }
    uint32_t Width() const override
    {
        uint32_t width = 0;
        return width;
    }
    uint32_t Height() const override
    {
        uint32_t height = 0;
        return height;
    }
    uint8_t HDCPMajor() const override
    {
        uint8_t major = 0;
        return major;
    }
    uint8_t HDCPMinor() const override
    {
        uint8_t minor = 0;
        return minor;
    }
    HDRType Type() const override
    {
        HDRType type = HDR_OFF;
        return type;
    }

private:
    void UpdateTotalGpuRam(uint64_t& totalRam) const
    {
        NEXUS_MemoryStatus status;
        NEXUS_Error rc = NEXUS_UNKNOWN;

#if NEXUS_MEMC0_GRAPHICS_HEAP
        if (_platformConfig.heap[NEXUS_MEMC0_GRAPHICS_HEAP]) {
            rc = NEXUS_Heap_GetStatus(_platformConfig.heap[NEXUS_MEMC0_GRAPHICS_HEAP], &status);
            if (rc == NEXUS_SUCCESS) {
                totalRam = static_cast<uint64_t>(status.size);
            }
        }
#endif
#if NEXUS_MEMC1_GRAPHICS_HEAP
        if (_platformConfig.heap[NEXUS_MEMC1_GRAPHICS_HEAP]) {
            rc = NEXUS_Heap_GetStatus(_platformConfig.heap[NEXUS_MEMC1_GRAPHICS_HEAP], &status);
            if (rc == NEXUS_SUCCESS) {
                totalRam += static_cast<uint64_t>(status.size);
            }
        }
#endif
#if NEXUS_MEMC2_GRAPHICS_HEAP
        if (_platformConfig.heap[NEXUS_MEMC2_GRAPHICS_HEAP]) {
            rc = NEXUS_Heap_GetStatus(_platformConfig.heap[NEXUS_MEMC2_GRAPHICS_HEAP], &status);
            if (rc == NEXUS_SUCCESS) {
                totalRam += static_cast<uint64_t>(status.size);
            }
        }
#endif
    }

private:
    mutable uint32_t _refCount;
    uint64_t _totalGpuRam;
    NEXUS_PlatformConfiguration _platformConfig;

    mutable WPEFramework::Core::CriticalSection _adminLock;
};
}
}

/* static */ Core::ProxyType<Plugin::IDeviceProperties> Plugin::IDeviceProperties::Instance()
{
    static Device::Implementation::RPIPlatform nexusPlatform;
    return static_cast<Core::ProxyType<Plugin::IDeviceProperties>>(nexusPlatform);
}
}
