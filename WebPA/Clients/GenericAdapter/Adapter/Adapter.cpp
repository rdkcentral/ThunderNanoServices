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
 
#include "Adapter.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <wdmp-c.h>
#ifdef __cplusplus
}
#endif

namespace WPEFramework {
namespace WebPA {

//This class is used to interact with wdmp anonymous typedef structs, since it is not able to forward declared
class Adapter::Helper {
public:
    Helper() = delete;
    Helper(const Helper&) = delete;
    Helper& operator= (const Helper&) = delete;

    Helper(Parameter* parameter, Attribute* attribute)
        : _parameter(parameter)
        , _attribute(attribute)
    {
    }
    virtual ~Helper() {}

    void UpdateRebootReason(const req_struct*& reqObj);
    WebPAStatus GetParameters(const req_struct*& reqObj, res_struct*& resObj) const;
    WebPAStatus GetAttributes(const req_struct*& reqObj, res_struct*& resObj) const;
    WebPAStatus SetParameters(const req_struct*& reqObj, res_struct*& resObj);
    WebPAStatus SetAttributes(const req_struct*& reqObj, res_struct*& resObj);

private:
    WebPAStatus ParameterNamesOfGetters(const req_struct*& reqObj, std::vector<std::string>& parameterNames) const;
    WebPAStatus ParameterNamesOfSetters(const req_struct*& reqObj, std::vector<std::string>& parameterNames) const;
    void AllocateGetResponse(const req_struct*& reqObj, res_struct*& resObj) const;
    template <typename TYPE>
    void AllocateParamResponse(const req_struct*& reqObj, res_struct*& resObj, TYPE type) const;

private:
    Parameter* _parameter;
    Attribute* _attribute;
};

Adapter::Adapter(Handler* handler)
    : _callback(nullptr)
    , _adapterCallback(nullptr)
    , _dataModel(nullptr)
    , _parameter(nullptr)
    , _attribute(nullptr)
    , _notifier(nullptr)
    , _adminLock()
{
    _dataModel = new DataModel(handler);
    ASSERT(_dataModel != nullptr);

    _parameter = new Parameter(handler, _dataModel);
    ASSERT(_parameter != nullptr);

    _notifier = new Notifier(_parameter);
    ASSERT(_notifier != nullptr);

    _attribute = new Attribute(_notifier, handler);
    ASSERT(_attribute != nullptr);

    _adapterCallback = new NotificationCallback(this);
    ASSERT(_adapterCallback != nullptr);
}

Adapter::~Adapter()
{
    if (nullptr != _attribute) {
        delete _attribute;
        _attribute = nullptr;
    }
    if (nullptr != _notifier) {
        delete _notifier;
        _notifier = nullptr;
    }
    if (nullptr != _parameter) {
        delete _parameter;
        _parameter = nullptr;
    }
    if (nullptr != _adapterCallback) {
        delete _adapterCallback;
        _adapterCallback = nullptr;
    }
    if (nullptr != _dataModel) {
       delete _dataModel;
       _dataModel = nullptr;
    }
}

bool Adapter::LoadDataModel(const std::string& dataFile)
{
    DMStatus dmRet = DM_FAILURE;
    bool status = false;
    // Load Document model
    if (!dataFile.empty()) {
        dmRet = _dataModel->LoadDM(dataFile);

        if (dmRet != DM_SUCCESS) {
            TRACE(Trace::Information, (_T("Error loading webpa database")));
        } else {
            TRACE(Trace::Information, (_T("DM Handle is Valid")));
            status = true;
        }
    }
    return status;
}

void Adapter::NotifierConfigFile(const std::string& nofityConfigFile)
{
    _notifier->ConfigurationFile(nofityConfigFile);
}

void Adapter::SetNotifyCallback(Implementation::ICallback* callback)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    _adminLock.Lock();
    _callback = callback;

