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

#include "WebPADataTypes.h"

#define MAX_PARAM_LENGTH (2 * 1024)
#define WEBPA_FREE(__x__) if(__x__ != nullptr) { free((void*)(__x__)); __x__ = nullptr;}

namespace WPEFramework {
class Utils {
public:
    Utils(const Utils&) = delete;
    Utils& operator= (const Utils&) = delete;

    Utils() {}
    ~Utils() {}

    static bool IsWildCardParam(const std::string& paramName)
    {
        bool isWildCard = false;
        if (paramName.empty() != true) {
            size_t position = paramName.find_last_of(".");
            if ((position != std::string::npos) && (position == (paramName.length() - 1))) {
                isWildCard = true;
            }
        }
        return isWildCard;
    }

    static Variant::ParamType ConvertToParamType(const std::string& dataType)
    {
        Variant::ParamType paramType = Variant::ParamType::TypeNone;
        if (dataType == "string") {
            paramType = Variant::ParamType::TypeString;
        } else if (dataType == "unsignedInt") {
            paramType = Variant::ParamType::TypeUnsignedInteger;
        } else if (dataType == "int") {
            paramType = Variant::ParamType::TypeInteger;
        } else if (dataType == "unsignedLong") {
            paramType = Variant::ParamType::TypeUnsignedLong;
        } else if (dataType == "boolean") {
            paramType = Variant::ParamType::TypeBoolean;
        } else if (dataType == "hexBinary") {
            paramType = Variant::ParamType::TypeString;
        } else {
            paramType = Variant::ParamType::TypeString;
        }
        return paramType;
    }

    static std::string ConvertParamValueToString(const Data& param)
    {
        char strValue[MAX_PARAM_LENGTH];
        memset(strValue, 0, MAX_PARAM_LENGTH);

        switch (param.Value().Type())
        {
        case Variant::ParamType::TypeInteger:
            snprintf(strValue, MAX_PARAM_LENGTH, "%d", param.Value().Integer());
            break;
        case Variant::ParamType::TypeBoolean:
            snprintf(strValue, MAX_PARAM_LENGTH, "%d", param.Value().Boolean());
            break;
        case Variant::ParamType::TypeUnsignedInteger:
            snprintf(strValue, MAX_PARAM_LENGTH, "%u", static_cast<uint32_t>(param.Value().UnsignedInteger()));
            break;
        case Variant::ParamType::TypeUnsignedLong:
            snprintf (strValue, MAX_PARAM_LENGTH, "%lu", static_cast<unsigned long int>(param.Value().UnsignedLong()));
            break;
        case Variant::ParamType::TypeString:
            strncat(strValue, param.Value().String().c_str(), param.Value().String().length());
            break;
        case Variant::ParamType::TypeDateTime:
            snprintf (strValue, MAX_PARAM_LENGTH, "%lu", static_cast<unsigned long int>(param.Value().DateTime()));
            break;
        case Variant::ParamType::TypeBase64:
        {
            std::vector<unsigned char> base64Data = param.Value().Base64();
            for (uint16_t i = 0; i < base64Data.size(); i++) {
                strValue[i] = base64Data[i];
            }
            break;
        }
        default: //Ignore the value
            break;
        }
        return strValue;
    }

    static WebPAStatus ConvertFaultCodeToWPAStatus(const FaultCode faultCode)
    {
        WebPAStatus retWalStatus = WEBPA_FAILURE;
        switch(faultCode)
        {
        case FaultCode::NoFault:
            retWalStatus = WEBPA_SUCCESS;
            break;
        case FaultCode::RequestDenied:
            retWalStatus = WEBPA_FAILURE;
            break;
        case FaultCode::AttemptToSetaNonWritableParameter:
            retWalStatus = WEBPA_ERR_NOT_WRITABLE;
            break;
        case FaultCode::InvalidParameterName:
            retWalStatus = WEBPA_ERR_INVALID_PARAMETER_NAME;
            break;
        case FaultCode::InvalidParameterType:
            retWalStatus = WEBPA_ERR_INVALID_PARAMETER_TYPE;
            break;
        case FaultCode::InvalidParameterValue:
            retWalStatus = WEBPA_ERR_INVALID_PARAMETER_VALUE;
            break;
        case FaultCode::MethodNotSupported:
            retWalStatus = WEBPA_ERR_METHOD_NOT_SUPPORTED;
            break;
        default:
            retWalStatus = WEBPA_FAILURE;
            break;
        }
        return retWalStatus;
    }

