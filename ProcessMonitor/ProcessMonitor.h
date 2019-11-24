#ifndef __PROCESS_MONITOR_H
#define __PROCESS_MONITOR_H

#include "Module.h"
#include <string>

namespace WPEFramework {
namespace Plugin {

class ProcessMonitor: public PluginHost::IPlugin,
        public PluginHost::IWeb,
        public PluginHost::JSONRPC
{
private:
    ProcessMonitor(const ProcessMonitor&);
    ProcessMonitor& operator=(const ProcessMonitor&);

    class Config: public Core::JSON::Container
    {
    private:
        Config(const Config&);
        Config& operator=(const Config&);

    public:
        Config()
            : Core::JSON::Container(), ExitTimeout()
        {
            Add(_T("exittimeout"), &ExitTimeout);
        }
        ~Config()
        {
        }
    public:
        Core::JSON::DecUInt32 ExitTimeout;
    };

    class RemoteNotification: public RPC::IRemoteConnection::INotification
    {
    private:
        RemoteNotification() = delete;
        RemoteNotification(const RemoteNotification&) = delete;
        RemoteNotification& operator=(const RemoteNotification&) = delete;

    public:
        explicit RemoteNotification(ProcessMonitor *parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        ~RemoteNotification()
        {
        }

    public:
        virtual void Activated(RPC::IRemoteConnection *connection)
        {
            _parent.AddProcess(connection->Callsign(), connection->RemoteId());
        }
        virtual void Deactivated(RPC::IRemoteConnection *connection)
        {
            _parent.RemoveProcess(connection->Callsign());
        }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
        END_INTERFACE_MAP

    private:
        ProcessMonitor &_parent;
    };

    class LocalNotification: public PluginHost::IPlugin::INotification
    {
    private:
        LocalNotification(const LocalNotification&) = delete;
        LocalNotification& operator=(const LocalNotification&) = delete;

    public:
        class Job: public Core::IDispatchType<void>
        {
        private:
            Job() = delete;
            Job(const Job &copy) = delete;
            Job& operator=(const Job &RHS) = delete;

        public:
            Job(LocalNotification *parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~Job()
            {
            }
            virtual void Dispatch() override
            {
                _parent.EvalueProcess();
            }

        private:
            LocalNotification &_parent;
        };

        class ProcessObject
        {
        public:
            ProcessObject() = delete;
            ProcessObject& operator=(const ProcessObject&) = delete;

        public:
            ProcessObject(
                const uint32_t processId)
                : _processId(processId)
                , _exitTime(0)
                , _deactivated(false)
            {
                ASSERT(_processId != 0);
            }
            ~ProcessObject()
            {
            }
            void Deactivate(const uint64_t exitTime)
            {
                _exitTime = exitTime;
                _deactivated = true;
            }
            bool Deactivated()
            {
                return _deactivated;
            }
            uint32_t ProcessId()
            {
                return _processId;
            }
            uint64_t ExitTime()
            {
                return _exitTime;
            }

        private:
            const uint32_t _processId;
            uint64_t _exitTime;
            bool _deactivated;
        };

    public:
        LocalNotification(ProcessMonitor *parent)
            : _adminLock()
            , _job(Core::ProxyType<Job>::Create(this))
            , _service(nullptr)
            , _parent(*parent)
        {
        }
        virtual ~LocalNotification()
        {
        }

    public:
        inline void Open(PluginHost::IShell *service, const uint32_t exittimeout)
        {
            ASSERT((service != nullptr) && (_service == nullptr));

            _service = service;
            _service->AddRef();

            _adminLock.Lock();

            _exittimeout = exittimeout * 1000 * 1000; // microseconds

            _adminLock.Unlock();

            PluginHost::WorkerPool::Instance().Submit(_job);
        }
        inline void Close()
        {
            ASSERT(_service != nullptr);

            PluginHost::WorkerPool::Instance().Revoke(_job);

            _adminLock.Lock();

            _processMap.clear();

            _adminLock.Unlock();

            _service->Release();
            _service = nullptr;
        }
        virtual void StateChange(PluginHost::IShell *service)
        {
            PluginHost::IShell::state currentState(service->State());
            if (currentState == PluginHost::IShell::ACTIVATED) {
            }
            else if (currentState == PluginHost::IShell::DEACTIVATION) {

                _adminLock.Lock();

                std::map<string, ProcessObject>::iterator itr(
                        _processMap.find(service->Callsign()));
                if (itr != _processMap.end()) {
                    const uint64_t exitTime = Core::Time::Now().Ticks()
                            + _exittimeout;
                    itr->second.Deactivate(exitTime);
                }

                _adminLock.Unlock();
            }
            else if (currentState == PluginHost::IShell::DEACTIVATED) {
            }
        }

        void AddProcess(const string callsign, const uint32_t processId)
        {
            _adminLock.Lock();

            _processMap.insert(
                    std::pair<string, ProcessObject>(callsign,
                            ProcessObject(processId)));

            _adminLock.Unlock();
        }

        void RemoveProcess(const string callsign)
        {
            _adminLock.Lock();

            _processMap.erase(callsign);

            _adminLock.Unlock();
        }

        void EvalueProcess()
        {
            uint64_t currTime(Core::Time::Now().Ticks());

            _adminLock.Lock();

            for (auto itr = _processMap.begin(); itr != _processMap.end();
                    ++itr) {
                if (itr->second.Deactivated()) {
                    if (itr->second.ExitTime() < currTime) {
                        kill(itr->second.ProcessId(), SIGKILL);
                    }
                }
            }

            _adminLock.Unlock();

            uint64_t nextSlot = currTime + static_cast<uint64_t>(1000000);
            PluginHost::WorkerPool::Instance().Schedule(nextSlot, _job);
        }

        BEGIN_INTERFACE_MAP(LocalNotification)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP

    private:
        Core::CriticalSection _adminLock;
        std::map<string, ProcessObject> _processMap;
        Core::ProxyType<Core::IDispatchType<void>> _job;
        PluginHost::IShell *_service;
        ProcessMonitor &_parent;
        uint32_t _exittimeout;
    };

public:
    ProcessMonitor()
        : _skipURL(0)
        , _localNotification(Core::Service<LocalNotification>::Create<LocalNotification>(this))
        , _remoteNotification(this)
    {
        RegisterAll();
    }
    virtual ~ProcessMonitor()
    {
        UnregisterAll();
        _localNotification->Release();
    }
    void AddProcess(const string callsign, const uint32_t processId)
    {
        _localNotification->AddProcess(callsign, processId);
    }
    void RemoveProcess(const string callsign)
    {
        _localNotification->RemoveProcess(callsign);
    }

    BEGIN_INTERFACE_MAP(ProcessMonitor)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    INTERFACE_ENTRY(PluginHost::IWeb)
    INTERFACE_ENTRY(PluginHost::IDispatcher)
    END_INTERFACE_MAP

public:
    //  IPlugin methods
    virtual const string Initialize(PluginHost::IShell *service);
    virtual void Deinitialize(PluginHost::IShell *service);
    virtual string Information() const;

    //  IWeb methods
    virtual void Inbound(Web::Request &request);
    virtual Core::ProxyType<Web::Response> Process(const Web::Request &request);

private:
    uint8_t _skipURL;
    Config _config;
    LocalNotification *_localNotification;
    Core::Sink<RemoteNotification> _remoteNotification;

private:
    void RegisterAll();
    void UnregisterAll();
};
}
}

#endif // __PROCESS_MONITOR_H
