#pragma once

#include "Parameter.h"

namespace WPEFramework {
namespace WebPA {

class Notifier {
private:
   static constexpr const TCHAR* NotifyTypeStr = "VALUE_CHANGE_NOTIFICATION";
   static constexpr const TCHAR* DeviceStbMACParam = "Device.DeviceInfo.STB_MAC";
   static constexpr const TCHAR* UnknownParamValue = "Unknown";

public:
    Notifier() = delete;
    Notifier(const Notifier&) = delete;
    Notifier& operator= (const Notifier&) = delete;

public:
    Notifier(Parameter* parameter);
    virtual ~Notifier();

    void ConfigurationFile(const std::string& nofityConfigFile);
    uint32_t Parameters(std::vector<std::string>& notifyParameters);
    std::string Process(const NotifyData& notifyData);
    std::string Destination();
    std::string Source();

private:
    char CharToLower(char c);
    void StringToLower(string& str);

private:
    std::string _configFile;
    std::string _notificationSource;

    Parameter* _parameter;

    Core::CriticalSection _adminLock;
};

} // WebPA
} // WPEFramework
