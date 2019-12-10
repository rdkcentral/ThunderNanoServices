#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

struct IDevicePlatform {

    virtual ~IDevicePlatform() {}

    virtual uint64_t TotalGpuRam() const;
    virtual uint64_t FreeGpuRam() const;
    static Core::ProxyType<IDevicePlatform> Instance();
};

}
}
