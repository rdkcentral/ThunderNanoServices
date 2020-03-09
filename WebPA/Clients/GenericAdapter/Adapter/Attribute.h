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

#include "Handler.h"
#include "Notifier.h"

namespace WPEFramework {
namespace WebPA {

class Attribute {
private:
    static constexpr const uint32_t MaxParameterLen = 512;
    static constexpr const uint8_t MaxParameterValueLen = 128;

public:
    Attribute() = delete;
    Attribute(const Attribute&) = delete;
    Attribute& operator= (const Attribute&) = delete;

public:
    Attribute(Notifier* notifier, Handler* handler);
    virtual ~Attribute();

    void Values(const std::vector<std::string>& parameterNames, std::map<Data, WebPAStatus>& attributesList) const;
    WebPAStatus Values(const std::map<std::string, Data>& attributes, std::vector<WebPAStatus>& status);

private:
    const WebPAStatus Values(const std::string& parameterName, Data& attributes) const;
    WebPAStatus Values(const std::string& parameterName, const Data& attribute);

private:
    Notifier* _notifier;
    Handler* _handler;

    mutable Core::CriticalSection _adminLock;
};

} // WebPA
} // WPEFramework