    // Get Notification handler instance
    NotificationHandler* handler = NotificationHandler::GetInstance();
    if (handler) {
        handler->SetNotifyCallback(_adapterCallback);
    } else {
        TRACE(Trace::Information, (_T("Unable to get Notification Handler, Error in Callback Registration")));
    }
    _adminLock.Unlock();
}

void Adapter::InitializeNotifyParameters()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    std::vector<std::string> notifyParameters;

    // Read Notify configuration file
    uint32_t status = _notifier->Parameters(notifyParameters);
    if (status == Core::ERROR_NONE) {
        for (auto& notifyParameter: notifyParameters) {
            bool value = false;
            Data param(notifyParameter, static_cast<Variant>(value));

            TRACE(Trace::Information, (_T("notifyParameters: %s"), notifyParameter));

            std::map<std::string, Data> attributes = {std::make_pair(notifyParameter, param)};
            std::vector<WebPAStatus> status;
            WebPAStatus ret = _attribute->Values(attributes, status);
            if (ret != WEBPA_SUCCESS) {
                TRACE(Trace::Information, (_T("Failed to turn notification ON for parameter : %s ret: %d"), notifyParameter, ret));
            } else {
                TRACE(Trace::Information, (_T("Successfully set notification ON for parameter : %s ret: %d"), notifyParameters, ret));
            }
        }
    }
    TRACE(Trace::Information, (_T("End of setInitial Notify")));
}

void Adapter::ProcessRequest(char* reqPayload,char* transactionId, char** resPayload)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));

    req_struct* reqObj = nullptr;
    wdmp_parse_request(reqPayload, &reqObj);

    if (nullptr != reqObj) {
        TRACE(Trace::Information, (_T("Request:> Type : %d"), reqObj->reqType));
        res_struct* resObj = (res_struct*) malloc(sizeof(res_struct));
        ASSERT(resObj);
        if (nullptr != resObj) {
            memset(resObj, 0, sizeof(res_struct));
            resObj->reqType = reqObj->reqType;
            const req_struct* requestObj = reqObj;

            Helper helper(_parameter, _attribute);
            switch(reqObj->reqType)
            {
            case GET:
            {
                TRACE(Trace::Information, (_T("Processing GET request")));
                helper.GetParameters(requestObj, resObj);
                break;
            }

            case GET_ATTRIBUTES:
            {
                TRACE(Trace::Information, (_T("Processing GETATTR request")));
                helper.GetAttributes(requestObj, resObj);
                break;
            }
            case SET:
            {
                TRACE(Trace::Information, (_T("Processing SET request")));
                helper.SetParameters(requestObj, resObj);
                break;
            }
            case SET_ATTRIBUTES:
            {
                TRACE(Trace::Information, (_T("Processing SET ATTRIBUTES request")));
                helper.SetAttributes(requestObj, resObj);
                break;
            }
            default:
                break;
            }

            char* payload = nullptr;
            wdmp_form_response(resObj, &payload);

            *resPayload = payload;
            if (resPayload) {
                TRACE(Trace::Information, (_T("Response:> Payload = %s"), *resPayload));
            }

            wdmp_free_res_struct(resObj);
        }
        wdmp_free_req_struct(reqObj);
    }

    TRACE(Trace::Information, (_T("%s:End"), __FUNCTION__));
}

void Adapter::NotificationCallback::NotifyEvent()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    _signaled.SetEvent();
}

