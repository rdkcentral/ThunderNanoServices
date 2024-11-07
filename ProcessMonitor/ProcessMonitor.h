/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
 
#ifndef __PROCESS_MONITOR_H
#define __PROCESS_MONITOR_H

#include "Module.h"

#include <string>
#include <syslog.h>
#include <unordered_map>

namespace Thunder {
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
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

        using Job = Core::WorkerPool::JobType<Notification&>;

        class ProcessObject
        {
        public:
            ProcessObject() = delete;
            ProcessObject(const ProcessObject&) = default;
            ProcessObject& operator=(const ProcessObject&) = default;
            ProcessObject(ProcessObject&&) = default;
            ProcessObject& operator=(ProcessObject&&) = default;

        public:
            ProcessObject(
                const uint32_t processId)
                : _processId(processId)
                , _exitTime(0)
            {
                ASSERT(_processId != 0);
            }
            ~ProcessObject()
            {
            }
            uint32_t ProcessId() const
            {
                return _processId;
            }
            void SetExitTime(const uint64_t exitTime)
            {
                _exitTime = exitTime;
            }
            uint64_t ExitTime() const
            {
                return _exitTime;
            }

        private:
            uint32_t _processId;
            uint64_t _exitTime;
        };

    public:
        Notification()
            : _adminLock()
            , _processMap()
            , _job(*this)
            , _service(nullptr)
            ,_exittimeout(10000000)
        {
        }
        ~Notification() override
        {
            ASSERT(_service == nullptr);
        }

    public:
        inline void Open(PluginHost::IShell* service, const uint32_t exittimeout)
        {
            ASSERT((service != nullptr) && (_service == nullptr));
            
            _exittimeout = exittimeout * 1000 * 1000; // microseconds

            _service = service;
            _service->AddRef();

            _service->Register(static_cast<IPlugin::INotification*>(this));
            _service->Register(
                    static_cast<RPC::IRemoteConnection::INotification*>(this));
        }
        inline void Close()
        {
            ASSERT(_service != nullptr);

            _service->Unregister(static_cast<IPlugin::INotification*>(this));
            _service->Unregister(
                    static_cast<RPC::IRemoteConnection::INotification*>(this));

            _service->Release();
            _service = nullptr;

            _job.Revoke();

            _processMap.clear();
        }

        void Activated(const string&, PluginHost::IShell*) override
        {
        }

        void Deactivated(const string& callsign, PluginHost::IShell*) override
        {
            uint64_t exitTime = 0;

            _adminLock.Lock();

            std::unordered_map<string, ProcessObject>::iterator itr(
                    _processMap.find(callsign));
            if (itr != _processMap.end()) {
                exitTime = Core::Time::Now().Ticks() + _exittimeout;
                itr->second.SetExitTime(exitTime);
            }

            if (exitTime != 0) {
                ScheduleJob();
            }

            _adminLock.Unlock();
        }

        void Unavailable(const string&, PluginHost::IShell*) override
        {
        }

        void AddProcess(const string callsign, const uint32_t processId)
        {
            _adminLock.Lock();

            _processMap.emplace(callsign, ProcessObject(processId));

            _adminLock.Unlock();
        }
        void Dispatch()
        {
            uint64_t currTime(Core::Time::Now().Ticks());

            _adminLock.Lock();

            auto itr = _processMap.begin();
            while (itr != _processMap.end()) {

                uint64_t exitTime = itr->second.ExitTime();
                if ((exitTime != 0) && (exitTime <= currTime)) {
                    Core::Process proc(itr->second.ProcessId());
                    if (proc.IsActive()) {
                        proc.Kill(true);
                        SYSLOG(Logging::Notification,
                                (_T("ProcessMonitor killed: [%s]!"),
                                        itr->first.c_str()));
                    }
                    itr = _processMap.erase(itr);
                }
                else {
                    itr++;
                }
            }

            ScheduleJob();

            _adminLock.Unlock();
        }
        void ScheduleJob()
        {
            uint64_t scheduleTime = 0;

            for (auto itr : _processMap) {
                uint64_t exitTime = itr.second.ExitTime();
                if (exitTime != 0) {
                    if ((scheduleTime == 0) || (scheduleTime > exitTime)) {
                        scheduleTime = exitTime;
                    }
                }
            }

            if (scheduleTime != 0) {
                _job.Revoke();
                _job.Reschedule(scheduleTime);
            }
        }
        void Activated(RPC::IRemoteConnection* connection) override
        {
            RPC::IMonitorableProcess* proc =
                    connection->QueryInterface<RPC::IMonitorableProcess>();
            if (proc != nullptr) {
                AddProcess(proc->Callsign(), connection->RemoteId());
                proc->Release();
            }
        }
        void Deactivated(RPC::IRemoteConnection*) override
        {
        }
        void Terminated(RPC::IRemoteConnection*) override
        {
        }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
        END_INTERFACE_MAP

    private:
        Core::CriticalSection _adminLock;
        std::unordered_map<string, ProcessObject> _processMap;
        Job  _job;
        PluginHost::IShell* _service;
        uint32_t _exittimeout;
    };

public:
    ProcessMonitor()
        : _notification()
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
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

private:
    Core::SinkType<Notification> _notification;
};
}
}

#endif // __PROCESS_MONITOR_H
