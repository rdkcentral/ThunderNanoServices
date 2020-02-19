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
 
#include "Attribute.h"

namespace WPEFramework {
namespace WebPA {

Attribute::Attribute(Notifier* notifier, Handler* handler)
    : _notifier(notifier)
    , _handler(handler)
    , _adminLock()
{
}

Attribute::~Attribute()
{
}

void Attribute::Values(const std::vector<std::string>& parameterNames, std::map<Data, WebPAStatus>& attributesList) const
{
    for (auto& name: parameterNames) {
        Data attribute(name, static_cast<bool>(0));
        const WebPAStatus ret = Values(name, attribute);
        if (ret == WEBPA_SUCCESS) {
            TRACE(Trace::Information, (_T( "Parameter Name: %s has attribute"), name.c_str()));
        }
        attributesList.insert(std::make_pair(attribute, ret));
    }
}

WebPAStatus Attribute::Values(const std::map<std::string, Data>& attributes, std::vector<WebPAStatus>& status)
{
    TRACE(Trace::Information, (_T("[%s:%s:%d] Inside Values, Param Count = %d"), __FILE__, __FUNCTION__, __LINE__, attributes.size()));
    WebPAStatus ret = WEBPA_FAILURE;

    for (auto& attribute: attributes) {
        ret = Values(attribute.first, attribute.second);
        status.push_back(ret);
        if (ret != WEBPA_SUCCESS) {
            break;
        }
    }
    return ret;
}

const WebPAStatus Attribute::Values(const std::string& parameterName, Data& attribute) const
{
    WebPAStatus ret = WEBPA_FAILURE;

    std::vector<std::string> notifyParameters;

    // Read Notify configuration file
    int status = _notifier->Parameters(notifyParameters);
    if (status == Core::ERROR_NONE) {
        // Check if parameterName is in the list of notification parameters and check if the parameter is one among them
        for (auto& notifyParameter: notifyParameters) {
            if (!parameterName.compare(notifyParameter)) {
                Data param(parameterName);
                _adminLock.Lock();
                ret = Utils::ConvertFaultCodeToWPAStatus((static_cast<const Handler&>(*_handler)).Attribute(param));
                _adminLock.Unlock();
                if (ret == WEBPA_SUCCESS) {
                    attribute = param;
                }
                break;
            }
        }
    }
    return ret;
}

WebPAStatus Attribute::Values(const std::string& parameterName, const Data& attribute)
{
    WebPAStatus ret = WEBPA_FAILURE;

    std::vector<std::string> notifyParameters;

    // Read Notify configuration file
    uint32_t status = _notifier->Parameters(notifyParameters);

    // Enable only for notification parameters in the config file
    if (status == Core::ERROR_NONE) {
        for (auto& notifyParameter: notifyParameters) {
            if (!parameterName.compare(notifyParameter)) {
                TRACE(Trace::Information, (_T("[%s:%s:%d] Inside Values, Param:%s is found"), __FILE__, __FUNCTION__, __LINE__, parameterName.c_str()));
                const Data param(parameterName, attribute.Value());
                _adminLock.Lock();
                ret = Utils::ConvertFaultCodeToWPAStatus(_handler->Attribute(param));
                _adminLock.Unlock();
                break;
            }
        }
    }
    return ret;
}

} // WebPA
} // WPEFramework
