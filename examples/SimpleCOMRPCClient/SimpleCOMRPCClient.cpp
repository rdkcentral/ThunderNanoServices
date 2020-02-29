#include <core/core.h>
#include <com/com.h>
#include "../SimpleCOMRPCInterface/ISimpleCOMRPCInterface.h"

using namespace WPEFramework;

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
        printf("The wallclock reports that %d seconds have elapsed since the last callback\n");
    }
};

#ifdef __WINDOWS__
const TCHAR defaultAddress[] = _T("127.0.0.1:63000");
#else
const TCHAR defaultAddress[] = _T("/tmp/comserver");
#endif

bool ParseOptions(int argc, char** argv, Core::NodeId& comChannel)
{
    int index = 1;
    bool showHelp = false;
    Core::NodeId nodeId(defaultAddress);

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

    if (ParseOptions(argc, argv, comChannel) == true) {
        printf("\nSimpleCOMRPCClient is the counterpart for the SimpleCOMRPCServer\n");
        printf("Options:\n");
        printf("-connect <IP/FQDN>:<port> [default: %s]\n", defaultAddress);
        printf("-h This text\n\n");
    }
    else
    {
        int element;
        bool subscribed(false);
        Exchange::IWallClock* clock(nullptr);
        Core::Sink<Sink> sink;
        RPC::CommunicatorClient client(comChannel);

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case 'C': 
                if (clock != nullptr) {
                    printf("There is no need to create a clock, we already have one!\n");
                }
                else {
                    clock = client.Aquire<Exchange::IWallClock>(3000, _T("WallClockImplementation"), ~0);
                }
                break;
            case 'D':
                if (clock == nullptr) {
                    printf("We can not destroy the clock, because we have no clock :-)\n");
                }
                else {
                    clock->Release();
                    clock = nullptr;
                }
                break;
            case 'I':
                if (clock == nullptr) {
                    printf("We do not have a clock interface, so we can not get the time\n");
                }
                else {
                    printf ("The Ticker is at: %llu\n", clock->Now());
                }
                break;
            case 'T':
                if (clock == nullptr) {
                    printf("We do not have a clock interface, so we can not register the callback\n");
                }
                else if (subscribed == true){
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
            case 'Q': break;
            default: break;
            }

        } while (element != 'Q');
    }

    Core::Singleton::Dispose();

    return 0;
}
