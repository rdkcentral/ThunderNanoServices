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
       , _totalGpuRam(0)
       , _audioPassthrough(false)
       , _refCount(0)
       , _adminLock() {

        NEXUS_Error rc = NxClient_Join(NULL);
        ASSERT(!rc);
        NEXUS_Platform_GetConfiguration(&_platformConfig);

        UpdateChipset(_chipset);
        UpdateFirmwareVersion(_firmwareVersion);

        UpdateTotalGpuRam(_totalGpuRam);
        UpdateDisplayInfo(_connected, _width, _height, _major, _minor, _type);

        UpdateAudioPassthrough(_audioPassthrough);
        RegisterCallback();
    }

    NexusPlatform(const NexusPlatform&) = delete;
    NexusPlatform& operator= (const NexusPlatform&) = delete;
    virtual ~NexusPlatform()
    {
        NxClient_StopCallbackThread();
        NxClient_Uninit();
        Stop();
        Wait(Thread::STOPPED | Thread::BLOCKED, Core::infinite);
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
    IGraphicsProperties*  GraphicsInstance() override
    {
        return static_cast<Plugin::IGraphicsProperties*>(_nexusPlatform);
    }
    IConnectionProperties*  ConnectionInstance() override
    {
        return static_cast<Plugin::IConnectionProperties*>(_nexusPlatform);
    }
    virtual void AddRef() const
    {
        Core::InterlockedIncrement(_refCount);
    }
    virtual uint32_t Release() const
    {
        if (Core::InterlockedDecrement(_refCount) == 0) {
            delete this;

            return (Core::ERROR_CONNECTION_CLOSED);
        }
        return (Core::ERROR_NONE);
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

    bool IsAudioPassthrough () const override
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
    void Type(HDRType type)
    {
        _adminLock.Lock();
        _type = type;
        _adminLock.Unlock();
        Run();
    }
    static Device::Implementation::NexusPlatform* Instance()
    {
        return _nexusPlatform;
    }

private:
    inline void UpdateFirmwareVersion(string& firmwareVersion) const
    {
        char version[256];
        sprintf(version, "Nexus Release %d.%d", (NEXUS_P_GET_VERSION(NEXUS_PLATFORM) / NEXUS_PLATFORM_VERSION_UNITS), (NEXUS_P_GET_VERSION(NEXUS_PLATFORM) % NEXUS_PLATFORM_VERSION_UNITS));
        firmwareVersion = version;
    }
    inline void UpdateChipset(string& chipset) const
    {
        NEXUS_PlatformStatus status;
        NEXUS_Error rc = NEXUS_UNKNOWN;

        rc = NEXUS_Platform_GetStatus(&status);
        if (rc == NEXUS_SUCCESS) {
            char chipId[10];
            sprintf(chipId, "%x", status.chipId);
            char revision[10];
            sprintf(revision, "%c%c", (((status.chipRevision >> 4) & 0xF) + 'A' - 1), (((status.chipRevision) & 0xF) + '0'));
            chipset = "BCM" + string(chipId) + " " + string(revision);
        }
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

    inline void UpdateAudioPassthrough(bool& audioPassthrough)
    {
        NxClient_AudioStatus status;
        NEXUS_Error rc = NEXUS_UNKNOWN;
        rc = NxClient_GetAudioStatus(&status);
        if (rc == NEXUS_SUCCESS) {
            if (status.hdmi.outputMode == NxClient_AudioOutputMode_ePassthrough) {
                audioPassthrough = true;
            }
        }
    }
    inline void UpdateDisplayInfo(bool& connected, uint32_t& width, uint32_t& height, uint8_t& major, uint8_t& minor, bool type) const
    {
        NEXUS_Error rc = NEXUS_SUCCESS;
        NxClient_DisplayStatus status;

        rc = NxClient_GetDisplayStatus(&status);
        if (rc == NEXUS_SUCCESS) {
            if (status.hdmi.status.connected == true) {
                connected = true;
                // Read HDR status
                switch (status.hdmi.dynamicRangeMode) {
                case NEXUS_VideoDynamicRangeMode_eHdr10: {
                    type = HDR_10;
                    break;
                }
                case NEXUS_VideoDynamicRangeMode_eHdr10Plus: {
                    type = HDR_10PLUS;
                    break;
                }
                default:
                    break;
                }

                // Check HDCP version
                if (status.hdmi.hdcp.hdcp2_2Features == true) {
                    major = 2;
                    minor = 2;
                } else if (status.hdmi.hdcp.hdcp1_1Features == true) {
                    major = 1;
                    minor = 1;
                }
            }
        }
        // Read display width and height
        NEXUS_DisplayCapabilities capabilities;
        NEXUS_GetDisplayCapabilities(&capabilities);
        width = capabilities.display[0].graphics.width;
        height = capabilities.display[0].graphics.height;
    }
    void RegisterCallback()
    {
        NxClient_CallbackThreadSettings settings;
        NxClient_GetDefaultCallbackThreadSettings(&settings);

        settings.hdmiOutputHotplug.callback = Callback;
        settings.hdmiOutputHotplug.context = reinterpret_cast<void*>(this);
        settings.hdmiOutputHotplug.param = 0;

        settings.displaySettingsChanged.callback = Callback;
        settings.displaySettingsChanged.param = 1;
        settings.displaySettingsChanged.context = reinterpret_cast<void*>(this);

        if (NxClient_StartCallbackThread(&settings) != NEXUS_SUCCESS) {
            TRACE_L1(_T("Error in starting nexus callback thread"));
        }
    }
    static void Callback(void *cbData, int param)
    {
        NEXUS_Error rc = NEXUS_SUCCESS;
        NexusPlatform* platform = static_cast<NexusPlatform*>(cbData);

        switch (param) {
        case 0: {
            NxClient_DisplayStatus status;
            rc = NxClient_GetDisplayStatus(&status);
            if (rc == NEXUS_SUCCESS) {
                if (status.hdmi.status.connected == true) {
                    platform->Connected(true);
                } else {
                    platform->Connected(false);
                }
            }
            break;
        }
        case 1: {
            NxClient_DisplayStatus status;
            rc = NxClient_GetDisplayStatus(&status);
            if (rc == NEXUS_SUCCESS) {
                // Read HDR status
                switch (status.hdmi.dynamicRangeMode) {
                case NEXUS_VideoDynamicRangeMode_eHdr10: {
                    platform->Type(HDR_10);
                    break;
                }
                case NEXUS_VideoDynamicRangeMode_eHdr10Plus: {
                    platform->Type(HDR_10PLUS);
                    break;
                }
                default:
                    break;
                }
            }
        }
        default:
            break;
        }
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

    uint8_t _major;
    uint8_t _minor;
    HDRType _type;

    uint64_t _totalGpuRam;
    bool _audioPassthrough;

    mutable uint32_t _refCount;
    std::list<IConnectionProperties::INotification*> _observers;

    NEXUS_PlatformConfiguration _platformConfig;

    mutable WPEFramework::Core::CriticalSection _adminLock;
    static Device::Implementation::NexusPlatform* _nexusPlatform;
};
}
}

Device::Implementation::NexusPlatform* Device::Implementation::NexusPlatform::_nexusPlatform = new Device::Implementation::NexusPlatform();

/* static */ Plugin::IDeviceProperties* Plugin::IDeviceProperties::Instance()
{
    return static_cast<Plugin::IDeviceProperties*>(Device::Implementation::NexusPlatform::Instance());
}
}
