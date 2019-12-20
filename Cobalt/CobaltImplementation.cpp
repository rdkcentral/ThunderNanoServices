#include "Module.h"
#include <interfaces/IMemory.h>
#include <interfaces/IBrowser.h>

#include "starboard/export.h"
extern int StarboardMain(int argc, char **argv);

namespace WPEFramework {
namespace Plugin {

class CobaltImplementation:
        public Exchange::IBrowser,
        public PluginHost::IStateControl {
private:
    class Config: public Core::JSON::Container {
    private:
        Config(const Config&);
        Config& operator=(const Config&);

    public:
        Config() :
            Core::JSON::Container(), Url() {
            Add(_T("url"), &Url);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);
            Add(_T("clientidentifier"), &ClientIdentifier);
        }
        ~Config() {
        }

    public:
        Core::JSON::String Url;
        Core::JSON::DecUInt16 Width;
        Core::JSON::DecUInt16 Height;
        Core::JSON::String ClientIdentifier;
    };

    class NotificationSink: public Core::Thread {
    private:
        NotificationSink() = delete;
        NotificationSink(const NotificationSink&) = delete;
        NotificationSink& operator=(const NotificationSink&) = delete;

    public:
        NotificationSink(CobaltImplementation &parent) :
                _parent(parent), _waitTime(0), _command(
                        PluginHost::IStateControl::SUSPEND) {
        }
        virtual ~NotificationSink() {
            Block();
            Wait(Thread::STOPPED | Thread::BLOCKED, Core::infinite);
        }

    public:
        void RequestForStateChange(
                const PluginHost::IStateControl::command command) {
            _command = command;
            Run();
        }

    private:
        virtual uint32_t Worker() {
            bool success = false;

            if ((IsRunning() == true) && (success == false)) {
                success = _parent.RequestForStateChange(_command);
            }
            Block();
            _parent.StateChangeCompleted(success, _command);
            return (Core::infinite);
        }

    private:
        CobaltImplementation &_parent;
        uint32_t _waitTime;
        PluginHost::IStateControl::command _command;
    };

    class CobaltWindow : public Core::Thread {
    private:
        CobaltWindow(const CobaltWindow&) = delete;
        CobaltWindow& operator=(const CobaltWindow&) = delete;

    public:
        CobaltWindow()
            : Core::Thread(0, _T("Cobalt"))
            , _url{"https://www.youtube.com/tv"}
        {
        }
        virtual ~CobaltWindow()
        {
            kill(getpid(), SIGHUP);
            Block();
            Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
        }

        uint32_t Configure(PluginHost::IShell* service) {
            uint32_t result = Core::ERROR_NONE;

            Config config;
            config.FromString(service->ConfigLine());
            Core::SystemInfo::SetEnvironment(_T("HOME"), service->PersistentPath());
            Core::SystemInfo::SetEnvironment(_T("COBALT_TEMP"), service->VolatilePath());
            if (config.ClientIdentifier.IsSet() == true) {
                string value(service->Callsign() + ',' + config.ClientIdentifier.Value());
                Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), value);
            } else {
                Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), service->Callsign());
            }

            string width(Core::NumberType<uint16_t>(config.Width.Value()).Text());
            string height(Core::NumberType<uint16_t>(config.Height.Value()).Text());
            Core::SystemInfo::SetEnvironment(_T("COBALT_RESOLUTION_WIDTH"), width);
            Core::SystemInfo::SetEnvironment(_T("COBALT_RESOLUTION_HEIGHT"), height);

            if (width.empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_WIDTH"), width);
            }

            if (height.empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_HEIGHT"), height);
            }

            if (config.Url.IsSet() == true) {
              _url = config.Url.Value();
            }

            Run();
            return result;
        }

        bool Suspend(const bool suspend)
        {
            if (suspend == true) {
                kill(getpid(), SIGUSR1);
            }
            else {
                kill(getpid(), SIGCONT);
            }
            return (true);
        }

        string Url() const { return _url; }

    private:
        virtual uint32_t Worker()
        {
            const char* argv[] = {"Cobalt", _url.c_str()};
            while (IsRunning() == true) {
                StarboardMain(2, const_cast<char**>(argv));
            }
            return (Core::infinite);
        }

        string _url;
    };

