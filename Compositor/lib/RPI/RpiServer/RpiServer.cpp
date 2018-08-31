#include "RpiServer.h"

namespace WPEFramework {
namespace Rpi {

Platform::Platform(const string& callsign, IStateChange* stateChanges,
        IClient* clientChanges, const std::string& configuration)
: _clientHandler(clientChanges)
, _stateHandler(stateChanges) {
    _instance = new IpcServer(
            *this, Platform::ClientConnect, Platform::ClientDisconnect);
    StateChange(OPERATIONAL);
}

Platform::~Platform() {
    delete _instance;
}

void Platform::ClientConnect(void *instance, void *clientCon) {
    fprintf(stderr, "RpiServer::: Platform::ClientConnect\n");
    Platform* platform = reinterpret_cast<Platform*>(instance);
    platform->Add(clientCon);
}

void Platform::ClientDisconnect(void *instance, void *clientCon) {
    fprintf(stderr, "RpiServer::: Platform::ClientDisconnect\n");
    Platform* platform = reinterpret_cast<Platform*>(instance);
    platform->Remove(clientCon);
}

void Platform::Add(void *clientCon) {
    if (_clientHandler != nullptr) {
        Exchange::IComposition::IClient* entry(
                Platform::Client::Create(clientCon));
        _clientHandler->Attached(entry);
        entry->Release();
    }
}

void Platform::Remove(void *clientCon) {
    if (_clientHandler != nullptr) {
        _clientHandler->Detached(
                ((Platform::Client::ClientContext*)clientCon)->displayName);
    }
}

void Platform::StateChange(server_state state) {
    _state = state;
    if (_stateHandler != nullptr) {
        _stateHandler->StateChange(_state);
    }
};

string Platform::Client::Name() const {
    return (::std::string(_clientCon.displayName));
}

void Platform::Client::Kill() {
    fprintf(stderr, "Platform::Client::Kill\n");
}

void Platform::Client::Opacity(const uint32_t value) {
    fprintf(stderr, "Platform::Client::Opacity %d\n", value);
}

void Platform::Client::Geometry(
        const uint32_t X, const uint32_t Y,
        const uint32_t width, const uint32_t height) {
    fprintf(stderr, "Platform::Client::Geometry %d %d %d %d\n", X, Y, width, height);
}

void Platform::Client::Visible(const bool visible) {
    fprintf(stderr, "Platform::Client::Visible %d\n", visible);
}

void Platform::Client::SetTop() {
    fprintf(stderr, "Platform::Client::SetTop\n");
}

void Platform::Client::SetInput() {
    fprintf(stderr, "Platform::Client::SetInput\n");
}

Platform::IpcServer::IpcServer(
        Platform &platform,
        void (*connectFunc)(void *, void *),
        void (*disconnectFunc)(void *, void *))
: _parent(platform) {
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
                    read(newSocket, _recvBuf, IPC_DATABUF_SIZE);
                    memcpy((void *)&clientCon[i][0],
                            (void *)_recvBuf, IPC_DATABUF_SIZE);
                    (_connectFunc)((void *)
                            &this->_parent, (void *)&clientCon[i][0]);
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
                    (_disconnectFunc)((void *)
                            &this->_parent, (void *)&clientCon[i][0]);
                }
            }
        }
    }
}
}
} // WPEFramework
