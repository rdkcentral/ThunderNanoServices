#pragma once

#include "Module.h"

#include "ConnectionProperties.h"
#include "GraphicsProperties.h"

namespace WPEFramework {
namespace Plugin {

struct IDeviceProperties {

    virtual const std::string Chipset() const = 0;
    virtual const std::string FirmwareVersion() const = 0;

    virtual Core::ProxyType<IGraphicsProperties>  GraphicsInstance() = 0;
    virtual Core::ProxyType<IConnectionProperties>  ConnectionInstance() = 0;

    static Core::ProxyType<IDeviceProperties> Instance();
};

}
}
