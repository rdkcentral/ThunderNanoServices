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
#include <signal.h>
#include <string>

#include "Adapter.h"
#include "Handler.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <libparodus.h>
#ifdef __cplusplus
}
#endif

namespace WPEFramework {
namespace Implementation {

class GenericAdapter : public Exchange::IWebPA::IWebPAClient, public Core::Thread {
private:
   static constexpr const TCHAR* ContentTypeJson = "application/json";

private:
    class Config : public Core::JSON::Container {
    public:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        Config()
            : Core::JSON::Container()
            , DataModelFile(_T(""))
            , ClientURL(_T("tcp://127.0.0.1:6667"))
            , ParodusURL(_T("tcp://127.0.0.1:6666"))
            , NotifyConfigFile(_T(""))
            , MaxClientRetry(1)
        {
            Add(_T("datamodelfile"), &DataModelFile);
            Add(_T("genericclienturl"), &ClientURL);
            Add(_T("paroduslocalurl"), &ParodusURL);
            Add(_T("notifyconfigfile"), &NotifyConfigFile);
            Add(_T("maxclientretry"), &MaxClientRetry);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::String DataModelFile;
        Core::JSON::String ClientURL;
        Core::JSON::String ParodusURL;
        Core::JSON::String NotifyConfigFile;
        Core::JSON::DecUInt8 MaxClientRetry;
    };

    class NotificationCallback : public ICallback {
    public:
        NotificationCallback() = delete;
        NotificationCallback(const NotificationCallback&) = delete;
        NotificationCallback& operator= (const NotificationCallback&) = delete;

    public:
        NotificationCallback(GenericAdapter* parent)
            : _parent(parent)
        {
        }
        virtual ~NotificationCallback()
        {
            TRACE(Trace::Information, (_T("%s destructed. Line: %d"), __PRETTY_FUNCTION__, __LINE__));
        }
        virtual void NotifyEvent(const std::string& payload, const std::string& source, const std::string& destination) override;

    private:
        GenericAdapter* _parent;
    };

public:
    GenericAdapter(const GenericAdapter&) = delete;
    GenericAdapter& operator=(const GenericAdapter&) = delete;

public:
    GenericAdapter ()
        : Core::Thread(0, _T("WebPAClient"))
        , _adminLock()
        , _notificationCallback(nullptr)
        , _maxRetry(0)
    {
        TRACE(Trace::Information, (_T("GenericAdapter::Construct()")));
        _adapter = new WebPA::Adapter(&_msgHandler);
        _notificationCallback = new NotificationCallback(this);
    }

    virtual ~GenericAdapter()
    {
        TRACE(Trace::Information, (_T("GenericAdapter::Destruct()")));

        if (true == IsRunning()) {
            DisconnectFromParodus();

            Stop();
            Wait(Thread::STOPPED, Core::infinite);
        }
        if (nullptr != _notificationCallback) {
            delete _notificationCallback;
            _notificationCallback = nullptr;
        }
        if (nullptr != _adapter) {
            delete _adapter;
            _adapter = nullptr;
        }
    }

    BEGIN_INTERFACE_MAP(GenericAdapter)
       INTERFACE_ENTRY(Exchange::IWebPA::IWebPAClient)
    END_INTERFACE_MAP

    //WebPAClient Interface
    virtual uint32_t Configure(PluginHost::IShell* service) override
    {
        ASSERT(nullptr != service);
        Config config;
        config.FromString(service->ConfigLine());
        TRACE_GLOBAL(Trace::Information, (_T("DataModelFile = [%s] ParodusURL = [%s] ClientURL = [%s]"), config.DataModelFile.Value().c_str(), config.ParodusURL.Value().c_str(), config.ClientURL.Value().c_str()));
        if (config.DataModelFile.Value().empty() == false) {
            _dataModelFile = config.DataModelFile.Value();
        }
        _clientURL = config.ClientURL.Value();
        _parodusURL = config.ParodusURL.Value();
        _maxRetry = config.MaxClientRetry.Value();
        if (config.NotifyConfigFile.Value().empty() == false) {
             TRACE_GLOBAL(Trace::Information, (_T("NotifyConfigFile = [%s]"), config.NotifyConfigFile.Value().c_str()));
            _adapter->NotifierConfigFile(config.NotifyConfigFile.Value());
        }

        _msgHandler.Configure(service);

        return Core::ERROR_NONE;
    }

