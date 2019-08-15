#pragma once

#include "WebPADataTypes.h"

#define MAX_PARAM_LENGTH (2*1024)
namespace WPEFramework {
namespace Utils {
    bool IsDigit(const std::string& str);
    bool MatchComponent(const std::string& paramName, const string& key, std::string& name, uint32_t& instance);

} // Utils
} // WPEFramework



