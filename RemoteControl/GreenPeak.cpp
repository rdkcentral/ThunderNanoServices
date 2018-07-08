/*****************************************************************************
 *                    Includes Definitions
 *****************************************************************************/
#define _TRACE_LEVEL 3
#include "GreenPeak.h"
#include "RemoteAdministrator.h"

//#define GP_DEVICE_NAME "/dev/gpK5"
#define GP_DEVICE_NAME "/dev/gpK7C"


extern "C" {

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP
#define GP_MODULE_ID GP_MODULE_ID_APP

#include <gpAssert.h>
#include <gpBaseComps.h>
#include <gpBsp.h>
#include <gpLog.h>
#include <gpMacDispatcher.h>
#include <gpPd.h>
#include <gpReset.h>
#include <gpRf4ce.h>
//#include <gpRf4ceBindAuto.h>
#include <gpRf4ceBindValidation.h>
#include <gpRf4ceDispatcher.h>
#include <gpRf4ceUserControl.h>
#include <gpSched.h>
#include <gpVersion.h>
#include <hal.h>

#define GP_COMPONENT_ID_ZRCRECIPIENT 130    // XXX: defined in rf4ce-hal-gp make file
#define GP_COMPONENT_ID_MSORECIPIENT 152
#define GP_COMPONENT_ID_RF4CEBINDGDP 49

#include <gpZrc.h>
#include <gpMsoRecipient.h>
#include <gpNvm.h>

//#include <gpApplication_MW.h>
void gpApplication_IndicateBindSuccessToMiddleware(UInt8 bindingRef, UInt8 profileId);
void gpApplication_IndicateBindFailureToMiddleware(UInt8 result);

/*****************************************************************************
 *                    Macro Definitions
 *****************************************************************************/
#define NUM_OF_EL_DEV_TYPE_LIST 3
#define NUM_OF_EL_PROFILE_ID_LIST 2
#define APP_CAPABILITIES_LISTS (((((NUM_OF_EL_DEV_TYPE_LIST) << GP_RF4CE_APP_CAPABILITY_NBR_DEVICES_IDX) & GP_RF4CE_APP_CAPABILITY_NBR_DEVICES_BM)) | (BM(GP_RF4CE_APP_CAPABILITY_USER_STRING_IDX)) | ((((NUM_OF_EL_PROFILE_ID_LIST) << GP_RF4CE_APP_CAPABILITY_NBR_PROFILES_IDX) & GP_RF4CE_APP_CAPABILITY_NBR_PROFILES_BM)))

#define TARGET_NODE_CAPABILITIES 0x0F // Normalization - security - power supply - target

gpRf4ce_VendorString_t Application_VendorString = { XSTRINGIFY(GP_RF4CE_NWKC_VENDOR_STRING) };

static Bool _resetWithColdStart;
static int _threadResult;


/********************************************************************************
 *                  Polling Renegotiation
 ********************************************************************************/
static void Application_ConfigurePolling(void)
{
    gpZrc_Msg_t messageZrc;

    messageZrc.RecipientSetPollingConfiguration.methodIdentifier = gpRf4ceCmd_AplGdpPollMethod_HeartBeat;
    messageZrc.RecipientSetPollingConfiguration.triggerConfiguration = 0x01;
    messageZrc.RecipientSetPollingConfiguration.keyPressInt = 0x00;
    messageZrc.RecipientSetPollingConfiguration.timeInterval = 30000; //default 30s
    messageZrc.RecipientSetPollingConfiguration.timeout = 0x64;

    gpZrc_Set(gpZrc_MsgId_RecipientSetPollingConfiguration, &messageZrc);
}

/*****************************************************************************
 *                  ZRC
 *****************************************************************************/
static void gpApplication_ZRCBindSetup(Bool AllowInteractiveValidation, Bool PushButton)
{
    gpZrc_Result_t result = gpZrc_ResultSuccess;
    gpZrc_Msg_t messageZrc;

    result = gpZrc_GetDefault(gpZrc_MsgId_RecipientSetOriginatorConfiguration, &messageZrc);

    if(result == gpZrc_ResultSuccess) {
        /* update a field if needed (application override) */
        messageZrc.RecipientSetOriginatorConfiguration.devType = gpRf4ce_DeviceTypeWildcard;
        messageZrc.RecipientSetOriginatorConfiguration.primaryClassDescriptor = PushButton?0x21:0x15;
        messageZrc.RecipientSetOriginatorConfiguration.secondaryClassDescriptor = PushButton?0x0C:0x16;
        messageZrc.RecipientSetOriginatorConfiguration.tertiaryClassDescriptor = PushButton?0x0e:0x7;

        result = gpZrc_Set(gpZrc_MsgId_RecipientSetOriginatorConfiguration, &messageZrc);
        if(result == gpZrc_ResultSuccess) {
            messageZrc.RecipientInteractiveValidationSetup.interactiveValidationAllowed = PushButton ? false : AllowInteractiveValidation;
            result = gpZrc_Set(gpZrc_MsgId_RecipientInteractiveValidationSetup, &messageZrc);
            if(result != gpZrc_ResultSuccess) {
                TRACE_L1("FAIL in set gpZrc_MsgId_RecipientInteractiveValidationSetup (0x%x)", result);
                return;
            }
        }
        else {
            TRACE_L1("FAIL in set gpZrc_MsgId_RecipientSetOriginatorConfiguration (0x%x)", result);
            return;
        }
    }
    else {
        TRACE_L1("FAIL in get default of gpZrc_MsgId_RecipientSetOriginatorConfiguration (0x%x)", result);
        return;
    }

    if(PushButton) {
        result = gpZrc_Msg(gpZrc_MsgId_RecipientPushButton, NULL);
        TRACE_L1("%s: gpZrc_MsgId_RecipientPushButton result (0x%x)", __FUNCTION__, result);
        if(result != gpZrc_ResultSuccess) {
            TRACE_L1("FAIL in set of gpZrc_MsgId_RecipientPushButton (0x%x)", result);
            return;
        }
    }
}

static void gpApplication_ZRCUnbind(UInt8 bindingId)
{
    gpZrc_Result_t result = gpZrc_ResultSuccess;
    gpZrc_Msg_t messageZrc;

    messageZrc.UnbindRequest.bindingId = bindingId;
    result = gpZrc_Msg(gpZrc_MsgId_UnbindRequest, &messageZrc);
}

#define NVM_TAG_PROFILEIDLIST       0x00
static Bool SendRemappableData = true;
gpRf4ce_ProfileId_t gpProfileIdList[GP_RF4CE_NWKC_MAX_PAIRING_TABLE_ENTRIES];

static void gpApplication_BindConfirm(gpRf4ce_Result_t result , UInt8 bindingRef, UInt8 profileId)
{

    if(result == gpRf4ce_ResultSuccess) {
        if(bindingRef < GP_RF4CE_NWKC_MAX_PAIRING_TABLE_ENTRIES) {
            gpProfileIdList[bindingRef] = profileId;
            /*Backup NVM Data*/
            //printf("gpNvm_Backup\n");
            //gpNvm_Backup(GP_COMPONENT_ID, NVM_TAG_PROFILEIDLIST, NULL);       // XXX: core
        }

        if(profileId == gpRf4ce_ProfileIdZrc2_0) {
            //gpApplication_ConfigureActionMapping(bindingRef);
            //gpApplication_SetActionMapping(bindingRef, SendRemappableData);
            //gpSched_ScheduleEvent(0, Application_ConfigurePolling);
        }

        gpApplication_IndicateBindSuccessToMiddleware(bindingRef, profileId);
    }
    else
    {
        gpApplication_IndicateBindFailureToMiddleware(result);
    }

    gpApplication_ZRCBindSetup(true,false);
}

void gpZrc_cbMsg(gpZrc_MsgId_t MsgId, UInt8 length, gpZrc_Msg_t* pMsg)
{
    UIntLoop index=0;

    switch(MsgId) {
        case gpZrc_MsgId_cbRecipientBindingIndication: {
            TRACE_L1("%s: gpZrc_MsgId_cbRecipientBindingIndication MsgId 0x%x", __FUNCTION__, MsgId);
            break;
        }

        case gpZrc_MsgId_cbRecipientBindingConfirm: {
            TRACE_L1("%s: gpZrc_MsgId_cbRecipientBindingConfirm MsgId 0x%x", __FUNCTION__, MsgId);
            gpRf4ce_ProfileId_t ProfileId = gpRf4ce_ProfileIdZrc;

            if(pMsg->cbRecipientBindingConfirm.result == gpRf4ce_ResultSuccess) {
                for(index=0;index < pMsg->cbRecipientBindingConfirm.profileIdListSize; index++) {
                    if(pMsg->cbRecipientBindingConfirm.pProfileIdList[index] == gpRf4ce_ProfileIdZrc2_0) {
                        ProfileId = gpRf4ce_ProfileIdZrc2_0;
                        break;
                    }
                }
            }
            gpApplication_BindConfirm(pMsg->cbRecipientBindingConfirm.result, pMsg->cbRecipientBindingConfirm.bindingRef, ProfileId);
            break;
        }

        case gpZrc_MsgId_cbRecipientPowerStatusNotifyIndication: {
            TRACE_L4("%s: gpZrc_MsgId_cbRecipientPowerStatusNotifyIndication MsgId 0x%x", __FUNCTION__, MsgId);
            break;
        }

        case gpZrc_MsgId_cbRecipientValidationResultIndication: {
            TRACE_L4("%s: gpZrc_MsgId_cbRecipientValidationResultIndication MsgId 0x%x", __FUNCTION__, MsgId);
            break;
        }

        case gpMsoRecipient_MsgId_ValidationComplete:{
            TRACE_L4("%s: gpMsoRecipient_MsgId_ValidationComplete MsgId 0x%x", __FUNCTION__, MsgId);
            break;
        }

        case gpZrc_MsgId_cbRecipientActionCodesIndicationNoPayload: {
            for(index=0; index < pMsg->cbActionCodesIndicationNoPayload.numberOfActions; index++) {

                UInt8 keyCode = pMsg->cbActionCodesIndicationNoPayload.ActionRecordList[index].ActionCode;
                switch(pMsg->cbActionCodesIndicationNoPayload.ActionRecordList[index].ActionControl)
                {
                    case gpRf4ceCmd_ZrcActionTypeAtomic:
                    case gpRf4ceCmd_ZrcActionTypeStart: {
                        TRACE_L1("%s: Key Pressed keyCode=0x%x", __FUNCTION__, keyCode);
                        WPEFramework::Plugin::GreenPeak::Dispatch(true, keyCode, 0);
                        break;
                    }
                    case gpRf4ceCmd_ZrcActionTypeReleased: {
                        TRACE_L1("%s: Key Released keyCode=0x%x", __FUNCTION__, keyCode);
                        WPEFramework::Plugin::GreenPeak::Dispatch(false, keyCode, 0);
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            break;

        }

        case gpZrc_MsgId_cbUnbindConfirm:
            TRACE_L1("%s: gpZrc_MsgId_cbUnbindConfirm", __FUNCTION__);
            if (pMsg->cbUnbindConfirm.status == gpRf4ce_ResultNoPairing) {
                TRACE_L1("Invalid binding id (0x%x)", pMsg->cbUnbindConfirm.bindingId);
            }
            // XXX: gpRf4ce_ResultNoAck (0xe9) ???
            break;

        case gpZrc_MsgId_cbUnbindIndication:
            TRACE_L1("%s: gpZrc_MsgId_cbUnbindIndication bindingId=%d status=0x%x", __FUNCTION__, pMsg->cbUnbindConfirm.bindingId, pMsg->cbUnbindConfirm.status);
            break;

        default:
            TRACE_L1("%s: MsgId 0x%x", __FUNCTION__, MsgId);
            break;
    }

}

/******************************************************************************
 *                   gpMsoRecipient Callbacks
 ******************************************************************************/
gpMsoRecipient_Result_t gpMsoRecipient_cbMsg(gpMsoRecipient_MsgId_t msgId, UInt8 index, UInt8 length, gpMsoRecipient_Msg_t* pMsg)
{
}

/*****************************************************************************
 *                    Static Data Definitions
 *****************************************************************************/
static Bool DispatcherDataIndication(UInt8 pairingRef, gpRf4ce_ProfileId_t profileId, gpRf4ce_VendorId_t vendorId, UInt8 rxFlags, UInt8 nsduLength, gpPd_Offset_t nsduOffset, gpPd_Handle_t pdHandle)
{
    if (profileId == gpRf4ce_ProfileIdZid) {
        static UInt16 lastCode = 0;
        UInt16 code;

        // ZID frame is not decoded. By watching the sniffer the key code is at position 4 in the string.
        // modifiers = gpPd_ReadByte(pdHandle, nsduOffset+0) | ( gpPd_ReadByte(pdHandle, nsduOffset+1) << 8) | ( gpPd_ReadByte(pdHandle, nsduOffset+2) << 16);
        code = gpPd_ReadByte(pdHandle, nsduOffset + 4) | (gpPd_ReadByte(pdHandle, nsduOffset + 3) << 8);

        if (code == 0) {
            WPEFramework::Plugin::GreenPeak::Dispatch(false, lastCode, 0);
            lastCode = 0;
        }
        else {
            lastCode = code | 0x8000;
            WPEFramework::Plugin::GreenPeak::Dispatch(true, lastCode, 0);
        }

        gpPd_FreePd(pdHandle);
        return true;
    }
    else {
        return false;
    }
}

void DispatcherResetIndication(Bool setDefault)
{
    // called
}

/*****************************************************************************
 *                    gpRf4ceUserControl Callbacks
 *****************************************************************************/
static uint16_t _releasedCode = static_cast<uint16_t>(~0);
static uint8_t  _binidngId = static_cast<uint8_t>(~0);

static void KeyReleased()
{
    if (_releasedCode != static_cast<uint16_t>(~0)) {
        WPEFramework::Plugin::GreenPeak::Dispatch(false, _releasedCode, 0);
        _releasedCode = static_cast<uint16_t>(~0);
    }
}

void gpRf4ceUserControl_cbUserControlIndication(
    UInt8 bindingId,
    gpRf4ce_ProfileId_t profileId,
    gpRf4ce_VendorId_t vendorId,
    gpRf4ceUserControl_KeyStatus_t keyStatus,
    gpRf4ceUserControl_CommandCode_t commandCode,
    Int8 signalStrength,
    UInt8 linkQuality,
    UInt8 rxFlags)
{

    if (keyStatus == gpRf4ceUserControl_KeyStatusReleased) {
        // Do not send the release code right away as this might be a retrigger of the already pressed key.
        // We want to prevent these additional messages, as an external timer will check for repeat keys
        // on a higher level, we should only report a key up, if the key is actually up.
        _releasedCode = commandCode;

        // If within 10ms we receive the same key down, it is definitely a "repeat" key...
        gpSched_ScheduleEvent(10000, KeyReleased);
    }
    else if ((_releasedCode == static_cast<uint16_t>(~0)) || (_releasedCode != commandCode)) {
        if (_releasedCode != static_cast<uint16_t>(~0)) {
            WPEFramework::Plugin::GreenPeak::Dispatch(false, _releasedCode, 0);
            _releasedCode = static_cast<uint16_t>(~0);
        }

        WPEFramework::Plugin::GreenPeak::Dispatch((keyStatus == gpRf4ceUserControl_KeyStatusPressed), commandCode, 0);
    }
    else {
        _releasedCode = static_cast<uint16_t>(~0);
    }
}

/*****************************************************************************
 *                    gpRf4ceBindAuto Callbacks
 *****************************************************************************/

void gpRf4ceBindAuto_cbRecipientBindConfirm(UInt8 bindingId, gpRf4ce_Result_t status)
{
    string text;

    if (status == gpRf4ce_ResultSuccess) {
        text = WPEFramework::Trace::Format(_T("Pairing successfull. ID: [%d]"), static_cast<uint16_t>(bindingId));
    }
    else {
        text = WPEFramework::Trace::Format(_T("Pairing Failed. Error: [%X]"), static_cast<uint16_t>(status));
    }

    WPEFramework::Plugin::GreenPeak::Report(text);
}

void gpRf4ceBindAuto_cbUnbindIndication(UInt8 bindingId)
{
    TRACE_L1("Unbind Indication id: %u", 0, (UInt16)bindingId);
}

void gpRf4ceBindAuto_cbUnbindConfirm(UInt8 bindingId, gpRf4ce_Result_t status)
{
    TRACE_L1("Unbind Confirm id: %u", 0, (UInt16)bindingId);
}

/*****************************************************************************
 *                    gpRf4ceDispatcher Callbacks
 *****************************************************************************/
static void cbInitializationDone(void)
{
    gpVersion_ReleaseInfo_t version;
    Int8 DefaultTXPowers[16];

    MEMSET(DefaultTXPowers, 3, 16);

    gpVersion_GetSoftwareVersion(&version);

    gpRf4ce_SetRxOn(true);
    gpMacDispatcher_SetTransmitPower(3, 0);
    gpMacDispatcher_SetDefaultTransmitPowers(DefaultTXPowers);

    {
        char userString[20] = "Samsung&UPC";
        gpRf4ce_SetUserString(userString);
    }

    gpApplication_ZRCBindSetup(true,false);

    WPEFramework::Plugin::GreenPeak::Initialized(
        static_cast<uint16_t>(version.major),
        static_cast<uint16_t>(version.minor),
        static_cast<uint16_t>(version.revision),
        static_cast<uint16_t>(version.patch),
        static_cast<uint32_t>(gpVersion_GetChangelist()));
}

void gpRf4ceDispatcher_cbResetConfirm(gpRf4ce_Result_t status)
{
    gpRf4ce_SetVendorId(GP_RF4CE_NWKC_VENDOR_IDENTIFIER);
    gpRf4ce_SetVendorString(&Application_VendorString);

    if (_resetWithColdStart) {
        gpRf4ce_StartRequest();
    }
    else {
        gpSched_ScheduleEvent(0, cbInitializationDone);
    }
}

void gpRf4ce_cbStartConfirm(gpRf4ce_Result_t status)
{
    cbInitializationDone();
}

void hal_ResetUc(void)
{
}

extern GP_RF4CE_DISPATCHER_CONST gpRf4ceDispatcher_DataCallbacks_t
    ROM gpApp_DataCallbacks
        FLASH_PROGMEM;
extern GP_RF4CE_DISPATCHER_CONST gpRf4ceDispatcher_BindCallbacks_t
    ROM gpApp_BindCallbacks
        FLASH_PROGMEM;

GP_RF4CE_DISPATCHER_CONST gpRf4ceDispatcher_DataCallbacks_t
    ROM gpApp_DataCallbacks
        FLASH_PROGMEM
    = {
        DispatcherResetIndication,
        DispatcherDataIndication,
        nullptr,
        nullptr,
        nullptr
      };

GP_RF4CE_DISPATCHER_CONST gpRf4ceDispatcher_BindCallbacks_t
    ROM gpApp_BindCallbacks
        FLASH_PROGMEM
    = {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
      };

void target_DoR4ceReset(void)
{
    _resetWithColdStart = gpRf4ce_IsColdStartAdvised(TARGET_NODE_CAPABILITIES);
    gpRf4ceDispatcher_ResetRequest(_resetWithColdStart, TARGET_NODE_CAPABILITIES);
}

void target_ActivatePairing()
{
    WPEFramework::Plugin::GreenPeak::Report(string("Entering the PairingMode."));
    #if 1
        gpApplication_ZRCBindSetup(false,true);
    #else
    static const gpRf4ceBindAuto_RecDiscParams_t discoveryParameters = {
        0x15,
        { gpRf4ce_DeviceTypeTelevision,
            gpRf4ce_DeviceTypeSetTopBox,
            gpRf4ce_DeviceTypeGeneric },
        { gpRf4ce_ProfileIdZrc,
            gpRf4ce_ProfileIdZid }
    };

    gpRf4ceBindAuto_RecipientBindRequest(&discoveryParameters);
    #endif
}

void target_ActivateUnpairing()
{
    WPEFramework::Plugin::GreenPeak::Report(string("Unpairing."));
    gpApplication_ZRCUnbind(_binidngId);
}

static void terminateThread()
{
    printf("Terminating the GreenPeak dedicated thread.\n");
    pthread_exit(&_threadResult);
    printf("After the terminating GreenPeak dedicated thread.\n");
}

void Application_Init()
{
    gpBaseComps_StackInit();
    gpSched_ScheduleEvent(0, target_DoR4ceReset);
}

// Missing -DMAIN_FUNCTION_NAME=gpMain
MAIN_FUNCTION_RETURN_TYPE gpMain()
{
    gpSched_Main_Init();
    for (;;)  {
        // Check if the system can go to sleep
        gpSched_GoToSleep();
        gpSched_Main_Body();
    }
    MAIN_FUNCTION_RETURN_VALUE;
}

} // extern "C"



/*****************************************************************************
*  GreenPeak C++ Wrapper class. Hide C artifacts
*****************************************************************************/

namespace WPEFramework {
namespace Plugin {

    static GreenPeak* _singleton(Core::Service<GreenPeak>::Create<GreenPeak>());

    GreenPeak::Activity::Activity()
        : Core::Thread(Core::Thread::DefaultStackSize(), _T("GreenPeak"))
    {
    }

    void GreenPeak::Activity::Dispose()
    {
        gpSched_ScheduleEvent(0, terminateThread);
        Core::Thread::SignalTermination();

        // Wait till gpMain has ended......
        TRACE_L1("%s: Waiting for GreenPeakDriver to stop:", __FUNCTION__);
        Wait(Thread::STOPPED | Thread::BLOCKED);
        TRACE_L1("%s: Done!!! ", __FUNCTION__);
    }

    /* virtual */ GreenPeak::Activity::~Activity()
    {
    }

    /* virtual */ uint32_t GreenPeak::Activity::Worker()
    {
        gpMain();

        Block();
        return (Core::infinite);
    }

    GreenPeak::GreenPeak()
        : _readySignal(false, true)
        , _callback(nullptr)
        , _worker()
        , _error(static_cast<uint32_t>(~0))
        , _present(WPEFramework::Core::File(_T(GP_DEVICE_NAME), false).Exists())
        , _codeMask(0)
        , _resourceName("GreenPeakRF4CE")
    {
        if (_present == true) {
            _worker.Run();
            Remotes::RemoteAdministrator::Instance().Announce(*this);
        }
    }

    GreenPeak::~GreenPeak()
    {
        if (_present == true) {
            Remotes::RemoteAdministrator::Instance().Revoke(*this);
            _worker.Dispose();
        }
    }

    bool GreenPeak::WaitForReady(const uint32_t time) const
    {
        return (_readySignal.Lock(time) == 0);
    }

    /* virtual */ uint32_t GreenPeak::Error() const
    {
        return (_error);
    }

    /* virtual */ bool GreenPeak::Pair()
    {
        bool activated = false;

        _adminLock.Lock();

        if (_info.Major.IsSet() == true) {
            gpSched_ScheduleEvent(0, target_ActivatePairing);
            activated = true;
        }

        _adminLock.Unlock();

        return (activated);
    }


    /* virtual */ bool GreenPeak::Unpair(string bindingId)
    {
        bool unpaired = false;

        _adminLock.Lock();

        if (_info.Major.IsSet() == true) {
            _binidngId = atoi(bindingId.c_str());
            gpSched_ScheduleEvent(0, target_ActivateUnpairing);
            unpaired = true;
        }

        _adminLock.Unlock();

        return (unpaired);
    }

    /* virtual */ uint32_t GreenPeak::Callback(Exchange::IKeyHandler* callback)
    {
        _adminLock.Lock();

        // This is a 1-1 relation. No way you can set more than
        // 1 listener. First clear the previous one (nullptr) before
        // Setting a new one.
        ASSERT((callback != nullptr) ^ (_callback != nullptr));

        if (callback == nullptr) {
            // We are unlinked. Deinitialize the stuff.
            // Deinitialize();
            _callback = nullptr;
        }
        else {
            // Initialize();
            TRACE_L1("%s: callback=%p _callback=%p", __FUNCTION__, callback, _callback);
            _callback = callback;
        }

        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }

    /* virtual */ string GreenPeak::MetaData() const
    {
        string result;

        _adminLock.Lock();

        _info.ToString(result);

        _adminLock.Unlock();

        return (result);
    }

    void GreenPeak::_Dispatch(const bool pressed, const uint16_t code, const uint16_t modifiers)
    {
        _adminLock.Lock();

        if (_callback != nullptr) {
            _callback->KeyEvent(pressed, (code | (modifiers << 16)), _resourceName);
        }

        _adminLock.Unlock();
    }

    /* static */ void GreenPeak::Dispatch(const bool pressed, const uint16_t code, const uint16_t modifiers)
    {
        _singleton->_Dispatch(pressed, code, modifiers);
    }

    void GreenPeak::_Initialized(const uint16_t major,
        const uint16_t minor,
        const uint16_t revision,
        const uint16_t patch,
        const uint32_t change)
    {
        _info.Major = major;
        _info.Minor = minor;
        _info.Revision = revision;
        _info.Patch = patch;
        _info.Change = change;

        _error = 0;
        _readySignal.Unlock();
    }

    /* static */ void GreenPeak::Initialized(const uint16_t major,
        const uint16_t minor,
        const uint16_t revision,
        const uint16_t patch,
        const uint32_t change)
    {
        _singleton->_Initialized(major, minor, revision, patch, change);
    }

    /* static */ void GreenPeak::Report(const string& text)
    {
        TRACE_GLOBAL(Trace::Information, (_T("RF4CE Report: %s"), text.c_str()));
    }
}
}
