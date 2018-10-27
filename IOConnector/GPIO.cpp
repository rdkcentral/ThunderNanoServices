#include "GPIO.h"

namespace WPEFramework {

namespace GPIO {

// ----------------------------------------------------------------------------------------------------
// Class: PIN
// ----------------------------------------------------------------------------------------------------

/* static */ Pin::Monitor Pin::_monitor;

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
            _monitor.Unregister(*this);
        }

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
        _monitor.Register (*this);
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
            _monitor.Unregister (*this);
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

int Pin::Descriptor() const {

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
