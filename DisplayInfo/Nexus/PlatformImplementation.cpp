#include "../DeviceProperties.h"
#include <nexus_config.h>
#include <nexus_platform.h>
#include <nxclient.h>

namespace WPEFramework {
namespace Device {
namespace Implementation {

class NexusPlatform : public Plugin::IDeviceProperties, public Plugin::IGraphicsProperties, public Plugin::IConnectionProperties, public Core::Thread {
public:
    NexusPlatform()
       : _width(0)
       , _height(0)
       , _connected(false)
       , _major(0)
       , _minor(0)
       , _type(HDR_OFF)
       , _refCount(0)
       , _adminLock()
       , _totalGpuRam(0) {

        NEXUS_Error rc = NxClient_Join(NULL);
        ASSERT(!rc);
        NEXUS_Platform_GetConfiguration(&_platformConfig);

        UpdateChipset(_chipset);
        UpdateFirmwareVersion(_firmwareVersion);

        UpdateTotalGpuRam(_totalGpuRam);
        UpdateDisplayInfo(_connected, _width, _height)
    }

    NexusPlatform(const NexusPlatform&) = delete;
    NexusPlatform& operator= (const NexusPlatform&) = delete;
    virtual ~NexusPlatform()
    {
        NxClient_Uninit();
    }

public:

    // Device Propertirs interface
    const std::string Chipset() const override
    {
        return _chipset;
    }
    const std::string FirmwareVersion() const override
    {
        return _firmwareVersion;
    }
    Core::ProxyType<IGraphicsProperties>  GraphicsInstance() override
    {
        return static_cast<Core::ProxyType<Plugin::IGraphicsProperties>>(_nexusPlatform);
    }
    Core::ProxyType<IConnectionProperties>  ConnectionInstance() override
    {
        return static_cast<Core::ProxyType<Plugin::IConnectionProperties>>(_nexusPlatform);
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
        return _connected;
    }
    void Connected(bool connected)
    {
        _adminLock.Lock();
        _connected = connected;
        _adminLock.Unlock();
        Run();
    }
    uint32_t Width() const override
    {
        return _width;
    }
    uint32_t Height() const override
    {
        return _height;
    }
    uint8_t HDCPMajor() const override
    {
        return _major;
    }
    uint8_t HDCPMinor() const override
    {
        return _minor;
    }
    HDRType Type() const override
    {
        return _type;
    }

    static Core::ProxyType<Device::Implementation::NexusPlatform> Instance()
    {
        _nexusPlatform = Core::ProxyType<Device::Implementation::NexusPlatform>::Create();
        return _nexusPlatform;
    }

private:
    inline void UpdateFirmwareVersion(string& firmwareVersion) const
    {
    }
    inline void UpdateChipset(string& chipset) const
    {
    }
    inline void UpdateTotalGpuRam(uint64_t& totalRam) const
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

    inline void UpdateDisplayInfo(bool& connected, uint32_t& width, uint32_t& height) const
    {
    }
    void Updated() const
    {
        _adminLock.Lock();
        std::list<IConnectionProperties::INotification*>::const_iterator index = _observers.begin();

        if (index != _observers.end()) {
            (*index)->Updated();
        }

        _adminLock.Unlock();
    }

    uint32_t Worker() override
    {
        if (IsRunning() == true) {
            Updated();
            Block();
        }
        return (Core::infinite);
    }
private:
    string _chipset;
    string _firmwareVersion;

    uint32_t _width;
    uint32_t _height;
    bool _connected;

    uint8_t major;
    uint8_t minor;
    HDRType _type;

    mutable uint32_t _refCount;
    uint64_t _totalGpuRam;
    std::list<IConnectionProperties::INotification*> _observers;

    NEXUS_PlatformConfiguration _platformConfig;

    mutable WPEFramework::Core::CriticalSection _adminLock;
    static Core::ProxyType<Device::Implementation::NexusPlatform> _nexusPlatform;
};
}
}

/* static */ Core::ProxyType<Plugin::IDeviceProperties> Plugin::IDeviceProperties::Instance()
{
    static Core::ProxyType<Plugin::IDeviceProperties> nexusPlatform(Device::Implementation::NexusPlatform::Instance());
    return nexusPlatform;
}
}
