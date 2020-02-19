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
#include "Module.h"
#include "Handler.h"
#include "Utils.h"

#include <tinyxml.h>

namespace WPEFramework {

typedef enum
{
    DM_SUCCESS = 0,
    DM_FAILURE,
    DM_ERR_WILDCARD_NOT_SUPPORTED,
    DM_ERR_INVALID_PARAMETER,
    DM_ERR_TIMEOUT,
    DM_ERR_NOT_EXIST
}
DMStatus;

class DataModel {
private:
    static constexpr const uint32_t  MaxNumParameters = 2048;
    static constexpr const TCHAR* InstanceNumberIndicator = "{i}.";

public:
    DataModel() = delete;
    DataModel(const DataModel&) = delete;
    DataModel& operator= (const DataModel&) = delete;
public:
    DataModel(Handler* handler);
    ~DataModel();

    DMStatus LoadDM(const std::string& filename);
    DMStatus Parameters(const std::string& paramName, std::map<uint32_t, std::pair<std::string, std::string>>& paramList) const;
    bool IsValidParameter(const std::string& paramName, std::string& dataType) const;
    int DMHandle() { return _dmHandle; }

private:
    TiXmlNode* Parameters(TiXmlNode* pParent, const std::string& paramName, std::string& currentParam, std::map<uint32_t, std::pair<std::string, std::string>>& paramList) const;
    void CheckforParameterMatch(TiXmlNode *pParent, const std::string& paramName, bool& pMatch, std::string& dataType) const;
    bool IsParamEndsWithInstance(const std::string& paramName) const;
    void ReplaceWithInstanceNumber(string& paramName, uint16_t instanceNumber) const;
    bool CheckMatchingParameter(const char* attrValue, const char* paramName, uint32_t& ret) const;
    bool ValidateParameterInstance(const std::string& paramName, std::string& dataType) const;
    uint8_t FindInstanceOccurance(const std::string& paramName, std::map<uint8_t, std::pair<std::size_t, std::size_t>>& positions) const;
    int16_t ParameterInstanceCount(const std::string& paramName) const;

private:
    int _dmHandle;
    Handler* _handler;
};
}
