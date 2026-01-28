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
 
// COMRPCApp.cpp : This file contains the 'main' function. Program execution begins and ends there.

// Since lower levels 
// #define _TRACE_LEVEL 5

#define MODULE_NAME ChannelClosedClient

#include <core/core.h>
#include <com/com.h>
#include <definitions/definitions.h>
#include <plugins/Types.h>
#include <qa_interfaces/ITestChannelClosed.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;

class Callback : public QualityAssurance::ITestChannelClosed::ICallback {
public:
    Callback(const Callback&) = delete;
    Callback& operator=(const Callback&) = delete;
    Callback(Callback&&) = delete;
    Callback& operator=(Callback&&) = delete;

    Callback() = default;
    ~Callback() override = default;

    void Done() override
    {
        printf("\n\ndone received...\n\n");
    }

    BEGIN_INTERFACE_MAP(Callback)
    INTERFACE_ENTRY(QualityAssurance::ITestChannelClosed::ICallback)
    END_INTERFACE_MAP
};

class CommandLineParameters : public Core::Options
{
public :
    CommandLineParameters() = delete;

    CommandLineParameters(const CommandLineParameters&) = delete;
    CommandLineParameters(CommandLineParameters&&) = delete;

    CommandLineParameters& operator=(const CommandLineParameters&) = delete;
    CommandLineParameters& operator=(CommandLineParameters&&) = delete;

    CommandLineParameters(int count, TCHAR* arguments[])
        : Core::Options{ count, arguments, _T("n:?") }
        , _node{}
        , _processName{ Core::FileNameOnly(arguments[0]) }
        , _requestedUsage{ false }
    {
        Parse();
    }

    ~CommandLineParameters() override = default;

    void Option(const TCHAR option, const TCHAR* argument) override
    {
PUSH_WARNING(DISABLE_WARNING_IMPLICIT_FALLTHROUGH);
        switch(option) {
        case 'n' : _node = Core::NodeId{ argument };
                   break;
        case '?' :
        default  :
                   if (_requestedUsage != true) {
                      PrintUsage();
                   }
                   _requestedUsage = true;
        }
POP_WARNING();
    }

    const Core::NodeId& EndPoint() const
    {
        return _node;
    }

    bool RequestedUsage() const
    {
        return _requestedUsage;
    }

private :

    void PrintUsage() const
    {
        printf("Usage : %s -n <domain socket or ip address / port string> | -?\n", _processName.data());
        printf("  -n NodeId specification, e.g. '/tmp/domainsocket' or '127.0.0.0:8080'\n");
        printf("  -? Help.\n");
    }

    mutable Core::NodeId _node;
    const std::string _processName;
    mutable bool _requestedUsage;
};

int main(int argc, char* argv[])
{
    CommandLineParameters parameters(argc, argv);

    if (   argc > 1
        && parameters.HasErrors() != false
    ) {

        if (parameters.RequestedUsage() != true) {
            const Core::NodeId& endPoint{ parameters.EndPoint() };

            if (endPoint.IsValid() != false) {

                Core::ProxyType<RPC::CommunicatorClient> client = Core::ProxyType<RPC::CommunicatorClient>::Create(endPoint, Core::ProxyType<Core::IIPCServer>(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create()));

                QualityAssurance::ITestChannelClosed* testinterface = client->Open<QualityAssurance::ITestChannelClosed>(_T("TestChannelClosed"));

                if (testinterface == nullptr) {
                    printf("Could not acquire testinterface\n");
                } else {
                    printf("Testinterface acquired\n\n");
                    printf("Press 'T' (successively) to start / execute the next test and 'Q' to quit followed by ENTER/RETURN. : ");

                    Core::SinkType<Callback> callback;

                    char keyPress;

                    do {
                        keyPress = toupper(getchar());

                        switch (keyPress) {
                        case 'T': {
                            printf("Begin Testcase...\n");

                            std::thread t2;
                            std::thread t1([&]() {
                                t2 = std::move(std::thread([]() {
                                    SleepMs(4000); // give other thread time to send request and then kill this client so all channels get closed...
                                    printf("Kill client\n");
                                    abort();
                                }));
                                Core::ProxyType<RPC::CommunicatorClient> client = Core::ProxyType<RPC::CommunicatorClient>::Create(endPoint, Core::ProxyType<Core::IIPCServer>(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create()));
                                QualityAssurance::ITestChannelClosed* testinterface = client->Open<QualityAssurance::ITestChannelClosed>(_T("TestChannelClosed"));
                                if (testinterface != nullptr) {
                                    SleepMs(1000); // give main thread time to send block call and block workerpool
                                    printf("Send test request\n");
                                    testinterface->Test();
            //                        testinterface->Test(&callback);
                                    testinterface->Release();
                                    client->Close(RPC::CommunicationTimeOut);
                                    client = std::move(Core::ProxyType<RPC::CommunicatorClient>());
                                }
                            });
                            printf("Blocking on testinterface\n");
                            testinterface->Block(15); // block for 15 seconds, might return sooner when comrpc timeout smaller of course
                            printf("Block ended\n");

                            t1.join();
                            t2.join();

                            break;
                        }
                        case 'Q':
                            break;
                        default:
                            break;
                        };
                    } while (keyPress != 'Q');

                    testinterface->Release();

                    callback.WaitReleased(RPC::CommunicationTimeOut);
                }

                client->Close(RPC::CommunicationTimeOut);

                client = std::move(Core::ProxyType<RPC::CommunicatorClient>());

            } else {
                printf("Invalid endpoint specification\n");
                parameters.Option('?', nullptr);
            }
        }

    } else {

        if (parameters.RequestedUsage() != true) {
            printf("Invalid parameter specification\n");
            parameters.Option('?', nullptr);
        }

    }

    Core::Singleton::Dispose();

    return 0;
}
