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



