#include "RemoteAdministrator.h"

#include <libudev.h>
#include <linux/uinput.h>
#include <interfaces/IKeyHandler.h>

namespace WPEFramework {
namespace Plugin {

    static char Locator[] = _T("/dev/input");

    class LinuxDevice : public Exchange::IKeyProducer, Core::Thread {
    private:
        LinuxDevice(const LinuxDevice&) = delete;
        LinuxDevice& operator=(const LinuxDevice&) = delete;

    public:
        LinuxDevice()
            : Core::Thread(Core::Thread::DefaultStackSize(), _T("LinuxInputSystem"))
            , _devices()
            , _monitor(nullptr)
            , _update(-1) {
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
            }
            else {
                struct udev* udev = udev_new();

                // Set up a monitor to monitor event devices
                _monitor = udev_monitor_new_from_netlink(udev, "udev");
                udev_monitor_filter_add_match_subsystem_devtype(_monitor, "input", nullptr);
                udev_monitor_enable_receiving(_monitor);
                // Get the file descriptor (fd) for the monitor
                _update = udev_monitor_get_fd(_monitor);

                udev_unref(udev);
                Remotes::RemoteAdministrator::Instance().Announce(*this);
            }
        }
        virtual ~LinuxDevice() {

            Block();

            Clear();

            if (_pipe[0] != -1) {
                Remotes::RemoteAdministrator::Instance().Revoke(*this);

                close(_pipe[0]);
                close(_pipe[1]);
            }

            if (_update != -1) {
                ::close(_update);
            }

            if (_monitor != nullptr) {
                udev_monitor_unref(_monitor);
            }

        }

    public:
        virtual const TCHAR* Name() const
        {
            return (_T("LinuxDevice"));
        }
            virtual void Configure(const string&)
            {
            Pair();
                }
        virtual bool Pair()
        {
            // Make sure we are not processing anything.
            Block();

            Refresh();

            // We are done, start observing again.
            Run();

            return(true);
        }
        virtual bool Unpair(string bindingId)
        {
            // Make sure we are not processing anything.
            Block();

            Refresh();

            // We are done, start observing again.
            Run();

            return(true);
        }
        virtual uint32_t Callback(Exchange::IKeyHandler* callback) {
            ASSERT((callback == nullptr) ^ (_callback == nullptr));

            if (callback == nullptr) {
                // We are unlinked. Deinitialize the stuff.
                _callback = nullptr;
            }
            else {

                TRACE_L1("%s: callback=%p _callback=%p", __FUNCTION__, callback, _callback);
                _callback = callback;
            }

            return (Core::ERROR_NONE);
        }
        virtual uint32_t Error() const {
            return (Core::ERROR_NONE);
        }
        virtual string MetaData() const {
            return (Name());
        }

        BEGIN_INTERFACE_MAP(LinuxDevice)
            INTERFACE_ENTRY(Exchange::IKeyProducer)
        END_INTERFACE_MAP

    private:
        void Refresh() {
            // Remove all current open devices.
            Clear();

            // find devices in /dev/input/
            Core::Directory dir (Locator);
            while (dir.Next() == true) {

                Core::File entry (dir.Current(), false);
                if ( (entry.IsDirectory() == false) && (entry.FileName().substr(0,5) == _T("event")) ) {

                    TRACE(Trace::Information, (_T("Opening input device: %s"), entry.Name().c_str()));

                    if (entry.Open(true) == true) {
                        _devices.push_back(entry.DuplicateHandle());
                    }
                }
            }
        }
        void Clear() {
            for (std::vector<int>::const_iterator it = _devices.begin(), end = _devices.end();
                it != end; ++it) {
                close(*it);
            }
            _devices.clear();
        }
        void Block()
        {
            Core::Thread::Block();
            write(_pipe[1], " ", 1);
            Wait (Core::Thread::INITIALIZED|Core::Thread::BLOCKED|Core::Thread::STOPPED, Core::infinite);
        }
        virtual uint32_t Worker () {

            while (IsRunning() == true) {
                fd_set readset;
                FD_ZERO(&readset);
                FD_SET(_pipe[0], &readset);
                FD_SET(_update, &readset);

                int result = std::max(_pipe[0], _update);

                // set up all the input devices
                for (std::vector<int>::const_iterator index = _devices.begin(), end = _devices.end(); index != end; ++index) {
                    FD_SET(*index, &readset);
                    result = std::max(result, *index);
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
                            bool reload = ((nodeId != nullptr) && (strncmp(Locator, nodeId, sizeof(Locator)-1) == 0));
                            udev_device_unref(dev);

                            TRACE_L1("Changes from udev perspective. Reload (%s)", reload ? _T("true") : _T("false"));
                            if (reload == true) {
                                Refresh();
                            }
                        }
                    }


                        // find the devices to read from
                    std::vector<int>::iterator index = _devices.begin();

                    while (index != _devices.end()) {
                        if (FD_ISSET(*index, &readset)) {

                            if (HandleInput(*index) == false) {
                                // fd closed?
                                close(*index);
                                index = _devices.erase(index);
                            }
                            else {
                                ++index;
                            }
                        }
                        else {
                            ++index;
                        }
                    }


                }
            }
            return (Core::infinite);
        }
        bool HandleInput(const int fd) {
            input_event entry[16];
            int index = 0;
            int result = ::read(fd, entry, sizeof(entry));

            if (result > 0) {
                while (result >= sizeof(input_event)) {

                    ASSERT (index < (sizeof(entry) / sizeof(input_event)));

                    // If it is a KEY and it is *NOT* a repeat, send it..
                    // Repeat gets constructed by the framework anyway.
                    if ( (entry[index].type == EV_KEY) && (entry[index].value != 2) ) {

                        const uint16_t code = entry[index].code;
                        const bool pressed  = entry[index].value != 0;
                        TRACE(Trace::Information, (_T("Sending pressed: %s, code: 0x%04X"), (pressed ? _T("true") : _T("false")), code));
                        _callback->KeyEvent(pressed, code, Name());
                    }
                    index++;
                    result -= sizeof(input_event);
                }
            }

            return (result >= 0);
        }
    private:
        std::vector<int> _devices;
        int _pipe[2];
                udev_monitor* _monitor;
                int _update;
        Exchange::IKeyHandler* _callback;
    };

    static LinuxDevice* _singleton(Core::Service<LinuxDevice>::Create<LinuxDevice>());

}
}
