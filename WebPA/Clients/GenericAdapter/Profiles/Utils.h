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

namespace WPEFramework {
class Utils {
public:
    Utils(const Utils&) = delete;
    Utils& operator= (const Utils&) = delete;

    Utils() {}
    ~Utils() {}

    static bool IsDigit(const std::string& str)
    {
        bool isDigit = true;
        for (uint8_t i = 0; i < str.length(); i++) {
            if (isdigit(str[i]) == false) {
                isDigit = false;
            }
        }
        return isDigit;
    }
    static bool MatchComponent(const std::string& paramName, const std::string& key, std::string& name, uint32_t& instance)
    {
        bool ret = false;
        instance = 0;

        if (!paramName.compare(0, key.length(), key)) {
            std::size_t position = paramName.find('.', key.length());
            if ((position != std::string::npos) && (position != key.length())) {

                std::string instanceStr = paramName.substr(key.length(), (position - key.length()));
                if (IsDigit(instanceStr) == true) {
                    std::string::size_type size = 0;
                    instance = std::stol(instanceStr.c_str(), &size);
                    name.assign(paramName.substr(position + 1, string::npos));
                } else {
                    name.assign(instanceStr);
                }
            } else {
                name.assign(paramName, key.length(), string::npos);
            }
            ret = true;
        }

        return ret;
    }
}; // Utils
} // WPEFramework