    virtual uint32_t Launch() override
    {
        uint32_t status = Core::ERROR_GENERAL;
        if (true == IsRunning()) {
            DisconnectFromParodus();
        }

        Block();
        Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
        if (State() == Thread::BLOCKED) {
            // Load Data model
            TRACE_GLOBAL(Trace::Information, (_T("WebPAClient Start() : ")));
            _msgHandler.ConfigureProfileControllers();

            TRACE_GLOBAL(Trace::Information, (_T("WebPAClient LoadDataModel() : ")));
            if (_adapter->LoadDataModel(_dataModelFile) == true) {
                // First connect to parodus
                status = ConnectToParodus();
                if (status == Core::ERROR_NONE) {

                    // Lets set the Notify Callback function
                     _adapter->SetNotifyCallback(_notificationCallback);

                    // Lets set the initial Notification
                    _adapter->InitializeNotifyParameters();

                    // Call Parodus listner worker function
                    Run();
                } else {
                    TRACE(Trace::Error, (_T("WebPAClient Connection to Parodus Failed ")));
                }
            } else {
                TRACE(Trace::Error, (_T("WebPAClient LoadDataModel() Failed")));
            }
        }
        return status;
    }

private:
    virtual uint32_t Worker();
    uint32_t ConnectToParodus();
    void DisconnectFromParodus();
    long TimeValDiff(struct timespec *starttime, struct timespec *finishtime);

private:
    Core::CriticalSection _adminLock;
    NotificationCallback*  _notificationCallback;

    std::string _dataModelFile;

    Handler _msgHandler;
    WebPA::Adapter* _adapter;

    libpd_instance_t _libparodusInstance;
    uint8_t _maxRetry;
    std::string _parodusURL;
    std::string _clientURL;
};

// The essence of making the IWebPAClient interface available. This instantiates
// an object that can be created from the outside of the library by looking
// for the GenericAdapter class name, that realizes the IStateControl interface.
SERVICE_REGISTRATION(GenericAdapter, 1, 0);

uint32_t GenericAdapter::ConnectToParodus()
{
    TRACE(Trace::Information, (_T("ConnectToParodus")));
    uint8_t maxRetry = _maxRetry;
    uint32_t status = Core::ERROR_GENERAL;

    uint16_t backoffRetryTime = 0;
    uint16_t backoffMaxTime = 5;
    //Retry Backoff count shall start at c=2 & calculate 2^c - 1.
    uint16_t counter = 2;
    uint16_t maxRetrySleep = (uint16_t) pow(2, backoffMaxTime) - 1;
    TRACE(Trace::Information, (_T("maxRetrySleep = %d"), maxRetrySleep));

    libpd_cfg_t clientCFG = {.service_name = "config",
                             .receive = true, .keepalive_timeout_secs = 64,
                             .parodus_url = _parodusURL.c_str(),
                             .client_url = _clientURL.c_str()
                            };

    TRACE(Trace::Information, (_T("parodusUrl = %s clientUrl = %s"), _parodusURL.c_str(), _clientURL.c_str()));

    while (maxRetry > 0)
    {
        if (backoffRetryTime < maxRetrySleep) {
            backoffRetryTime = (uint16_t) pow(2, counter) - 1;
        }

        TRACE(Trace::Information, (_T("New backoffRetryTime value calculated = %d seconds"), backoffRetryTime));
        int ret = libparodus_init (&_libparodusInstance, &clientCFG);
        if (ret == 0) {
            TRACE(Trace::Information, (_T("Init for parodus Success..!!")));
            TRACE(Trace::Information, (_T("WebPA is now ready to process requests")));
            status = Core::ERROR_NONE;
            break;
        } else {
            TRACE(Trace::Information, (_T("Init for parodus failed: '%s'"), libparodus_strerror((libpd_error_t)ret)));
            sleep(backoffRetryTime);
            counter++;

            if (backoffRetryTime == maxRetrySleep) {
                counter = 2;
                backoffRetryTime = 0;
                TRACE(Trace::Information, (_T("backoffRetryTime reached max value, reseting to initial value")));
                maxRetry--;
            }
        }
        libparodus_shutdown(&_libparodusInstance);
        TRACE(Trace::Information, (_T("libparodus_shutdown retval %d"), ret));
    }
    return status;
}

void GenericAdapter::DisconnectFromParodus()
{
    libparodus_shutdown(&_libparodusInstance);
}

uint32_t GenericAdapter::Worker()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));

