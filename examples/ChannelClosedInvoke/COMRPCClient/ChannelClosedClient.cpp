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
//

#include <iostream>



// Since lower levels 
// #define _TRACE_LEVEL 5

#define MODULE_NAME ChannelClosedClient

#include <core/core.h>
#include <com/com.h>
#include <definitions/definitions.h>
#include <plugins/Types.h>
#include <interfaces/ITestChannelClosed.h>
#include <iostream>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;


class Callback : public Exchange::ITestChannelClosed::ICallback {
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
    INTERFACE_ENTRY(Exchange::ITestChannelClosed::ICallback)
    END_INTERFACE_MAP
};

int main(int /* argc */, char** /* argv */)
{
 
    #ifdef __WINDOWS__
    Core::NodeId nodeId("127.0.0.1:62010");
    #else
    Core::NodeId nodeId("/tmp/communicator");
    #endif
    Core::ProxyType<RPC::CommunicatorClient> client = Core::ProxyType<RPC::CommunicatorClient>::Create(nodeId, Core::ProxyType<Core::IIPCServer>(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create()));

    Exchange::ITestChannelClosed* testinterface = client->Open<Exchange::ITestChannelClosed>(_T("TestChannelClosed"));

    if (testinterface == nullptr) {
        printf("Could not acquire testinterface\n");
    } else {
        printf("Testinterface acquired\n\n");


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
                    Core::ProxyType<RPC::CommunicatorClient> client = Core::ProxyType<RPC::CommunicatorClient>::Create(nodeId, Core::ProxyType<Core::IIPCServer>(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create()));
                    Exchange::ITestChannelClosed* testinterface = client->Open<Exchange::ITestChannelClosed>(_T("TestChannelClosed"));
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

    Core::Singleton::Dispose();

    return 0;
}
