#include "WebShell.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(WebShell, 1, 0);

#define SLOT_ALLOCATION 8

    class SessionMonitor : public Core::Thread {
    private:
        class Session {
        private:
            Session& operator= (const Session&);

        public:
            Session()
                : _channel()
                , _process() 
                , _buffer()
                , _leftSize(0) {
            }
            Session(PluginHost::Channel& channel, Core::ProxyType< Core::Process> process)
                : _channel(&channel)
                , _process(process)
                , _buffer()
                , _leftSize(0) {
            }
            Session(const Session& copy)
                : _channel(copy._channel)
                , _process(copy._process)
                , _buffer()
                , _leftSize(0) {
            }
            ~Session () {
            }

        public:
            inline void Release() {
                _channel = nullptr;
                _process.Release();
            }
            inline PluginHost::Channel& Channel() {

                ASSERT (_channel != nullptr);
                return (*_channel);
            }
            inline const PluginHost::Channel& Channel() const {

                ASSERT (_channel != nullptr);
                return (*_channel);
            }
            inline Core::ProxyType< Core::Process> Process() {

                return (_process);
            }
            inline const Core::ProxyType<Core::Process> Process() const {

                return (_process);
            }
            inline bool operator== (const PluginHost::Channel& rhs) const {
                return ( (_channel != nullptr) && (rhs.Id() == _channel->Id()) );
            }
            inline bool operator!= (const PluginHost::Channel& rhs) const {
                return (!operator== (rhs));
            }
            inline bool operator== (const Core::ProxyType<Core::Process>& rhs) const {
                return ( rhs == _process );
            }
            inline bool operator!= (const Core::ProxyType< Core::Process>& rhs) const {
                return (!operator== (rhs));
            }
            uint16_t Backup(const uint8_t data[], const uint16_t size) {
                uint16_t copySize = (size <= (sizeof(_buffer) - _leftSize) ? size : (sizeof(_buffer) - _leftSize));

                ::memcpy (&(_buffer[_leftSize]), data, copySize);
                _leftSize += copySize;
                return (copySize);
            }
            void Write() {
                uint32_t writtenBytes = ::write (_process->Input(), _buffer, _leftSize);
                if (writtenBytes < _leftSize) {
                    ::memcpy(_buffer, &(_buffer[writtenBytes]), (_leftSize - writtenBytes));
                }
                _leftSize -= writtenBytes;
            }
            bool WriteRequired() const {
                return (_leftSize != 0);
            }

        private:
            PluginHost::Channel* _channel;
            Core::ProxyType< Core::Process> _process;
            uint8_t  _buffer[1024];
            uint32_t _leftSize;
        };

        typedef std::list<Session> Sessions;

        SessionMonitor(const SessionMonitor&) = delete;
        SessionMonitor& operator=(const SessionMonitor&) = delete;

        static constexpr uint32_t MonitorStackSize = 64 * 1024;

    public:
        SessionMonitor()
            : Core::Thread(MonitorStackSize, _T("SessionHandler"))
            , _adminLock()
            , _sessions()
            , _signalFD(-1)
            , _maxSlots(SLOT_ALLOCATION)
            , _slots(static_cast<struct pollfd*>(::malloc((sizeof(struct pollfd) * (_maxSlots + 1)))))
        {
        }
        ~SessionMonitor()
        {
            Stop ();
            
            Wait(Thread::STOPPED, Core::infinite);
        }

    public:
        inline uint32_t Size() const {
            return (_sessions.size());
        }
        bool Open(PluginHost::Channel& channel, Core::Process::Options& options)
        {
            uint32_t pid = 0;

            Core::ProxyType<Core::Process> process(Core::ProxyType<Core::Process>::Create(true));

            if ((process.IsValid() == true) && (process->Launch(options, &pid) == Core::ERROR_NONE)) {

                ASSERT (process->HasConnector() == true);

                _adminLock.Lock();

                _sessions.push_back(Session(channel, process));

                if (_sessions.size() == 1) {
                    Run();
                }
                else {
                    Signal(SIGUSR1);
                }

                _adminLock.Unlock();
            }

            return (pid != 0);
        }
        void Close(PluginHost::Channel& channel)
        {
            _adminLock.Lock();

            Sessions::iterator index(std::find(_sessions.begin(), _sessions.end(), channel));

            ASSERT (index != _sessions.end());

            if (index != _sessions.end()) {

                index->Release();

                Signal(SIGUSR1);
            }

            _adminLock.Unlock();
        }

        uint32_t Read (const uint32_t channelId, uint8_t data[], const uint16_t length) const
        {
            uint32_t result = 0;
            Sessions::const_iterator index (_sessions.begin());

            while ((index != _sessions.end()) && (index->Channel().Id() != channelId)) index++;

            if (index != _sessions.end()) {

                // Seems we found a process that could feed this stream.
		int load = ::read(index->Process()->Output(), data, length);

		if (load > 0) {
		    result = load;
		}

		// Is there still some space left to load the the error stream ?
		if (result < length) {
                	load = ::read(index->Process()->Error(), &(data[result]), (length - result));

			if (load > 0) {
				result += load;
			}
		}
		if (result < length) {
		    data[result] = '\0'; 
		}
            }

            return (result);
        }
        uint32_t Write (const uint32_t channelId, const uint8_t data[], const uint16_t length)
        {
            uint32_t result = 0;
            Sessions::iterator index (_sessions.begin());

            while ((index != _sessions.end()) && (index->Channel().Id() != channelId)) index++;

            if (index != _sessions.end()) {
                // Seems we found a process that should receive the data..
                result = ::write(index->Process()->Input(), data, length);

                if (result < length) {
                    // We need to backup the data, we could not push
                    result += index->Backup(&data[result], (length - result));

                    // Make sure we will read the input again..
                    Signal(SIGUSR1);
                }
            }
            return (result);
        }

    private:
        virtual bool Initialize()
        {
            int err;
            sigset_t sigset;

            /* Create a sigset of all the signals that we're interested in */
            err = sigemptyset(&sigset);
            ASSERT(err == 0);
            err = sigaddset(&sigset, SIGUSR1);
            ASSERT(err == 0);

            /* We must block the signals in order for signalfd to receive them */
            err = pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

            assert(err == 0);

            /* Create the signalfd */
            _signalFD = signalfd(-1, &sigset, 0);
            ASSERT(_signalFD != -1);

            return (err == 0);
        }

        virtual uint32_t Worker()
        {

            uint32_t delay = 0;

            // Add entries not in the Array before we start !!!
            _adminLock.Lock();

            // Do we have enough space to allocate all file descriptors ?
            if (((_sessions.size() * 3) + 1) > _maxSlots) {
                _maxSlots = (((((_sessions.size() * 3) + 1) / SLOT_ALLOCATION) + 1) * SLOT_ALLOCATION);

                ::free(_slots);

                // Resize the array to fit..
                _slots = static_cast<struct pollfd*>(::malloc(sizeof(struct pollfd) * _maxSlots));

                _slots[0].fd = _signalFD;
                _slots[0].events = POLLIN;
                _slots[0].revents = 0;
            }

            int filledFileDescriptors = 1;
            Sessions::iterator index(_sessions.begin());

            // Fill in all entries required/updated..
            while (index != _sessions.end()) {
                Core::ProxyType< Core::Process> port (index->Process());

                if (port.IsValid() == false) {
                    index = _sessions.erase(index);

                    if (_sessions.size() == 0) {
                        Block();
                        delay = Core::infinite;
                    }
                }
                else {
                    _slots[filledFileDescriptors].fd = port->Input();
                    _slots[filledFileDescriptors].events = (index->WriteRequired() ? POLLOUT : 0);
                    _slots[filledFileDescriptors].revents = 0;
                    filledFileDescriptors++;
                    _slots[filledFileDescriptors].fd = port->Output();
                    _slots[filledFileDescriptors].events = POLLIN;
                    _slots[filledFileDescriptors].revents = 0;
                    filledFileDescriptors++;
                    _slots[filledFileDescriptors].fd = port->Error();
                    _slots[filledFileDescriptors].events = POLLIN;
                    _slots[filledFileDescriptors].revents = 0;
                    filledFileDescriptors++;
                    index++;
                }
            }

            if (filledFileDescriptors > 1) {
                _adminLock.Unlock();

                int result = poll(_slots, filledFileDescriptors, -1);

                _adminLock.Lock();

                if (result == -1) {
                    TRACE_L1("poll failed with error <%d>", errno);
                }
                else if (_slots[0].revents & POLLIN) {
                    /* We have a valid signal, read the info from the fd */
                    struct signalfd_siginfo info;

                    uint32_t VARIABLE_IS_NOT_USED bytes = read(_signalFD, &info, sizeof(info));

                    ASSERT(bytes == sizeof(info));

                    // Clear the signal port..
                    _slots[0].revents = 0;
                }
            }
            else {
                Block();
                delay = Core::infinite;
            }

            // We are only interested in the filedescriptors that have a corresponding client.
            // We also know that once a file descriptor is not found, we handled them all...
            int fd_index = 1;
            index = _sessions.begin();

            while ((index != _sessions.end()) && (fd_index < filledFileDescriptors)) {
                Core::ProxyType< Core::Process > port (index->Process());

                if (port.IsValid() == false) {
                    index = _sessions.erase(index);

                    if (_sessions.size() == 0) {
                        Block();
                        delay = Core::infinite;
                    }
                }
                else {
                    int fd = port->Input();

                    // Find the current file descriptor in our array..
                    while ((fd_index < filledFileDescriptors) && (_slots[fd_index].fd != fd)) {
                        fd_index += 3;
                    }

                    if (fd_index < filledFileDescriptors) {
                        if ((_slots[fd_index].revents & POLLOUT) != 0) {
                            Input(*index);
                        }
                        if ((_slots[fd_index+1].revents & POLLIN) != 0) {
                            Output(*index);
                        }
                        if ((_slots[fd_index+2].revents & POLLIN) != 0) {
                            Error(*index);
                        }
                    }
                }

                index++;
                fd_index += 3;
            }

            _adminLock.Unlock();

            return (delay);
        }

        void Input (Session& session)
        {
            // See if there was somthing left..
            session.Write();
        }
        void Output (Session& session)
        {
           session.Channel().RequestOutbound(); 
        }
        void Error (Session& session)
        {
           session.Channel().RequestOutbound(); 
        }

    private:
        Core::CriticalSection _adminLock;
        Sessions _sessions;
        int _signalFD;
        uint32_t _maxSlots;
        struct pollfd* _slots;
    };

    /* virtual */ const string WebShell::Initialize(PluginHost::IShell* service)
    {
        ASSERT (_sessionMonitor == nullptr);

        _config.FromString(service->ConfigLine());

        service->EnableWebServer(_T("UI"), EMPTY_STRING);

        _sessionMonitor = new SessionMonitor();

        ASSERT (_sessionMonitor != nullptr);

        // On succes return nullptr, to indicate there is no error text.
        return (_T(""));
    }

    /* virtual */ void WebShell::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT (_sessionMonitor != nullptr);

        service->DisableWebServer();

        delete _sessionMonitor;

        _sessionMonitor = nullptr;
    }

    // Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
    // Whenever the channel is closed, it is reported via the detach method.
    /* virtual */ bool WebShell::Attach(PluginHost::Channel& channel)
    {
        bool added = false;

        // See if we are still allowed to create a new connection..
        if (_sessionMonitor->Size() < _config.Connections.Value()) {

            Core::Process::Options options(_T("sh"));
            // options.Set(_T("/temp"), EMPTY_STRING);

            added = _sessionMonitor->Open(channel, options);

	    TRACE(Connectivity, (_T("Attaching sesssion ID: %d. Open status %s"), channel.Id(), (added ? _T("true") : _T("false"))));
        }
	else {
	    TRACE(Connectivity, (_T("Attaching of session not allowed. Connect ID: %d rejected"), channel.Id()));
	}

        return (added);
    }

    /* virtual */ void WebShell::Detach(PluginHost::Channel& channel)
    {
        // See if we can forward this info..
        _sessionMonitor->Close(channel);
	TRACE(Connectivity, (_T("Removing sesssion ID: %d"), channel.Id()));
    }

    /* virtual */ string WebShell::Information() const
    {
        // No additional info to report.
        return (string());
    }

    // IChannel methods
    // -------------------------------------------------------------------------------------------------------
    // Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of receiving
    // raw data for the plugin. Raw data received on this link will be exposed to the plugin via this interface.
    /* virtual */ uint32_t WebShell::Inbound(const uint32_t ID, const uint8_t data[], const uint16_t length)
    {
        uint32_t result = length;

	TRACE(Interactivity, (_T("IN: \"%s\""), string(reinterpret_cast<const char*>(&data[0]), length).c_str()));

        result = _sessionMonitor->Write (ID, data, length);

        return (result);
    }

    // Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of sending
    // raw data to the initiator of the websocket.
    /* virtual */ uint32_t WebShell::Outbound(const uint32_t ID, uint8_t data[], const uint16_t length) const
    {
        uint32_t result = _sessionMonitor->Read (ID, data, length);

	if (result > 0) {
		TRACE(Interactivity, (_T("OUT: \"%s\""), string(reinterpret_cast<char*>(&data[0]), result).c_str()));
	}

        return (result);
    }
}
}
