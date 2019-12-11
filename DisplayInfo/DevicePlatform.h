#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

struct IDevicePlatform {

    virtual ~IDevicePlatform() {}

    virtual uint64_t TotalGpuRam() const = 0;
    virtual uint64_t FreeGpuRam() const = 0;
    static Core::ProxyType<IDevicePlatform> Instance();
};

}
}