void Adapter::Helper::AllocateGetResponse(const req_struct*& reqObj, res_struct*& resObj) const
{
    resObj->timeSpan = nullptr;

    int paramCount = static_cast<int>(reqObj->u.getReq->paramCnt);
    resObj->paramCnt = 0;

    resObj->u.getRes = (get_res_t*) malloc(sizeof(get_res_t));
    ASSERT(resObj->u.getRes);
    memset(resObj->u.getRes, 0, sizeof(get_res_t));

    resObj->u.getRes->paramCnt = paramCount;

    resObj->u.getRes->params = static_cast<param_t**> (malloc(sizeof(param_t*) * paramCount));
    ASSERT(resObj->u.getRes->params);
    memset(resObj->u.getRes->params, 0, sizeof(param_t*) * paramCount);

    resObj->u.getRes->retParamCnt = static_cast<size_t*> (calloc(paramCount, sizeof(size_t)));
    ASSERT(resObj->u.getRes->retParamCnt);

    resObj->u.getRes->paramNames = static_cast<char**> (malloc(sizeof(char*) * paramCount));
    ASSERT(resObj->u.getRes->paramNames);
    memset(resObj->u.getRes->paramNames, 0, sizeof(char*) * paramCount);

}

template <typename TYPE>
void Adapter::Helper::AllocateParamResponse(const req_struct*& reqObj, res_struct*& resObj, TYPE type) const
{
    resObj->timeSpan = nullptr;

    int paramCount = static_cast<int>(type->paramCnt);
    resObj->paramCnt = 0;

    resObj->u.paramRes = (param_res_t*) malloc(sizeof(param_res_t));
    ASSERT(resObj->u.paramRes);
    memset(resObj->u.paramRes, 0, sizeof(param_res_t));

    resObj->u.paramRes->params = static_cast<param_t*> (malloc(sizeof(param_t) * paramCount));
    ASSERT(resObj->u.paramRes->params);
    memset(resObj->u.paramRes->params, 0, sizeof(param_t*) * paramCount);
}

WebPAStatus Adapter::Helper::GetParameters(const req_struct*& reqObj, res_struct*& resObj) const
{
    resObj->retStatus = (WDMP_STATUS*) calloc(reqObj->u.getReq->paramCnt, sizeof(WDMP_STATUS));
    ASSERT(resObj->retStatus);

    AllocateGetResponse(reqObj, resObj); // Need to be allocated for error case as well, since the wdmp-c lib is not validating the nullptr before accessing the members.
    std::vector<std::string> parameterNames;
    WebPAStatus status = ParameterNamesOfGetters(reqObj, parameterNames);

    if ((status == WEBPA_SUCCESS) && (reqObj->u.getReq->paramCnt > 0)) {
        resObj->paramCnt = reqObj->u.getReq->paramCnt;
        std::map<std::vector<Data>, WebPAStatus> parametersList;
        _parameter->Values(parameterNames, parametersList);
        if (parametersList.size() > 0) {

            int i = 0;
            for (std::map<std::vector<Data>, WebPAStatus>::iterator parameters = parametersList.begin(); parameters != parametersList.end(); parameters++, i++) {
                resObj->u.getRes->paramNames[i] = strdup(parameterNames[i].c_str());
                resObj->u.getRes->retParamCnt[i] = parameters->first.size();
                resObj->retStatus[i] = static_cast<WDMP_STATUS>(parameters->second);

                TRACE(Trace::Information, (_T("Response:> paramNames[%d] = %s"), i, resObj->u.getRes->paramNames[i]));
                TRACE(Trace::Information, (_T("Response:> retParamCnt[%d] = %zu"), i, resObj->u.getRes->retParamCnt[i]));
                TRACE(Trace::Information, (_T("Response:> retStatus[%d] = %d"), i, resObj->retStatus[i]));
 
                resObj->u.getRes->params[i] = static_cast<param_t*> (calloc(sizeof(param_t), parameters->first.size()));
                ASSERT(resObj->u.getRes->params[i]);
                int j = 0;
                for (auto& parameter: parameters->first) {
                    resObj->u.getRes->params[i][j].name = strdup(parameter.Name().c_str());
                    resObj->u.getRes->params[i][j].type = static_cast<DATA_TYPE>(parameter.Value().Type());
                    resObj->u.getRes->params[i][j].value = strdup(Utils::ConvertParamValueToString(parameter).c_str());
                    j++;
                }
            }
            parametersList.clear();
        }
    } else {
        resObj->retStatus[0] = static_cast<WDMP_STATUS>(status);
    }

    parameterNames.clear();

    return status;
}

