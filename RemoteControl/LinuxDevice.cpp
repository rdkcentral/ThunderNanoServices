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

#include "RemoteAdministrator.h"

#include <interfaces/IKeyHandler.h>
#include <libudev.h>
#include <linux/uinput.h>

namespace WPEFramework {
namespace Plugin {

    static char Locator[] = _T("/dev/input");

    class LinuxDevice : Core::Thread {
    public:
        enum type {
            NONE,
            KEYBOARD,
            MOUSE,
            TOUCH,
            JOYSTICK
        };
    private:
        static constexpr const TCHAR* InputDeviceSysFilePath = _T("/sys/class/input/");
        static constexpr const TCHAR* DeviceNamePath = _T("/device/name");

    private:
        LinuxDevice(const LinuxDevice&) = delete;
        LinuxDevice& operator=(const LinuxDevice&) = delete;

        struct IDevInputDevice {
            virtual ~IDevInputDevice() { }
            virtual type Type() const { return (type::NONE); }
            virtual bool Setup() { return true; }
            virtual bool Teardown() { return true; }
            virtual bool HandleInput(uint16_t code, uint16_t type, int32_t value) = 0;
            virtual void ProducerEvent(const Exchange::ProducerEvents event) { }
        };

        class KeyDevice : public Exchange::IKeyProducer, public IDevInputDevice {
        public:
            KeyDevice(const KeyDevice&) = delete;
            KeyDevice& operator=(const KeyDevice&) = delete;

            KeyDevice(LinuxDevice* parent)
                : _parent(parent)
                , _callback(nullptr)
            {
                ASSERT(_parent != nullptr);
                Remotes::RemoteAdministrator::Instance().Announce(*this);
            }
            virtual ~KeyDevice()
            {
                Remotes::RemoteAdministrator::Instance().Revoke(*this);
            }
            string Name() const override
            {
                return (_T("DevInput"));
            }
            void Configure(const string&) override
            {
                Pair();
            }
            bool Pair() override
            {
                ProducerEvent(WPEFramework::Exchange::ProducerEvents::PairingStarted);
                bool result = _parent->Pair();
                if (result) {
                    ProducerEvent(
                            WPEFramework::Exchange::ProducerEvents::PairingSuccess);
                } else {
                    ProducerEvent(
                            WPEFramework::Exchange::ProducerEvents::PairingFailed);
                }
                return result;
            }
            bool Unpair(string bindingId) override
            {
                ProducerEvent(WPEFramework::Exchange::ProducerEvents::UnpairingStarted);
                bool result = _parent->Unpair(bindingId);
                if (result) {
                    ProducerEvent(
                            WPEFramework::Exchange::ProducerEvents::UnpairingSuccess);
                } else {
                    ProducerEvent(
                            WPEFramework::Exchange::ProducerEvents::UnpairingFailed);
                }
                return result;
            }
            uint32_t Callback(Exchange::IKeyHandler* callback) override
            {
                ASSERT((callback == nullptr) ^ (_callback == nullptr));
                _callback = callback;
                return (Core::ERROR_NONE);
            }
            uint32_t Error() const override
            {
                return (Core::ERROR_NONE);
            }
            string MetaData() const override
            {
                return (Name());
            }
            type Type() const override
            {
                return type::KEYBOARD;
            }
            bool HandleInput(uint16_t code, uint16_t type, int32_t value) override
            {
                if (type == EV_KEY) {
                    if ((code < BTN_MISC) || (code >= KEY_OK)) {
                        if (value != 2) {
                            _callback->KeyEvent((value != 0), code, Name());
                        }
                        return true;
                    }
                }
                return false;
            }
            void ProducerEvent(const Exchange::ProducerEvents event) override
            {
                if (_callback) {
                    _callback->ProducerEvent(Name(), event);
                }
            }

            BEGIN_INTERFACE_MAP(KeyDevice)
            INTERFACE_ENTRY(Exchange::IKeyProducer)
            END_INTERFACE_MAP

