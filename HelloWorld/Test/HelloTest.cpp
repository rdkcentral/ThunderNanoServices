/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 
#include <core/core.h>
#include <com/com.h>
#include <tracing/tracing.h>
#include <interfaces/IHelloWorld.h>

using namespace WPEFramework;

int main (int argc, char** argv)
{
    const char* hostname = _T("/tmp/helloworld");
    Core::NodeId comChannel = Core::NodeId(hostname);


    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> engine(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create());
        Core::ProxyType<RPC::CommunicatorClient> client (
            Core::ProxyType<RPC::CommunicatorClient>::Create(
                comChannel, 
                Core::ProxyType<Core::IIPCServer>(engine)
            ));
    engine->Announcements(client->Announcement());

     SleepMs(4000);

    ASSERT(client.IsValid() == true);

    // Open up the COMRPC Client connection.
    if (client->Open(2000) != Core::ERROR_NONE) {
        printf("Failed to open up a COMRPC link with the server. Is the server running ?\n");
    }

    //Acquire the interface
    Exchange::IHelloWorld* hw = client->Aquire<Exchange::IHelloWorld>(2000, _T("HelloWorldImplementation"), ~0);
    if (hw == nullptr) {
        printf("Instantiation failed. A HelloWorld interface was not returned.\n");
    } else {
        printf("Instantiation and retrieving the interface succeeded.\n");
        const string msg = "Hello COM RPC!! ";
        string out;
        hw->Greet(msg, out);
        printf(" Reply - %s\n",out.c_str());
    }

    // You can do this explicitely but it should not be nessecary as the destruction of the Client will close the link anyway.
    client->Close(1000);

}
