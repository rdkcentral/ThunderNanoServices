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

#include "Handler.h"
#include "Utils.h"
#include "DataModel.h"

namespace WPEFramework {

namespace WebPA {

typedef enum
{
    WEBPA_SET = 0,
    WEBPA_ATOMIC_SET,
    WEBPA_ATOMIC_SET_XPC
} WEBPA_SET_TYPE;

class Parameter {

public:
    Parameter() = delete;
    Parameter(const Parameter&) = delete;
    Parameter& operator= (const Parameter&) = delete;

public:
    Parameter(Handler* handler, DataModel* dataModel);
    virtual ~Parameter();

    const void Values(const std::vector<std::string>& parameterNames, std::map<std::vector<Data>, WebPAStatus>& parametersList) const;
    WebPAStatus Values(const std::vector<Data>& parameters, std::vector<WebPAStatus>& status);

private:
    const WebPAStatus Values(const std::string& parameterName, std::vector<Data>& parameters) const;
    WebPAStatus Values(const Data& parameter);

private:
    DataModel* _dataModel;
    Handler* _handler;

    mutable Core::CriticalSection _adminLock;
};

} // WebPA
} // WPEFramework
