#ifndef RTSPSOCKET_H
#define RTSPSOCKET_H

#include <string>

namespace WPEFramework {
namespace Plugin {

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
        int Receive(std::string &buffer, unsigned long timeoutMS = 100);
        bool isValid() { return (sockfd > 0); }

    private:
        int sockfd;
        struct sockaddr_in serv_addr;
};

}} // WPEFramework::Plugin

#endif
