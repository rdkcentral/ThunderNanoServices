#include "../GraphicsProperties.h"

#include <bcm_host.h>

namespace WPEFramework {
namespace Device {
namespace Implementation {

class RPIPlatform : public Plugin::IGraphicsProperties {
public:
    RPIPlatform() {
        bcm_host_init();
        UpdateTotalGpuRam(_totalGpuRam);
    }

    RPIPlatform(const RPIPlatform&) = delete;
    RPIPlatform& operator= (const RPIPlatform&) = delete;
    virtual ~RPIPlatform()
    {
        bcm_host_deinit();
    }

public:
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

private:
    inline void UpdateTotalGpuRam(uint64_t& totalRam)
    {
        Command("get_mem reloc_total ", totalRam);
    }
    void Command(const char request[], std::string& value) const
    {
        char buffer[512];

        // Reset the string
        buffer[0] = '\0';

        // Most VC API calls are guarded but we want to be sure anyway
        _mutualExclusion.Lock();

        int VARIABLE_IS_NOT_USED status = vc_gencmd(buffer, sizeof(buffer), &request[0]);
        assert((status == 0) && "Error: vc_gencmd failed.\n");

        _mutualExclusion.Unlock();

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
        value = std::string (equal);
    }
    template<typename VALUE>
    void Command(const char request[], VALUE& result) const
    {
        std::string response;

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

private:
    uint64_t _totalGpuRam;
    mutable WPEFramework::Core::CriticalSection _mutualExclusion;
};
}
}

/* static */ Core::ProxyType<Plugin::IGraphicsProperties> Plugin::IGraphicsProperties::Instance()
{
    static Core::ProxyType<Device::Implementation::RPIPlatform> rpiPlatform(Core::ProxyType<Device::Implementation::RPIPlatform>::Create());
    return static_cast<Core::ProxyType<Plugin::IGraphicsProperties>>(rpiPlatform);
}
}
