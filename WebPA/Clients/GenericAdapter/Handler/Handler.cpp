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
 
#include "Handler.h"

namespace WPEFramework {

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LoadProfileFunc)();

IProfileControl* WebPAProfileInstance(const char* name) {
    return (WebPA::Administrator::Instance().Find(name));
}

#ifdef __cplusplus
}
#endif

void Handler::NotificationCallback::NotifyEvent(const EventId& eventId, const EventData& eventData)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));

    NotificationHandler* instance =  nullptr;
    instance = NotificationHandler::GetInstance();
    if (instance)
        instance->AddNotificationToQueue(eventId, eventData);
}

Handler::Handler()
    : _systemLibraries()
    , _signaled(false, true)
    , _adminLock()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    _notificationCallback = new NotificationCallback(this);
    ASSERT(nullptr != _notificationCallback);
    Run();
}

Handler::~Handler()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    if (IsRunning() == true) {
        Stop();

        _signaled.SetEvent();
        Wait(Thread::STOPPED, Core::infinite);
    }

    if (_notificationCallback) {
        delete _notificationCallback;
    }
    for (auto& profileController: _systemProfileControllers) {
        profileController.second.control->Deinitialize();
    }
    _systemLibraries.clear();
}

uint32_t Handler::Configure(PluginHost::IShell* service)
{
    ASSERT(nullptr != service);
    Config config;
    config.FromString(service->ConfigLine());
    const std::string locator(service->DataPath() + config.Location.Value());

    Core::Directory entry(locator.c_str(), _T("*.profile"));
    std::map<const std::string, IProfileControl*> profile;

    while (entry.Next() == true) {
        Core::Library library(entry.Current().c_str());
        if (library.IsLoaded() == true) {
            LoadProfileFunc handle = reinterpret_cast<LoadProfileFunc>(library.LoadFunction(_T("LoadProfile")));
            if (handle != nullptr) {
                handle();
                _systemLibraries.push_back(library);
            }
        }
    }
    Core::JSON::ArrayType< Config::Link >::ConstIterator index (static_cast<const Config&>(config).Profiles.Elements());

    while (index.Next () == true) {
        const std::string name(index.Current().ProfileControl.Value());

        if (name.empty() == false) {
            SystemProfileController systemProfileController;
            systemProfileController.name = name;

            systemProfileController.control = WebPAProfileInstance(name.c_str());
            if (systemProfileController.control) {
                systemProfileController.control->Initialize();
                _systemProfileControllers.insert(std::pair<const std::string, SystemProfileController>(index.Current().ProfileName.Value(), systemProfileController));
            }
        } else {
            TRACE_GLOBAL(Trace::Information, (_T("Required adapter not found for %s"), index.Current().ProfileName.Value()));
        }
    }

    if (_systemProfileControllers.size() == 0) {
        TRACE(Trace::Information, (_T("No adapter provided")));
    }
    return Core::ERROR_NONE;
}

uint32_t Handler::Worker()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    if (IsRunning() == true) {

        for (auto& profileController: _systemProfileControllers) {
            profileController.second.control->CheckForUpdates();
        }

        _signaled.Lock(MaxWaitTime);
    }
    return Core::infinite;
}

const FaultCode Handler::Parameter(Data& parameter) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode ret = FaultCode::NoFault;

    /* Find the respective manager and forward the request*/
    const IProfileControl* control = GetProfileController(parameter.Name());

    if (control) {
        ret = control->Parameter(parameter);
    }

    return ret;
}

FaultCode Handler::Parameter(const Data& parameter)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode ret = FaultCode::NoFault;

    /* Find the respective manager and forward the request*/
    IProfileControl* control = GetProfileController(parameter.Name());

    if (control) {
        ret = control->Parameter(parameter);
    }

    return ret;
}

const FaultCode Handler::Attribute(Data& parameter) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode ret = FaultCode::NoFault;

    /* Find the respective manager and forward the request*/
    const IProfileControl* control = GetProfileController(parameter.Name());

    if (control) {
        ret = control->Attribute(parameter);
    }
    return ret;
}

FaultCode Handler::Attribute(const Data& parameter)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    FaultCode ret = FaultCode::NoFault;

    /* Find the respective manager and forward the request*/
    IProfileControl* control = GetProfileController(parameter.Name());
    if (control) {
        ret = control->Attribute(parameter);
    }

    return ret;
}

void Handler::FreeData(Data* parameter)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    if (parameter) {
        g_free(parameter);
    }
}