    static WebPAStatus ConvertValueToParamType(Data& parameter, const Variant::ParamType actualType, const string& paramStrValue)
    {
        WebPAStatus status = WEBPA_SUCCESS;
        std::string strValue = Trim(paramStrValue.c_str(), " ");
        switch (actualType)
        {
        case Variant::ParamType::TypeString: {
            Variant value(strValue);
            parameter.Value(value);
            break;
        }
        case Variant::ParamType::TypeInteger: {
            std::string::size_type size;
            Variant value(static_cast<int>(std::stoi(strValue, &size)));
            if (size != strValue.length()) {
                status =  WEBPA_ERR_INVALID_PARAMETER_VALUE;
            }
            parameter.Value(value);
            break;
        }
        case Variant::ParamType::TypeUnsignedInteger: {
            std::string::size_type size;
            Variant value(static_cast<unsigned int>(std::stoul(strValue, &size)));
            if (size != strValue.length()) {
                status =  WEBPA_ERR_INVALID_PARAMETER_VALUE;
            }
            parameter.Value(value);
            break;
        }
        case Variant::ParamType::TypeBoolean: {
            bool boolValue = false;
            if ((strValue == "1") || (strValue == "true")) {
                boolValue = static_cast<bool>(true);
            } else if ((strValue == "0") || (strValue == "false")) {
                boolValue =static_cast<bool>(false);
            } else {
                status = WEBPA_ERR_INVALID_PARAMETER_VALUE;
            }
            Variant value(boolValue);
            parameter.Value(value);
            break;
        }
        case Variant::ParamType::TypeDateTime: {
            std::string::size_type size;
            Variant value(static_cast<unsigned int>(std::stoul(strValue, &size, 0)));
            if (size != strValue.length()) {
                status =  WEBPA_ERR_INVALID_PARAMETER_VALUE;
            }
            parameter.Value(value);
            break;
        }
        case Variant::ParamType::TypeLong: {
            std::string::size_type size;
            Variant value(static_cast<long>(std::stol(strValue, &size, 0)));
            if (size != strValue.length()) {
                status =  WEBPA_ERR_INVALID_PARAMETER_VALUE;
            }
            parameter.Value(value);
            break;
        }
        case Variant::ParamType::TypeUnsignedLong: {
            std::string::size_type size;
            Variant value(static_cast<unsigned long>(std::stoul(strValue, &size, 0)));
            if (size != strValue.length()) {
                status =  WEBPA_ERR_INVALID_PARAMETER_VALUE;
            }
            parameter.Value(value);
            break;
        }
        case Variant::ParamType::TypeFloat: {
            std::string::size_type size;
            Variant value(static_cast<float>(std::stof(strValue, &size)));
            if (size != strValue.length()) {
                status =  WEBPA_ERR_INVALID_PARAMETER_VALUE;
            }
            parameter.Value(value);
            break;
        }
        case Variant::ParamType::TypeDouble: {
            std::string::size_type size;
            Variant value(static_cast<double>(std::stod(strValue, &size)));
            if (size != strValue.length()) {
                status =  WEBPA_ERR_INVALID_PARAMETER_VALUE;
            }
            break;
        }
        case Variant::ParamType::TypeByte: {
            Variant value(static_cast<unsigned char>(strValue.at(0)));
            parameter.Value(value);
            break;
        }
        default:
            break;
        }
        return status;
    }

private:
    static std::string Trim(const std::string& str, const std::string& substr)
    {
        std::string tempStr = str;
        size_t start = tempStr.find_first_not_of(substr);
        tempStr.assign((start == std::string::npos) ? "" : tempStr.substr(start));

        size_t end = tempStr.find_last_not_of(substr);
        return (end == std::string::npos) ? "" : tempStr.substr(0, end + 1);
    }

}; // Utils

} // WPEFramework
