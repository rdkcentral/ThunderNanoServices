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
 
#include "Module.h"

#include "DeviceInfo.h"
#include "DeviceControl.h"

namespace WPEFramework {

bool DeviceControl::Initialize()
{
   TRACE(Trace::Information, (string(__FUNCTION__)));
   _prefixList.push_back("Device.DeviceInfo.ProcessStatus.");
   _prefixList.push_back("Device.DeviceInfo.MemeoryStatus.");
   _prefixList.push_back("Device.DeviceInfo.");
   return true;
}

bool DeviceControl::Deinitialize()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    _notifier.clear();
    _prefixList.clear();
    return true;
}

DeviceControl::DeviceControl()
    : _notifier()
    , _prefixList()
    , _adminLock()
    , _callback(nullptr)
{
}

DeviceControl::~DeviceControl()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
}

FaultCode DeviceControl::Parameter(Data& parameter) const {
    TRACE(Trace::Information, (string(__FUNCTION__)));

    FaultCode ret = FaultCode::Error;
    uint32_t instance = 0;
    for (auto& prefix : _prefixList) {
        if (parameter.Name().compare(0, prefix.length(), prefix) == 0) {
            std::string name;
            if (Utils::MatchComponent(parameter.Name(), prefix, name, instance)) {
                _adminLock.Lock();
                DeviceInfo* deviceInfo = DeviceInfo::Instance();
                if (deviceInfo) {
                    bool changed;
                    ret = deviceInfo->Parameter(name, parameter, changed);
                }
                _adminLock.Unlock();
                break;
            } else {
                ret = FaultCode::InvalidParameterName;
           }
        }
    }

    return ret;
}

FaultCode DeviceControl::Parameter(const Data& parameter) {
    TRACE(Trace::Information, (string(__FUNCTION__)));

    FaultCode ret = FaultCode::Error;
    uint32_t instance = 0;

    for (auto& prefix : _prefixList) {
        if (parameter.Name().compare(0, prefix.length(), prefix) == 0) {
            std::string name;
            if (Utils::MatchComponent(parameter.Name(), prefix, name, instance)) {
                _adminLock.Lock();
                DeviceInfo* deviceInfo = DeviceInfo::Instance();
                if (deviceInfo) {
                     ret = deviceInfo->Parameter(name, parameter);
                }
                _adminLock.Unlock();
                break;
            } else {
                ret = FaultCode::InvalidParameterName;
            }
        }
    }

    return ret;
}

FaultCode DeviceControl::Attribute(Data& parameter) const {
    TRACE(Trace::Information, (string(__FUNCTION__)));

    FaultCode ret = FaultCode::Error;

    _adminLock.Lock();
    NotifierMap::const_iterator notifier = _notifier.find(parameter.Name());
    if (notifier != _notifier.end()) {
         parameter.Value(notifier->second);
         ret = FaultCode::NoFault;
    } else {
        ret = FaultCode::InvalidParameterName;
    }
    _adminLock.Unlock();

    return ret;
}

FaultCode DeviceControl::Attribute(const Data& parameter) {
    TRACE(Trace::Information, (string(__FUNCTION__)));

    FaultCode ret = FaultCode::Error;

    _adminLock.Lock();
    _notifier.insert(std::make_pair(parameter.Name(), parameter.Value().Boolean()));
    _adminLock.Unlock();

    return ret;
}

void DeviceControl::SetCallback(IProfileControl::ICallback* cb)
{
    _adminLock.Lock();
    _callback = cb;
    _adminLock.Unlock();
}

void DeviceControl::CheckForUpdates()
{
    TRACE_GLOBAL(Trace::Information, (string(__FUNCTION__)));

    for (auto& index : _notifier) {
        if (index.second == true) {
            bool changed = false;
            Data parameter(index.first);
            for (auto& prefix : _prefixList) {
                uint32_t instance = 0;
                std::string name;
                if (Utils::MatchComponent(parameter.Name(), prefix, name, instance)) {
                    _adminLock.Lock();
                    DeviceInfo* deviceInfo = DeviceInfo::Instance();
                    if (deviceInfo) {
                        deviceInfo->Parameter(name, parameter, changed);
                        if (changed == true) {
                            if (_callback) {
                                _callback->NotifyEvent(EVENT_VALUECHANGED, parameter);
                            }
                        }
                    }
                    _adminLock.Unlock();
                }
            }
        }
    }
}
}

#ifdef __cplusplus
extern "C" {
#endif

void LoadProfile() {
    static WPEFramework::WebPA::Administrator::ProfileImplementationType<WPEFramework::DeviceControl> Register;
}

#ifdef __cplusplus
}
#endif
