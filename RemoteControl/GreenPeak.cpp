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

#include "Module.h"
#include "RemoteAdministrator.h"

#include <sys/syscall.h>

extern "C" {

static void terminateThread(void);
static void target_DoR4ceReset(void);
static void target_ActivatePairing(void);
static void target_ActivateUnpairing(void);

static uint8_t _bindingId = static_cast<uint8_t>(~0);

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP
#define GP_MODULE_ID GP_MODULE_ID_APP

#include <gpAssert.h>
#include <gpBaseComps.h>
#include <gpBsp.h>
#include <gpLog.h>
#include <gpMacDispatcher.h>
#include <gpMsoRecipient.h>
#include <gpNvm.h>
#include <gpPd.h>
#include <gpReset.h>
#include <gpRf4ce.h>
#include <gpRf4ceBindValidation.h>
#include <gpRf4ceDispatcher.h>
#include <gpRf4ceUserControl.h>
#include <gpSched.h>
#include <gpUtils.h>
#include <gpVersion.h>
#include <gpZrc.h>
#include <hal.h>
}

/*****************************************************************************
 *  GreenPeak C++ Wrapper class. Hide C artifacts
 *****************************************************************************/

namespace WPEFramework {
namespace Plugin {

    static uint32_t LoadModule(const string& moduleName)
    {
        uint32_t result(Core::ERROR_OPENING_FAILED);
        int fd(::open(moduleName.c_str(), O_RDONLY));

        if (fd >= 0) {
            const char params[] = "";
            struct stat st;
            fstat(fd, &st);
            void* image = malloc(st.st_size);
            read(fd, image, st.st_size);
            close(fd);
            if (::syscall(__NR_init_module, image, st.st_size, params) != 0) {
                perror("init_module failed");
                result = Core::ERROR_BAD_REQUEST;
            } else {
                result = Core::ERROR_NONE;
            }
            ::free(image);
        }

        return result;
    }

    static uint32_t UnloadModule(const string& moduleName)
    {

        if (syscall(__NR_delete_module, moduleName.c_str(), O_NONBLOCK | O_TRUNC) != 0) {
            return (Core::ERROR_BAD_REQUEST);
        }
        return (Core::ERROR_NONE);
    }

    class GreenPeak : public Exchange::IKeyProducer {
    private:
        GreenPeak(const GreenPeak&) = delete;
        GreenPeak& operator=(const GreenPeak&) = delete;

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , RemoteId(_T("Samsung&UPC"))
                , Module()
                , NodeId()
                , AutoPairing(false)
            {
                Add(_T("remoteid"), &RemoteId);
                Add(_T("module"), &Module);
                Add(_T("nodeid"), &NodeId);
                Add(_T("autopairing"), &AutoPairing);
            }
            ~Config()
            {
            }

            Core::JSON::String RemoteId;
            Core::JSON::String Module;
            Core::JSON::DecUInt8 NodeId;
            Core::JSON::Boolean AutoPairing;
        };
        class Activity : public Core::Thread {
        private:
            Activity(const Activity&) = delete;
            Activity& operator=(const Activity&) = delete;

        public:
            Activity()
                : Core::Thread(Core::Thread::DefaultStackSize(), _T("GreenPeak"))
            {
            }
            virtual ~Activity()
            {
            }

        public:
            void Dispose()
            {
                Stop();
                gpSched_ScheduleEvent(0, terminateThread);

                // Wait till gpMain has ended......
                TRACE(Trace::Information, (_T("%s: Waiting for GreenPeakDriver to stop:"), __FUNCTION__));
                Wait(Thread::STOPPED | Thread::BLOCKED);
                TRACE(Trace::Information, (_T("%s: Done!!! "), __FUNCTION__));
            }

