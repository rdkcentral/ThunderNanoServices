/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the License);
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
#include "../SimpleCOMRPCInterface/ISimpleCOMRPCInterface.h"

using namespace WPEFramework;

class Math : public Exchange::IMath {
public:
    Math(const Math&) = delete;
    Math& operator= (const Math&) = delete;

    Math() {
    }
    ~Math() override {
    }

public:
    // Inherited via IMath
    uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum) const override
    {
        sum = A + B;
        return (Core::ERROR_NONE);
    }
    uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum) const override
    {
        sum = A - B;
        return (Core::ERROR_NONE);
    }

    BEGIN_INTERFACE_MAP(Math)
        INTERFACE_ENTRY(Exchange::IMath)
    END_INTERFACE_MAP
};

class Sink : public Exchange::IWallClock::ICallback {
public:
    Sink(const Sink&) = delete;
    Sink& operator= (const Sink&) = delete;

    Sink() {
    }
    ~Sink() override {
    }

public:
    BEGIN_INTERFACE_MAP(Sink)
        INTERFACE_ENTRY(Exchange::IWallClock::ICallback)
    END_INTERFACE_MAP

    void Elapsed(const uint16_t seconds) override {
        printf("The wallclock reports that %d seconds have elapsed since the last callback\n", seconds);
    }
};

bool ParseOptions(int argc, char** argv, Core::NodeId& comChannel)
{
    int index = 1;
    bool showHelp = false;
    comChannel = Core::NodeId(Exchange::SimpleTestAddress);

    while ((index < argc) && (!showHelp)) {
        if (strcmp(argv[index], "-connect") == 0) {
            comChannel = Core::NodeId(argv[index + 1]);
            index++;
        }
        else if (strcmp(argv[index], "-h") == 0) {
            showHelp = true;
        }
        index++;
    }

    return (showHelp);
}


int main(int argc, char* argv[])
{
    // The core::NodeId can hold an IPv4, IPv6, domain, HCI, L2CAP or netlink address
    // Here we create a domain socket address
    Core::NodeId comChannel;

    printf("\nSimpleCOMRPCClient is the counterpart for the SimpleCOMRPCServer\n");

    if (ParseOptions(argc, argv, comChannel) == true) {
        printf("Options:\n");
        printf("-connect <IP/FQDN>:<port> [default: %s]\n", Exchange::SimpleTestAddress);
        printf("-h This text\n\n");
    }
    else
    {
        int element;
        bool subscribed(false);
        Exchange::IWallClock* clock(nullptr);
        Core::Sink<Sink> sink;
        Core::ProxyType<RPC::CommunicatorClient> client(Core::ProxyType<RPC::CommunicatorClient>::Create(comChannel));
        Math* outbound = Core::Service<Math>::Create<Math>();
        printf("Channel: %s:[%d]\n\n", comChannel.HostAddress().c_str(), comChannel.PortNumber());

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case 'O':
                printf("Offering our IMath interface to te otherside..!\n");
                if (client->IsOpen() == false) {
                    client->Open(2000);
                }

                if (client->IsOpen() == false) {
                    printf("Could not open a connection to the server. No exchange of interfaces happened!\n");
                }
                else {
                    uint32_t result = client->Offer<Exchange::IMath>(outbound);

                    if (result == Core::ERROR_NONE) {
                        printf("Our IMath nterface has been offered to the other side!\n");
                    }
                    else {
                        printf("Our offer has not been accepted, Error: %d!\n", result);
                    }
                }
                break;
            case 'C':
                if (clock != nullptr) {
                    printf("There is no need to create a clock, we already have one!\n");
                }
                else {
                    if (client->IsOpen() == false) {
                        client->Open(2000);
                    }

                    if (client->IsOpen() == false) {
                        printf("Could not open a connection to the server. No exchange of interfaces happened!\n");
                    }
                    else {
                        clock = client->Aquire<Exchange::IWallClock>(3000, _T("WallClockImplementation"), ~0);

                        if (clock == nullptr) {
                            printf("Tried aquiring the IWallclock, but it is not available\n");
                        }
                        else {
                            printf("Aquired the IWallclock, ready for use\n");
                        }
                    }
                }
                break;
            case 'D':
                if (clock == nullptr) {
                    printf("We can not destroy the clock, because we have no clock :-)\n");
                }
                else {
                    clock->Release();
                    clock = nullptr;
                    printf("Released the IWallclock, no more service available\n");
                }
                break;
            case 'I':
                if (clock == nullptr) {
                    printf("We do not have a clock interface, so we can not get the time\n");
                }
                else {
                    printf("The Ticker is at: %llu\n", clock->Now());
                }
                break;
            case 'T':
                if (clock == nullptr) {
                    printf("We do not have a clock interface, so we can not register the callback\n");
                }
                else if (subscribed == true) {
                    subscribed = false;
                    clock->Callback(nullptr);
                    printf("We removed the callback from the wallclock. We will nolonger be updated\n");
                }
                else {
                    subscribed = true;
                    clock->Callback(&sink);
                    printf("We set the callback on the wallclock. We will be updated\n");
                }
                break;
            case 'E': exit(0); break;
            case 'Q': break;
            case'?':
                printf("Options available:\n");
                printf("=======================================================================\n");
                printf("<O> Offer the IMath interface to the server, for calculating.\n");
                printf("<C> Create the clock interface, usefull for telling the time.\n");
                printf("<D> Destroy the clock interface, we are nolonger interested in the time.\n");
                printf("<I> Information required, tell  me the current time.\n");
                printf("<T> Toggle subscription for continous updates.\n");
                printf("<Q> We are done playing around, eave the application properly.\n");
                printf("<E> Eject, this is an emergency, bail out, just kill the app.\n");
                printf("<?> Have no clue what I can do, tell me.\n");
            default: break;
            }

        } while (element != 'Q');

        if (outbound != nullptr) {
            outbound->Release();
        }
    }

    Core::Singleton::Dispose();

    return 0;
}