        private:
            LinuxDevice* _parent;
            Exchange::IKeyHandler* _callback;
        };

        class WheelDevice : public Exchange::IWheelProducer, public IDevInputDevice {
        public:
            WheelDevice(const WheelDevice&) = delete;
            WheelDevice& operator=(const WheelDevice&) = delete;

            WheelDevice(LinuxDevice* parent)
                : _parent(parent)
                , _callback(nullptr)
            {
                Remotes::RemoteAdministrator::Instance().Announce(*this);
            }
            virtual ~WheelDevice()
            {
                //Remotes::RemoteAdministrator::Instance().Revoke(*this);
            }
            string Name() const override
            {
                return (_T("DevWheelInput"));
            }
            void Configure(const string&) override
            {
                _parent->Pair();
            }
            uint32_t Callback(Exchange::IWheelHandler* callback) override
            {
                ASSERT((callback == nullptr) ^ (_callback == nullptr));
                _callback = callback;
                return (Core::ERROR_NONE);
            }
            uint32_t Error() const override
            {
                return (Core::ERROR_NONE);
            }
            string MetaData() const override
            {
                return (Name());
            }
            bool HandleInput(uint16_t code, uint16_t type, int32_t value) override
            {
                if (type == EV_REL) {
                    switch(code)
                    {
                    case REL_WHEEL:
                        _callback->AxisEvent(0, value);
                        return true;
                    case REL_HWHEEL:
                        _callback->AxisEvent(value, 0);
                        return true;
                    }
                }
                return false;
            }

            BEGIN_INTERFACE_MAP(WheelDevice)
            INTERFACE_ENTRY(Exchange::IWheelProducer)
            END_INTERFACE_MAP

        private:
            LinuxDevice* _parent;
            Exchange::IWheelHandler* _callback;
        };

        class PointerDevice : public Exchange::IPointerProducer, public IDevInputDevice {
        public:
            PointerDevice(const PointerDevice&) = delete;
            PointerDevice& operator=(const PointerDevice&) = delete;

            PointerDevice(LinuxDevice* parent)
                : _parent(parent)
                , _callback(nullptr)
            {
                Remotes::RemoteAdministrator::Instance().Announce(*this);
            }

            string Name() const override
            {
                return (_T("DevPointerInput"));
            }
            void Configure(const string&) override
            {
                _parent->Pair();
            }
            uint32_t Callback(Exchange::IPointerHandler* callback) override
            {
                ASSERT((callback == nullptr) ^ (_callback == nullptr));
                _callback = callback;
                return (Core::ERROR_NONE);
            }
            uint32_t Error() const override
            {
                return (Core::ERROR_NONE);
            }
            string MetaData() const override
            {
                return (Name());
            }
            bool HandleInput(uint16_t code, uint16_t type, int32_t value) override
            {
                if (type == EV_REL) {
                    switch(code)
                    {
                    case REL_X:
                        _callback->PointerMotionEvent(value, 0);
                        return true;
                    case REL_Y:
                        _callback->PointerMotionEvent(0, value);
                        return true;
                    }
                }
                else if (type == EV_KEY) {
                    if ((code >= BTN_MOUSE) && (code <= BTN_TASK)) {
                        _callback->PointerButtonEvent((value != 0), (code - BTN_MOUSE));
                        return true;
                    }
                }
                return false;
            }

            BEGIN_INTERFACE_MAP(PointerDevice)
            INTERFACE_ENTRY(Exchange::IPointerProducer)
            END_INTERFACE_MAP

        private:
            LinuxDevice* _parent;
            Exchange::IPointerHandler* _callback;
        };

        class TouchDevice : public Exchange::ITouchProducer, public IDevInputDevice {
        public:
            static const int ABS_MULTIPLIER_PRECISSION = 12;

            TouchDevice(const TouchDevice&) = delete;
            TouchDevice& operator=(const TouchDevice&) = delete;

