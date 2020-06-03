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

namespace WPEFramework {

namespace Implementation {
    struct ICallback {
        virtual ~ICallback() {}
        virtual void NotifyEvent(const std::string& payload, const std::string& source, const std::string& destination) = 0;
    };
} // Implementation

class IProfileControl;

namespace WebPA {

    struct ICallback {
        virtual ~ICallback() {}
        virtual void NotifyEvent() = 0;
    };

class Administrator {
public:
    Administrator(const Administrator&) = delete;
    Administrator& operator= (const Administrator&) = delete;

    ~Administrator() {
    }

    template <typename PROFILE>
    class ProfileImplementationType: public PROFILE {
    public:
        ProfileImplementationType(const ProfileImplementationType<PROFILE>&) = delete;
        ProfileImplementationType<PROFILE> operator= (const ProfileImplementationType<PROFILE>&) = delete;

        ProfileImplementationType() : PROFILE()
        {
            Administrator::Instance().Announce(Core::ClassNameOnly(typeid(PROFILE).name()).Text(), this);
        }
        virtual ~ProfileImplementationType()
        {
            Administrator::Instance().Revoke(Core::ClassNameOnly(typeid(PROFILE).name()).Text());
        }
    };

public:
    static Administrator& Instance()
    {
        static Administrator _singleton;
        return (_singleton);
    }

    IProfileControl* Find(const string& name)
    {
        IProfileControl* result = nullptr;
        std::map<const string, IProfileControl*>::iterator index (_profiles.find(name));
        if (index != _profiles.end()) {
            result = index->second;
        }
        return (result);
    }

private:
    Administrator() : _profiles() {}

    void Announce(const string& name, IProfileControl* profile)
    {
        _profiles.emplace(name, profile);
    }
    void Revoke(const string& name)
    {
        std::map<const string, IProfileControl*>::iterator index (_profiles.find(name));
        if (index != _profiles.end()) {
            _profiles.erase(index);
        }
    }

private:
    std::map<const string, IProfileControl*> _profiles;
};
} // WebPA

#ifdef __cplusplus
extern "C" {
#endif

IProfileControl*  WebPAProfileInstance(const char* name);

#ifdef __cplusplus
}
#endif

struct IProfileControl {
public:
    struct ICallback {
        virtual ~ICallback() {}
        virtual void NotifyEvent(const EventId& eventId, const EventData& eventData) = 0;
    };

    static IProfileControl* Instance(const char* profileName) {
        return (WebPAProfileInstance(profileName));
    }

    virtual ~IProfileControl() {}
    virtual bool Initialize() = 0;
    virtual bool Deinitialize() = 0;

    // Getter...
    virtual FaultCode Attribute(Data& parameter) const = 0;
    // Setter...
    virtual FaultCode Attribute(const Data& parameter) = 0;

    // Getter...
    virtual FaultCode Parameter(Data& parameter) const = 0;
    // Setter...
    virtual FaultCode Parameter(const Data& parameter) = 0;

    virtual void SetCallback(ICallback* cb) = 0;
    virtual void CheckForUpdates() = 0;
};

} // WPEFramework
