/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "IAdapter.h"

namespace Thunder {

class DeviceInfo;

class DeviceControl : public IProfileControl {
private:
    typedef std::map<std::string, bool> NotifierMap;
    typedef std::list<std::string> ParameterPrefixList;

public:
    DeviceControl(const DeviceControl&) = delete;
    DeviceControl& operator=(const DeviceControl&) = delete;

public:
    DeviceControl();
    ~DeviceControl() override;

    bool Initialize() override;
    bool Deinitialize() override;

    FaultCode Parameter(Data& parameter) const override;
    FaultCode Parameter(const Data& parameter) override;

    FaultCode Attribute(Data& parameter) const override;
    FaultCode Attribute(const Data& parameter) override;

    void SetCallback(IProfileControl::ICallback* cb) override;
    void CheckForUpdates() override;

private:
    NotifierMap _notifier;
    ParameterPrefixList _prefixList;

    mutable Core::CriticalSection _adminLock;
    IProfileControl::ICallback* _callback;
};

}
