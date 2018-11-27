#ifndef __LINUX_GPIO_H__
#define __LINUX_GPIO_H__

#include "Module.h"

#include <interfaces/IExternal.h>

namespace WPEFramework {

namespace GPIO {

class Pin : public Exchange::ExternalBase<Exchange::IExternal::GPIO>, public Core::IResource {
private:
    Pin () = delete;
    Pin (const Pin&) = delete;
    Pin& operator= (const Pin&) = delete;

    typedef Exchange::ExternalBase<Exchange::IExternal::GPIO> BaseClass;

public:
    enum pin_mode {
        INPUT,
        OUTPUT,
        PWM_TONE,
        PWM,
        CLOCK
    };

    enum pull_mode {
        OFF = 0,
        DOWN = 1,
        UP = 2
    };

    enum trigger_mode {
        NONE    = 0x00,
        FALLING = 0x01,
        RISING  = 0x02,
        BOTH    = 0x03,
        HIGH    = 0x04,
        LOW     = 0x08
    };

public:
    Pin(const uint8_t id, const bool activeLow);
    virtual ~Pin();
  
public:
    bool Get () const;
    void Set (const bool value);

    void Trigger (const trigger_mode mode);
    void Mode (const pin_mode mode);
    void Pull (const pull_mode mode);

    bool HasChanged() const;
    void Align();
    
    inline void Subscribe(Exchange::IExternal::INotification* sink) {
        BaseClass:Register(sink);
        Core::ResourceMonitor::Instance().Register(*this);
    }
    inline void Unsubscribe(Exchange::IExternal::INotification* sink) {
        Core::ResourceMonitor::Instance().Unregister(*this);
        BaseClass:Unregister(sink);
    }

    virtual void Trigger() override;
    virtual uint32_t Get(int32_t& value) const override;
    virtual uint32_t Set(const int32_t value) override;
 
private: 
    virtual Core::IResource::handle Descriptor() const override;
    virtual uint16_t Events() override;
    virtual void Handle(const uint16_t events) override;

    virtual void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) override;
    virtual void Revoke(const Core::ProxyType<Core::IDispatch>& job) override;

    void Flush();

private:
    const uint8_t _pin;
    uint8_t _activeLow;
    bool _lastValue;
    mutable int _descriptor;
};

} } // namespace WPEFramework::GPIO

#endif // __LINUX_GPIO_H__
