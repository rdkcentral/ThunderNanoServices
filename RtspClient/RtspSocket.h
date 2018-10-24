#ifndef RTSPSOCKET_H
#define RTSPSOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string>

class RtspSocket
{
    public:
    RtspSocket();
    ~RtspSocket();

    int Create();
    int Connect(const std::string &server, const int port);
    int Disconnect();
    int Reconnect();
    int Send(const std::string &buffer);
    int Recv(std::string &buffer, unsigned long timeoutMS = 100);

    private:
        int sockfd;
        struct sockaddr_in serv_addr;
        bool isValid() { return (sockfd > 0); }
};

#endif
