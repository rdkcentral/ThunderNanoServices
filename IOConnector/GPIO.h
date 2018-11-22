#ifndef __LINUX_GPIO_H__
#define __LINUX_GPIO_H__

#include "Module.h"

namespace WPEFramework {

namespace GPIO {

class Pin : public Core::IResource {
private:
    Pin () = delete;
    Pin (const Pin&) = delete;
    Pin& operator= (const Pin&) = delete;

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

    struct IObserver {
        virtual ~IObserver() {};

        virtual void Activity (Pin&  pin) = 0;
    };

public:
    Pin(const uint8_t id);
    virtual ~Pin();
  
public:
    inline uint8_t Id () const {
        return (_pin);
    }

    void Register(IObserver* callback);
    void Unregister(IObserver* callback);

    bool Get () const;
    void Set (const bool value);

    void Trigger (const trigger_mode mode);
    void Mode (const pin_mode mode);
    void Pull (const pull_mode mode);

private: 
    virtual Core::IResource::handle Descriptor() const override;
    virtual uint16_t Events() override;
    virtual void Handle(const uint16_t events) override;

    void Flush();

    void Notify () {

        Flush();

        _adminLock.Lock();

        std::list<IObserver*>::iterator index (_observers.begin());

        while (index != _observers.end()) {
            (*index)->Activity (*this);
	    index++;
        }

        _adminLock.Unlock();
    }

private:
    Core::CriticalSection _adminLock;
    const uint8_t _pin;
    mutable int _descriptor;
    std::list<IObserver*> _observers;
};

} } // namespace WPEFramework::GPIO

#endif // __LINUX_GPIO_H__
