#include "../Module.h"
#include <interfaces/IDisplayInfo.h>

#include <bcm_host.h>
#include <fstream>

namespace WPEFramework {
namespace Plugin {

class DisplayInfoImplementation : public Exchange::IDeviceProperties, public Exchange::IGraphicsProperties, public Exchange::IConnectionProperties {
    static constexpr const TCHAR* CPUInfoFile= _T("/proc/cpuinfo");

private:
    class Job : public Core::IDispatch {
    public:
        Job() = delete;
        Job(const Job&) = delete;
        Job& operator=(const Job&) = delete;

    public:
        Job(DisplayInfoImplementation* parent)
            : _pendingJob(false)
            , _parent(*parent)
            , _adminLock()
        {
        }
        virtual ~Job()
        {
            PluginHost::WorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(*this));
        }

    public:
        void Submit()
        {
            _adminLock.Lock();
            if (_pendingJob != true) {
                PluginHost::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                _pendingJob = true;
            }
            _adminLock.Unlock();
        }

    private:
        virtual void Dispatch()
        {
            _parent.Run();
            _adminLock.Lock();
            _pendingJob = false;
            _adminLock.Unlock();
        }

    private:
        bool _pendingJob;
        DisplayInfoImplementation& _parent;
        WPEFramework::Core::CriticalSection _adminLock;
    };

public:
    DisplayInfoImplementation()
        : _width(0)
        , _height(0)
        , _connected(false)
        , _totalGpuRam(0)
        , _audioPassthrough(false)
        , _refCount(0)
        , _adminLock()
        , _activity(Core::ProxyType<Job>::Create(this)) {

        bcm_host_init();

        UpdateChipset(_chipset);
        UpdateFirmwareVersion(_firmwareVersion);

        UpdateTotalGpuRam(_totalGpuRam);

        UpdateDisplayInfo(_connected, _width, _height, _audioPassthrough);
        RegisterDisplayCallback();
    }

    DisplayInfoImplementation(const DisplayInfoImplementation&) = delete;
    DisplayInfoImplementation& operator= (const DisplayInfoImplementation&) = delete;
    virtual ~DisplayInfoImplementation()
    {
        bcm_host_deinit();
    }

public:
    // Device Propertirs interface
    const string Chipset() const override
    {
        return _chipset;
    }
    const string FirmwareVersion() const override
    {
        return _firmwareVersion;
    }

    // Graphics Properties interface
    uint64_t TotalGpuRam() const override
    {
        return _totalGpuRam;
    }
    uint64_t FreeGpuRam() const override
    {
        uint64_t result;
        Command("get_mem reloc ", result);
        return (result);
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
        return _audioPassthrough;
    }
    bool Connected() const override
    {
        return _connected;
    }
    uint32_t Width() const override
    {
        return _width;
    }
    uint32_t Height() const override
    {
        return _height;
    }
    // HDCP support is not used for RPI now, it is always settings as DISPMANX_PROTECTION_NONE
    uint8_t HDCPMajor() const override
    {
        return 0;
    }
    uint8_t HDCPMinor() const override
    {
        return 0;
    }
    HDRType Type() const override
    {
        return HDR_OFF;
    }