            TouchDevice(LinuxDevice* parent)
                : _parent(parent)
                , _callback(nullptr)
                , _abs_latch()
                , _abs_slot(0)
                , _abs_x_multiplier(0)
                , _abs_y_multiplier(0)
                , _have_abs(false)
                , _have_multitouch(false)
            {
                _abs_latch.fill(AbsInfo());
                Remotes::RemoteAdministrator::Instance().Announce(*this);
            }
            string Name() const override
            {
                return (_T("DevTouchInput"));
            }
            void Configure(const string&) override
            {
                _parent->Pair();
            }
            bool Setup() override
            {
                _have_multitouch = false;

                for (auto& index : _parent->Devices()) {
                    uint8_t absbits[(ABS_MAX / 8) + 1];
                    memset(absbits, 0, sizeof(absbits));

                    if (ioctl(index.second.first, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) >= 0) {
                        if (CheckBit(absbits, ABS_X) == true) {
                            // Note: Only multitouch protocol B supported. In case of protocol A multitouch device, will run as single-touch.
                            _have_multitouch = (CheckBit(absbits, ABS_MT_SLOT) == true) && (CheckBit(absbits, ABS_MT_POSITION_X) == true);

                            struct input_absinfo absinfo;
                            if (_have_multitouch == true) {
                                if (ioctl(index.second.first, EVIOCGABS(ABS_MT_POSITION_X), &absinfo) >= 0) {
                                    _abs_x_multiplier = ((1 << 16) << ABS_MULTIPLIER_PRECISSION) / absinfo.maximum;
                                }
                                if (ioctl(index.second.first, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo) >= 0) {
                                    _abs_y_multiplier = ((1 << 16) << ABS_MULTIPLIER_PRECISSION) / absinfo.maximum;
                                }
                            } else {
                                if (ioctl(index.second.first, EVIOCGABS(ABS_X), &absinfo) >= 0) {
                                    _abs_x_multiplier = ((1 << 16) << ABS_MULTIPLIER_PRECISSION) / absinfo.maximum;
                                }
                                if (ioctl(index.second.first, EVIOCGABS(ABS_Y), &absinfo) >= 0) {
                                    _abs_y_multiplier = ((1 << 16) << ABS_MULTIPLIER_PRECISSION) / absinfo.maximum;
                                }
                            }

                            // Note: Only one connected touchscreen supported.
                            break;
                        }
                    }
                }

                return (true);
            }
            bool Teardown() override
            {
                _have_multitouch = false;
                _abs_x_multiplier = 0;
                _abs_y_multiplier = 0;
                return (true);
            }
            uint32_t Callback(Exchange::ITouchHandler* callback) override
            {
                ASSERT((callback == nullptr) ^ (_callback == nullptr));
                _callback = callback;
                return (Core::ERROR_NONE);
            }
            uint32_t Error() const override
            {
                return (Core::ERROR_NONE);
            }
            string MetaData() const override
            {
                return (Name());
            }
            bool HandleInput(uint16_t code, uint16_t type, int32_t value) override
            {
                if (type == EV_KEY) {
                    if (code == BTN_TOUCH) {
                       _abs_latch[_abs_slot].Touch(value >= 0);
                        _have_abs = true;
                        if (value == 0) {
                            _abs_slot = 0;
                            // This was the last contact released.
                        } else if (_abs_slot == 0) {
                            // The is the first contact, if any, move data from temporary slot to the first one.
                            // If only one contact is present, the device might not sent ABS_MT_SLOT information.
                            _abs_slot = 1;
                            _abs_latch[1] = _abs_latch[0];
                        }
                        return true;
                    }
                }
                else if (type == EV_ABS) {
                    switch(code)
                    {
                    case ABS_X:
                        if (_have_multitouch == false) { // fall-through
                    case ABS_MT_POSITION_X:
                        _abs_latch[_abs_slot].X(static_cast<uint16_t>((_abs_x_multiplier * value) >> ABS_MULTIPLIER_PRECISSION));
                        _have_abs = true;
                        }
                        return true;
                    case ABS_Y:
                        if (_have_multitouch == false) { // fall-through
                    case ABS_MT_POSITION_Y:
                        _abs_latch[_abs_slot].Y(static_cast<uint16_t>((_abs_y_multiplier * value) >> ABS_MULTIPLIER_PRECISSION));
                        _have_abs = true;
                        }
                        return true;
                    case ABS_MT_SLOT:
                        _abs_slot = (value + 1);
                        return true;
                    case ABS_MT_TRACKING_ID:
                        // Multi-touch touch/release events
                        _abs_latch[_abs_slot].Touch(value >= 0);
                        _have_abs = true;
                        if (value < 0) {
                            // Touch released, if any, next events will go to a temporary slot until the contact index is known
                            _abs_slot = 0;
                        }
                        return true;
                    }
                }
                else if (type == EV_SYN) {
                    if (_have_abs == true) {
                        _have_abs = false;
                        for (size_t i = 1; i < _abs_latch.size(); i++) {
                            if (_abs_latch[i].Action() != AbsInfo::absaction::IDLE) {
                                _callback->TouchEvent((i - 1), _abs_latch[i].State(), _abs_latch[i].X(), _abs_latch[i].Y());
                                _abs_latch[i].Reset();
                            }
                        }
                    }
                }
                return false;
            }