        private:
            virtual uint32_t Worker() override
            {
                GP_UTILS_INIT_STACK();
                HAL_INITIALIZE_GLOBAL_INT();

                // Hardware initialisation
                HAL_INIT();

                // Radio interrupts that occur will only be handled later on in the main loop
                // Other interrupt source do not trigger any calls to blocks that are not initialized yet
                HAL_ENABLE_GLOBAL_INT();

                gpBaseComps_StackInit();
                gpSched_ScheduleEvent(0, target_DoR4ceReset);
                GP_UTILS_DUMP_STACK_POINTER();
                GP_UTILS_CHECK_STACK_PATTERN();
                GP_UTILS_CHECK_STACK_POINTER();

                while (IsRunning()) {
                    // Check if the system can go to sleep
                    gpSched_GoToSleep();
                    gpSched_Main_Body();
                }
                hal_DeInit();
                return (Core::infinite);
            }
        };

        class Info : public Core::JSON::Container {
        private:
            Info(const Info&) = delete;
            Info& operator=(const Info&) = delete;

        public:
            Info()
                : Core::JSON::Container()
                , Major(static_cast<uint16_t>(~0))
                , Minor(static_cast<uint16_t>(~0))
                , Revision(static_cast<uint16_t>(~0))
                , Patch(static_cast<uint16_t>(~0))
                , Change(static_cast<uint16_t>(~0))
            {
                Add(_T("major"), &Major);
                Add(_T("minor"), &Minor);
                Add(_T("revision"), &Revision);
                Add(_T("patch"), &Patch);
                Add(_T("change"), &Change);
            }
            ~Info()
            {
            }

        public:
            Core::JSON::DecUInt16 Major;
            Core::JSON::DecUInt16 Minor;
            Core::JSON::DecUInt16 Revision;
            Core::JSON::DecUInt16 Patch;
            Core::JSON::DecUInt16 Change;
        };

    public:
        GreenPeak()
            : _readySignal(false, true)
            , _callback(nullptr)
            , _worker()
            , _error(static_cast<uint32_t>(~0))
            , _loadedModule()
            , _pairing(false)
        {
            Remotes::RemoteAdministrator::Instance().Announce(*this);
        }
        virtual ~GreenPeak()
        {
            Remotes::RemoteAdministrator::Instance().Revoke(*this);
        }

    public:
        inline bool WaitForReady(const uint32_t time) const
        {
            return (_readySignal.Lock(time) == 0);
        }
        string Name() const override
        {
            return (_resourceName.c_str());
        }
        virtual uint32_t Error() const override
        {
            return (_error);
        }
        virtual bool Pair() override
        {
            bool activated = false;

            _adminLock.Lock();

            if (!_pairing) {
                if (_info.Major.IsSet() == true) {
                    gpSched_ScheduleEvent(0, target_ActivatePairing);
                    _pairing = true;
                }
            }
            else {
                TRACE(Trace::Error, (_T("The device is already in pairing mode")));
            }
            activated = _pairing;

            _adminLock.Unlock();

            return (activated);
        }
        virtual bool Unpair(string bindingId) override
        {
            bool unpaired = false;

            _adminLock.Lock();

            if (_info.Major.IsSet() == true) {
                _bindingId = atoi(bindingId.c_str());
                gpSched_ScheduleEvent(0, target_ActivateUnpairing);
                unpaired = true;
            }

            _adminLock.Unlock();

            return (unpaired);
        }
        virtual uint32_t Callback(Exchange::IKeyHandler* callback) override
        {
            _adminLock.Lock();

            // This is a 1-1 relation. No way you can set more than
            // 1 listener. First clear the previous one (nullptr) before
            // Setting a new one.
            ASSERT((callback != nullptr) ^ (_callback != nullptr));

            if (callback == nullptr) {
                // We are unlinked. Deinitialize the stuff.
                _callback = nullptr;
                _worker.Dispose();

                if (_loadedModule.empty() == false) {
                    UnloadModule(_loadedModule.c_str());
                }
            } else {
                // Initialize();
                TRACE(Trace::Information, (_T("%s: callback=%p _callback=%p"), __FUNCTION__, callback, _callback));
                _callback = callback;
            }

            _adminLock.Unlock();

            return (Core::ERROR_NONE);
        }
        virtual string MetaData() const override
        {
            string result;

            _adminLock.Lock();

            _info.ToString(result);

            _adminLock.Unlock();

            return (result);
        }
        virtual void Configure(const string& configure) override
        {
            Config config;
            config.FromString(configure);

            if ((config.Module.IsSet() == true) && (config.Module.Value().empty() == false)) {
                if (LoadModule(config.Module.Value()) == Core::ERROR_NONE) {
                    _loadedModule = Core::File::FileName(config.Module.Value());
                    if (config.NodeId.IsSet() == true) {
                        string nodeName("/dev/" + _loadedModule);
                        ::mknod(nodeName.c_str(), 0755 | S_IFCHR, ::makedev(config.NodeId.Value(), 0));
                    }
                }
            }

            ::strncpy(_userString, config.RemoteId.Value().c_str(), sizeof(_userString));
            _autopairing = config.AutoPairing.Value();
            _worker.Run();
        }