    wrp_msg_t *wrpMsg;
    int ret = libparodus_receive (_libparodusInstance, &wrpMsg, 2000);
    if (0 == ret) {
        if (wrpMsg->msg_type == WRP_MSG_TYPE__REQ) {
            wrp_msg_t *responseWrpMsg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
            memset(responseWrpMsg, 0, sizeof(wrp_msg_t));
            struct timespec start;
            _adapter->CurrentTime(&start);
            _adapter->ProcessRequest((char*)wrpMsg->u.req.payload, wrpMsg->u.req.transaction_uuid, ((char **)(&(responseWrpMsg->u.req.payload))));

            TRACE(Trace::Information, (_T("Response payload is %s\n"), (char *)(responseWrpMsg->u.req.payload)));
            if (nullptr != responseWrpMsg->u.req.payload) {
                responseWrpMsg->u.req.payload_size = strlen((const char *)responseWrpMsg->u.req.payload);
            }
            responseWrpMsg->msg_type = wrpMsg->msg_type;
            responseWrpMsg->u.req.source = wrpMsg->u.req.dest;
            responseWrpMsg->u.req.dest = wrpMsg->u.req.source;
            responseWrpMsg->u.req.transaction_uuid = wrpMsg->u.req.transaction_uuid;
            char *contentType = (char *)malloc(sizeof(char)*(strlen(ContentTypeJson) + 1));
            strncpy(contentType, ContentTypeJson, strlen(ContentTypeJson) + 1);
            responseWrpMsg->u.req.content_type = contentType;
            int sendStatus = libparodus_send(_libparodusInstance, responseWrpMsg);

            if (sendStatus == 0) {
                TRACE(Trace::Information, (_T("Sent message successfully to parodus")));
            } else {
                TRACE(Trace::Error, (_T("Failed to send message: '%s'"), libparodus_strerror((libpd_error_t )sendStatus)));
            }
            struct timespec end;
            _adapter->CurrentTime(&end);
            TRACE(Trace::Information, (_T("Elapsed time : %ld ms"), TimeValDiff(&start, &end)));
            wrp_free_struct (responseWrpMsg);
        }
    } else {
        if (1 != ret) {
            sleep(5);
        }
        TRACE(Trace::Information, (_T("Libparodus failed to recieve message: '%s'"), libparodus_strerror((libpd_error_t)ret)));
    }

    TRACE(Trace::Information, (_T("End of Worker\n")));
    return Core::infinite;
}

void GenericAdapter::NotificationCallback::NotifyEvent(const std::string& data, const std::string& source, const std::string& destination)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    wrp_msg_t *notifyWrpMsg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
    memset(notifyWrpMsg, 0, sizeof(wrp_msg_t));

    notifyWrpMsg->msg_type = WRP_MSG_TYPE__EVENT;
    TRACE(Trace::Information, (_T("source: %s\n"), source.c_str()));
    notifyWrpMsg ->u.event.source = strdup(source.c_str());
    TRACE(Trace::Information, (_T("destination: %s\n"), destination.c_str()));
    notifyWrpMsg ->u.event.dest = strdup(destination.c_str());
    char *contentType = (char *) malloc(sizeof(char) * (strlen(ContentTypeJson) + 1));
    strncpy(contentType, ContentTypeJson, strlen(ContentTypeJson) + 1);
    notifyWrpMsg->u.event.content_type = contentType;
    TRACE(Trace::Information, (_T("content_type is %s\n"), notifyWrpMsg->u.event.content_type));

    TRACE(Trace::Information, (_T("Notification payload: %s\n"), data.c_str()));
    char *payload = (char *) malloc(sizeof(char) * (data.length()));
    notifyWrpMsg ->u.event.payload = (void *)payload;
    notifyWrpMsg ->u.event.payload_size = strlen((const char*)notifyWrpMsg->u.event.payload);


    int retryCount = 0;
    while (retryCount <= 3) {
        int count = 2;
        int backoffRetryTime = (int) pow(2, count) -1;

        int sendStatus = libparodus_send(_parent->_libparodusInstance, notifyWrpMsg);
        if (sendStatus == 0) {
            retryCount = 0;
            TRACE(Trace::Information, (_T("Notification successfully sent to parodus\n")));
            break;
        } else {
            TRACE(Trace::Information, (_T("sendStatus is %d\n"), sendStatus));
            TRACE(Trace::Information, (_T("NotifyEvent backoffRetryTime %d seconds\n"), backoffRetryTime));
            sleep(backoffRetryTime);
            count++;
            retryCount++;
        }
    }

    wrp_free_struct(notifyWrpMsg);
    TRACE(Trace::Information, (_T("Freed notifyWrpMsg struct.\n")));
}

long GenericAdapter::TimeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
    long msec;
    msec = (finishtime->tv_sec-starttime->tv_sec) * 1000;
    msec += (finishtime->tv_nsec-starttime->tv_nsec) / 1000000;
    return msec;
}
} // Implementation
} //WPEFramework