        private:
            bool CheckBit(const uint8_t bitfield[], const uint16_t bit)
            {
                return (bitfield[bit / 8] & (1 << (bit % 8)));
            }

            class AbsInfo {
            public:
                enum class absaction {
                    IDLE,
                    POSITION,
                    RELEASED,
                    PRESSED,
                };

                AbsInfo()
                {
                    Reset();
                    x = y = 0;
                }
                void Reset()
                {
                    action = absaction::IDLE;
                }
                void X(const uint16_t value)
                {
                    x = value;
                    if (action == absaction::IDLE) {
                        action = absaction::POSITION;
                    }
                }
                uint16_t X()
                {
                    return x;
                }
                void Y(const uint16_t value)
                {
                    y = value;
                    if (action == absaction::IDLE) {
                        action = absaction::POSITION;
                    }
                }
                uint16_t Y()
                {
                    return y;
                }
                void Touch(const bool touched)
                {
                    action = (touched? absaction::PRESSED : absaction::RELEASED);
                }
                absaction Action()
                {
                    return action;
                }
                Exchange::ITouchHandler::touchstate State()
                {
                    return ((Action() == absaction::RELEASED)? Exchange::ITouchHandler::touchstate::TOUCH_RELEASED
                            : ((Action() == absaction::PRESSED)? Exchange::ITouchHandler::touchstate::TOUCH_PRESSED : Exchange::ITouchHandler::touchstate::TOUCH_MOTION));
                }
            private:
                uint16_t x;
                uint16_t y;
                absaction action;
            };

            BEGIN_INTERFACE_MAP(TouchDevice)
            INTERFACE_ENTRY(Exchange::ITouchProducer)
            END_INTERFACE_MAP

        private:
            LinuxDevice* _parent;
            Exchange::ITouchHandler* _callback;
            std::array<AbsInfo,(1 + 10)> _abs_latch; // one extra slot!
            int16_t _abs_slot;
            uint32_t _abs_x_multiplier;
            uint32_t _abs_y_multiplier;
            bool _have_abs;
            bool _have_multitouch;
        };

