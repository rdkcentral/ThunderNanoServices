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

#include <nexus_config.h>
#include <nexus_platform.h>
#include <nexus_platform_standby.h>
#include <nxclient.h>
#include <pmlib.h>

/* =============================================================================================
   THE ACTUAL IMPLEMENTATION OF THE IPower INTERFACE FOR BROADCOM BOARDS
   ============================================================================================ */

namespace WPEFramework {

enum gpiotype {
    STANDARD = NEXUS_GpioType_eStandard,
    SPECIAL = NEXUS_GpioType_eSpecial,
    UNUSED = NEXUS_GpioType_eTvMicro,
    AON_STANDARD = NEXUS_GpioType_eAonStandard,
    AON_SPECIAL = NEXUS_GpioType_eAonSpecial
};

ENUM_CONVERSION_BEGIN(gpiotype)

    { gpiotype::STANDARD, _TXT("Standard") },
    { gpiotype::SPECIAL, _TXT("Special") },
    { gpiotype::UNUSED, _TXT("Unused") },
    { gpiotype::AON_STANDARD, _TXT("AONStandard") },
    { gpiotype::AON_SPECIAL, _TXT("AONSpecial") },

ENUM_CONVERSION_END(gpiotype);

} 

using namespace WPEFramework;

class PowerImplementation {
private:
    class Job {
    public:
        Job () = delete;
        Job(const Job&) = delete;
        Job& operator= (const Job&) = delete;

        Job (PowerImplementation& parent) 
            : _parent(parent) 
            , _sleepTime(0)
            , _reachStateTime(0)
            , _mode(Exchange::IPower::PCState::On) {
        }
        ~Job() {
        }

    public:
        void Set(Exchange::IPower::PCState mode, const uint32_t sleepTime, const uint8_t reachStateTime) {
            _sleepTime = sleepTime;
            _mode = mode;
            _reachStateTime = reachStateTime;
        }
        void Dispatch() {
            _parent.SetState(_mode, _sleepTime, _reachStateTime);
        }

    private:
        PowerImplementation& _parent;
        uint32_t _sleepTime;
        uint8_t _reachStateTime;
        Exchange::IPower::PCState _mode;
    };
    class Config : public Core::JSON::Container {
    public:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

        Config()
            : Core::JSON::Container()
            , GPIOPin(0)
            , GPIOType(AON_STANDARD)
            , StateChangeTime(4)
        {
            Add(_T("gpiopin"), &GPIOPin);
            Add(_T("gpiotype"), &GPIOType);
            Add(_T("statechange"), &StateChangeTime);
        }
        ~Config()
        {
        }

        Core::JSON::DecUInt32 GPIOPin;
        Core::JSON::EnumType<gpiotype> GPIOType;
        Core::JSON::DecUInt8 StateChangeTime;
    };

public:
    PowerImplementation() = delete;
    PowerImplementation(const PowerImplementation&) = delete;
    PowerImplementation& operator=(const PowerImplementation&) = delete;