private:
    CobaltImplementation(const CobaltImplementation&) = delete;
    CobaltImplementation& operator=(const CobaltImplementation&) = delete;

public:
    CobaltImplementation() :
            _adminLock(),
            _state(PluginHost::IStateControl::UNINITIALIZED),
            _cobaltClients(),
            _stateControlClients(),
            _sink(*this) {
    }

    virtual ~CobaltImplementation() {
    }

    virtual uint32_t Configure(PluginHost::IShell *service) {
        uint32_t result = _window.Configure(service);
        _state = PluginHost::IStateControl::RESUMED;
        return (result);
    }

    virtual void SetURL(const string &URL) override {
    }

    virtual string GetURL() const override {
        return _window.Url();
    }

    virtual uint32_t GetFPS() const override {
        return 0;
    }

    virtual void Hide(const bool hidden) {
    }

    virtual void Register(Exchange::IBrowser::INotification *sink) {
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(
                std::find(_cobaltClients.begin(), _cobaltClients.end(), sink)
                        == _cobaltClients.end());

        _cobaltClients.push_back(sink);
        sink->AddRef();

        _adminLock.Unlock();
    }

    virtual void Unregister(Exchange::IBrowser::INotification *sink) {
        _adminLock.Lock();

        std::list<Exchange::IBrowser::INotification*>::iterator index(
                std::find(_cobaltClients.begin(), _cobaltClients.end(), sink));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _cobaltClients.end());

        if (index != _cobaltClients.end()) {
            (*index)->Release();
            _cobaltClients.erase(index);
        }

        _adminLock.Unlock();
    }

    virtual void Register(PluginHost::IStateControl::INotification *sink) {
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(
                std::find(_stateControlClients.begin(),
                        _stateControlClients.end(), sink)
                        == _stateControlClients.end());

        _stateControlClients.push_back(sink);
        sink->AddRef();

        _adminLock.Unlock();
    }

    virtual void Unregister(PluginHost::IStateControl::INotification *sink) {
        _adminLock.Lock();

        std::list<PluginHost::IStateControl::INotification*>::iterator index(
                std::find(_stateControlClients.begin(),
                        _stateControlClients.end(), sink));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _stateControlClients.end());

        if (index != _stateControlClients.end()) {
            (*index)->Release();
            _stateControlClients.erase(index);
        }

        _adminLock.Unlock();
    }

    virtual PluginHost::IStateControl::state State() const {
        return (_state);
    }

    virtual uint32_t Request(const PluginHost::IStateControl::command command) {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        _adminLock.Lock();

        if (_state == PluginHost::IStateControl::UNINITIALIZED) {
            // Seems we are passing state changes before we reached an operational Cobalt.
            // Just move the state to what we would like it to be :-)
            _state = (
                    command == PluginHost::IStateControl::SUSPEND ?
                            PluginHost::IStateControl::SUSPENDED :
                            PluginHost::IStateControl::RESUMED);
            result = Core::ERROR_NONE;
        } else {
            switch (command) {
            case PluginHost::IStateControl::SUSPEND:
                if (_state == PluginHost::IStateControl::RESUMED) {
                    _sink.RequestForStateChange(
                            PluginHost::IStateControl::SUSPEND);
                    result = Core::ERROR_NONE;
                }
                break;
            case PluginHost::IStateControl::RESUME:
                if (_state == PluginHost::IStateControl::SUSPENDED) {
                    _sink.RequestForStateChange(
                            PluginHost::IStateControl::RESUME);
                    result = Core::ERROR_NONE;
                }
                break;
            default:
                break;
            }
        }

        _adminLock.Unlock();

        return result;
    }

    void StateChangeCompleted(bool success,
            const PluginHost::IStateControl::command request) {
        if (success) {
            switch (request) {
            case PluginHost::IStateControl::RESUME:

                _adminLock.Lock();

                if (_state != PluginHost::IStateControl::RESUMED) {
                    StateChange(PluginHost::IStateControl::RESUMED);
                }

                _adminLock.Unlock();
                break;
            case PluginHost::IStateControl::SUSPEND:

                _adminLock.Lock();

                if (_state != PluginHost::IStateControl::SUSPENDED) {
                    StateChange(PluginHost::IStateControl::SUSPENDED);
                }

                _adminLock.Unlock();
                break;
            default:
                ASSERT(false);
                break;
            }
        } else {
            StateChange(PluginHost::IStateControl::EXITED);
        }
    }

    BEGIN_INTERFACE_MAP (CobaltImplementation)INTERFACE_ENTRY (Exchange::IBrowser)INTERFACE_ENTRY (PluginHost::IStateControl)END_INTERFACE_MAP