    public:
        LinuxDevice()
            : Core::Thread(Core::Thread::DefaultStackSize(), _T("LinuxInputSystem"))
            , _devices()
            , _monitor(nullptr)
            , _update(-1)
        {
            _pipe[0] = -1;
            _pipe[1] = -1;
            if (::pipe(_pipe) < 0) {
                // Pipe not successfully opened. Close, if needed;
                if (_pipe[0] != -1) {
                    close(_pipe[0]);
                }
                if (_pipe[1] != -1) {
                    close(_pipe[1]);
                }
                _pipe[0] = -1;
                _pipe[1] = -1;
            } else {
                struct udev* udev = udev_new();

                // Set up a monitor to monitor event devices
                _monitor = udev_monitor_new_from_netlink(udev, "udev");
                udev_monitor_filter_add_match_subsystem_devtype(_monitor, "input", nullptr);
                udev_monitor_enable_receiving(_monitor);
                // Get the file descriptor (fd) for the monitor
                _update = udev_monitor_get_fd(_monitor);

                udev_unref(udev);

                _inputDevices.emplace_back(Core::Service<KeyDevice>::Create<KeyDevice>(this));
                _inputDevices.emplace_back(Core::Service<WheelDevice>::Create<WheelDevice>(this));
                _inputDevices.emplace_back(Core::Service<PointerDevice>::Create<PointerDevice>(this));
                _inputDevices.emplace_back(Core::Service<TouchDevice>::Create<TouchDevice>(this));

                Pair();
            }
        }
        virtual ~LinuxDevice()
        {
            Block();

            Clear();

            if (_pipe[0] != -1) {
                close(_pipe[0]);
                close(_pipe[1]);
            }

            if (_update != -1) {
                ::close(_update);
            }

            if (_monitor != nullptr) {
                udev_monitor_unref(_monitor);
            }

            for (auto& device : _inputDevices) {
                device->Teardown();
            }

            _inputDevices.clear();
        }

    public:
        bool Pair()
        {
            // Make sure we are not processing anything.
            Block();

            Refresh();

            // We are done, start observing again.
            Run();

            return (true);
        }
        bool Unpair(string bindingId)
        {
            // Make sure we are not processing anything.
            Block();

            Refresh();

            // We are done, start observing again.
            Run();

            return (true);
        }

