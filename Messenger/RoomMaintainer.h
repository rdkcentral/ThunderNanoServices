#pragma once

#include "Module.h"
#include <interfaces/IMessenger.h>

namespace WPEFramework {

namespace Plugin {

    class RoomImpl;

    class RoomMaintainer : public Exchange::IRoomAdministrator {
    public:
        RoomMaintainer(const RoomMaintainer&) = delete;
        RoomMaintainer& operator=(const RoomMaintainer&) = delete;

        RoomMaintainer()
            : _observers()
            , _roomMap()
            , _adminLock()
        { /* empty */}

        // IRoomAdministrator methods
        virtual IRoom* Join(const string& roomId, const string& userId, IRoom::IMsgNotification* messageSink) override;
        virtual void Register(INotification* sink) override;
        virtual void Unregister(const INotification* sink) override;

        // RoomMaintainer methods
        void Exit(const RoomImpl* roomUser);
        void Send(const string& message, RoomImpl* roomUser);
        void Notify(RoomImpl* roomUser);

        // QueryInterface implementation
        BEGIN_INTERFACE_MAP(RoomMaintainer)
            INTERFACE_ENTRY(Exchange::IRoomAdministrator)
        END_INTERFACE_MAP

    private:
        std::list<INotification*> _observers;
        std::map<string, std::list<RoomImpl*>> _roomMap;
        mutable Core::CriticalSection _adminLock;
    };

} // namespace Plugin

} // namespace WPEFramework
