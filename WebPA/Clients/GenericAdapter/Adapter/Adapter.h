#pragma once

#include "Attribute.h"
#include "Notifier.h"
#include "Parameter.h"
#include "WalDB.h"

namespace WPEFramework {
namespace WebPA {

class Adapter {
private:
    class Helper;
    static constexpr const TCHAR* DeviceInfoRebootReasonName = _T("Device.DeviceInfo.RebootReason");
    static constexpr const TCHAR* DeviceRebootParam = _T("Device.X_CISCO_COM_DeviceControl.RebootDevice");
    static constexpr const TCHAR* DeviceRebootValue = _T("Device");
    static constexpr const uint16_t MaxParameterNameLen = 256;

private:
    class NotificationCallback : public ICallback, public Core::Thread {
    public:
        NotificationCallback() = delete;
        NotificationCallback(const NotificationCallback&) = delete;
        NotificationCallback& operator= (const NotificationCallback&) = delete;

    public:
        NotificationCallback(Adapter* parent)
            : _parent(parent)
            , _signaled(false, true)
            , _adminLock()
        {
            Run();
            printf("%s constructed. Line: %d\n", __PRETTY_FUNCTION__,  __LINE__);
        }
        virtual ~NotificationCallback()
        {
            Stop();
            _signaled.SetEvent();

            Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
            TRACE_L1("%s destructed. Line: %d", __PRETTY_FUNCTION__, __LINE__);
        }
        virtual void NotifyEvent() override;

    private:
    virtual uint32_t Worker();

    private:
        Adapter* _parent;

        Core::Event _signaled;
        Core::CriticalSection _adminLock;
    };

public:
    Adapter() = delete;
    Adapter(const Adapter&) = delete;
    Adapter& operator= (const Adapter&) = delete;

public:
    Adapter(Handler* msgHandler);
    ~Adapter();

    bool LoadDataModel(const std::string& dataFile);

    void SetNotifyCallback(Implementation::ICallback* cb);
    void InitializeNotifyParameters(void);
    void NotifierConfigFile(const std::string& nofityConfigFile);

    void ProcessRequest(char* reqPayload, char* transactionId, char** resPayload);
    void CurrentTime(struct timespec* timer);

private:
    WebPAStatus ValidateParameter(void* param, uint32_t paramCount);
    void FreeNotificationData(NotifyData* notifyMsg);

private:
    Implementation::ICallback* _callback;
    NotificationCallback*  _adapterCallback;

    WalDB *_walDB;
    Parameter* _parameter;
    Attribute* _attribute;
    Notifier* _notifier;

    Core::CriticalSection _adminLock;
};

} // WebPA
} // WPEFramework