WebPAStatus Adapter::Helper::GetAttributes(const req_struct*& reqObj, res_struct*& resObj) const
{
    TRACE(Trace::Information, (_T("Request:> ParamCount = %zu"), reqObj->u.getReq->paramCnt));

    resObj->retStatus = (WDMP_STATUS*) calloc(reqObj->u.getReq->paramCnt, sizeof(WDMP_STATUS));
    ASSERT(resObj->retStatus);

    AllocateParamResponse(reqObj, resObj, reqObj->u.getReq); // Need to be allocated for error case as well, since the wdmp-c lib is not validating the nullptr before accessing the members.

    std::vector<std::string> parameterNames;
    WebPAStatus status = ParameterNamesOfGetters(reqObj, parameterNames);

    if ((status == WEBPA_SUCCESS) && (reqObj->u.getReq->paramCnt > 0)) {
        std::map<Data, WebPAStatus> attributes;
        _attribute->Values(parameterNames, attributes);

        if (attributes.size() > 0) {

            resObj->paramCnt = reqObj->u.getReq->paramCnt;
            int i = 0;
            for (auto& attribute: attributes) {
                resObj->retStatus[i] = static_cast<WDMP_STATUS>(attribute.second);

                TRACE(Trace::Information, (_T("Response:> retStatus[%d] = %d"), i, resObj->retStatus[i]));

                resObj->u.paramRes->params[i].name = strdup(attribute.first.Name().c_str());
                resObj->u.paramRes->params[i].type = static_cast<DATA_TYPE>(attribute.first.Value().Type());
                resObj->u.paramRes->params[i].value = strdup(Utils::ConvertParamValueToString(attribute.first).c_str());
                i++;
            }
            attributes.clear();
        }
    } else {
        resObj->retStatus[0] = static_cast<WDMP_STATUS>(status);
    }

    parameterNames.clear();
    return status;
}

WebPAStatus Adapter::Helper::SetParameters(const req_struct*& reqObj, res_struct*& resObj)
{
    TRACE(Trace::Information, (_T("Request:> ParamCount = %zu"), reqObj->u.setReq->paramCnt));

    resObj->retStatus = static_cast<WDMP_STATUS*> (calloc(resObj->paramCnt, sizeof(WDMP_STATUS)));
    ASSERT(resObj->retStatus);

    AllocateParamResponse(reqObj, resObj, reqObj->u.setReq); // Need to be allocated for error case as well, since the wdmp-c lib is not validating the nullptr before accessing the members.
    UpdateRebootReason(reqObj);

    std::vector<std::string> parameterNames;
    WebPAStatus status = ParameterNamesOfSetters(reqObj, parameterNames);

    if ((status == WEBPA_SUCCESS) && (reqObj->u.setReq->paramCnt > 0)) {
        resObj->paramCnt = reqObj->u.setReq->paramCnt;

        std::vector<Data> parameters;
        std::vector<WebPAStatus> parameterStatus;

        for (uint32_t i = 0; i < reqObj->u.setReq->paramCnt; i++) {
            Data parameter(reqObj->u.setReq->param[i].name);
            WebPAStatus status = Utils::ConvertValueToParamType(parameter, static_cast<Variant::ParamType>(reqObj->u.setReq->param[i].type),  static_cast<std::string>(reqObj->u.setReq->param[i].value));
            parameters.push_back(parameter);
            parameterStatus.push_back(status);
        }
        status = _parameter->Values(parameters, parameterStatus);

        int i = 0;
        for (auto& parameter: parameters) {
            resObj->u.paramRes->params[i].name = strdup(parameter.Name().c_str());
            resObj->u.paramRes->params[i].type = WDMP_STRING;
            resObj->u.paramRes->params[i].value = nullptr;
            TRACE(Trace::Information, (_T("Response:> retStatus[%d] = %d"), i, parameterStatus[i]));
            resObj->retStatus[i] = static_cast<WDMP_STATUS>(parameterStatus[i]);
            i++;
        }
        parameters.clear();
        parameterStatus.clear();
    } else {
	resObj->retStatus[0] = static_cast<WDMP_STATUS>(status);
	TRACE(Trace::Error, (_T("Response:> resObj->retStatus[0] = %d"), resObj->retStatus[0]));
    }

    parameterNames.clear();
    return status;
}

