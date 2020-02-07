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
 
#include "DataModel.h"

namespace WPEFramework {

DataModel::DataModel(Handler* handler)
    : _dmHandle(0)
    , _handler(handler)
{
}

DataModel::~DataModel()
{
    if (_dmHandle) {
        TiXmlDocument* doc = reinterpret_cast<TiXmlDocument*>(_dmHandle);
        delete (doc);
    }
}

DMStatus DataModel::LoadDM(const std::string& filename)
{
    TiXmlDocument* doc = new TiXmlDocument(filename.c_str());
    DMStatus status = DM_FAILURE;

    if (nullptr != doc) {
        bool loadOK = doc->LoadFile();
        if (loadOK) {
            _dmHandle = reinterpret_cast<int>(doc);
            status = DM_SUCCESS;
        } else {
            _dmHandle = 0;
        }
    } else {
        _dmHandle = 0;
    }
    return status;
}

int16_t DataModel::ParameterInstanceCount(const std::string& paramName) const
{
    int16_t instanceCount = 0;

    if (paramName.empty() != true) {
        std::size_t found = paramName.find(InstanceNumberIndicator);
        if (found != std::string::npos) {

            // Get the number of instances from Adapter
            std::string paramType;

            if (IsValidParameter (paramName, paramType)) {
                TRACE(Trace::Information, (_T( "Valid Parameter..! ")));
                string name(paramName, 0, found - 1 );
                Data param(name + "NumberOfEntries", static_cast<const int>(0));

                FaultCode status = (static_cast<const Handler&>(*_handler)).Parameter(param);
                if (status != FaultCode::NoFault) {
                    TRACE(Trace::Error, (_T("[%s:%s:%d] Error in Get Message Handler : faultCode = %d"), __FILE__, __FUNCTION__, __LINE__, status));
                } else {
                    TRACE(Trace::Information, (_T("[%s:%s:%d] The value for param: %s is %d"), __FILE__, __FUNCTION__, __LINE__, param.Name().c_str(), param.Value().Integer()));
                    instanceCount = param.Value().Integer();
                }

            }
        }
    }
    return instanceCount;
}


void DataModel::ReplaceWithInstanceNumber(string& paramName, uint16_t instanceNumber) const
{
    if (paramName.empty() != true) {
        std::size_t position = paramName.find(InstanceNumberIndicator);
        if (position != std::string::npos) {
            std::string number = std::to_string(instanceNumber);
            paramName.replace(position, strlen(InstanceNumberIndicator) - 1, number);
        }
    }
}

bool DataModel::CheckMatchingParameter(const char* attrValue, const char* paramName, uint32_t& ret) const
{
    bool status = false;

    int i = 10;
    int inst = 0;

    while (true) {
        if (!(*attrValue && *paramName && (*attrValue == *paramName))) {

            ret = 0;
            if (*attrValue == '{' && *paramName >= 48 && *paramName <= 56) {
                attrValue += 3;
                while(*paramName &&* paramName != '.') {
                    ret = ret*i + (*paramName - 48);
                    paramName++;
                    inst = ret;
                }
            } else {

                ret = 0;
                if (!*paramName) {
                    if (!*attrValue && (*(attrValue - 2) == '}')) {
                        ret = inst;
                    }
                    status = true;
                }
                break;
            }
        }
        attrValue++;
        paramName++;
    }
    return status;
}

TiXmlNode* DataModel::Parameters(TiXmlNode* parent, const std::string& paramName, std::string& currentParam, std::map<uint32_t, std::pair<std::string, std::string>>& paramList) const
{
    // If parent is Null Return
    if (!parent) {
        return nullptr;
    }

    int isReccursiveCall = 0;
    // Identify whether the call is recursive or initial call
    if (currentParam.empty() != true) {
        isReccursiveCall = 1;
    }

    TiXmlNode* child;
    // Goto actual Object node ie "Device."
    if (parent->Type() != TiXmlNode::TINYXML_ELEMENT) {
        for (child = parent->FirstChild(); child != 0;) {
            if (child->Type() != TiXmlNode::TINYXML_ELEMENT) {
                child = child->NextSibling();
            } else {
                if (!strcmp (child->Value(), "object"))
                    break;
                child = child->FirstChild();
            }
        }
    } else {
        child = parent;
    }

    std::string zeroInstance;
    while (child) {
        TiXmlElement* pElement =  child->ToElement();
        TiXmlAttribute* pAttrib = pElement->FirstAttribute();
        uint32_t inst = 0;
        int status = 0;

        // Check if node is an Object
        if (!strcmp(child->Value(),"object")) {
            if (strstr(pAttrib->Value(), paramName.c_str()) || (status = CheckMatchingParameter(pAttrib->Value(), paramName.c_str(), inst))) {
                // If the number of instances are 0 then skip this object and go to next sibling
                if ((zeroInstance.empty() != true) && strstr(pAttrib->Value(), zeroInstance.c_str())) {
                    child = child->NextSibling();
                    continue;
                } else if (zeroInstance.empty() != true) {
                    zeroInstance = "";
                }
                // If matching found update the current parameter with wild card input std::string
                if (status && !isReccursiveCall) {
                    currentParam = paramName;
                }
                currentParam = pAttrib->Value();
                TiXmlNode* bChild, *sChild;
                bChild = child;
                // Goto the parameters
                child = child->FirstChild();

                // Object not having any parameter thus go to next Sibling
                if (nullptr == child) {
                    child = bChild->NextSibling();
                }

                char* endPtr = nullptr;
                const std::string maxEntries = pElement->Attribute("maxEntries");
                // Seems like a {i} instance
                if (maxEntries.empty() != true && ((!strcmp(maxEntries.c_str(),"unbounded")) || (strtol(maxEntries.c_str(), &endPtr, 10) > 1))) {
                    // Make Sure that its ends with {i}
                    if (IsParamEndsWithInstance(pAttrib->Value()) == true ) {
                        uint16_t instanceNumber = 0;
                        uint16_t i = 1;
                        // Get the Number of instances for that attribute
                        uint16_t actualInstance = ParameterInstanceCount(pAttrib->Value());
                        if (inst) {
                            // Check if valid instance count is given in input wild card if not make it as zero, this will skip current branch
                            if (actualInstance >= inst)
                                i = instanceNumber = inst;
                            else
                                instanceNumber = 0;
                        } else {
                            instanceNumber = actualInstance;

                        }
                        sChild = child;
                        // Number of instances are > 0 go through each and populate data for each instance
                        std::string tempParamName;
                        while (i <= instanceNumber) {
                            tempParamName = child->Parent()->ToElement()->FirstAttribute()->Value();
                            currentParam = tempParamName;

                            // Replace {i} with current instance number and call recursively
                            ReplaceWithInstanceNumber(currentParam, i);

                            sChild = Parameters(child, tempParamName, currentParam, paramList);
                            i++;
                        }
                        child = sChild;
                        // Seems like instance count is empty
                        if (!instanceNumber) {
                            zeroInstance = pAttrib->Value();
                            child = child->Parent();
                        }
                    }
                }
            }
            else if (isReccursiveCall) { // Tree found once and processed and going to another branch so break
                return child;
            }
            else { // Tree not found yet goto next sibling and get it
                child = child->NextSibling();
            }
        }
        // Found the Parameter
        else if (!strcmp(child->Value(),"parameter")) {
            TiXmlNode* bChild;
            // Find all parameters
            for (bChild = child ; child ; child=child->NextSibling()) {
                if (paramList.size() <= MaxNumParameters) {
                    if (currentParam.length() > 0) {
                        if (child->ToElement()->Attribute("getIdx") && strtol(child->ToElement()->Attribute("getIdx"),nullptr,10) >= 1) {
                            paramList.insert(std::make_pair(paramList.size(), std::make_pair(currentParam + child->ToElement()->FirstAttribute()->Value(), child->FirstChild()->FirstChild()->Value())));
                        }
                    }
                }
            }
            // Go to next object
            child = bChild->Parent();
            child = child->NextSibling();
        }
    }
    return child;
}

DMStatus DataModel::Parameters(const std::string& paramName, std::map<uint32_t, std::pair<std::string, std::string>>& paramList) const
{
    std::string currentParam;
    std::string parameterName = paramName;

    ASSERT(_dmHandle != 0);
    DMStatus status = DM_SUCCESS;
    if (Utils::IsWildCardParam(parameterName)) {
        Parameters(reinterpret_cast<TiXmlDocument*>(_dmHandle), parameterName, currentParam, paramList);
        if (paramList.size() == 0) {
            status = DM_ERR_INVALID_PARAMETER;
        }
    } else {
        status = DM_ERR_WILDCARD_NOT_SUPPORTED;
    }
    return status;
}

void DataModel::CheckforParameterMatch(TiXmlNode* parent, const std::string& paramName, bool& match, std::string& dataType) const
{
    if (!parent)
        return;

    static bool isObject = false;
    static bool isMatched = false;

    if (parent->Type() == TiXmlNode::TINYXML_ELEMENT) {
        TiXmlElement* pElement = parent->ToElement();
        TiXmlAttribute* pAttrib = pElement->FirstAttribute();
        if (!strcmp(parent->Value(), "object")) {
            isObject = true;
        }

        if (pAttrib) {
            // Construct Object without parameter from input ParamName
            std::size_t found = paramName.find_last_of(".");

            std::string tempObject;
            if (found != std::string::npos) {
                tempObject.assign(paramName, 0, found + 1);
            } else {
                tempObject = paramName;
            }

            static std::string objectName;
            if (!strcmp(pAttrib->Value(), tempObject.c_str())) {
                objectName = pAttrib->Value();
                isMatched = true;
            }

            if (isMatched || !isObject) {
                if (objectName == paramName) {
                    match = true;
                    return;
                } else {
                    isObject = 0;
                    if (!strcmp(parent->Value(), "parameter")) {
                        std::string nameFromParamater = objectName + pAttrib->Value();

                        if ((objectName + pAttrib->Value()) == paramName) {
                            dataType = parent->FirstChild()->FirstChild()->Value();
                            match = true;
                            return;
                        }
                    }
                }
            }
        }
    }

    for (TiXmlNode* child = parent->FirstChild(); child != 0; child = child->NextSibling()) {
        CheckforParameterMatch(child, paramName, match, dataType);
        if (match == true) {
            break;
        }
    }
    isMatched = 0;
}

uint8_t DataModel::FindInstanceOccurance(const std::string& paramName, std::map<uint8_t, std::pair<std::size_t, std::size_t>>& positions) const
{
    uint8_t count = 0;
    std::string name = paramName;
    std::size_t separator = 0;
    while (true) {
        std::size_t position =  name.find_first_of("0123456789", separator);
        if (position != std::string::npos) {
            separator = name.find(".", position);
            if (separator != std::string::npos) {
                positions.insert(std::make_pair(count, std::make_pair(position, separator)));
                count++;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    return count;
}

bool DataModel::ValidateParameterInstance(const std::string& paramName, std::string& dataType) const
{
    bool valid = false;

    std::map<uint8_t, std::pair<std::size_t, std::size_t>> positions;
    uint8_t occurrences = FindInstanceOccurance(paramName, positions);

    for (uint8_t i = occurrences; (i > 0) && (valid != true); --i) {
        uint8_t index = occurrences - i;

        std::string tempName = paramName;
        std::map<uint8_t, std::pair<std::size_t, std::size_t>>::iterator position = positions.find(index);
        if (positions.end() != position) {
            tempName.replace(position->second.first, ((position->second.second + 1) - position->second.first), InstanceNumberIndicator);
            uint16_t maxCombinations = pow(2, i - 1);
            for (uint16_t j = 0; j < maxCombinations; ++j) {
                std::string name = tempName;
                int8_t newPosition = 0;
                for (uint8_t k = 0; k < i; ++k) {
                    if ((j & (1 << k))) {

                        index = (occurrences - 1) - k;
                        std::map<uint8_t, std::pair<std::size_t, std::size_t>>::iterator position = positions.find(index);
                        if (positions.end() != position) {

                            newPosition = strlen(InstanceNumberIndicator) - ((position->second.second + 1) - position->second.first);
                            name.replace(position->second.first + newPosition, ((position->second.second + 1) - position->second.first), InstanceNumberIndicator);
                        }
                    }
                }
                CheckforParameterMatch(reinterpret_cast<TiXmlDocument*>(_dmHandle), name, valid, dataType);
                if (valid == true) {
                    break;
                }
            }
        }
    }
    return valid;
}

bool DataModel::IsValidParameter(const std::string& paramName, std::string& dataType) const
{
    bool valid = false;
    ASSERT(_dmHandle != 0);
    CheckforParameterMatch(reinterpret_cast<TiXmlDocument*>(_dmHandle), paramName, valid, dataType);
    if (valid != true) {
        valid = ValidateParameterInstance(paramName, dataType);
    }
    return valid;
}

bool DataModel::IsParamEndsWithInstance(const std::string& paramName) const
{
    bool isInstance = false;

    if (paramName.empty() != true) {
        std::size_t position = paramName.find(string(InstanceNumberIndicator));
        if ((position != std::string::npos) && ((position + strlen(InstanceNumberIndicator)) != std::string::npos)) {
            isInstance = true;
        }
    }
    return isInstance;
}
}
