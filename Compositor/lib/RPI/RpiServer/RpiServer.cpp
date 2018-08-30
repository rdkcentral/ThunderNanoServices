#include "RpiServer.h"

namespace WPEFramework {
namespace Rpi {

Platform* Platform::_implementation = nullptr;
Platform::Platform(const string& callsign, IStateChange* stateChanges,
        IClient* clientChanges, const std::string& configuration) {
    ASSERT(_implementation == nullptr);
    _implementation = this;
    _instance = new IpcServer(
            Platform::ClientConnect, Platform::ClientDisconnect);
}

Platform::~Platform() {
    _implementation = nullptr;
    delete _instance;
}

void Platform::ClientConnect(void *con) {
    fprintf(stderr, "RpiServer::: Platform::ClientConnect\n");
}

void Platform::ClientDisconnect(void *con) {
    fprintf(stderr, "RpiServer::: Platform::ClientDisconnect\n");
}

Platform::IpcServer::IpcServer(
        void (*connectFunc)(void *), void (*disconnectFunc)(void *)) {
    _connectFunc = connectFunc;
    _disconnectFunc = disconnectFunc;
    _threadState = IPC_SERVER_STARTED;
    pthread_create(&_threadHandle, NULL, ThreadFunc, (void*)this);
}

Platform::IpcServer::~IpcServer() {
    _threadState = IPC_SERVER_STOPPED;
    pthread_join(_threadHandle, NULL);
}

void *Platform::IpcServer::ThreadFunc(void *con) {
    Platform::IpcServer *server = reinterpret_cast<Platform::IpcServer*>(con);
    server->RunServer();
    return NULL;
}

void Platform::IpcServer::RunServer() {

    fd_set readfds;
    struct timeval tv;
    struct sockaddr_un address;
    int addrlen, activity, i, sizeRead;
    int clientSocket[IPC_MAX_CLIENTS];
    int masterSocket = -1, newSocket, sd, maxSd;

    while (1) {

        if ((_threadState == IPC_SERVER_STOPPED) ||
                (_threadState == IPC_SERVER_ERROR)) {

            if (masterSocket > 0) {
                close(masterSocket);
                masterSocket = -1;
            }
            for (i = 0; i < IPC_MAX_CLIENTS; i++) {
                sd = clientSocket[i];
                if (sd > 0)
                    close(sd);
            }
            if (_threadState == IPC_SERVER_STOPPED) break;
            else usleep(2000000); // 2 sec
        }

        if (masterSocket < 0) {
            if ((masterSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
                fprintf(stderr, "Platform::IpcServer::RunServer: socket error\n");
                _threadState = IPC_SERVER_ERROR;
                continue;
            }

            int opt = 1;
            setsockopt(masterSocket,
                    SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

            memset(&address, 0, sizeof(address));
            address.sun_family = AF_UNIX;
            strncpy(address.sun_path, "\0hidden", sizeof(address.sun_path)-1);

            if (bind(masterSocket,
                    (struct sockaddr *)&address, sizeof(address)) < 0) {
                fprintf(stderr, "Platform::IpcServer::RunServer: bind failed\n");
                _threadState = IPC_SERVER_ERROR;
                continue;
            }
            listen(masterSocket, 8);

            for (i = 0; i < IPC_MAX_CLIENTS; i++) {
                clientSocket[i] = 0;
            }
        }

        addrlen = sizeof(address);
        FD_ZERO(&readfds);
        FD_SET(masterSocket, &readfds);
        maxSd = masterSocket;
        for (i = 0; i < IPC_MAX_CLIENTS; i++) {
            sd = clientSocket[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > maxSd)
                maxSd = sd;
        }

        tv.tv_sec = 2;
        tv.tv_usec = 0;
        activity = select(maxSd+1, &readfds, NULL, NULL, &tv);
        if (activity <= 0) {
            continue;
        }

        if (FD_ISSET(masterSocket, &readfds)) {
            if ((newSocket = accept(masterSocket,
                    (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                fprintf(stderr, "Platform::IpcServer::RunServer: accept failed\n");
                _threadState = IPC_SERVER_ERROR;
                continue;
            }

            for (i = 0; i < IPC_MAX_CLIENTS; i++) {
                if (clientSocket[i] == 0) {
                    clientSocket[i] = newSocket;
                    (_connectFunc)((void *)this);
                    break;
                }
            }
        }

        for (i = 0; i < IPC_MAX_CLIENTS; i++) {
            sd = clientSocket[i];
            if (FD_ISSET(sd, &readfds)) {
                if ((sizeRead = read(sd , _recvBuf, IPC_DATABUF_SIZE)) <= 0) {
                    close(sd);
                    clientSocket[i] = 0;
                    (_disconnectFunc)((void *)this);
                }
            }
        }
    }
}

}
} // WPEFramework
