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

#include "../../Implementation.h"

/* =============================================================================================
   THE ACTUAL IMPLEMENTATION OF THE IPower INTERFACE FOR LINUX KERNELS
   https://www.kernel.org/doc/Documentation/power/interface.txt
   ============================================================================================ */

using namespace WPEFramework;

struct mode {
    Exchange::IPower::PCState id;
    const char* label;
    const uint8_t length;
};

static mode modes[] = {
    { Exchange::IPower::PCStateOn,             _TXT("")        },
    { Exchange::IPower::PCStateActiveStandby,  _TXT("freeze")  },
    { Exchange::IPower::PCStatePassiveStandby, _TXT("standby") },
    { Exchange::IPower::PCStateSuspendToRAM,   _TXT("mem")     },
    { Exchange::IPower::PCStateHibernate,      _TXT("disk")    },
    { Exchange::IPower::PCStatePowerOff,       _TXT("")        }
};

class PowerImplementation {
private:
    static constexpr char DiskFile[]    = "/sys/power/disk";
    static constexpr char StateFile[]   = "/sys/power/state";
    static constexpr char TriggerFile[] = "/sys/power/early_suspend_trigger";

    class Job {
    public:
        Job () = delete;
        Job(const Job&) = delete;
        Job& operator= (const Job&) = delete;

        Job (PowerImplementation& parent)
            : _parent(parent)
            , _mode(Exchange::IPower::PCState::On) {
        }
        ~Job() {
        }

    public:
        void Set(Exchange::IPower::PCState mode) {
            _mode = mode;
        }
        void Dispatch() {
            _parent.SetState(_mode);
        }

    private:
        PowerImplementation& _parent;
        Exchange::IPower::PCState _mode;
    };

public:
    PowerImplementation(power_state_change callback, void* userData)
        : _adminLock()
        , _currentState(PCState::On)
        , _triggerFile(-1)
        , _callback(callback)
        , _userData(userData)
         _job(*this)
    {
        TRACE(Trace::Information, (_T("LinuxPowerImplementation()")));

        _stateFile = ::open(StateFile, O_WRONLY);
        if (_stateFile < 0) {
            TRACE(Trace::Error, (_T("Not able to access %s."), StateFile));
        }
        else {
            _triggerFile = ::open(TriggerFile, O_WRONLY);
    
            if (_triggerFile < 0) {        
                TRACE(Trace::Error, (_T("Not able to access %s."), TriggerFile));
            }
            else {
                ::write(_triggerFile, "0", 1);
            }
	}
    }
    ~PowerImplementation()
    {
        TRACE(Trace::Information, (_T("~LinuxPowerImplementation()")));

        if (_triggerFile >= 0) {
            ::close(_triggerFile);
        }

        if (_stateFile >= 0) {
            ::close(_stateFile);
        }
    }

public:
    bool SetState(const Exchange::IPower::PCState state, const uint32_t)
    {
        bool result = (_triggerFile > 0);

        if (result == true) {
            Core::ProxyType<Core::IDispatch> job(_job.Aquire());

            if (job.IsValid() == false) {
                result = false;
            }
            else {
                static_cast<Job&>(_job).Set(state);
                Core::IWorkerPool::Instance().Submit(job);
            }
        }
        return (result);
    }
    PCState GetState() const {
        return (_currentState);
    }

private:
    mode* FindMode(const PCState state) const {
        uint8_t index = 0;
        mode* result = nullptr;
        while ((result == nullptr) && (index < (sizeof(modes)/sizeof(mode)))) {
            if (state == modes[index].id) {
                result = &(modes[index]);
            }
            else {
                index++;
            }
        }
        return (result);
    }
    void SetState(const PCState state) {
        mode* newMode = FindMode(state);

        if (newMode != nullptr) {

            _currentState = state;

            /* Since only ON & STR is supported; query can be done when in ON. */
            /* Kept for future extension. */
            ::write(_triggerFile, "1", 1);

            /* We will be able to write state only if we are in 'On' State. */
            ::write(_stateFile, newMode->label, newMode->length);

            ::write(_triggerFile, "0", 1);

            Notify(state);

            if (_currentState != Exchange::IPower::PCStateOn) {
                /*** `STATE_REVERT_LOGIC` ***/
                /* STR FREEZE happens only after ~100ms from trigger. */
                /* This sleep will make the code to execute after wake-up. */
                /* Schedule a wake-up state change event. */
                Core::ProxyType<Core::IDispatch> job(_job.Aquire());

                if (job.IsValid() == true) {
                    static_cast<Job&>(_job).Set(state);
                    Core::IWorkerPool::Instance().Schedule(Core::Time::Now().Add(100), job);
                }
            }
        }
    }
    void Notify (const Exchange::IPower::PCState mode) {
       _adminLock.Lock();

       if (_callback != nullptr) {
           _callback(_userData, mode);
       }

       _adminLock.Unlock();
    }

private:
    PCState _currentState;
    int _stateFile;
    int _triggerFile;
    power_state_change _callback;
    void* _userData;
    Core::ThreadPool::JobType<Job> _job;
};

static PowerImplementation* implementation = nullptr;

void power_initialize(power_state_change callback, void* userData, const char* ) {
    ASSERT (implementation == nullptr);
    implementation = new PowerImplementation(callback, userData);
}

void power_deinitialize() {
    ASSERT (implementation != nullptr);

    if (implementation != nullptr) {
        delete implementation;
    }
}

uint32_t power_set_state(const enum WPEFramework::Exchange::IPower::PCState state, const uint32_t sleepTime) {
    ASSERT (implementation != nullptr);

    if (implementation != nullptr) {
        if (implementation->SetState(state, sleepTime) == true) {
            return (Core::ERROR_NONE);
        }
    }
    return (Core::ERROR_UNAVAILABLE);
}

WPEFramework::Exchange::IPower::PCState power_get_state() {
    ASSERT (implementation != nullptr);

    WPEFramework::Exchange::IPower::PCState result = WPEFramework::Exchange::IPower::PCState::PowerOff;

    if (implementation != nullptr) {
        result = implementation->GetState();
    }
    return (result);
}

