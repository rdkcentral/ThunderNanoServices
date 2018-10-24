#include "Module.h"
#include <interfaces/IPower.h>

#include <nexus_config.h>
#include <nexus_platform.h>
#include <nexus_platform_standby.h>
#include <nxclient.h>

namespace WPEFramework {

class PowerImplementation :
    public Exchange::IPower,
    virtual public WPEFramework::Core::Thread {

private:
    PowerImplementation(const PowerImplementation&) = delete;
    PowerImplementation& operator=(const PowerImplementation&) = delete;

public:
    class Config : public Core::JSON::Container {
    private:
        Config (const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        enum gpiotype {
            STANDARD = NEXUS_GpioType_eStandard,
            SPECIAL = NEXUS_GpioType_eSpecial,
            UNUSED = NEXUS_GpioType_eTvMicro,
            AON_STANDARD = NEXUS_GpioType_eAonStandard,
            AON_SPECIAL = NEXUS_GpioType_eAonSpecial
        };

    public:
        Config ()
            : Core::JSON::Container()
            , GPIOPin(0)
            , GPIOType(AON_STANDARD)
        {
            Add(_T("gpiopin"), &GPIOPin);
            Add(_T("gpiotype"), &GPIOType);
        }
        ~Config()
        {
        }

        Core::JSON::DecUInt32 GPIOPin;
        Core::JSON::EnumType<gpiotype> GPIOType;
    };

public:
    PowerImplementation ()
        : _pmContext(nullptr)
        , _event(nullptr)
        , _wakeupEvent(nullptr)
        , _mode(NEXUS_PlatformStandbyMode_eOn)
        , _coldBoot(false)
        , _eventTriggered(false)
        , _timeout(0)
        , _gpioHandle(nullptr)
        , _gpioType(NEXUS_GpioType_eAonStandard)
        , _gpioPin(0)
    {
        TRACE(Trace::Information, (_T("BcmPowerManager()")));
        Init();
        Run();
    }

    virtual ~PowerImplementation()
    {
        Deinit();
    }

    uint32_t Worker();
    BEGIN_INTERFACE_MAP(PowerImplementation)
        INTERFACE_ENTRY(Exchange::IPower)
    END_INTERFACE_MAP

    // IPower methods
    virtual PCState GetState() const override;
    virtual PCStatus SetState(const PCState, const uint32_t) override;
    virtual void PowerKey() override;
    virtual void Configure(const string& settings) override;

private:
    void Init();
    void Deinit();
    void StopWorker();
    PCStatus SetPowerState();
    PCStatus SetStandbyState();
    void SetWakeEvent();
    void PrintWakeup();
    static void gpioInterrupt(void *context, int param);

private:
    void* _pmContext;
    bool _coldBoot;
    bool _eventTriggered;
    bool _isSetState;

    uint32_t _timeout;
    BKNI_EventHandle _event;
    BKNI_EventHandle _wakeupEvent;
    NEXUS_PlatformStandbyMode _mode;
    NEXUS_GpioHandle _gpioHandle;
    NEXUS_GpioType _gpioType;
    uint32_t _gpioPin;
    
    Core::CriticalSection _lock;
};

// The essence of making the IPower interface available. This instantiates 
// an object that can be created from the outside of the library by looking
// for the PowerImplementation class name, that realizes the IPower interface.
SERVICE_REGISTRATION(PowerImplementation, 1, 0);

ENUM_CONVERSION_BEGIN(PowerImplementation::Config::gpiotype)

    { PowerImplementation::Config::STANDARD,  _TXT("Standard")  },
    { PowerImplementation::Config::SPECIAL, _TXT("Special") },
    { PowerImplementation::Config::UNUSED, _TXT("Unused") },
    { PowerImplementation::Config::AON_STANDARD,   _TXT("AONStandard")   },
    { PowerImplementation::Config::AON_SPECIAL,   _TXT("AONSpecial")   },

ENUM_CONVERSION_END(PowerImplementation::Config::gpiotype);

}

/* =============================================================================================
   THE ACTUAL IMPLEMENTATION OF THE IPower INTERFACE FOR BROADCOM BOARDS
   ============================================================================================ */

#include <cstdlib>
#include <stdio.h>

#define REPEAT_COUNT 15
#define PC_MODULE_NAME "power"

#ifdef __cplusplus
extern "C" {
#include <pmlib.h>
}
#endif

using namespace WPEFramework;

uint32_t PowerImplementation::Worker()
{
    while (IsRunning() == true) {
        _lock.Lock();
        if (_isSetState) {
            NEXUS_PlatformStandbyMode prevMode = _mode;
            PCStatus status = SetStandbyState();
            if (status == PCSuccess && (prevMode != NEXUS_PlatformStandbyMode_eOn)) {
                BKNI_WaitForEvent(_event, BKNI_INFINITE);
                status = SetPowerState();
                if (status == PCSuccess) {
                    if (_mode == NEXUS_PlatformStandbyMode_eOn) {
                        TRACE(Trace::Information, (_T("SetPowerState Changed Successfully")));
                    } else {
                        TRACE(Trace::Information, (_T("SetPowerState Failed!!")));
                    }
                }
            }
            _isSetState = false;
        }
        _lock.Unlock();
    }
    TRACE(Trace::Information, (_T("Worker thread to Block state")));
    return (WPEFramework::Core::infinite);
}

void PowerImplementation::Init()
{
    TRACE(Trace::Information, (_T("Init()")));
    NxClient_JoinSettings joinSettings;
    NxClient_GetDefaultJoinSettings(&joinSettings);
    snprintf(joinSettings.name, NXCLIENT_MAX_NAME, "%s", PC_MODULE_NAME);
    NEXUS_Error returnCode = NxClient_Join(&joinSettings);
    if (returnCode) {
        TRACE(Trace::Error, (_T("NxClient_Join Failed")));
    } else {
        TRACE(Trace::Information, (_T("NxClient_Join Succeeded")));
        BKNI_CreateEvent(&_event);
        BKNI_CreateEvent(&_wakeupEvent);

        _pmContext = brcm_pm_init();

        NxClient_UnregisterAcknowledgeStandby(NxClient_RegisterAcknowledgeStandby());
        
        if (_gpioPin != 0) {
            NEXUS_GpioSettings gpioSettings;
            TRACE(Trace::Information, (_T("Enabling wakeup GPIO: %d-%d"),_gpioType, _gpioPin));

            NEXUS_Gpio_GetDefaultSettings(NEXUS_GpioType_eAonStandard, &gpioSettings);
            gpioSettings.mode = NEXUS_GpioMode_eInput;
            gpioSettings.interruptMode = NEXUS_GpioInterrupt_eRisingEdge;
            gpioSettings.interrupt.callback = gpioInterrupt;
            gpioSettings.interrupt.context = this;
            _gpioHandle = NEXUS_Gpio_Open(_gpioType, _gpioPin, &gpioSettings);
        }
    }
}

void PowerImplementation::Deinit()
{
    TRACE(Trace::Information, (_T("Deinit")));
    StopWorker();
    if (_pmContext) {
        brcm_pm_close(_pmContext);
    }
    if (_wakeupEvent) {
       BKNI_DestroyEvent(_wakeupEvent);
    }
    if (_event) {
        BKNI_DestroyEvent(_event);
    }

    if (_gpioHandle) {
        NEXUS_Gpio_Close(_gpioHandle);
    }

    NxClient_Uninit();
}

void PowerImplementation::StopWorker()
{
    _eventTriggered = true;
    BKNI_SetEvent(_wakeupEvent);
    Block();
    Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
}

void PowerImplementation::PowerKey()
{
    TRACE(Trace::Information, (_T("KeyEventHandler keycode.")));
    _eventTriggered = true;
    SetWakeEvent();
}

void PowerImplementation::Configure(const string& settings)
{
    TRACE(Trace::Information, (_T("Configure()")));

    Config config; 
    config.FromString(settings);

    if (config.GPIOPin.IsSet() == true) {
        _gpioPin = config.GPIOPin.Value();
        if (config.GPIOType.IsSet() == true) {
            _gpioType = static_cast<NEXUS_GpioType>(config.GPIOType.Value());
        }
    }
}

void PowerImplementation::PrintWakeup()
{
    TRACE(Trace::Information, (_T("PrintWakeup")));
    NxClient_StandbyStatus standbyStatus;
    NEXUS_Error rc;

    rc = NxClient_GetStandbyStatus(&standbyStatus);
    if (rc) {
        BERR_TRACE(rc);
        return;
    }

    TRACE(Trace::Information, (_T(
        "Wake up Status:\n"
        "IR      : %d\n"
        "UHF     : %d\n"
        "XPT     : %d\n"
        "CEC     : %d\n"
        "GPIO    : %d\n"
        "KPD     : %d\n"
        "Timeout : %d\n"),
        standbyStatus.status.wakeupStatus.ir,
        standbyStatus.status.wakeupStatus.uhf,
        standbyStatus.status.wakeupStatus.transport,
        standbyStatus.status.wakeupStatus.cec,
        standbyStatus.status.wakeupStatus.gpio,
        standbyStatus.status.wakeupStatus.keypad,
        standbyStatus.status.wakeupStatus.timeout));
}

void PowerImplementation::gpioInterrupt(void *context, int param)
{
    PowerImplementation* power = static_cast<PowerImplementation*>(context);

    power->SetWakeEvent();
    power->_mode = NEXUS_PlatformStandbyMode_eOn;
    BKNI_SetEvent(power->_event);
    power->_eventTriggered = false;

    if (power->_gpioHandle)
    {
        NEXUS_Gpio_ClearInterrupt(power->_gpioHandle);
    }
}

Exchange::IPower::PCStatus PowerImplementation::SetStandbyState()
{
    TRACE(Trace::Information, (_T("SetStandbyState")));
    NxClient_StandbyStatus standbyStatus;
    NxClient_GetStandbyStatus(&standbyStatus);

    if (_mode == standbyStatus.settings.mode) { // both modes are same
        if ((_mode == NEXUS_PlatformStandbyMode_eDeepSleep) && (_coldBoot)) {
            TRACE(Trace::Information, (_T("Requesting for Cold Boot")));
        } else {
            TRACE(Trace::Information, (_T("Both Modes are same")));
            return PCSameMode;
        }
    } else {
        TRACE(Trace::Information, (_T("Requesting for new state")));
    }

    PCStatus status = SetPowerState();
    if (status == PCSuccess) {
        if (_mode == NEXUS_PlatformStandbyMode_eOn) {
            return PCSuccess;
        }
        unsigned count = REPEAT_COUNT;

        // Wait for nexus to enter standby.
        while (count) {
            NxClient_GetStandbyStatus(&standbyStatus);
            if (standbyStatus.transition == NxClient_StandbyTransition_eDone)
                break;
            BKNI_Sleep(1000);
            count--;
        }

        // Return if standby fails.
        if (standbyStatus.settings.mode == NEXUS_PlatformStandbyMode_eOn) {
            return PCFailure;
        }

        NEXUS_Error rc;
        if (_mode == NEXUS_PlatformStandbyMode_eActive || !_pmContext) {
            uint32_t timeout;
            if (_timeout)
                timeout = _timeout * 1000;
            else
                timeout = BKNI_INFINITE;
 
            rc = BKNI_WaitForEvent(_wakeupEvent, timeout);
        } else if (_mode == NEXUS_PlatformStandbyMode_ePassive) {
            brcm_pm_suspend(_pmContext, BRCM_PM_STANDBY);
        } else if (_mode == NEXUS_PlatformStandbyMode_eDeepSleep) {
            if (!_coldBoot) {
                brcm_pm_suspend(_pmContext, BRCM_PM_SUSPEND);
            } else {
                system("poweroff");
            }
        }

        PrintWakeup();
        NxClient_GetStandbyStatus(&standbyStatus);
        if ((rc == NEXUS_TIMEOUT) || standbyStatus.status.wakeupStatus.timeout ||
            standbyStatus.status.wakeupStatus.ir || _eventTriggered) {
            _mode = NEXUS_PlatformStandbyMode_eOn;
            BKNI_SetEvent(_event);
            _eventTriggered = false;
        }
    }
    return status;
}

Exchange::IPower::PCStatus PowerImplementation::SetPowerState()
{
    TRACE(Trace::Information, (_T("SetPowerState")));
    NxClient_StandbySettings standbySettings;

    NxClient_GetDefaultStandbySettings(&standbySettings);
    standbySettings.settings.mode = _mode;
    standbySettings.settings.wakeupSettings.ir = true;
    standbySettings.settings.wakeupSettings.gpio = true;
    standbySettings.settings.wakeupSettings.timeout = _timeout;

    NEXUS_Error rc = NxClient_SetStandbySettings(&standbySettings);
    if (rc)
        return PCFailure;

    return PCSuccess;
}

void PowerImplementation::SetWakeEvent()
{
    TRACE(Trace::Information, (_T("SetWakeEvent")));
    if (_mode != NEXUS_PlatformStandbyMode_eOn) {
        if (_mode == NEXUS_PlatformStandbyMode_eActive) {
            BKNI_SetEvent(_wakeupEvent);
        }
    }
}

Exchange::IPower::PCStatus PowerImplementation::SetState(const Exchange::IPower::PCState state, const uint32_t timeout)
{
    TRACE(Trace::Information, (_T("SetState state is [%d], Timeout is : [%d]"), state,timeout));
    PCStatus status = PCSuccess;
    _timeout = timeout;

    switch (state) {
    case On:
        _mode = NEXUS_PlatformStandbyMode_eOn;
        break;
    case ActiveStandby:
        _mode = NEXUS_PlatformStandbyMode_eActive;
        break;
    case PassiveStandby:
        _mode = NEXUS_PlatformStandbyMode_ePassive;
        break;
    case SuspendToRAM:
        _mode = NEXUS_PlatformStandbyMode_eDeepSleep;
        break;
    case Hibernate:
        status = PCNotSupportedState;
        break;
    case PowerOff:
        _mode = NEXUS_PlatformStandbyMode_eDeepSleep;
        _coldBoot = true;
        break;
    default:
        status = PCInvalidState;
        break;
    }

    if (status == PCSuccess) {
        _lock.Lock();
        _isSetState= true;
        _lock.Unlock();
    } else {
        TRACE(Trace::Error, (_T("SetPowerState given state is not supported!!!")));
    }

    return status;
}

Exchange::IPower::PCState PowerImplementation::GetState() const
{
    TRACE(Trace::Information, (_T("GetState")));
    Exchange::IPower::PCState state;
    NxClient_StandbyStatus standbyStatus;
    NxClient_GetStandbyStatus(&standbyStatus);

    switch (standbyStatus.settings.mode) {
    case NEXUS_PlatformStandbyMode_eOn:
        state = Exchange::IPower::On;
        break;
    case NEXUS_PlatformStandbyMode_eActive:
        state = Exchange::IPower::ActiveStandby;
        break;
    case NEXUS_PlatformStandbyMode_ePassive:
        state = Exchange::IPower::PassiveStandby;
        break;
    case NEXUS_PlatformStandbyMode_eDeepSleep:
        state = Exchange::IPower::SuspendToRAM;
        break;
    default:
        ASSERT(false); // Invalid state.
    }
    return state;
}
