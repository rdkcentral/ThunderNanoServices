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

#include <string>
#include <vector>
#include <iostream>
#include <iterator>
#include <iomanip>

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
    { Exchange::IPower::PCState::On,             _TXT("")        },
    { Exchange::IPower::PCState::ActiveStandby,  _TXT("freeze")  },
    { Exchange::IPower::PCState::PassiveStandby, _TXT("standby") },
    { Exchange::IPower::PCState::SuspendToRAM,   _TXT("mem")     },
    { Exchange::IPower::PCState::Hibernate,      _TXT("disk")    },
    { Exchange::IPower::PCState::PowerOff,       _TXT("poweroff")}
};

class PowerImplementation {
private:
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
            _parent.UpdateState(_mode);
        }

    private:
        PowerImplementation& _parent;
        Exchange::IPower::PCState _mode;
    };

public:
    PowerImplementation(power_state_change callback, void* userData)
        : _supportedModes(0)
        , _stateFile(-1)
        , _triggerFile(-1)
        , _callback(callback)
        , _userData(userData)
        , _job(*this)
        , _currentState(Exchange::IPower::PCState::On)
    {
        TRACE(Trace::Information, (_T("LinuxPowerImplementation()")));
        static constexpr char StateFile[]   = "/sys/power/state";
        static constexpr char TriggerFile[] = "/sys/power/early_suspend_trigger";

        /* Get the list of supported modes from /sys/power/state */
        char buffer[128] = {};
        int fd = ::open(StateFile, O_RDONLY);
        if (fd >= 0) {
            int position = 0;
            position = ::read(fd, &buffer, sizeof(buffer));
            ::close(fd);
            buffer[position] = 0;
            string data(buffer);
            if (data.length()) {
                std::istringstream iss(data);
                std::vector<std::string> result {
                    std::istream_iterator<std::string>(iss), {}
                };
                for (auto& split: result) {
                    TRACE(Trace::Information, (_T("Splitted /sys/power/state : '%s'"), split.c_str()));
                    uint8_t index = 0;
                    mode* result = nullptr;
                    while ((result == nullptr) && (index < (sizeof(modes)/sizeof(mode)))) {
                        if (split == modes[index].label) {
                            _supportedModes |= (1 << modes[index].id);
                        }
                        index++;
                    }
                }
                TRACE(Trace::Information, (_T("SupportedModes : %d"), _supportedModes));
            }
        }

        /* Add POWEROFF since its software implementation. */
        _supportedModes |= (1 << Exchange::IPower::PCState::PowerOff);

        _stateFile = ::open(StateFile, O_WRONLY);
        if (_stateFile < 0) {
            TRACE(Trace::Error, (_T("Not able to access %s."), StateFile));
        } else {
            _triggerFile = ::open(TriggerFile, O_WRONLY);

            if (_triggerFile < 0) {
                TRACE(Trace::Error, (_T("Not able to access %s."), TriggerFile));
            } else {
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
    bool IsSupported(const Exchange::IPower::PCState state) const {
        return ((_supportedModes & (1 << state)) != 0);
    }
    bool SetState(const Exchange::IPower::PCState state, const uint32_t waitTimeOut)
    {
        bool result = (_triggerFile > 0);

        if (result == true) {
            Core::ProxyType<Core::IDispatch> job(_job.Aquire());

            if (job.IsValid() == false) {
                result = false;
            } else {
                static_cast<Job&>(_job).Set(state);
                TRACE(Trace::Information, (_T("Scheduled Job: change Power mode to %d after %dsec."),
                                state, waitTimeOut));
                Core::IWorkerPool::Instance().Schedule(Core::Time::Now().Add(waitTimeOut * 1000), job);
            }
        }
        return (result);
    }
    Exchange::IPower::PCState GetState() const {
        return (_currentState);
    }

private:
    mode* FindMode(const Exchange::IPower::PCState state) const {
        uint8_t index = 0;
        mode* result = nullptr;
        while ((result == nullptr) && (index < (sizeof(modes)/sizeof(mode)))) {
            if (state == modes[index].id) {
                result = &(modes[index]);
            } else {
                index++;
            }
        }
        return (result);
    }
    void UpdateState(Exchange::IPower::PCState state) {
        if (_currentState != state) {
            mode* newMode = FindMode(state);

            if (newMode != nullptr) {
                uint8_t value = -1;
                TRACE(Trace::Information, (_T("Scheduled Job: changing Power mode to %s[%d]."),
                            newMode->label, state));
                Notify(state);
                _currentState = state;

                switch (state) {
                    case Exchange::IPower::PCState::On: break;
                    case Exchange::IPower::PCState::ActiveStandby:
                    case Exchange::IPower::PCState::PassiveStandby:
                    case Exchange::IPower::PCState::SuspendToRAM:
                    case Exchange::IPower::PCState::Hibernate:
                        {
                            ::write(_triggerFile, "1", 1);
                            /* We will be able to write state only if we are in 'On' State. */
                            ::write(_stateFile, newMode->label, newMode->length);
                            /* Read possible only if device is awaken by extrenal Power 'On' Trigger. */
                            lseek(_triggerFile, 0, SEEK_SET);
                            ::read(_triggerFile, &value, 1);
                            /* Reset the early wakeup trigger for low level drivers. */
                            ::write(_triggerFile, "0", 1);
                            /* Reset the State to 'On' once device wakes-up. */
                            SetState(Exchange::IPower::PCState::On, 0);
                        }
                        break;
                    case Exchange::IPower::PCState::PowerOff:
                        system("poweroff");
                        break;
                    default:
                        TRACE(Trace::Error, (_T("Should not reach here at any case...!!!")));
                }
            }
        }
    }
    void Notify (const Exchange::IPower::PCState mode) {
       if (_callback != nullptr) {
           _callback(_userData, mode);
       }
    }

private:
    uint32_t _supportedModes;
    int _stateFile;
    int _triggerFile;
    power_state_change _callback;
    void* _userData;
    Core::ThreadPool::JobType<Job> _job;
    Exchange::IPower::PCState _currentState;
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
        if ((implementation->IsSupported(state)) && (implementation->SetState(state, sleepTime) == true)) {
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