    BEGIN_INTERFACE_MAP(DisplayInfoImplementation)
        INTERFACE_ENTRY(Exchange::IDeviceProperties)
        INTERFACE_ENTRY(Exchange::IGraphicsProperties)
        INTERFACE_ENTRY(Exchange::IConnectionProperties)
    END_INTERFACE_MAP

private:
    inline void UpdateFirmwareVersion(string& firmwareVersion) const
    {
        Command("version", firmwareVersion);
        if (firmwareVersion.length() > 0) {

            string::size_type i = 0;
            while (i < firmwareVersion.length()) {
                i = firmwareVersion.find_first_of("\n\r", i);
                if (i != std::string::npos) {
                    firmwareVersion.replace(i, 1, ", ");
                }
            }
        }
    }
    inline void UpdateChipset(string& chipset) const
    {
        string line;
        std::ifstream file(CPUInfoFile);
        if (file.is_open()) {
            while (getline(file, line)) {
                if (line.find("Hardware") != std::string::npos) {
                    std::size_t position = line.find(':');
                    if (position != std::string::npos) {
                        chipset.assign(line.substr(position + 1, string::npos));
                    }
                }
            }
            file.close();
        }
    }
    inline void UpdateTotalGpuRam(uint64_t& totalRam) const
    {
        Command("get_mem reloc_total ", totalRam);
    }
    void Command(const char request[], string& value) const
    {
        char buffer[512];

        // Reset the string
        buffer[0] = '\0';

        // Most VC API calls are guarded but we want to be sure anyway
        _adminLock.Lock();

        int VARIABLE_IS_NOT_USED status = vc_gencmd(buffer, sizeof(buffer), &request[0]);
        assert((status == 0) && "Error: vc_gencmd failed.\n");

        _adminLock.Unlock();

        // Make sure it is null-terminated
        buffer[sizeof(buffer) - 1] = '\0';

        // We do not need the stuff that is before the '=', we know what we requested :-)
        char* equal = strchr(buffer, '=');
        if (equal != nullptr) {
            equal++;
        }
        else {
            equal = buffer;
        }

        // Create string from buffer.
        Core::ToString(equal, value);
    }
    template<typename VALUE>
    void Command(const char request[], VALUE& result) const
    {
        string response;

        Command(request, response);

        const char* start = response.c_str();
        const char* index = start;

        // move the index to the unit inidicatuion type
        while (::isdigit(*index) || (*index == ' ') || (*index == '.') || (*index == ',')) {
            index++;
        }

        result = WPEFramework::Core::NumberType<VALUE>(WPEFramework::Core::TextFragment(start, (index - start))).Value();

        // Convert into bytes, if necessary.
        if ( (*index == 'M') && (index[1] == '\0') ) {
            // Multiply with MB
            result *= (1024 * 1024);
        }
        else if ( (*index == 'K') && (index[1] == '\0') ) {
            // Multiply with KB
            result *= 1024;
        }
    }

    inline void UpdateDisplayInfo(bool& connected, uint32_t& width, uint32_t& height, bool& audioPassthrough) const
    {
        TV_DISPLAY_STATE_T tvState;
        if (vc_tv_get_display_state(&tvState) == 0) {

            if (tvState.display.hdmi.width && tvState.display.hdmi.height) {
                width = tvState.display.hdmi.width;
                height = tvState.display.hdmi.height;
            }
            if ((tvState.state & VC_HDMI_ATTACHED) || ((tvState.state & VC_SDTV_ATTACHED) && ((tvState.state & VC_SDTV_NTSC) || (tvState.state & VC_SDTV_PAL))))  {
                connected = true;
            } else {
                connected = false;
            }
            if (tvState.state & VC_HDMI_HDMI) {
                audioPassthrough = true;
            } else {
                audioPassthrough = false;
            }
        }
    }
    inline void RegisterDisplayCallback()
    {
        vc_tv_register_callback(&DisplayCallback, reinterpret_cast<void*>(this));
    }
    static void DisplayCallback(void *cbData, uint32_t reason, uint32_t, uint32_t)
    {
        DisplayInfoImplementation* platform = static_cast<DisplayInfoImplementation*>(cbData);
        ASSERT(platform != nullptr);

        if (platform != nullptr) {
            switch (reason) {
            case VC_HDMI_UNPLUGGED:
            case VC_SDTV_UNPLUGGED:
            case VC_HDMI_ATTACHED:
            case VC_SDTV_ATTACHED: {
                platform->Submit();
                break;
            }
            default: {
                // Ignore all other reasons
                break;
            }
            }
        }
    }
    void Submit()
    {
        _activity->Submit();
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
    void Run()
    {
        _adminLock.Lock();
        UpdateDisplayInfo(_connected, _width, _height, _audioPassthrough);
        _adminLock.Unlock();

        Updated();
    }

private:
    string _chipset;
    string _firmwareVersion;

    uint32_t _width;
    uint32_t _height;
    bool _connected;
    uint64_t _totalGpuRam;
    bool _audioPassthrough;

    mutable uint32_t _refCount;
    std::list<IConnectionProperties::INotification*> _observers;

    mutable WPEFramework::Core::CriticalSection _adminLock;

    Core::ProxyType<Job> _activity;
};

    SERVICE_REGISTRATION(DisplayInfoImplementation, 1, 0);
}
}