    PowerImplementation(power_state_change callback, void* userData, const string& configuration)
        : _adminLock()
        , _pmContext(nullptr)
        , _timeout(0)
        , _gpioHandle(nullptr)
        , _gpioType(NEXUS_GpioType_eAonStandard)
        , _gpioPin(0)
        , _reachStateChange(0)
        , _callback(callback)
        , _userData(userData)
        , _job(*this)
    {
        TRACE(Trace::Information, (_T("BroadcomPowerImplementation()")));

        NxClient_JoinSettings joinSettings;
        NxClient_GetDefaultJoinSettings(&joinSettings);
        snprintf(joinSettings.name, NXCLIENT_MAX_NAME, "%s", EXPAND_AND_QUOTE(MODULE_NAME));
        NEXUS_Error returnCode = NxClient_Join(&joinSettings);
        if (returnCode) {
            TRACE(Trace::Error, (_T("NxClient_Join Failed")));
        } else {
            TRACE(Trace::Information, (_T("NxClient_Join Succeeded")));

            Config config; config.FromString(configuration);

            if (config.GPIOPin.IsSet() == true) {
                _gpioPin = config.GPIOPin.Value();
                if (config.GPIOType.IsSet() == true) {
                    _gpioType = static_cast<NEXUS_GpioType>(config.GPIOType.Value());
                }
                NEXUS_GpioSettings gpioSettings;
                TRACE(Trace::Information, (_T("Enabling wakeup GPIO: %d-%d"), _gpioType, _gpioPin));

                NEXUS_Gpio_GetDefaultSettings(NEXUS_GpioType_eAonStandard, &gpioSettings);
                gpioSettings.mode = NEXUS_GpioMode_eInput;
                gpioSettings.interruptMode = NEXUS_GpioInterrupt_eRisingEdge;
                gpioSettings.interrupt.callback = gpioInterrupt;
                gpioSettings.interrupt.context = this;
                _gpioHandle = NEXUS_Gpio_Open(_gpioType, _gpioPin, &gpioSettings);
            }

            _reachStateChange = config.StateChangeTime.Value();
            _pmContext = brcm_pm_init();

            NxClient_UnregisterAcknowledgeStandby(NxClient_RegisterAcknowledgeStandby());
        }
    }
    virtual ~PowerImplementation()
    {
        TRACE(Trace::Information, (_T("~BroadcomPowerImplementation()")));

        _adminLock.Lock();

        _callback = nullptr;
        _userData = nullptr;

        _adminLock.Unlock();

        Core::IWorkerPool::Instance().Revoke(_job.Reset());

        if (_pmContext) {
            brcm_pm_close(_pmContext);
        }
        if (_gpioHandle) {
            NEXUS_Gpio_Close(_gpioHandle);
        }

        NxClient_Uninit();
    }

public:
    bool SetState(const Exchange::IPower::PCState state, const uint32_t sleepTime)
    {
        Core::ProxyType<Core::IDispatch> job(_job.Aquire());

        if (job.IsValid()) {
            static_cast<Job&>(_job).Set(state, sleepTime, _reachStateChange);
            Core::IWorkerPool::Instance().Submit(job);
        }
        return (job.IsValid());
    }
    Exchange::IPower::PCState GetState() const
    {
        TRACE(Trace::Information, (_T("GetState")));
        NxClient_StandbyStatus standbyStatus;
        NxClient_GetStandbyStatus(&standbyStatus);

        return (Convert(standbyStatus.settings.mode));
    }

private:
    void SetState (Exchange::IPower::PCState mode, const uint32_t sleepTime, const uint8_t secondsToReachState)
    {
        if (mode == Exchange::IPower::PCState::PowerOff) {
            TRACE(Trace::Information, (_T("Firing off the Harakiri script to kill it all and stop...")));

            // Fire off a script, to commit Harakiri....
            system("poweroff");

            Notify (Exchange::IPower::PCState::PowerOff);
        }
        else {
            TRACE(Trace::Information, (_T("Requesting for new state")));

            NxClient_StandbySettings standbySettings;

            NxClient_GetDefaultStandbySettings(&standbySettings);
            standbySettings.settings.mode = Convert(mode);
            standbySettings.settings.wakeupSettings.ir = true;
            standbySettings.settings.wakeupSettings.gpio = true;
            standbySettings.settings.wakeupSettings.timeout = sleepTime;

            NxClient_SetStandbySettings(&standbySettings);

            NxClient_StandbyStatus standbyStatus;
            uint16_t attempts = (secondsToReachState * 10);

            do {
                NxClient_GetStandbyStatus(&standbyStatus);
                if (standbyStatus.transition != NxClient_StandbyTransition_eDone)
                    ::SleepMs(100);
                }
            while ((standbyStatus.transition != NxClient_StandbyTransition_eDone) && (attempts-- > 0));

            if (standbyStatus.transition == NxClient_StandbyTransition_eDone) {

                Notify (Convert(standbyStatus.settings.mode));

                if (standbySettings.settings.mode == NEXUS_PlatformStandbyMode_ePassive) {
                    brcm_pm_suspend(_pmContext, BRCM_PM_STANDBY);
                } else if (standbySettings.settings.mode == NEXUS_PlatformStandbyMode_eDeepSleep) {
                    brcm_pm_suspend(_pmContext, BRCM_PM_SUSPEND);
                }
            }
        }
    }
    Exchange::IPower::PCState Convert(NEXUS_PlatformStandbyMode mode) const {
        switch (mode) {
        case NEXUS_PlatformStandbyMode_eOn:        return (Exchange::IPower::On);             break;
        case NEXUS_PlatformStandbyMode_eActive:    return (Exchange::IPower::ActiveStandby);  break;
        case NEXUS_PlatformStandbyMode_ePassive:   return (Exchange::IPower::PassiveStandby); break;
        case NEXUS_PlatformStandbyMode_eDeepSleep: return (Exchange::IPower::SuspendToRAM);   break;
        default:                                   ASSERT (false);                            break;
        }
        return(Exchange::IPower::PowerOff); 
    }
    NEXUS_PlatformStandbyMode Convert(Exchange::IPower::PCState mode) const {
        switch(mode) {
        case Exchange::IPower::PCState::On:             return (NEXUS_PlatformStandbyMode_eOn);        break;
        case Exchange::IPower::PCState::ActiveStandby:  return (NEXUS_PlatformStandbyMode_eActive);    break;
        case Exchange::IPower::PCState::PassiveStandby: return (NEXUS_PlatformStandbyMode_ePassive);   break;
        case Exchange::IPower::PCState::SuspendToRAM:   return (NEXUS_PlatformStandbyMode_eDeepSleep); break;
        case Exchange::IPower::PCState::Hibernate:      return (NEXUS_PlatformStandbyMode_eDeepSleep); break;
        case Exchange::IPower::PCState::PowerOff:                                                      break;
        default: ASSERT(false);                                                                        break;
        }
        return(static_cast<NEXUS_PlatformStandbyMode>(~0)); 
    }
    void Notify (const Exchange::IPower::PCState mode) {
       _adminLock.Lock();

       if (_callback != nullptr) {
           _callback(_userData, mode);
       }

       _adminLock.Unlock();
    }
    void PrintWakeup() {
        NxClient_StandbyStatus standbyStatus;
        NEXUS_Error rc;

        rc = NxClient_GetStandbyStatus(&standbyStatus);
        if (!rc) {
            TRACE(Trace::Information, (_T("Wake up Status: IR=%d UHF=%d XPT=%d CEC=%d GPIO=%d KPD=%d Timeout=%d"),
                standbyStatus.status.wakeupStatus.ir, 
                standbyStatus.status.wakeupStatus.uhf, 
                standbyStatus.status.wakeupStatus.transport, 
                standbyStatus.status.wakeupStatus.cec, 
                standbyStatus.status.wakeupStatus.gpio, 
                standbyStatus.status.wakeupStatus.keypad, 
                standbyStatus.status.wakeupStatus.timeout));
        }
    }
    static void gpioInterrupt(void* context, int param) {
        PowerImplementation* power = static_cast<PowerImplementation*>(context);

        power->PrintWakeup();

        power->SetState(Exchange::IPower::PCState::On, 0);

        if (power->_gpioHandle) {
            NEXUS_Gpio_ClearInterrupt(power->_gpioHandle);
        }
    }

private:
    Core::CriticalSection _adminLock;
    void* _pmContext;
    uint32_t _timeout;
    NEXUS_GpioHandle _gpioHandle;
    NEXUS_GpioType _gpioType;
    uint32_t _gpioPin;
    uint8_t _reachStateChange;
    power_state_change _callback;
    void* _userData;
    Core::ThreadPool::JobType<Job> _job;
};

static PowerImplementation* implementation = nullptr;

void power_initialize(power_state_change callback, void* userData, const char* config) {
    ASSERT (implementation == nullptr);
    string configuration;
    if (config != nullptr) {
        Core::ToString(config, configuration);
    }

    implementation = new PowerImplementation(callback, userData, configuration);
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
