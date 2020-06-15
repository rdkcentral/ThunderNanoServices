/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "GPIO.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(GPIO::Pin::trigger_mode)

    { GPIO::Pin::RISING, _TXT(_T("rising")) },
    { GPIO::Pin::FALLING, _TXT(_T("falling")) },
    { GPIO::Pin::BOTH, _TXT(_T("both")) },
    { GPIO::Pin::HIGH, _TXT(_T("high")) },
    { GPIO::Pin::LOW, _TXT(_T("low")) },

ENUM_CONVERSION_END(GPIO::Pin::trigger_mode)

ENUM_CONVERSION_BEGIN(GPIO::Pin::pin_mode)

    { GPIO::Pin::INPUT, _TXT(_T("in")) },
    { GPIO::Pin::OUTPUT, _TXT(_T("out")) },
    { GPIO::Pin::PWM_TONE, _TXT(_T("tone")) },
    { GPIO::Pin::PWM, _TXT(_T("pwm")) },
    { GPIO::Pin::CLOCK, _TXT(_T("clock")) },

ENUM_CONVERSION_END(GPIO::Pin::pin_mode)

ENUM_CONVERSION_BEGIN(GPIO::Pin::pull_mode)

    { GPIO::Pin::OFF, _TXT(_T("0")) },
    { GPIO::Pin::DOWN, _TXT(_T("1")) },
    { GPIO::Pin::UP, _TXT(_T("2")) },

ENUM_CONVERSION_END(GPIO::Pin::pull_mode)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

namespace GPIO
{

    // ----------------------------------------------------------------------------------------------------
    // Class: PIN
    // ----------------------------------------------------------------------------------------------------

    Pin::Pin(const uint16_t pin, const bool activeLow)
        : BaseClass(pin, IExternal::regulator, IExternal::general, IExternal::logic, 0)
        , _pin(pin)
        , _activeLow(activeLow ? 1 : 0)
        , _lastValue(false)
        , _descriptor(-1)
        , _timedPin(this)
    {
        if (_pin != 0xFFFF) {
            struct stat properties;
            char buffer[64];
            sprintf(buffer, "/sys/class/gpio/gpio%d/value", _pin);
            // See if this pin already exists
            if (stat(buffer, &properties) < 0) {
                int fd = open("/sys/class/gpio/export", O_WRONLY);
                if (fd > 0) {
                    // Time to register the pin
                    char id[8] = {};
                    int index = 0;
                    int pin = _pin;
                    do {
                        index++;
                        id[sizeof(id) - index] = '0' + (pin % 10);
                        pin /= 10;
                    } while ((pin > 0) && (index < static_cast<int>(sizeof(id) - 1)));
                    (void)write(fd, &(id[sizeof(id) - index]), index);
                    close(fd);
                }
            }

            _descriptor = open(buffer, O_RDWR);
        }
        // The derived class shoud set, the initial value of the modifiers...
        _timedPin.AddRef();
        _timedPin.AddReference();
    }

    /* virtual */ Pin::~Pin()
    {
        if (_descriptor != -1) {

            Core::ResourceMonitor::Instance().Unregister(*this);

            close(_descriptor);
            _descriptor = -1;

            int fd = open("/sys/class/gpio/unexport", O_WRONLY);
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

                (void)write(fd, &(buffer[sizeof(buffer) - index]), index);
                close(fd);
            }
        }

        Period(0);

