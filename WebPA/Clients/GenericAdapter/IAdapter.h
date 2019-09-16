#pragma once
#include "WebPADataTypes.h"

namespace WPEFramework {

namespace Implementation {
    struct ICallback {
        virtual ~ICallback() {}
        virtual void NotifyEvent(const std::string& payload, const std::string& source, const std::string& destination) = 0;
    };
} // Implementation

namespace WebPA {

    struct ICallback {
        virtual ~ICallback() {}
        virtual void NotifyEvent() = 0;
    };
} // WebPA

class IProfileControl;

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
