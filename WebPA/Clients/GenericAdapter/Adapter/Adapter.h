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
 
#pragma once

#include "Attribute.h"
#include "Notifier.h"
#include "Parameter.h"
#include "DataModel.h"

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
            TRACE(Trace::Information, (_T("%s destructed. Line: %d"), __PRETTY_FUNCTION__, __LINE__));
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

    DataModel *_dataModel;
    Parameter* _parameter;
    Attribute* _attribute;
    Notifier* _notifier;

    Core::CriticalSection _adminLock;
};

} // WebPA
} // WPEFramework
