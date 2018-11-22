#include "GPIO.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(GPIO::Pin::trigger_mode)

    { GPIO::Pin::RISING,  _TXT(_T("rising"))  },
    { GPIO::Pin::FALLING, _TXT(_T("falling")) },
    { GPIO::Pin::BOTH,    _TXT(_T("both"))    },
    { GPIO::Pin::HIGH,    _TXT(_T("high"))    },
    { GPIO::Pin::LOW,     _TXT(_T("low"))     },

ENUM_CONVERSION_END(GPIO::Pin::trigger_mode)

ENUM_CONVERSION_BEGIN(GPIO::Pin::pin_mode)

    { GPIO::Pin::INPUT,    _TXT(_T("in"))    },
    { GPIO::Pin::OUTPUT,   _TXT(_T("out"))   },
    { GPIO::Pin::PWM_TONE, _TXT(_T("tone"))  },
    { GPIO::Pin::PWM,      _TXT(_T("pwm"))   },
    { GPIO::Pin::CLOCK,    _TXT(_T("clock")) },

ENUM_CONVERSION_END(GPIO::Pin::pin_mode)

ENUM_CONVERSION_BEGIN(GPIO::Pin::pull_mode)

    { GPIO::Pin::OFF,    _TXT(_T("0")) },
    { GPIO::Pin::DOWN,   _TXT(_T("1")) },
    { GPIO::Pin::UP,     _TXT(_T("2")) },

ENUM_CONVERSION_END(GPIO::Pin::pull_mode)

