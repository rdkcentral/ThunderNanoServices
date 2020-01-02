#pragma once

#include "Module.h"

#include "ConnectionProperties.h"
#include "GraphicsProperties.h"

namespace WPEFramework {
namespace Plugin {

struct IDeviceProperties {
    virtual ~IDeviceProperties() { }

    virtual const std::string Chipset() const = 0;
    virtual const std::string FirmwareVersion() const = 0;

    virtual IGraphicsProperties*  GraphicsInstance() = 0;
    virtual IConnectionProperties*  ConnectionInstance() = 0;

    // Lifetime management
    virtual void AddRef() const = 0;
    virtual uint32_t Release() const = 0;

    static IDeviceProperties* Instance();
};

}
}
