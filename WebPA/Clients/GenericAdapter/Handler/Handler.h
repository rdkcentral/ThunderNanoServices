#pragma once

#include "Module.h"
#include "IAdapter.h"

#include <glib.h>
#include <interfaces/IWebPA.h>


namespace WPEFramework {

class NotificationHandler {
public:
    NotificationHandler();
    ~NotificationHandler();

    static NotificationHandler* GetInstance();
    NotifyData* NotificationData();
    void AddNotificationToQueue(const EventId& eventId, const EventData& eventData);
    void SetNotifyCallback(WebPA::ICallback* cb);

private:
    bool IsValidParameter(string paramName);

private:
    static NotificationHandler* _instance;
    WebPA::ICallback* _notificationCb;
    GAsyncQueue* _notificationQueue;

    Core::CriticalSection _adminLock;
};

class Handler : public Core::Thread {
private:
    static constexpr uint32_t MaxWaitTime = 60000;

public:
    class Config : public Core::JSON::Container {
    public:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        class Link : public Core::JSON::Container {
        private:
            Link& operator= (const Link&);

        public:
            Link ()
                : ProfileName()
                , ProfileControl() {
                Add("profilename", &ProfileName);
                Add("profilecontrol", &ProfileControl);
            }
            Link (const Link& copy)
                : ProfileName(copy.ProfileName)
                , ProfileControl(copy.ProfileControl) {
                Add("profilename", &ProfileName);
                Add("profilecontrol", &ProfileControl);
            }
            virtual ~Link() {
            }

        public:
            Core::JSON::String ProfileName;
            Core::JSON::String ProfileControl;
        };

    public:
        Config()
            : Core::JSON::Container()
            , Location()
            , Profiles()
        {
            Add(_T("location"), &Location);
            Add(_T("profiles"), &Profiles);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::String Location;
        Core::JSON::ArrayType<Link> Profiles;
    };

    class NotificationCallback : public IProfileControl::ICallback {
    public:
        NotificationCallback() = delete;
        NotificationCallback(const NotificationCallback&) = delete;
        NotificationCallback& operator= (const NotificationCallback&) = delete;

    public:
        NotificationCallback(Handler* parent)
            : _parent(parent)
        {
        }
        virtual ~NotificationCallback()
        {
        }

        void NotifyEvent(const EventId& eventId, const EventData& eventData);

    private:
        Handler* _parent;
    };

public:
    struct SystemProfileController {
        std::string name;
        IProfileControl* control;
    };

public:
    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;

public:
    Handler();
    ~Handler();

    const FaultCode Parameter(Data& value) const;
    FaultCode Parameter(const Data& value);

    const FaultCode Attribute(Data& value) const;
    FaultCode Attribute(const Data& value);

    void FreeData(Data* value);
    void ConfigureProfileControllers();
    uint32_t Configure(PluginHost::IShell* service);

private:
    virtual uint32_t Worker();
    IProfileControl* GetProfileController(const std::string& value);
    const IProfileControl* GetProfileController(const std::string& value) const;
    std::vector<std::string> SplitParam(std::string parameter, char delimeter) const;

private:
    std::string _configFile;
    std::list<Core::Library> _systemLibraries;

    std::map<const std::string, SystemProfileController> _systemProfileControllers;

    NotificationCallback* _notificationCallback;

    Core::Event _signaled;
    Core::CriticalSection _adminLock;
};

}