namespace GPIO {

// ----------------------------------------------------------------------------------------------------
// Class: PIN
// ----------------------------------------------------------------------------------------------------

Pin::Pin(const uint8_t pin)
    : _pin(pin)
    , _descriptor(-1) {
}

/* virtual */ Pin::~Pin() {
      if (_descriptor != -1)
      {
        if (_observers.size() > 0) {
            // Please unregister all observers before dumping the pin.
            ASSERT(false);
        }

        Core::ResourceMonitor::Instance().Unregister(*this);

        close (_descriptor);
        _descriptor = -1;

        int fd = open ("/sys/class/gpio/unexport", O_WRONLY);
        if (fd > 0) {
          // Time to register the pin
          char buffer[16];
          int index = 0;
          int pin = _pin;
          do {
            index++;
            buffer[sizeof(buffer) - index] = '0' + (pin % 10);
            pin /= 10;
          } while ((pin > 0) && (index < static_cast<int>(sizeof(buffer) - 1)));

          write(fd, &(buffer[sizeof(buffer) - index]), index);
          close(fd);
        }
      }
}

void Pin::Register(IObserver* callback) {

    _adminLock.Lock();

    // Only register a callback once.
    ASSERT (std::find(_observers.begin(), _observers.end(), callback) == _observers.end());

    // It is the first subscriber, Add it..
    _observers.push_back(callback);

    if (_observers.size() == 1) {
        Core::ResourceMonitor::Instance().Register (*this);
    }

    _adminLock.Unlock();
}

void Pin::Unregister(IObserver* callback) {

    _adminLock.Lock();

    std::list<IObserver*>::iterator index (std::find(_observers.begin(), _observers.end(), callback));

    // Do not unregister a callback that you did not register!!!
    ASSERT(index != _observers.end());

    if (index != _observers.end()) {

        // Now it is time to reevaluate, as the map is loosing an entry.
        _observers.erase(index);

        if (_observers.size() == 0) {
            Core::ResourceMonitor::Instance().Unregister (*this);
        }
    }

    _adminLock.Unlock();
}


void Pin::Flush() {

    if (_descriptor != -1) {

        int count;

        ioctl(_descriptor, FIONREAD, &count);

        while (count-- != 0) {

            char buffer;
            read(_descriptor, &buffer, 1);
        }

        lseek(_descriptor, 0, SEEK_SET);
    }
}

/* virtual */ Core::IResource::handle Pin::Descriptor() const {

      if ((_pin != 0xFF) && (_descriptor == -1))
      {
        struct stat properties;
        char buffer[64];
        sprintf(buffer, "/sys/class/gpio/gpio%d/value", _pin);

        // See if this pin already exists
        if (stat(buffer, &properties) < 0) {
          int fd = open ("/sys/class/gpio/export", O_WRONLY);
          if (fd > 0) {
            // Time to register the pin
            char id[4];
            int index = 0;
            int pin = _pin;
            do {
              index++;
              id[sizeof(id) - index] = '0' + (pin % 10);
              pin /= 10;
            } while ((pin > 0) && (index < static_cast<int>(sizeof(id) - 1)));

            write(fd, &(id[sizeof(id) - index]), index);
            close(fd);
          }
	}

        _descriptor = open(buffer, O_RDWR);
      }

      return (_descriptor);
}

/* virtual */ uint16_t Pin::Events() {
    return (_descriptor != -1 ? (POLLPRI|POLLERR) : 0);
}

/* virtual */ void Pin::Handle(const uint16_t events) {

    if ((events & (POLLPRI|POLLERR)) != 0) {

        unsigned char buffer[1];

        read(_descriptor, &buffer, sizeof(buffer));

        Notify();
    }
}

void Pin::Trigger (const trigger_mode mode) {
    if (Descriptor() != -1) {
	// Oke looks like we have a valid pin.
        char buffer[64];
        sprintf(buffer, "/sys/class/gpio/gpio%d/edge", _pin);

	Core::EnumerateType<trigger_mode> textMode(mode);

	if ( (textMode.IsSet() == true) && (textMode.Data() != nullptr) && (textMode.Data()[0] != '\0') ) {
            int fd = open(buffer, O_WRONLY);

	    if (fd > 0) {

                write(fd, textMode.Data(), strlen(textMode.Data()));
	        close(fd);
            }
        }
    }
}

bool Pin::Get() const {
    bool result = false;

    if (Descriptor() != -1) {
        uint8_t value;
        lseek(_descriptor, 0, SEEK_SET);
        read(_descriptor, &value, 1);
        result = (value != '0');
    }
    return (result);
}

void Pin::Set(const bool value) {
    if (Descriptor() != -1) {
        uint8_t newValue = (value ? '1' : '0');
        write (_descriptor, &newValue, 1);
    }
}

void Pin::Mode(const pin_mode mode) {
     if (Descriptor() != -1) {
	// Oke looks like we have a valid pin.
        char buffer[64];
        sprintf(buffer, "/sys/class/gpio/gpio%d/direction", _pin);

        if ( (mode == GPIO::Pin::INPUT) || (mode == GPIO::Pin::OUTPUT) ) {
            Core::EnumerateType<pin_mode> textMode(mode);

	    if ( (textMode.IsSet() == true) && (textMode.Data() != nullptr) && (textMode.Data()[0] != '\0') ) {
                int fd = open(buffer, O_WRONLY);

	        if (fd > 0) {

                    write(fd, textMode.Data(), strlen(textMode.Data()));
	            close(fd);
                }
            }
        }
    }
}

void Pin::Pull (const pull_mode mode) {
    if (Descriptor() != -1) {
	// Oke looks like we have a valid pin.
        char buffer[64];
        sprintf(buffer, "/sys/class/gpio/gpio%d/active_low", _pin);

        if ( (mode == GPIO::Pin::OFF) || (mode == GPIO::Pin::DOWN) || (mode == GPIO::Pin::DOWN)) {

            Core::EnumerateType<pull_mode> textMode(mode);

	    if ( (textMode.IsSet() == true) && (textMode.Data() != nullptr) && (textMode.Data()[0] != '\0') ) {
                int fd = open(buffer, O_WRONLY);

	        if (fd > 0) {

                    write(fd, textMode.Data(), strlen(textMode.Data()));
	            close(fd);
                }
            }
        }
    }
}


} } // namespace WPEFramework::Linux