std::vector<std::string> Handler::SplitParam(std::string parameter, char delimeter) const
{
    std::stringstream ss(parameter);
    std::string str;
    std::vector<std::string> splittedParams;
    while (std::getline(ss, str, delimeter))
    {
       splittedParams.push_back(str);
    }
    return splittedParams;
}

void Handler::ConfigureProfileControllers()
{
    for (auto& profileController: _systemProfileControllers) {
        profileController.second.control->SetCallback(_notificationCallback);
    }
}

IProfileControl* Handler::GetProfileController(const std::string& name)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    IProfileControl* pRet = nullptr;

    std::vector<std::string> paramComponents = SplitParam(name, '.');
    if (paramComponents.size() > 1) {
        std::map<const std::string, SystemProfileController>::iterator index(_systemProfileControllers.find(paramComponents[1]));
        if (_systemProfileControllers.end() != index) {
            pRet = index->second.control;
        } else {
            TRACE(Trace::Information, (_T("Could not able to find Profile controller for %s"), name.c_str()));
        }
    }

    return pRet;
}

const IProfileControl* Handler::GetProfileController(const std::string& name) const
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    const IProfileControl* pRet = nullptr;

    std::vector<std::string> paramComponents = SplitParam(name, '.');
    if (paramComponents.size() > 1) {
        std::map<const std::string, SystemProfileController>::const_iterator index(_systemProfileControllers.find(paramComponents[1]));
        if (_systemProfileControllers.end() != index) {
            pRet = index->second.control;
        } else {
            TRACE(Trace::Information, (_T("Could not able to find Profile controller for %s"), name.c_str()));
        }
    }

    return pRet;
}

NotificationHandler* NotificationHandler::_instance = nullptr;

NotificationHandler::NotificationHandler()
    : _notificationCb(nullptr)
    , _notificationQueue(nullptr)
    , _adminLock()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    // Initialize notificationQueue
    _notificationQueue = g_async_queue_new();
}

NotificationHandler::~NotificationHandler()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    g_async_queue_unref(_notificationQueue);
}

NotificationHandler* NotificationHandler::GetInstance()
{
    TRACE_GLOBAL(Trace::Information, (string(__FUNCTION__)));
    if (!_instance) {
        _instance = new NotificationHandler();
        ASSERT(nullptr != _instance);
    }
    return _instance;
}

NotifyData* NotificationHandler::NotificationData()
{
    _adminLock.Lock();
    NotifyData* notifyData = nullptr;
    if (nullptr != _notificationQueue) {
       notifyData = (NotifyData*) g_async_queue_timeout_pop (_notificationQueue, 1000);
    }
    _adminLock.Unlock();
    return notifyData;
}

void NotificationHandler::AddNotificationToQueue(const EventId& eventId, const EventData& eventData)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    ParamNotify *paramNotify = nullptr;

    switch (eventId)
    {
    case EVENT_ADD:
    case EVENT_REMOVE:
    case EVENT_VALUECHANGED:
    {
        if (IsValidParameter(eventData.Name())) {
            paramNotify = new ParamNotify(eventData.Name(), eventData.Value());
            ASSERT(nullptr != paramNotify);
        }

        break;
    }
    default:
        break;
    }

    if ((nullptr != _notificationCb) && (eventId == EVENT_VALUECHANGED) && IsValidParameter(eventData.Name())) {
        NotifyData *notifyData = new NotifyData();
        ASSERT(nullptr != notifyData);
        if (nullptr != notifyData) {
            notifyData->type = PARAM_VALUE_CHANGE_NOTIFY;
            notifyData->data.notify = paramNotify;

            // Add the notification to queue and call Webpa Callback
            _adminLock.Lock();
            if (nullptr != _notificationQueue) {
                g_async_queue_push(_notificationQueue, notifyData);
                _notificationCb->NotifyEvent();
            } else {

                TRACE(Trace::Information, (_T("Queue is not initialized ")));
                if (paramNotify) {
                    delete paramNotify;
                }
            }
            _adminLock.Unlock();
        } else {
            if (paramNotify) {
                delete paramNotify;
            }
        }
    }
}

void NotificationHandler::SetNotifyCallback(WebPA::ICallback* cb)
{
    _adminLock.Lock();
    _notificationCb = cb;
    _adminLock.Unlock();
}

bool NotificationHandler::IsValidParameter(string paramName)
{
    bool isValid = false;
    if ((paramName.empty() != true) && (paramName.back() != '.')) {
        isValid = true;
    }
    return isValid;
}
}
