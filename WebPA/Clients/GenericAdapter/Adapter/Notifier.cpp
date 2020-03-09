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
 
#include "Notifier.h"

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
            TRACE(Trace::Information, (_T("Notification Processed")));
            TRACE(Trace::Information, (_T("DeviceID: %s"), Source().c_str()));
            NotifierPayload notfierPayload;
            notfierPayload.DeviceID = Source();
            TRACE(Trace::Information, (_T("ParameterName: %s"), param->Name().c_str()));
            notfierPayload.Name = param->Name();
            TRACE(Trace::Information, (_T("ParameterType: %d"), param->Value().Type()));
            notfierPayload.Type = param->Value().Type();
            TRACE(Trace::Information, (_T("NotificationType: %s"), NotifyTypeStr));

            switch(param->Value().Type())
            {
            case Variant::ParamType::TypeString:
            {
                TRACE(Trace::Information, (_T("paramValue: %s"), param->Value().String().c_str()));
                Core::JSON::Variant value(static_cast<string>(param->Value().String()));
                notfierPayload.Value = value;
                break;
            }
            case Variant::ParamType::TypeInteger:
            {
                TRACE(Trace::Information, (_T("paramValue: %d"), param->Value().Integer()));
                Core::JSON::Variant value(static_cast<int32_t>(param->Value().Integer()));
                notfierPayload.Value = value;
                break;
            }
            case Variant::ParamType::TypeUnsignedInteger:
            {
                TRACE(Trace::Information, (_T("paramValue: %d"), param->Value().UnsignedInteger()));
                Core::JSON::Variant value(static_cast<uint32_t>(param->Value().Integer()));
                notfierPayload.Value = value;
                break;
            }
            case Variant::ParamType::TypeBoolean:
            {
                TRACE(Trace::Information, (_T("paramValue: %d"), param->Value().Boolean()));
                Core::JSON::Variant value(static_cast<bool>(param->Value().Integer()));
                notfierPayload.Value = value;
            }
            case Variant::ParamType::TypeUnsignedLong:
            {
                TRACE(Trace::Information, (_T("paramValue: %d"), param->Value().UnsignedLong()));
                Core::JSON::Variant value(static_cast<uint64_t>(param->Value().UnsignedLong()));
                notfierPayload.Value = value;
                break;
            }
            default:
            {
                break;
            }
            }
            notfierPayload.ToString(payload);
            TRACE(Trace::Information, (_T("Notification Processed ,Payload = %s"), payload));

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

    if ((notificationSource.empty() == true) || (notificationSource == UnknownParamValue)) {

        std::vector<std::string> parameterName = { DeviceMACParam };
        std::map<std::vector<Data>, WebPAStatus> paramaters;
        notificationSource = UnknownParamValue;

        _parameter->Values(parameterName, paramaters);
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
        Core::File configFile(_configFile);

        if (configFile.Open(true) == true) {

            NotifierList notifierList;
            Core::OptionalType<Core::JSON::Error> error;
            notifierList.IElement::FromFile(configFile, error);
            if (error.IsSet() == true) {
                SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
            }

            auto index(notifierList.Notifiers.Elements());
            while (index.Next()) {
                notifyParameters.push_back(index.Current().Value());
            }
            result = Core::ERROR_NONE;
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
