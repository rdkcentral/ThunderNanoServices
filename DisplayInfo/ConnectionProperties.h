#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

struct IConnectionProperties {

    enum HDRType {
        HDR_OFF,
        HDR_10,
        HDR_10PLUS,
        HDR_DOLBYVISION,
        HDR_TECHNICOLOR
    };

    struct INotification : virtual public Core::IUnknown {
        void Updated();
    };

    virtual uint32_t Register(INotification*) = 0;
    virtual uint32_t Unregister(INotification*) = 0;

    virtual bool IsAudioPassThrough () const = 0;
    virtual bool Connected() const = 0;
    virtual uint32_t Width() const = 0;
    virtual uint32_t Height() const = 0;
    virtual uint8_t HDCPMajor() const = 0;
    virtual uint8_t HDCPMinor() const = 0;
    virtual HDRType Type() const = 0;

    static Core::ProxyType<IConnectionProperties> Instance();
};

}
}
