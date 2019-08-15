#include "cJSON.h"
#include "Notifier.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <wdmp-c.h>
#ifdef __cplusplus
}
#endif

namespace WPEFramework {
namespace WebPA {

Notifier::Notifier(Parameter* parameter)
    : _configFile("")
    , _notificationSource("")
    , _parameter(parameter)
    , _adminLock()
{
}

Notifier::~Notifier()
{
}

void Notifier::ConfigurationFile(const std::string& configFile)
{
    if (configFile.empty() != true) {
        _adminLock.Lock();
        _configFile = configFile;
        TRACE(Trace::Information, (_T("Notify Configuration file set %s \n"), _configFile.c_str()));
        _adminLock.Unlock();
    } else {
        TRACE(Trace::Error, (_T("Unable to set Notify Configuration file\n")));
    }
}

string Notifier::Process(const NotifyData& notifyData)
{
    TRACE(Trace::Information, (_T("%s:Start"), __FUNCTION__));
    std::string payload;
    switch (notifyData.type)
    {
    case PARAM_NOTIFY:
    case TRANS_STATUS:
    case CONNECTED_CLIENT_NOTIFY:
    case UPSTREAM_MSG:
    case PARAM_VALUE_CHANGE_NOTIFY:
    {
        ParamNotify* param = notifyData.data.notify;
        if (param) {
            cJSON* notifyPayload = cJSON_CreateObject();
            cJSON_AddStringToObject(notifyPayload, "device_id", Source().c_str());
            cJSON_AddNumberToObject(notifyPayload, "datatype", param->Value().Type());
            cJSON_AddStringToObject(notifyPayload, "paramName", param->Name().c_str());
            cJSON_AddStringToObject(notifyPayload, "notificationType", NotifyTypeStr);
            switch(param->Value().Type())
            {
            case Variant::ParamType::TypeString:
            {
                cJSON_AddStringToObject(notifyPayload, "paramValue", param->Value().String().c_str());
                break;
            }
            case Variant::ParamType::TypeInteger:
            {
                const int32_t* paramNewValue = reinterpret_cast<const int32_t*>(param->Value().Integer());
                cJSON_AddNumberToObject(notifyPayload, "paramValue",* paramNewValue);
                break;
            }
            case Variant::ParamType::TypeUnsignedInteger:
            {
                const uint32_t* paramNewValue = reinterpret_cast<const uint32_t*>(param->Value().UnsignedInteger());
                cJSON_AddNumberToObject(notifyPayload, "paramValue",* paramNewValue);
                break;
            }
            case Variant::ParamType::TypeBoolean:
            {
                const bool*  paramNewValue = reinterpret_cast<const bool*>(param->Value().Boolean());
                cJSON_AddBoolToObject(notifyPayload, "paramValue",* paramNewValue);
                break;
            }
            case Variant::ParamType::TypeUnsignedLong:
            {
                const uint64_t* paramNewValue = reinterpret_cast<const uint64_t*>(param->Value().UnsignedLong());
                cJSON_AddNumberToObject(notifyPayload, "paramValue",* paramNewValue);
                break;
            }
            default:
            {
                cJSON_AddStringToObject(notifyPayload, "paramValue", param->Value().String().c_str());
                break;
            }
            }
            payload = cJSON_PrintUnformatted(notifyPayload);
            TRACE(Trace::Information, (_T("Notification Processed ,Payload = %s"), payload));

            cJSON_Delete(notifyPayload);
            delete param;
        } else {
            TRACE(Trace::Error, (_T("ParamNotify is nullptr.. !!")));
        }

        break;
    }
    }
    TRACE(Trace::Information, (_T("%s:End"), __FUNCTION__));
    return payload;
}

std::string Notifier::Source()
{
    TRACE(Trace::Information, (_T("%s:Start"), __FUNCTION__));

    _adminLock.Lock();
    std::string notificationSource = _notificationSource;
    _adminLock.Unlock();

    if (notificationSource.empty() == true) {

        std::vector<std::string> paramaterName = { DeviceStbMACParam };
        std::map<std::vector<Data>, WebPAStatus> paramaters;
        notificationSource = UnknownParamValue;

        _parameter->Values(paramaterName, paramaters);
        if (paramaters.size() > 0) {
            auto it = paramaters.begin();
            if (it->first.size() > 0) {
                std::string deviceMac = it->first[0].Value().String();
                TRACE(Trace::Information, (_T("[%s] Calling MacToLower for MAC:  %s"), __FUNCTION__, deviceMac.c_str()));

                StringToLower(deviceMac);
                TRACE(Trace::Information, (_T("[%s] Converted MAC:  %s "), __FUNCTION__, deviceMac.c_str()));

                notificationSource = "mac:" + deviceMac;
                TRACE(Trace::Information, (_T("[%s] Notify Source :-  %s "), __FUNCTION__, notificationSource));
            }
        } else {
            TRACE(Trace::Error, (_T("[%s] Unable to get Notification Source .!! \n"), __FUNCTION__));
        }

        _adminLock.Lock();
        _notificationSource = notificationSource;
        _adminLock.Unlock();
    }
    TRACE(Trace::Information, (_T("%s:End"), __FUNCTION__));
    return notificationSource;
}

string Notifier::Destination()
{
    TRACE(Trace::Information, (_T("%s:Start"), __FUNCTION__));
    std::string notifyDest = "event:VALUE_CHANGE_NOTIFICATION";
    TRACE(Trace::Information, (_T("Dest = %s... %s \n"), notifyDest, __FUNCTION__));
    TRACE(Trace::Information, (_T("%s:End"), __FUNCTION__));
    return notifyDest;
}

uint32_t Notifier::Parameters(std::vector<std::string>& notifyParameters)
{
    TRACE(Trace::Information, (_T("%s:Start"), __FUNCTION__));
    uint32_t result = Core::ERROR_GENERAL;

    // Read file notification Configuration file
    _adminLock.Lock();
    if (_configFile.empty() == true) {
        _adminLock.Unlock();
        TRACE(Trace::Error, (_T("WebPA notification file path not set")));
    } else {
        TRACE(Trace::Information, (_T("Inside getnotifyparamList trying to open %s\n"), _configFile.c_str()));
        FILE* fp = fopen(_configFile.c_str(), "r");
        if (fp == nullptr) {
            TRACE(Trace::Error, (_T("Failed to open cfg file %s\n"), _configFile.c_str()));
        } else {
            fseek(fp, 0, SEEK_END);
            int32_t chCount = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            char* notifycfgFileContent = static_cast<char*> (malloc(sizeof(uint8_t) * (chCount + 1)));
            if (nullptr != notifycfgFileContent) {
                fread(notifycfgFileContent, 1, chCount,fp);
                notifycfgFileContent[chCount] ='\0';

                if (chCount < 1) {
                    TRACE(Trace::Error, (_T("WebPA notification file is Empty %s\n"), _configFile.c_str()));
                } else {
                    cJSON* notifyCfg = cJSON_Parse(notifycfgFileContent);
                    cJSON* notifyArray = cJSON_GetObjectItem(notifyCfg, "Notify");
                    if (nullptr != notifyArray) {
                        for (int32_t i = 0 ; i < cJSON_GetArraySize(notifyArray) ; i++) {
                            char* tempPtr = cJSON_GetArrayItem(notifyArray, i)->valuestring;
                            if (tempPtr) {
                                notifyParameters.push_back(tempPtr);
                                TRACE(Trace::Information, (_T("Notify Param  = %s\n"), tempPtr));
                            }
                        }
                        result = Core::ERROR_NONE;
                    } else {
                        TRACE(Trace::Error, (_T("Unable to parse Configuration file")));
                    }
                }
                free(notifycfgFileContent);
            }
            fclose(fp);
        }
        _adminLock.Unlock();
    }

    TRACE(Trace::Information, (_T("%s:End"), __FUNCTION__));
    return result;
}

char Notifier::CharToLower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + 32;

    return c;
}

void Notifier::StringToLower(string& str)
{
    for (char &c: str) {
        c = CharToLower(c);
    }
}

} // WebPA
} // WPEFramework