private:
    inline bool RequestForStateChange(
            const PluginHost::IStateControl::command command) {
        bool result = false;

        switch (command) {
            case PluginHost::IStateControl::SUSPEND: {
                if (_window.Suspend(true) == true) {
                    result = true;
                }
                break;
            }
            case PluginHost::IStateControl::RESUME: {
                if (_window.Suspend(false) == true) {
                    result = true;
                }
                break;
            }
            default:
                ASSERT(false);
                break;
        }
        return result;
    }

    void StateChange(const PluginHost::IStateControl::state newState) {
        _adminLock.Lock();

        _state = newState;

        std::list<PluginHost::IStateControl::INotification*>::iterator index(
                _stateControlClients.begin());

        while (index != _stateControlClients.end()) {
            (*index)->StateChange(newState);
            index++;
        }

        _adminLock.Unlock();
    }
    CobaltWindow _window;
    mutable Core::CriticalSection _adminLock;
    PluginHost::IStateControl::state _state;
    std::list<Exchange::IBrowser::INotification*> _cobaltClients;
    std::list<PluginHost::IStateControl::INotification*> _stateControlClients;
    NotificationSink _sink;
};

SERVICE_REGISTRATION(CobaltImplementation, 1, 0);

}
/* namespace Plugin */

namespace Cobalt {

class MemoryObserverImpl: public Exchange::IMemory {
private:
    MemoryObserverImpl();
    MemoryObserverImpl(const MemoryObserverImpl&);
    MemoryObserverImpl& operator=(const MemoryObserverImpl&);

    public:
    MemoryObserverImpl(const RPC::IRemoteConnection* connection) :
        _main(connection == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId()) {
    }
    ~MemoryObserverImpl() {
    }

    public:
    virtual void Observe(const uint32_t pid) {
        if (pid == 0) {
            _observable = false;
        } else {
            _main = Core::ProcessInfo(pid);
            _observable = true;
        }
    }
    virtual uint64_t Resident() const {
        return (_observable == false ? 0 : _main.Resident());
    }
    virtual uint64_t Allocated() const {
        return (_observable == false ? 0 : _main.Allocated());
    }
    virtual uint64_t Shared() const {
        return (_observable == false ? 0 : _main.Shared());
    }
    virtual uint8_t Processes() const {
        return (IsOperational() ? 1 : 0);
    }

    virtual const bool IsOperational() const {
        return (_observable == false) || (_main.IsActive());
    }

    BEGIN_INTERFACE_MAP (MemoryObserverImpl)INTERFACE_ENTRY (Exchange::IMemory)END_INTERFACE_MAP

private:
    Core::ProcessInfo _main;
    bool _observable;
    };

    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection) {
        ASSERT(connection != nullptr);
        Exchange::IMemory* result = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);
        return (result);
    }
}
} // namespace