    private:
        void Refresh()
        {
            // find devices in /dev/input/
            Core::Directory dir(Locator);
            while (dir.Next() == true) {

                Core::File entry(dir.Current(), false);
                if ((entry.IsDirectory() == false) && (entry.FileName().substr(0, 5) == _T("event"))) {

                    TRACE(Trace::Information, (_T("Opening input device: %s"), entry.Name().c_str()));

                    if (entry.Open(true) == true) {
                        int fd = entry.DuplicateHandle();
                        std::map<string, std::pair<int, IDevInputDevice*>>::iterator device(_devices.find(entry.Name()));
                        if (device == _devices.end()) {
                            string deviceName;
                            ReadDeviceName(entry.Name(), deviceName);
                            std::transform(deviceName.begin(), deviceName.end(), deviceName.begin(), std::ptr_fun<int, int>(std::toupper));

                            IDevInputDevice* inputDevice = nullptr;
                            for (auto& device : _inputDevices) {
                                std::size_t found = deviceName.find(Core::EnumerateType<LinuxDevice::type>(device->Type()).Data());
                                if (found != std::string::npos) {
                                    inputDevice = device;
                                    break;
                                }
                            }

                            _devices.insert(std::make_pair(entry.Name(), std::make_pair(fd, inputDevice)));
                        }
                    }
                }
            }

            for (auto& device : _inputDevices) {
                device->Teardown();
                device->Setup();
            }
        }
        void Clear()
        {
            for (std::map<string, std::pair<int, IDevInputDevice*>>::const_iterator it = _devices.begin(), end = _devices.end();
                 it != end; ++it) {
                close(it->second.first);
            }
            _devices.clear();
        }
        void Block()
        {
            Core::Thread::Block();
            write(_pipe[1], " ", 1);
            Wait(Core::Thread::INITIALIZED | Core::Thread::BLOCKED | Core::Thread::STOPPED, Core::infinite);
        }
        virtual uint32_t Worker()
        {
            while (IsRunning() == true) {
                fd_set readset;
                FD_ZERO(&readset);
                FD_SET(_pipe[0], &readset);
                FD_SET(_update, &readset);

                int result = std::max(_pipe[0], _update);

                // set up all the input devices
                for (std::map<string, std::pair<int, IDevInputDevice*>>::const_iterator index = _devices.begin(), end = _devices.end(); index != end; ++index) {
                    FD_SET(index->second.first, &readset);
                    result = std::max(result, index->second.first);
                }

                result = select(result + 1, &readset, 0, 0, nullptr);

                if (result > 0) {
                    if (FD_ISSET(_pipe[0], &readset)) {
                        char buff;
                        (void)read(_pipe[0], &buff, 1);
                    }
                    if (FD_ISSET(_update, &readset)) {
                        // Make the call to receive the device. select() ensured that this will not block.
                        udev_device* dev = udev_monitor_receive_device(_monitor);
                        if (dev) {
                            const char* nodeId = udev_device_get_devnode(dev);
                            bool reload = ((nodeId != nullptr) && (strncmp(Locator, nodeId, sizeof(Locator) - 1) == 0));
                            udev_device_unref(dev);
                            TRACE(Trace::Information, (_T("Changes from udev perspective. Reload (%s)"), reload ? _T("true") : _T("false")));
                            if (reload == true) {
                                Refresh();
                            }
                        }
                    }

                    // find the devices to read from
                    std::map<string, std::pair<int, IDevInputDevice*>>::iterator index = _devices.begin();

                    while (index != _devices.end()) {
                        if (FD_ISSET(index->second.first, &readset)) {

                            if (HandleInput(index->second.first) == false) {
                                // fd closed?
                                close(index->second.first);
                                index = _devices.erase(index);
                            } else {
                                ++index;
                            }
                        } else {
                            ++index;
                        }
                    }
                }
            }
            return (Core::infinite);
        }
        bool HandleInput(const int fd)
        {
            input_event entry[16];
            int index = 0;
            int result = ::read(fd, entry, sizeof(entry));

            if (result > 0) {
                while (result >= static_cast<int>(sizeof(input_event))) {
                    ASSERT(index < static_cast<int>((sizeof(entry) / sizeof(input_event))));
                    for (auto& device : _inputDevices) {
                        if (device->HandleInput(entry[index].code,  entry[index].type, entry[index].value) == true) {
                            break;
                        }
                    }
                    index++;
                    result -= sizeof(input_event);
                }
            }

            return (result >= 0);
        }
        bool ReadDeviceName(const string& eventLocation, string& deviceName)
        {
            bool status;
            string eventName(eventLocation, sizeof(Locator));
            if (eventName.empty() != true) {
                string deviceFileName = InputDeviceSysFilePath + eventName + DeviceNamePath;
                Core::File deviceFile(deviceFileName);
                if (deviceFile.Open(true) == true) {
                    char buffer[1024];
                    uint16_t readBytes;
                    string name;

                    // Read whole info
                    while ((readBytes = deviceFile.Read(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer))) != 0) {
                        name.append(buffer, readBytes);
                    }

                    if (name.empty() != true) {
                        deviceName = name;
                        status = true;
                   }
                }
            }
            return status;
        }

        std::map<string, std::pair<int, IDevInputDevice*>> Devices() { return _devices; }

    private:
        std::map<string, std::pair<int, IDevInputDevice*>> _devices;
        int _pipe[2];
        udev_monitor* _monitor;
        int _update;
        std::vector<IDevInputDevice*> _inputDevices;
        static LinuxDevice* _singleton;
    };

    /* static */ LinuxDevice* LinuxDevice::_singleton = new LinuxDevice();
}

ENUM_CONVERSION_BEGIN(Plugin::LinuxDevice::type)
    { Plugin::LinuxDevice::type::NONE, _TXT("NONE") },
    { Plugin::LinuxDevice::type::KEYBOARD, _TXT("KEYBOARD") },
    { Plugin::LinuxDevice::type::KEYBOARD, _TXT("MOUSE") },
    { Plugin::LinuxDevice::type::KEYBOARD, _TXT("JOYSTICK") },
    { Plugin::LinuxDevice::type::KEYBOARD, _TXT("TOUCH") },
    ENUM_CONVERSION_END(Plugin::LinuxDevice::type);
}