        BEGIN_INTERFACE_MAP(GreenPeak)
        INTERFACE_ENTRY(Exchange::IKeyProducer)
        END_INTERFACE_MAP

        // -------------------------------------------------------------------------------
        // The greenpeak library is C oriented. These methods should not be used by the
        // consumers of this class but are to link the greenpeak C works to the C++ world.
        // These methods should only be used by the GreenPeak C methods in the GreenPeak
        // implementation file.
        // -------------------------------------------------------------------------------
        static char* UserString()
        {
            return (_singleton->_userString);
        }
        static void Dispatch(const bool pressed, const uint16_t code, const uint16_t modifiers)
        {
            _singleton->_Dispatch(pressed, code, modifiers);
        }
        static void Initialized(const uint16_t major, const uint16_t minor, const uint16_t revision, const uint16_t patch, const uint32_t change)
        {
            _singleton->_Initialized(major, minor, revision, patch, change);
        }
        static void Report(const string& text)
        {
            TRACE_GLOBAL(Trace::Information, (_T("RF4CE Report: %s"), text.c_str()));
        }

        static void SendEvent(Exchange::ProducerEvents event)
        {
            _singleton->_SendEvent(event);
        }

    private:
        inline void _Dispatch(const bool pressed, const uint16_t code, const uint16_t modifiers)
        {
            _adminLock.Lock();

            if (_callback != nullptr) {
                _callback->KeyEvent(pressed, (code | (modifiers << 16)), _resourceName);
            }

            _adminLock.Unlock();
        }
        inline void _SendEvent(Exchange::ProducerEvents event) {

            // Set the _pairing variable to false, If pairing stopped
            _SetPairingState(event);

            _CheckAndStartAutoPairing(event);

            if (_callback != nullptr) {
                _callback->ProducerEvent(Name(), event);
            }
        }
        inline void _SetPairingState(const Exchange::ProducerEvents& event) {
            _adminLock.Lock();
            switch (event) {
                case Exchange::ProducerEvents::PairingFailed:
                case Exchange::ProducerEvents::PairingSuccess:
                case Exchange::ProducerEvents::PairingTimedout:
                    _pairing = false;
                    break;
                default:
                    break;
            }
            _adminLock.Unlock();
        }
        inline void _CheckAndStartAutoPairing() {
            if (!_autopairing)
            {
                TRACE(Trace::Information, (_T("AutoPairing feature disabled")));
                return;
            }

            // Start Pairing mode, If we dont have any paired devices
            uint32_t pairedRCUCount = 0;
            if (0 == (pairedRCUCount = gpRf4ce_GetNrOfPairingEntries())) {
                TRACE(Trace::Information, (_T("Started Auto-Pairing mode")));
                Pair();
            }

            TRACE(Trace::Information, (_T("No of Paired RCU : %d"), pairedRCUCount));
        }
        inline void _CheckAndStartAutoPairing(const Exchange::ProducerEvents& event) {
            if ((event == Exchange::ProducerEvents::PairingFailed) ||
                (event == Exchange::ProducerEvents::PairingTimedout)) {
                _CheckAndStartAutoPairing();
            }
        }
        inline void _Initialized(const uint16_t major,
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
            _pairing = false;

            _CheckAndStartAutoPairing();

            _readySignal.Unlock();
        }

