#ifndef __PROCESS_MONITOR_H
#define __PROCESS_MONITOR_H

#include "Module.h"

#include <string>
#include <syslog.h>
#include <unordered_map>

namespace WPEFramework {
namespace Plugin {

class ProcessMonitor: public PluginHost::IPlugin
{
public:
    ProcessMonitor(const ProcessMonitor&) = delete;
    ProcessMonitor& operator=(const ProcessMonitor&) = delete;

    class Config: public Core::JSON::Container
    {
    public:
        Config(const Config&) =  delete;
        Config& operator=(const Config&) = delete;

    public:
        Config()
            : Core::JSON::Container(), ExitTimeout()
        {
            Add(_T("exittimeout"), &ExitTimeout);
        }
        ~Config() override
        {
        }
    public:
        Core::JSON::DecUInt32 ExitTimeout;
    };

    class Notification: public PluginHost::IPlugin::INotification,
            public RPC::IRemoteConnection::INotification
    {
    public:
        Notification() = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

        class Job: public Core::IDispatchType<void>
        {
        public:
            Job() = delete;
            Job(const Job &copy) = delete;
            Job& operator=(const Job &RHS) = delete;

        public:
            Job(Notification *parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Job() override
            {
            }
            void Dispatch() override
            {
                _parent.EvalueProcess();
            }

        private:
            Notification &_parent;
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
            {
                ASSERT(_processId != 0);
            }
            virtual ~ProcessObject()
            {
            }
            uint32_t ProcessId()
            {
                return _processId;
            }
            void SetExitTime(const uint64_t exitTime)
            {
                _exitTime = exitTime;
            }
            uint64_t ExitTime()
            {
                return _exitTime;
            }

        private:
            const uint32_t _processId;
            uint64_t _exitTime;
        };

    public:
        Notification(ProcessMonitor *parent)
            : _adminLock()
            , _job(Core::ProxyType<Job>::Create(this))
            , _service(nullptr)
            , _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        ~Notification() override
        {
            ASSERT(_service == nullptr);
        }

    public:
        inline void Open(PluginHost::IShell *service, const uint32_t exittimeout)
        {
            ASSERT((service != nullptr) && (_service == nullptr));

            _service = service;
            _service->AddRef();

            _service->Register(static_cast<IPlugin::INotification*>(this));
            _service->Register(
                    static_cast<RPC::IRemoteConnection::INotification*>(this));

            _exittimeout = exittimeout * 1000 * 1000; // microseconds
        }
        inline void Close()
        {
            ASSERT(_service != nullptr);

            PluginHost::WorkerPool::Instance().Revoke(_job);

            _adminLock.Lock();

            _processMap.clear();

            _adminLock.Unlock();

            _service->Unregister(static_cast<IPlugin::INotification*>(this));
            _service->Unregister(
                    static_cast<RPC::IRemoteConnection::INotification*>(this));

            _service->Release();
            _service = nullptr;
        }
        void StateChange(PluginHost::IShell *service) override
        {
            PluginHost::IShell::state currentState(service->State());
            if (currentState == PluginHost::IShell::ACTIVATED) {
            }
            else if (currentState == PluginHost::IShell::DEACTIVATION) {

                _adminLock.Lock();

                std::unordered_map<string, ProcessObject>::iterator itr(
                        _processMap.find(service->Callsign()));
                if (itr != _processMap.end()) {
                    const uint64_t exitTime = Core::Time::Now().Ticks()
                            + _exittimeout;
                    itr->second.SetExitTime(exitTime);
                }

                _adminLock.Unlock();

                PluginHost::WorkerPool::Instance().Submit(_job);
            }
            else if (currentState == PluginHost::IShell::DEACTIVATED) {
            }
        }
        void AddProcess(const string callsign, const uint32_t processId)
        {
            _adminLock.Lock();

            _processMap.emplace(callsign, ProcessObject(processId));

            _adminLock.Unlock();
        }
        void EvalueProcess()
        {
            uint64_t exitTime;
            uint64_t scheduleTime = 0;
            uint64_t currTime(Core::Time::Now().Ticks());

            _adminLock.Lock();

            auto itr = _processMap.begin();
            while (itr != _processMap.end()) {

                exitTime = itr->second.ExitTime();
                if (exitTime) {
                    if (exitTime < currTime) {
                        if (ProcessExists(itr->second.ProcessId())) {
                            kill(itr->second.ProcessId(), SIGKILL);
                            syslog(LOG_NOTICE, "ProcessMonitor killed %s\n",
                                    itr->first.c_str());
                        }
                        itr = _processMap.erase(itr);
                        continue;
                    } else {
                        if (scheduleTime) {
                            if (scheduleTime > exitTime)
                                scheduleTime = exitTime;
                        } else {
                            scheduleTime = exitTime;
                        }
                    }
                }
                itr++;
            }

            _adminLock.Unlock();

            if (scheduleTime) {
                PluginHost::WorkerPool::Instance().Schedule(scheduleTime, _job);
            }
        }
        bool ProcessExists(uint32_t pid)
        {
            std::ostringstream cmd;
            cmd << "cat /proc/" << pid
                    << "/stat 2> /dev/null | awk '{ print $3 }'";

            std::string result;
            std::array<char, 128> buffer;
            std::unique_ptr<FILE, decltype(&pclose)> pipe(
                    popen(string(cmd.str()).c_str(), "r"), pclose);
            if (!pipe) {
                throw std::runtime_error("popen() failed!");
            }
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
            }
            if ((result.size() == 0) || (result.compare(string("Z\n")) == 0)) {
                return false;
            }
            return true;
        }
        void Activated(RPC::IRemoteConnection *connection) override
        {
            RPC::IProcess *proc =
                    reinterpret_cast<RPC::IProcess*>(connection->QueryInterface(
                            RPC::IProcess::ID));
            if (proc) {
                AddProcess(proc->Callsign(), connection->RemoteId());
            }
        }
        void Deactivated(RPC::IRemoteConnection *connection) override
        {
        }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
        END_INTERFACE_MAP

    private:
        Core::CriticalSection _adminLock;
        std::unordered_map<string, ProcessObject> _processMap;
        Core::ProxyType<Core::IDispatchType<void>> _job;
        PluginHost::IShell* _service;
        ProcessMonitor& _parent;
        uint32_t _exittimeout;
    };

public:
    ProcessMonitor()
        : _notification(this)
    {
    }
    ~ProcessMonitor() override
    {
        _notification.Release();
    }

    BEGIN_INTERFACE_MAP(ProcessMonitor)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    END_INTERFACE_MAP

public:
    //  IPlugin methods
    const string Initialize(PluginHost::IShell *service) override;
    void Deinitialize(PluginHost::IShell *service) override;
    string Information() const override;

private:
    Core::Sink<Notification> _notification;
};
}
}

#endif // __PROCESS_MONITOR_H
