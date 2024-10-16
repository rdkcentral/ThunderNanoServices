 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// WebConsole.cpp : Defines the entry point for the console application.
//

#include "Protocols.h"

using namespace Thunder;

class ConsoleOptions : public Thunder::Core::Options {
public:
    ConsoleOptions(int argumentCount, TCHAR* arguments[])
        : Thunder::Core::Options(argumentCount, arguments, _T("v:hsp:d"))
        , LogLevel()
        , SSL(false)
        , Port(80)
        , Version(1)
    {
        Parse();
    }
    ~ConsoleOptions()
    {
    }

public:
    Thunder::Core::OptionalType<uint8_t> LogLevel;
    bool SSL;
    uint16_t Port;
    uint16_t Version;

private:
    virtual void Option(const TCHAR option, const TCHAR* argument)
    {
        switch (option) {
        case 'd':
            LogLevel = atoi(argument);
            break;
        case 's':
            SSL = true;
            break;
        case 'p':
            Port = atoi(argument);
            break;
        case 'v':
            Version = atoi(argument);
            break;
        case 'h':
        default:
            RequestUsage(true);
            break;
        }
    }
};


    template <typename STREAM>
    class WebSocketClient : public ::Thunder::Web::WebSocketClientType<STREAM> {
    public :

        template <typename... Args>
        WebSocketClient(
              const string& path
            , const string& protocol
            , const string& query
            , const string& origin
            , const bool binary
            , const bool masking
            , Args&&... args
        )
            : ::Thunder::Web::WebSocketClientType<STREAM>(path, protocol, query, origin, binary, masking, /* <Arguments SocketStream> */ std::forward<Args>(args)... /*</Arguments SocketStream>*/)
        {
//            std::cout << __LINE__ << " : " << __PRETTY_FUNCTION__ << " : Remote NodeId port = " << remoteNode.PortNumber() << std::endl;
//            std::cout << __LINE__ << " : " << __PRETTY_FUNCTION__ << " : Remote NodeId hostaddress = " << remoteNode.HostAddress() << std::endl;
        }

        ~WebSocketClient() override = default;

        bool IsIdle() const override { return true; }
        void StateChange() override
        {
            std::cout << __LINE__ << " : " << __PRETTY_FUNCTION__ << " :  IsOpen() = " << this->IsOpen() << std::endl;
            std::cout << __LINE__ << " : " << __PRETTY_FUNCTION__ << " :  IsSuspended() = " << this->IsSuspended() << std::endl;
            std::cout << __LINE__ << " : " << __PRETTY_FUNCTION__ << " :  IsClosed() = " << this->IsClosed() << std::endl;
            std::cout << __LINE__ << " : " << __PRETTY_FUNCTION__ << " :  IsWebServer() = " << this->IsWebServer() << std::endl;
            std::cout << __LINE__ << " : " << __PRETTY_FUNCTION__ << " :  IsUpgrading() = " << this->IsUpgrading() << std::endl;
            std::cout << __LINE__ << " : " << __PRETTY_FUNCTION__ << " :  IsWebSocket() = " << this->IsWebSocket() << std::endl;
            std::cout << __LINE__ << " : " << __PRETTY_FUNCTION__ << " :  IsCompleted() = " << this->IsCompleted() << std::endl;
 
        }

        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
            std::cout << __LINE__ << " : " << __FUNCTION__ << " : " << __PRETTY_FUNCTION__ << std::endl;
            return 0;
        }

        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
        {
            std::cout << __LINE__ << " : " << __FUNCTION__ << " : " << __PRETTY_FUNCTION__ << std::endl;
            return 0;
        }
    };


int main(int argc, char** argv)
{
    ConsoleOptions options(argc, argv);

    fprintf(stderr, "TestConsole\n"
                    "By Pierre Wielders <pierre@wielders.net>\n");

    if ((argc < 2) || (options.RequestUsage())) {
        fprintf(stderr, "Usage: TestConsole "
                        "<server address> [-p=<p>]\n");
        return (1);
    }

    // Give the server process some time to come up... Only needed for debugging both processes..
    SleepS(5);

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)

    // Scope to make sure that the sockets are destructed before we close the singletons !!!
    {
        // ------------------------------------------------------------------------------------
        // Test stuff
        // ------------------------------------------------------------------------------------
        // Create a parser to see what we will send..
        string message;

        // ------------------------------------------------------------------------------------
        // [5] WEB SOCKET TEXT LOCAL client
        // ------------------------------------------------------------------------------------
//        Thunder::TestSystem::EchoWebSocketClient webSocketLocalConnection(Thunder::Core::NodeId(options.Command(), 12345));

        const std::string webSocketURIPath;     // HTTP URI part, empty path allowed 
        const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
        const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
        const std::string webSocketOrigin;      // Optional, set by browser clients
        constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)  
        constexpr bool masking {true};          // Flag set by client to enable masking

        const TCHAR localHostName[] {"127.0.0.1"};
        const TCHAR remoteHostName[] {"127.0.0.1"};

//can we randomly select ?
        constexpr uint16_t tcpClientPort {9000};
        constexpr uint16_t tcpServerPort {12345}; // TCP, default 80 or 443 via SecureSocketPort (SSL)
        constexpr uint32_t tcpProtocol {0};      // HTTP or HTTPS but can only be set on raw sockets
    //
        // Arguments for SocketStream
        constexpr bool rawSocket {false};

// What is a suitable size?
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        const ::Thunder::Core::NodeId localNode {localHostName, tcpClientPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};
        const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

        WebSocketClient<::Thunder::Core::SocketStream> webSocketLocalConnection(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

        /* create a client websocket using dumb increment protocol */
        char keyPress;
        int testCount = 0;
        bool openState = false;

        printf("Ready to start processing events, start with 0 to connect.\n");
        do {
            keyPress = toupper(getchar());

            switch (keyPress) {
            case 'B': {
                if (openState == true) {
                    webSocketLocalConnection.Ping();
                }
                break;
            }
            case '0': {
                if (openState == false) {
                    printf("Opening up sockets...\n");
                    openState = true;
                    webSocketLocalConnection.Open(0);
                    printf("Opening up an external connection. Subject to change !!!!.\n");
                    SleepMs(1000);
                }
                else {
                    printf("Cosing up sockets...\n");
                    openState = false;
                    webSocketLocalConnection.Close(0);
                }
                break;
            }
            case '5': {
//                webSocketLocalConnection.Submit(_T("TestCase5"));
                break;
            }
            }

        } while (keyPress != 'Q');
    }

    Thunder::Core::Singleton::Dispose();

    printf("\nLeaving the main App !!!\n");

    return 0;
}
