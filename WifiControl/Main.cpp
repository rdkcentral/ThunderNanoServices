#include <core/core.h>

#include "Network.h"
#include "Controller.h"

using namespace Thunder;

void Help() {
	printf("Options to use:\n");
	printf(" a: Add node to network\n");
	printf(" r: Remove node from network\n");
	printf(" l: Learn network\n");
	printf(" c: Cancel network operations (add/remove/learn)\n");
	printf(" -----------------------------------------------\n");
	printf(" 0: Basic set on first node to 0\n");
	printf(" 1: Basic set on first node to 0xFF\n");
	printf(" -----------------------------------------------\n");
	printf(" t: Turn on/off debuging of the communication line\n");
}

class Sink : public Core::IDispatchType<const WPASupplicant::Controller::events> {
private:
    Sink(const Sink&) = delete;
    Sink& operator= (const Sink&) = delete;

public:
    Sink() {
    }
    virtual ~Sink() {
    }

private:
    virtual void Dispatch (const WPASupplicant::Controller::events& event) override {
        Core::EnumerateType<WPASupplicant::Controller::events> what (event);
        printf("WPASupplicant notification: %s\n", what.Data());
    }
};

int main(int argc, char** argv)
{
    {
        Core::ProxyType<WPASupplicant::Controller> controller(WPASupplicant::Controller::Create(_T("/run/wpa_supplicant"), _T("/tmp"), _T("wlan0")));

        char keyCode;

        if (controller->IsOperational() == false) {
            printf("Failed to open. Error: %d.\n", controller->Error());
        }
        else {
            Sink mySink;
            char buffer[80];
            controller->Callback(&mySink);
            Core::TextFragment element(buffer, sizeof(buffer));
            controller->Scan();

            do {
                Core::TextFragment first;
                Core::TextFragment second;
                Core::TextFragment third;
                printf("\n>>");
                ::fgets(buffer, sizeof(buffer), stdin);
		uint32_t marker = element.ForwardSkip(_T(" \t"), 0);
                keyCode = (marker < sizeof(buffer) ? ::toupper(element[marker]) : 0);
                if (marker < sizeof(buffer)) {
                    marker = element.ForwardSkip(_T(" \t"), marker+1);
                    if (marker < sizeof(buffer)) {
                        uint32_t end = element.ForwardFind(_T(" \t\n"), marker);
                        if (end > marker) {
                            first = Core::TextFragment(element, marker, end - marker);
                            if (marker < sizeof(buffer)) {
                                marker = element.ForwardSkip(_T(" \t"), end);
                                if (marker < sizeof(buffer)) {
                                    end = element.ForwardFind(_T(" \t\n"), marker);
                                    if (end > marker) {
                                        second = Core::TextFragment(element, marker, end - marker);

                                        if (marker < sizeof(buffer)) {
                                            marker = element.ForwardSkip(_T(" \t"), end);
                                            if (marker < sizeof(buffer)) {
                                                end = element.ForwardFind(_T(" \t\n"), marker);
                                                if (end > marker) {
                                                    third = Core::TextFragment(element, marker, end - marker);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                switch (keyCode) {
                    case 'P': {
                        uint32_t result = controller->Ping();

                        if (result == Core::ERROR_NONE) {
                            printf("Ping-Pong. I am a-live\n");
                        }
                        else {
                            printf("Ping-*******. Oops no response !!!\n");
                        }
			break;
                    }
                    case 'L': {
                        uint32_t level = Core::NumberType<uint32_t>(first);
                        printf ("Log level: %d\n", level);
                        controller->Debug(level);
                        break;
                    }
                    case 'I': {
                        WPASupplicant::Network::Iterator index (controller->Networks());
                        while (index.Next() == true) {
                            printf ("%04d - %s - %d - %d - %s\n", index->Id(), index->BSSIDString().c_str(), index->Frequency(), index->Signal(), index->SSID().c_str());
                            
                        }
                        break;
                    }
                    case 'E':
                    {
                        int32_t id = Core::NumberType<int32_t>(first);
                        if (id < 0) {
                            printf ("Enabling config: all\n");
                            controller->Enable();
                        }
                        else {
                            WPASupplicant::Config config (controller->Get(first.Text()));
                            if (config.IsValid() == true) {
                                printf ("Enabling config: %d\n", id);
                                config.Enable();
                            }
                        }
                        break;
                    }
                    case 'D':
                    {
                        int32_t id = Core::NumberType<int32_t>(first);
                        if (id < 0) {
                            printf ("Disabling config: all\n");
                            controller->Disable();
                        }
                        else {
                            WPASupplicant::Config config(controller->Get(first.Text()));
                            if (config.IsValid() == true) {
                                printf ("Disabling config: %d\n", id);
                                config.Disable();
                            }
                        }
                        break;
                    }
                    case 'G':
                    {
                        string response;
                        printf ("Get Key on (%s): %s = ", first.Text().c_str(), second.Text().c_str());
                        WPASupplicant::Config config (controller->Get(first.Text()));
                        if ( (config.IsValid() == false) || (config.GetKey(second.Text(), response) != Core::ERROR_NONE) ) {
                            printf ("??? [FAILED]\n");
                        }
                        else {
                            printf ("%s [SUCCEEDED]\n", response.c_str());
                        }
                        break;
                    }
                    case 'S':
                    {
                        string response;
                        printf ("Set Key on (%s): %s = %s ", first.Text().c_str(), second.Text().c_str(), third.Text().c_str());
                        WPASupplicant::Config config (controller->Get(first.Text()));
                        if ( (config.IsValid() == false) || (config.SetKey(second.Text(), third.Text()) != Core::ERROR_NONE)) {
                            printf ("[FAILED]\n");
                        }
                        else {
                            printf ("[SUCCEEDED]\n");
                        }
                        break;
                    }
                    case 'C':
                    {
                        WPASupplicant::Config config (controller->Create(first.Text()));
                        printf ("Create a config for %s \n", first.Text().c_str());
                        break;
                    }
 
                }
            } while (keyCode != 'Q');
        }

        controller.Release();
    }

    Core::Singleton::Dispose();

    return(0);
}