    private:
        mutable Core::CriticalSection _adminLock;
        mutable Core::Event _readySignal;
        Exchange::IKeyHandler* _callback;
        Activity _worker;
        Info _info;
        uint32_t _error;
        string _loadedModule;
        char _userString[20];
        static const string _resourceName;
        static GreenPeak* _singleton;
        bool _pairing;
        bool _autopairing;
    };

    /* static */ const string GreenPeak::_resourceName("RF4CE");
    /* static */ GreenPeak* GreenPeak::_singleton(Core::Service<GreenPeak>::Create<GreenPeak>());
}
} // namespace WPEFramework::Plugin

/*****************************************************************************
 * All intersting C stuff. Lowlevel Greenpeak stuff...
 *****************************************************************************/

extern "C" {

using namespace WPEFramework;

void gpApplication_IndicateBindSuccessToMiddleware(UInt8 bindingRef, UInt8 profileId);
void gpApplication_IndicateBindFailureToMiddleware(UInt8 result);

/*****************************************************************************
 *                    Macro Definitions
 *****************************************************************************/
#define TARGET_NODE_CAPABILITIES 0x0F // Normalization - security - power supply - target
#define NVM_TAG_PROFILEIDLIST 0x00

static Bool _resetWithColdStart;
static Bool _SendRemappableData = true;

gpRf4ce_ProfileId_t gpProfileIdList[GP_RF4CE_NWKC_MAX_PAIRING_TABLE_ENTRIES];
gpRf4ce_VendorString_t Application_VendorString = { XSTRINGIFY(GP_RF4CE_NWKC_VENDOR_STRING) };

/* NVM LUT variables are configurable and defined in an header file. */
#include "GreenPeakNvmLut.h"

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

    if (result == gpZrc_ResultSuccess) {
        /* update a field if needed (application override) */
        messageZrc.RecipientSetOriginatorConfiguration.devType = gpRf4ce_DeviceTypeWildcard;
        messageZrc.RecipientSetOriginatorConfiguration.primaryClassDescriptor = PushButton ? 0x21 : 0x15;
        messageZrc.RecipientSetOriginatorConfiguration.secondaryClassDescriptor = PushButton ? 0x0C : 0x16;
        messageZrc.RecipientSetOriginatorConfiguration.tertiaryClassDescriptor = PushButton ? 0x0e : 0x7;

        result = gpZrc_Set(gpZrc_MsgId_RecipientSetOriginatorConfiguration, &messageZrc);
        if (result == gpZrc_ResultSuccess) {
            messageZrc.RecipientInteractiveValidationSetup.interactiveValidationAllowed = PushButton ? false : AllowInteractiveValidation;
            result = gpZrc_Set(gpZrc_MsgId_RecipientInteractiveValidationSetup, &messageZrc);
            if (result != gpZrc_ResultSuccess) {
                TRACE_GLOBAL(Trace::Warning, (_T("FAIL in set gpZrc_MsgId_RecipientInteractiveValidationSetup (0x%x)"), result));
                return;
            }
        } else {
            TRACE_GLOBAL(Trace::Warning, (_T("FAIL in set gpZrc_MsgId_RecipientSetOriginatorConfiguration (0x%x)"), result));
            return;
        }
    } else {
        TRACE_GLOBAL(Trace::Warning, (_T("FAIL in get default of gpZrc_MsgId_RecipientSetOriginatorConfiguration (0x%x)"), result));
        return;
    }

    if (PushButton) {
        result = gpZrc_Msg(gpZrc_MsgId_RecipientPushButton, NULL);
        TRACE_GLOBAL(Trace::Information, (_T("%s: gpZrc_MsgId_RecipientPushButton result (0x%x)"), __FUNCTION__, result));
        if (result != gpZrc_ResultSuccess) {
            TRACE_GLOBAL(Trace::Warning, (_T("FAIL in set of gpZrc_MsgId_RecipientPushButton (0x%x)"), result));
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

static void gpApplication_BindConfirm(gpRf4ce_Result_t result, UInt8 bindingRef, UInt8 profileId)
{

    if (result == gpRf4ce_ResultSuccess) {
        if (bindingRef < GP_RF4CE_NWKC_MAX_PAIRING_TABLE_ENTRIES) {
            gpProfileIdList[bindingRef] = profileId;
            /*Backup NVM Data*/
            //printf("gpNvm_Backup\n");
            //gpNvm_Backup(GP_COMPONENT_ID, NVM_TAG_PROFILEIDLIST, NULL);       // XXX: core
        }

        if (profileId == gpRf4ce_ProfileIdZrc2_0) {
            //gpApplication_ConfigureActionMapping(bindingRef);
            //gpApplication_SetActionMapping(bindingRef, _SendRemappableData);
            //gpSched_ScheduleEvent(0, Application_ConfigurePolling);
        }

        gpApplication_IndicateBindSuccessToMiddleware(bindingRef, profileId);
    } else {
        gpApplication_IndicateBindFailureToMiddleware(result);
    }

    gpApplication_ZRCBindSetup(true, false);
}

static void gpApplication_UnBindConfirm(gpRf4ce_Result_t result)
{
    if (result == gpRf4ce_ResultSuccess) {
        WPEFramework::Plugin::GreenPeak::SendEvent(WPEFramework::Exchange::ProducerEvents::UnpairingSuccess);
    } else if (result == gpRf4ce_ResultDiscoveryTimeout) {
        WPEFramework::Plugin::GreenPeak::SendEvent(WPEFramework::Exchange::ProducerEvents::UnpairingTimedout);
    } else {
        WPEFramework::Plugin::GreenPeak::SendEvent(WPEFramework::Exchange::ProducerEvents::UnpairingFailed);
    }
}

void gpZrc_cbMsg(gpZrc_MsgId_t MsgId, UInt8 length, gpZrc_Msg_t* pMsg)
{
    UIntLoop index = 0;

    switch (MsgId) {
    case gpZrc_MsgId_cbRecipientBindingIndication: {
        TRACE_GLOBAL(Trace::Information, (_T("%s: gpZrc_MsgId_cbRecipientBindingIndication MsgId 0x%x"), __FUNCTION__, MsgId));
        break;
    }

    case gpZrc_MsgId_cbRecipientBindingConfirm: {
        TRACE_GLOBAL(Trace::Information, (_T("%s: gpZrc_MsgId_cbRecipientBindingConfirm MsgId 0x%x"), __FUNCTION__, MsgId));
        gpRf4ce_ProfileId_t ProfileId = gpRf4ce_ProfileIdZrc;

        if (pMsg->cbRecipientBindingConfirm.result == gpRf4ce_ResultSuccess) {
            for (index = 0; index < pMsg->cbRecipientBindingConfirm.profileIdListSize; index++) {
                if (pMsg->cbRecipientBindingConfirm.pProfileIdList[index] == gpRf4ce_ProfileIdZrc2_0) {
                    ProfileId = gpRf4ce_ProfileIdZrc2_0;
                    break;
                }
            }
        }
        gpApplication_BindConfirm(pMsg->cbRecipientBindingConfirm.result, pMsg->cbRecipientBindingConfirm.bindingRef, ProfileId);
        break;
    }

    case gpZrc_MsgId_cbRecipientPowerStatusNotifyIndication: {
        TRACE_GLOBAL(Trace::Information, (_T("%s: gpZrc_MsgId_cbRecipientPowerStatusNotifyIndication MsgId 0x%x"), __FUNCTION__, MsgId));
        break;
    }

    case gpZrc_MsgId_cbRecipientValidationResultIndication: {
        TRACE_GLOBAL(Trace::Information, (_T("%s: gpZrc_MsgId_cbRecipientValidationResultIndication MsgId 0x%x"), __FUNCTION__, MsgId));
        break;
    }

    case gpMsoRecipient_MsgId_ValidationComplete: {
        TRACE_GLOBAL(Trace::Information, (_T("%s: gpMsoRecipient_MsgId_ValidationComplete MsgId 0x%x"), __FUNCTION__, MsgId));
        break;
    }

    case gpZrc_MsgId_cbRecipientActionCodesIndicationNoPayload: {
        for (index = 0; index < pMsg->cbActionCodesIndicationNoPayload.numberOfActions; index++) {

            UInt8 keyCode = pMsg->cbActionCodesIndicationNoPayload.ActionRecordList[index].ActionCode;
            switch (pMsg->cbActionCodesIndicationNoPayload.ActionRecordList[index].ActionControl) {

            case gpRf4ceCmd_ZrcActionTypeStart: {
                TRACE_GLOBAL(Trace::Information, (_T("%s: Key Pressed keyCode=0x%x"), __FUNCTION__, keyCode));
                Plugin::GreenPeak::Dispatch(true, keyCode, 0);
                break;
            }
            case gpRf4ceCmd_ZrcActionTypeAtomic:
            case gpRf4ceCmd_ZrcActionTypeReleased: {
                TRACE_GLOBAL(Trace::Information, (_T("%s: Key Released keyCode=0x%x"), __FUNCTION__, keyCode));
                Plugin::GreenPeak::Dispatch(false, keyCode, 0);
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
        TRACE_GLOBAL(Trace::Information, (_T("%s: gpZrc_MsgId_cbUnbindConfirm", __FUNCTION__);
        if (pMsg->cbUnbindConfirm.status == gpRf4ce_ResultNoPairing) {
            TRACE_GLOBAL(Trace::Information, (_T("Invalid binding id (0x%x)"), pMsg->cbUnbindConfirm.bindingId));
        }

        // XXX: gpRf4ce_ResultNoAck (0xe9) ???
        gpApplication_UnBindConfirm(pMsg->cbUnbindConfirm.status);
        break;

    case gpZrc_MsgId_cbUnbindIndication:
        TRACE_GLOBAL(Trace::Information, (_T("%s: gpZrc_MsgId_cbUnbindIndication bindingId=%d status=0x%x"), __FUNCTION__, pMsg->cbUnbindConfirm.bindingId, pMsg->cbUnbindConfirm.status));
        break;

    default:
        TRACE_GLOBAL(Trace::Information, (_T("%s: MsgId 0x%x"), __FUNCTION__, MsgId));
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
            Plugin::GreenPeak::Dispatch(false, lastCode, 0);
            lastCode = 0;
        } else {
            lastCode = code | 0x8000;
            Plugin::GreenPeak::Dispatch(true, lastCode, 0);
        }

        gpPd_FreePd(pdHandle);
        return true;
    } else {
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

static void KeyReleased()
{
    if (_releasedCode != static_cast<uint16_t>(~0)) {
        Plugin::GreenPeak::Dispatch(false, _releasedCode, 0);
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
    } else if ((_releasedCode == static_cast<uint16_t>(~0)) || (_releasedCode != commandCode)) {
        if (_releasedCode != static_cast<uint16_t>(~0)) {
            Plugin::GreenPeak::Dispatch(false, _releasedCode, 0);
            _releasedCode = static_cast<uint16_t>(~0);
        }

        Plugin::GreenPeak::Dispatch((keyStatus == gpRf4ceUserControl_KeyStatusPressed), commandCode, 0);
    } else {
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
        text = Trace::Format(_T("Pairing successfull. ID: [%d]"), static_cast<uint16_t>(bindingId));
    } else {
        text = Trace::Format(_T("Pairing Failed. Error: [%X]"), static_cast<uint16_t>(status));
    }

    Plugin::GreenPeak::Report(text);
}

void gpRf4ceBindAuto_cbUnbindIndication(UInt8 bindingId)
{
    TRACE_GLOBAL(Trace::Information, (_T("Unbind Indication id: %u"), 0, (UInt16)bindingId));
}

void gpRf4ceBindAuto_cbUnbindConfirm(UInt8 bindingId, gpRf4ce_Result_t status)
{
    TRACE_GLOBAL(Trace::Information, (_T("Unbind Confirm id: %u"), 0, (UInt16)bindingId));
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

    gpRf4ce_SetUserString(Plugin::GreenPeak::UserString());

    gpApplication_ZRCBindSetup(true, false);

    Plugin::GreenPeak::Initialized(
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
    } else {
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

static void target_DoR4ceReset(void)
{
    _resetWithColdStart = gpRf4ce_IsColdStartAdvised(TARGET_NODE_CAPABILITIES);
    gpRf4ceDispatcher_ResetRequest(_resetWithColdStart, TARGET_NODE_CAPABILITIES);
}

static void target_ActivatePairing()
{
    Plugin::GreenPeak::Report(string("Entering the PairingMode."));
    WPEFramework::Plugin::GreenPeak::SendEvent(WPEFramework::Exchange::ProducerEvents::PairingStarted);
#if 1
    gpApplication_ZRCBindSetup(false, true);
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

static void target_ActivateUnpairing()
{
    Plugin::GreenPeak::Report(string("Unpairing."));
    WPEFramework::Plugin::GreenPeak::SendEvent(WPEFramework::Exchange::ProducerEvents::UnpairingStarted);
    gpApplication_ZRCUnbind(_bindingId);
}

static void terminateThread()
{
    printf("Terminating the loop.\n");
}

void Application_Init()
{
    gpBaseComps_StackInit();
    gpSched_ScheduleEvent(0, target_DoR4ceReset);
}

void gpRf4ce_cbDpiDisableConfirm(gpRf4ce_Result_t result)
{
    GP_ASSERT_DEV_EXT(result == gpRf4ce_ResultSuccess);
    GP_LOG_SYSTEM_PRINTF("Exiting Standby Mode", 0);
    //gpApplication_SetStandbyState(gpApp_StateIdle);
}

void gpApplication_IndicateBindSuccessToMiddleware(UInt8 bindingRef, UInt8 profileId)
{
    GP_LOG_SYSTEM_PRINTF("Bind Success. BindId: 0x%x, ProfileId: 0x%x", 0, bindingRef, profileId);
    WPEFramework::Plugin::GreenPeak::SendEvent(WPEFramework::Exchange::ProducerEvents::PairingSuccess);
}

void gpApplication_IndicateBindFailureToMiddleware(gpRf4ce_Result_t result)
{
    GP_LOG_SYSTEM_PRINTF("Bind Failure. Status 0x%x", 0, result);
    if (result == gpRf4ce_ResultDiscoveryTimeout)
    {
        WPEFramework::Plugin::GreenPeak::SendEvent(WPEFramework::Exchange::ProducerEvents::PairingTimedout);
    }
    else
    {
        WPEFramework::Plugin::GreenPeak::SendEvent(WPEFramework::Exchange::ProducerEvents::PairingFailed);
    }
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

} // extern "C"