WebPAStatus Adapter::Helper::SetAttributes(const req_struct*& reqObj, res_struct*& resObj)
{
    TRACE(Trace::Information, (_T("Request:> ParamCount = %zu"), reqObj->u.setReq->paramCnt));
    resObj->retStatus = static_cast<WDMP_STATUS*> (calloc(resObj->paramCnt, sizeof(WDMP_STATUS)));
    ASSERT(resObj->retStatus);

    AllocateParamResponse(reqObj, resObj, reqObj->u.setReq); // Need to be allocated for error case as well, since the wdmp-c lib is not validating the nullptr before accessing the members.
    UpdateRebootReason(reqObj);

    std::vector<std::string> parameterNames;
    WebPAStatus status = ParameterNamesOfSetters(reqObj, parameterNames);
    if ((status == WEBPA_SUCCESS) && (reqObj->u.setReq->paramCnt > 0)) {
        resObj->paramCnt = reqObj->u.setReq->paramCnt;

        std::map<std::string, Data> attributes;
        for (uint32_t i = 0; i < reqObj->u.setReq->paramCnt; i++) {
            Data attribute(reqObj->u.setReq->param[i].name, reqObj->u.setReq->param[i].value);
            attributes.insert(std::make_pair(parameterNames[i], attribute));
        }
        std::vector<WebPAStatus> attributeStatus;
        status = _attribute->Values(attributes, attributeStatus);

        int i = 0;
        for (auto& attribute: attributes) {
            resObj->u.paramRes->params[i].name = strdup(attribute.second.Name().c_str());
            resObj->u.paramRes->params[i].type = WDMP_STRING;
            resObj->u.paramRes->params[i].value = nullptr;
            TRACE(Trace::Information, (_T("Response:> retStatus[%d] = %d"), i, attributeStatus[i]));
            resObj->retStatus[i] = static_cast<WDMP_STATUS>(attributeStatus[i]);
            i++;
        }
        attributes.clear();
        attributeStatus.clear();
    } else {
        resObj->retStatus[0] = static_cast<WDMP_STATUS>(status);
        TRACE(Trace::Error, (_T("Response:> resObj->retStatus[0] = %d"), resObj->retStatus[0]));
    }

    parameterNames.clear();
    return status;
}

WebPAStatus Adapter::Helper::ParameterNamesOfGetters(const req_struct*& reqObj, std::vector<std::string>& parameterNames) const
{
    WebPAStatus status = WEBPA_SUCCESS;

    int paramCount = static_cast<int>(reqObj->u.getReq->paramCnt);

    for (int i = 0; i < paramCount; i++) {
        TRACE(Trace::Information, (_T("Request:> paramNames[%d] = %s"), i, reqObj->u.getReq->paramNames[i]));
        char* param = reqObj->u.getReq->paramNames[i];
        if (strlen(param) > MaxParameterNameLen) {
            status = WEBPA_ERR_INVALID_PARAMETER_NAME;
            break;
        } else {
            parameterNames.push_back(param);
        }
    }

    return status;
}

