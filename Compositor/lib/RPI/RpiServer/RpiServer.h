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

    public:
        typedef struct {
            char displayName[32];
            int x;
            int y;
            int width;
            int height;
        } ClientContext;

        Client(void *clientCon) {
            memcpy((void *)&_clientCon, clientCon, sizeof(_clientCon));
        }
        static Client* Create(void *clientCon) {
            Client* result =
                    Core::Service<Client>::Create<Client>(clientCon);
            return (result);
        }
        virtual ~Client() {
        }

        virtual string Name() const override;
        virtual void Kill() override;
        virtual void Opacity(const uint32_t value) override;
        virtual void Geometry(const uint32_t X, const uint32_t Y, const uint32_t width, const uint32_t height) override;
        virtual void Visible(const bool visible) override;
        virtual void SetTop() override;
        virtual void SetInput() override;

        BEGIN_INTERFACE_MAP(Entry)
        INTERFACE_ENTRY(Exchange::IComposition::IClient)
        END_INTERFACE_MAP

    private:
        ClientContext _clientCon;
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
        virtual void Detached(const char clientName[]) = 0;
    };

    struct IStateChange {

        virtual ~IStateChange() {}
        virtual void StateChange(server_state state) = 0;
    };

    class IpcServer {
    public:
        IpcServer(
                Platform &platform,
                void (*connectFunc)(void *, void *),
                void (*disconnectFunc)(void *, void *));
        virtual ~IpcServer();
        void RunServer();

    private:
        Platform& _parent;
        static void *ThreadFunc(void *con);

        enum IPC_THREAD_STATE {
            IPC_SERVER_STARTED = 1,
            IPC_SERVER_STOPPED,
            IPC_SERVER_ERROR
        };
        IPC_THREAD_STATE _threadState;
        pthread_t _threadHandle;

#define IPC_MAX_CLIENTS 8
#define IPC_DATABUF_SIZE 48
        int clientSocket[IPC_MAX_CLIENTS];
        char clientCon[IPC_MAX_CLIENTS][IPC_DATABUF_SIZE];
        char _sendBuf[IPC_DATABUF_SIZE];
        char _recvBuf[IPC_DATABUF_SIZE];
        void (*_connectFunc)(void *, void *);
        void (*_disconnectFunc)(void *, void *);
    };

public:
    Platform(const string& callSign, IStateChange* stateChanges,
            IClient* clientChanges, const string& configuration);
    virtual ~Platform();

private:
    static void ClientConnect(void *instance, void *clientCon);
    static void ClientDisconnect(void *instance, void *clientCon);
    void Add(void *clientCon);
    void Remove(void *clientCon);
    void StateChange(server_state state);

    IpcServer* _instance;
    server_state _state;
    IClient* _clientHandler;
    IStateChange* _stateHandler;
};
}
}
#endif //PLATFORMIMPLEMENTATION_H