        _timedPin.DropReference();
        _timedPin.CompositRelease();

    }

    void Pin::Flush()
    {

        if (_descriptor != -1) {

            int count;

            ioctl(_descriptor, FIONREAD, &count);

            while (count-- != 0) {

                char buffer;
                (void)read(_descriptor, &buffer, 1);
            }

            lseek(_descriptor, 0, SEEK_SET);
        }
    }

    /* virtual */ Core::IResource::handle Pin::Descriptor() const
    {
        return (_descriptor);
    }

    /* virtual */ uint16_t Pin::Events()
    {
        return (_descriptor != -1 ? (POLLPRI | POLLERR) : 0);
    }

    /* virtual */ void Pin::Handle(const uint16_t events)
    {

        if ((events & (POLLPRI | POLLERR)) != 0) {

            unsigned char buffer[1];

            read(_descriptor, &buffer, sizeof(buffer));

            // If we are only triggered on a falling edge, or a rising edge
            // the change is not detected compared to the previous value,
            // force HasChanged to be true!!
            _lastValue = !Get();

            Updated();
        }
    }

    void Pin::Trigger(const trigger_mode mode)
    {
        if (_descriptor != -1) {
            // Oke looks like we have a valid pin.
            char buffer[64];
            sprintf(buffer, "/sys/class/gpio/gpio%d/edge", _pin);

            Core::EnumerateType<trigger_mode> textMode(mode);

            if ((textMode.IsSet() == true) && (textMode.Data() != nullptr) && (textMode.Data()[0] != '\0')) {
                int fd = open(buffer, O_WRONLY);

                if (fd > 0) {

                    write(fd, textMode.Data(), strlen(textMode.Data()));
                    close(fd);
                }
            }
        }
    }

    bool Pin::HasChanged() const
    {
        return (Get() != _lastValue);
    }

    void Pin::Align()
    {
        _lastValue = Get();
    }

    bool Pin::Get() const
    {
        bool result = false;

        if (_descriptor != -1) {
            uint8_t value;
            lseek(_descriptor, 0, SEEK_SET);
            read(_descriptor, &value, 1);
            result = (value != '0');
            if (_activeLow != 0) {
                result = !result;
            }
        }
        return (result);
    }

    void Pin::Set(const bool value)
    {
        if (_descriptor != -1) {
            uint8_t newValue;
            if (_activeLow != 0) {
                newValue = (value ? '0' : '1');
            } else {
                newValue = (value ? '1' : '0');
            }
            write(_descriptor, &newValue, 1);
        }
    }

    void Pin::Mode(const pin_mode mode)
    {
        if (_descriptor != -1) {
            // Oke looks like we have a valid pin.
            char buffer[64];
            sprintf(buffer, "/sys/class/gpio/gpio%d/direction", _pin);

            if ((mode == GPIO::Pin::INPUT) || (mode == GPIO::Pin::OUTPUT)) {
                Core::EnumerateType<pin_mode> textMode(mode);

                if ((textMode.IsSet() == true) && (textMode.Data() != nullptr) && (textMode.Data()[0] != '\0')) {
                    int fd = open(buffer, O_WRONLY);

                    if (fd > 0) {

                        (void)write(fd, textMode.Data(), strlen(textMode.Data()));
                        close(fd);
                    }
                }
            }
        }
    }

    void Pin::Pull(const pull_mode mode)
    {
        if (_descriptor != -1) {
            // Oke looks like we have a valid pin.
            char buffer[64];
            sprintf(buffer, "/sys/class/gpio/gpio%d/active_low", _pin);

            if ((mode == GPIO::Pin::OFF) || (mode == GPIO::Pin::DOWN) || (mode == GPIO::Pin::DOWN)) {

                Core::EnumerateType<pull_mode> textMode(mode);

                if ((textMode.IsSet() == true) && (textMode.Data() != nullptr) && (textMode.Data()[0] != '\0')) {
                    int fd = open(buffer, O_WRONLY);

                    if (fd > 0) {

                        (void)write(fd, textMode.Data(), strlen(textMode.Data()));
                        close(fd);
                    }
                }
            }
        }
    }

    // IInput pin functionality. Get triggered by an IOPin if a marker has been reached
    // ---------------------------------------------------------------------------------
    void Pin::Register(IInputPin::INotification* sink) /* override */ {
        _timedPin.Register(sink);
    }

    void Pin::Unregister(IInputPin::INotification* sink) /* override */ {
        _timedPin.Register(sink);
    }

    void Pin::AddMarker(const uint32_t marker) /* override */ {
        _timedPin.Add(marker);
    }

    void Pin::RemoveMarker(const uint32_t marker) /* override */ {
        _timedPin.Remove(marker);
    }

    /* virtual */ void Pin::Evaluate()
    {
        if (HasChanged() == true) {
            _timedPin.Update(Get());
            BaseClass::Updated();
        }
    }

    /* virtual */ uint32_t Pin::Get(int32_t & value) const
    {
        value = (Get() ? 1 : 0);
        return (Core::ERROR_NONE);
    }

    /* virtual */ uint32_t Pin::Set(const int32_t value)
    {
        Set(value != 0);
        return (Core::ERROR_NONE);
    }
}
} // namespace WPEFramework::Linux

#pragma GCC diagnostic pop
