#pragma once

#include "Parameter.h"

namespace WPEFramework {
namespace WebPA {

class Notifier {
private:
   static constexpr const TCHAR* NotifyTypeStr = "VALUE_CHANGE_NOTIFICATION";
   static constexpr const TCHAR* DeviceMACParam = "Device.DeviceInfo.MACAddress";//FIXME: update it based on the actual param value
   static constexpr const TCHAR* UnknownParamValue = "Unknown";

private:
   class NotifierPayload : public Core::JSON::Container {
   public:
        NotifierPayload(const NotifierPayload&) = delete;
        NotifierPayload& operator=(const NotifierPayload&) = delete;

    public:
        NotifierPayload()
            : Core::JSON::Container()
            , DeviceID()
            , Type()
            , Name()
            , Value()
            , NotifyType()
        {
            Add(_T("device_id"), &DeviceID);
            Add(_T("datatype"), &Type);
            Add(_T("paramName"), &Name);
            Add(_T("paramValue"), &Value);
            Add(_T("notificationType"), &NotifyType);
        }
        virtual ~NotifierPayload()
        {
        }
    public:
        Core::JSON::String DeviceID;
        Core::JSON::DecUInt8 Type;
        Core::JSON::String Name;
        Core::JSON::Variant Value;
        Core::JSON::String NotifyType;
    };
    class NotifierList : public Core::JSON::Container {
    public:
        NotifierList(const NotifierList&) = delete;
        NotifierList& operator=(const NotifierList&) = delete;

    public:
        NotifierList()
            : Core::JSON::Container()
            , Notifiers()
        {
            Add(_T("notifiers"), &Notifiers);
        }
        virtual ~NotifierList()
        {
        }

    public:
        Core::JSON::ArrayType<Core::JSON::String> Notifiers;
    };

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
