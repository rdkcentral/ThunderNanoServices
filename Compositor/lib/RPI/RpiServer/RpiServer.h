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

    class IpcServer {
    public:
        IpcServer(void (*connectFunc)(void *), void (*disconnectFunc)(void *));
        virtual ~IpcServer();
        void RunServer();

    private:
        static void *ThreadFunc(void *con);

        enum IPC_THREAD_STATE {
            IPC_SERVER_STARTED = 1,
            IPC_SERVER_STOPPED,
            IPC_SERVER_ERROR
        };
        IPC_THREAD_STATE _threadState;
        pthread_t _threadHandle;

#define IPC_MAX_CLIENTS 8
#define IPC_DATABUF_SIZE 256

        char _sendBuf[IPC_DATABUF_SIZE];
        char _recvBuf[IPC_DATABUF_SIZE];
        void (*_connectFunc)(void *);
        void (*_disconnectFunc)(void *);
    };

public:
    Platform(const string& callSign, IStateChange* stateChanges,
            IClient* clientChanges, const string& configuration);
    virtual ~Platform();

private:
    static void ClientConnect(void *con);
    static void ClientDisconnect(void *con);

    IpcServer* _instance;
    static Platform* _implementation;
};
}
}
#endif //PLATFORMIMPLEMENTATION_H
