#ifndef PLATFORMIMPLEMENTATION_H
#define PLATFORMIMPLEMENTATION_H

#include <core/core.h>
#include <interfaces/IComposition.h>

namespace WPEFramework {
namespace Rpi {

class Platform {
private:
    Platform() = delete;
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

public:
    class Client : public Exchange::IComposition::IClient {
    private:
        Client() = delete;
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;

        BEGIN_INTERFACE_MAP(Entry)
        INTERFACE_ENTRY(Exchange::IComposition::IClient)
        END_INTERFACE_MAP
    };

    enum server_state {
        FAILURE = 0x00,
        UNITIALIZED = 0x01,
        INITIALIZING = 0x02,
        OPERATIONAL = 0x03,
        DEINITIALIZING = 0x04,
    };

    struct IClient {

        virtual ~IClient() {}
        virtual void Attached(Exchange::IComposition::IClient*) = 0;
        virtual void Detached(const char name[]) = 0;
    };

    struct IStateChange {

        virtual ~IStateChange() {}
        virtual void StateChange(server_state state) = 0;
    };

public:
    Platform(const string& callSign, IStateChange* stateChanges,
            IClient* clientChanges, const string& configuration);
    virtual ~Platform();
};
}
}
#endif //PLATFORMIMPLEMENTATION_H
