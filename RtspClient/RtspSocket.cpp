#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/unistd.h>

#include "RtspSocket.h"

#define BUFFER_LEN 1024

RtspSocket::RtspSocket()
    : sockfd(-1)
{
    //Create();
}

int RtspSocket::Create()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf(  "ERROR opening socket");
    }
    return 0;
}

int RtspSocket::Connect ( const std::string &host, const int port )
{

    int rc = 0;
    int connectTimeout = 10;

    if ( ! isValid() ) return -1;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons ( port );

    int status = inet_pton ( AF_INET, host.c_str(), &serv_addr.sin_addr );

    if ( errno == EAFNOSUPPORT ) {
        rc = -1;
    } else {
        if (connectTimeout) {           // use non-blocking socket
            if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
                printf(  "%s: fcntl(O_NONBLOCK) FAILED. errno=%d", __FUNCTION__, errno);
            } else {
                printf(  "%s: Using NON-BLOCKING socket", __FUNCTION__);
            }
            rc = ::connect ( sockfd, ( sockaddr * ) &serv_addr, sizeof ( serv_addr ) );
            if (errno == EINPROGRESS) {
                fd_set fdset;
                struct timeval tv;

                FD_ZERO(&fdset);
                FD_SET(sockfd, &fdset);
                tv.tv_sec = connectTimeout;
                tv.tv_usec = 0;

                if (select(FD_SETSIZE, NULL, &fdset, NULL, &tv) == 1) {
                    int sockError;
                    socklen_t len = sizeof sockError;

                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sockError, &len);

                    if (sockError == 0) {
                        rc = 0;
                    } else {
                        rc = -1;
                        printf(  "%s: sockError=%d ret=%d", __FUNCTION__, sockError, rc);
                    }
                }
            } else {
                printf(  "%s: connect returned %d errno=%d", __FUNCTION__, rc, errno );
            }
        } else {
            rc = ::connect ( sockfd, ( sockaddr * ) &serv_addr, sizeof ( serv_addr ) );
        }

        if (rc == 0) {
            printf(  "%s: Connected. sockfd=%d", __FUNCTION__, sockfd);
        } else {
            printf(  "%s: connect failed rc=%d", __FUNCTION__, rc);
        }
    }

    printf(  "%s: server=%s port=%d rc=%d", __FUNCTION__, host.c_str(), port, rc);
    return rc;
}

int RtspSocket::Send(const std::string &buffer)
{
    int rc = 0;
    int n;

    printf(  "%s: buffer.length=%d", __FUNCTION__, buffer.length());
    n = write (sockfd, buffer.c_str(), buffer.length());
    //n = send(sockfd, buffer.c_str(), buffer.length(), 0);
    if (n < 0)
         printf(  "ERROR writing to socket");
    return  rc;
}

int RtspSocket::Recv(std::string &buffer, unsigned long timeoutMS)
{
    int rc = 0;
    char buf[BUFFER_LEN];
    unsigned long to;
    fd_set rfds;
    struct timeval  tv;

    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    to = timeoutMS;
    to *= 1000;
    tv.tv_sec = to / 1000000;
    tv.tv_usec = to % 1000000;

    int ret = select(sockfd + 1, &rfds, NULL, NULL, &tv);

    if (ret > 0) {
        if (FD_ISSET(sockfd, &rfds)) {
            ret = recv(sockfd, buf, BUFFER_LEN, 0);
            if (ret < 0) {
                //printf(  "%s: ERROR reading from socket ret=%d", __FUNCTION__, ret);
                rc = -1;
            } else {
                // printf("%s\n",buf);
                buffer = std::string(buf, ret);
                rc = 0;
            }
        } else {
            printf(  "%s: FD_ISSET FAILED ret=%d", __FUNCTION__, ret);
            rc = -1;
        }
    } else {
        printf(  "%s: select FAILED ret=%d\n", __FUNCTION__, ret);
        rc = -1;
    }

    return  rc;
}

int RtspSocket::Disconnect()
{
    int rc = 0;
    rc = close(sockfd);
    return rc;
}

RtspSocket::~RtspSocket()
{
    Disconnect();
}