WebPAStatus Adapter::Helper::ParameterNamesOfSetters(const req_struct*& reqObj, std::vector<std::string>& parameterNames) const
{
    WebPAStatus status = WEBPA_SUCCESS;
    int paramCount = static_cast<int>(reqObj->u.setReq->paramCnt);

    for (int i = 0; i < paramCount; i++) {
        TRACE(Trace::Information, (_T("Request:> paramNames[%d] = %s"), i, reqObj->u.setReq->param[i].name));
        char* param = reqObj->u.setReq->param[i].name;
        if (strlen(param) > MaxParameterNameLen) {
            status = WEBPA_ERR_INVALID_PARAMETER_NAME;
            break;
        } else if (param[(strlen(param)-1)] == '.') {
            status = WEBPA_ERR_WILDCARD_NOT_SUPPORTED;
            break;
        } else {
            parameterNames.push_back(param);
        }
    }

    return status;
}
uint32_t Adapter::NotificationCallback::Worker()
{
    if ((_signaled.Lock(Core::infinite) == Core::ERROR_NONE) && (IsRunning() == true)) {
        _adminLock.Lock();
        NotificationHandler* handler = NotificationHandler::GetInstance();

        if (handler) {
            TRACE(Trace::Information, (_T("Got notification Instance")));

            do {
                NotifyData* notifyData = handler->NotificationData();
                if ((nullptr != notifyData) && (IsRunning() == true)) {
                    std::string notifySource = _parent->_notifier->Source();
                    std::string notifyDest = _parent->_notifier->Destination();

                    TRACE(Trace::Information, (_T("Calling Process request")));

                    std::string notifyPayload = _parent->_notifier->Process(*notifyData);

                    TRACE(Trace::Information, (_T("Notification Source = %s"), notifySource));
                    TRACE(Trace::Information, (_T("Notification Dest = %s"), notifyDest));

                    if (notifyPayload.empty() != true) {
                        TRACE(Trace::Information, (_T("Notification notifyPayload = %s"), notifyPayload));
                    } else {
                        TRACE(Trace::Information, (_T("Notification Payload is nullptr")));
                    }
                    if ((notifyPayload.empty() != true) && (notifySource.empty() != true) && (notifyDest.empty() != true)) {
                        if (_parent->_callback)
                            _parent->_callback->NotifyEvent(notifyPayload, notifySource, notifyDest);
                    } else {
                        TRACE(Trace::Error, (_T("Error in generating notification payload")));
                    }

                _parent->FreeNotificationData(notifyData);
                } else {
                    TRACE(Trace::Error, (_T("Notification Queue is Empty")));
                    break;
                }
            } while(true);
        }
        _adminLock.Unlock();
    }
    _signaled.ResetEvent();

    return Core::infinite;
}

void Adapter::Helper::UpdateRebootReason(const req_struct*& reqObj)
{
    // Detect device reboot through WEBPA and log message for device reboot through webpa
    for (uint32_t i = 0; i < reqObj->u.setReq->paramCnt; ++i) {
        if ((strcmp(reqObj->u.setReq->param[i].name, DeviceRebootParam) == 0) &&
            (strcmp(reqObj->u.setReq->param[i].value, DeviceRebootValue) == 0)) {

            TRACE(Trace::Information, (_T("REBOOT : Reboot triggered through WEBPA")));
            Data rebootParam("Device.DeviceInfo.UpdateRebootReason", "webpa-reboot");
            std::vector<Data> parameters = {rebootParam};
            std::vector<WebPAStatus> setStatus;
            WebPAStatus status = _parameter->Values(parameters, setStatus);
            if (status != WEBPA_SUCCESS) {
                TRACE(Trace::Error, (_T("Failed to set Reason with status %d"), status));
            } else {
                TRACE(Trace::Information, (_T("Successfully set Reason")));
            }
        }
    }
}

void Adapter::CurrentTime(struct timespec* timer)
{
    clock_gettime(CLOCK_REALTIME, timer);
}

void Adapter::FreeNotificationData(NotifyData* notifyData)
{
    TRACE(Trace::Information, (_T("%s:Start"), __FUNCTION__));
    if (notifyData) {
        WEBPA_FREE(notifyData->data.notify);
        WEBPA_FREE(notifyData);
    }
    TRACE(Trace::Information, (_T("%s:End"), __FUNCTION__));
}

} // WebPA
} // WPEFramework
