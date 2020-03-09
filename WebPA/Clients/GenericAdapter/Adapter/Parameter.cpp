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
 
#include "Parameter.h"

namespace WPEFramework {
namespace WebPA {

Parameter::Parameter(Handler* handler, DataModel* dataModel)
    : _dataModel(dataModel)
    , _handler(handler)
    , _adminLock()
{
}

Parameter::~Parameter()
{
}
const void Parameter::Values(const std::vector<std::string>& parameterNames, std::map<std::vector<Data>, WebPAStatus>& parametersList) const
{
    for (auto& name: parameterNames) {
        std::vector<Data> parameters;

        WebPAStatus ret = Values(name, parameters);
        parametersList.insert(std::make_pair(parameters, ret));
        if ((ret == WEBPA_SUCCESS) && (parameters.size() > 0)) {
            TRACE(Trace::Information, (_T( "Parameter Name: %s return: %d"), name.c_str(), parameters.size()));
        } else {
            TRACE(Trace::Information, (_T( "Parameter Name: %s return no value, so keeping empty values to get the status")));
        }
    }
}

WebPAStatus Parameter::Values(const std::vector<Data>& parameters, std::vector<WebPAStatus>& status)
{
    WebPAStatus ret = WEBPA_SUCCESS;
    for (uint16_t i = 0; i < parameters.size(); ++ i) {

        ret = Values(parameters[i]);
        status[i] = ret;

    }
    return ret;
}

const WebPAStatus Parameter::Values(const std::string& parameterName, std::vector<Data>& parameters) const
{
    WebPAStatus status = WEBPA_FAILURE; // Overall get status

    int32_t dmHandle = _dataModel->DMHandle();

    if (dmHandle) {
        if (Utils::IsWildCardParam(parameterName)) { // It is a wildcard Param
            /* Translate wildcard to list of parameters */
            std::map<uint32_t, std::pair<std::string, std::string>> dmParamters;
            DMStatus dmRet = _dataModel->Parameters(parameterName, dmParamters);
            if (dmRet == DM_SUCCESS && dmParamters.size() > 0) {
                for (auto&  dmParamter:  dmParamters) {
                    Variant value(Utils::ConvertToParamType(dmParamter.second.second));
                    Data param(dmParamter.second.first, value);

                    _adminLock.Lock();
                    WebPAStatus ret = Utils::ConvertFaultCodeToWPAStatus((static_cast<const Handler&>(*_handler)).Parameter(param));
                    _adminLock.Unlock();

                    // Fill Only if we can able to get Proper value
                    if (WEBPA_SUCCESS == ret) {
                        parameters.push_back(param);
                        status = WEBPA_SUCCESS; //Set status as success, if there is atleast one parameter
                    }
                } // End of Wild card for loop
            } else {
                TRACE(Trace::Error, (_T( " Wild card Param list is empty")));
                status = WEBPA_FAILURE;
            }
            dmParamters.clear();

        } else { // Not a wildcard Parameter Lets fill it
            TRACE(Trace::Information, (_T( "Get Request for a Non-WildCard Parameter")));
            std::string dataType;

            if (_dataModel->IsValidParameter (parameterName, dataType)) {
                TRACE(Trace::Information, (_T( "Valid Parameter..! ")));
                Variant value(Utils::ConvertToParamType(dataType));
                Data param(parameterName, value);

                // Convert param.paramType to ParamVal.type
                TRACE(Trace::Information, (_T( " Values parameterType is %d"), dataType));
                _adminLock.Lock();
                status = Utils::ConvertFaultCodeToWPAStatus((static_cast<const Handler&>(*_handler)).Parameter(param));
                _adminLock.Unlock();
                if (WEBPA_SUCCESS == status) {
                    parameters.push_back(param);
                } else {
                    TRACE(Trace::Error, (_T( "Failed Get Param Values From Handler: for Param Name :-  %s"), parameterName));
                }
            } else {
                TRACE(Trace::Error, (_T( "Invalid Parameter Name  :-  %s"), parameterName));
                status = WEBPA_ERR_INVALID_PARAMETER_NAME;
            }
        }
    } else {
        TRACE(Trace::Error, (_T( "Data base Handle is not Initialized %s"), parameterName));
    }
    return status;
}

WebPAStatus Parameter::Values(const Data& parameter)
{
    WebPAStatus ret = WEBPA_FAILURE;

    int32_t dmHandle = _dataModel->DMHandle();

    if (dmHandle) {

        std::string dataType;
        if (_dataModel->IsValidParameter(parameter.Name(), dataType)) {
            if (Utils::ConvertToParamType(dataType) == parameter.Value().Type()) {

                _adminLock.Lock();
                ret = Utils::ConvertFaultCodeToWPAStatus(_handler->Parameter(parameter));
                _adminLock.Unlock();
                TRACE(Trace::Information, (_T("handler::Parameter %d"), ret));
            } else {
                ret = WEBPA_ERR_INVALID_PARAMETER_TYPE;
            }
        } else {
            TRACE(Trace::Error, (_T(" Invalid Parameter name %s"), parameter.Name()));
            ret = WEBPA_ERR_INVALID_PARAMETER_NAME;
        }
    }
    return ret;
}

} // WebPA
} // WPEFramework
