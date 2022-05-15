#ifndef MODULE_NAME
#define MODULE_NAME CecDeviceAdapterTest
#endif

#include "cec_defines.h"
#include <cec_device_adapter.h>

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <thread>

#include <CECTypes.h>
#include <core/core.h>
#include <localtracer/localtracer.h>
// #include <messaging/messaging.h>
#include <tracing/tracing.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework::CEC;
using namespace WPEFramework;

namespace {
struct conversion_entry {
    int from; // Local value
    int to; // External value
};

static constexpr conversion_entry _tableDevType[] = {
    { .from = 1, .to = CEC_DEVICE_TV },
    { .from = 2, .to = CEC_DEVICE_RECORDER },
    { .from = 3, .to = CEC_DEVICE_TUNER },
    { .from = 4, .to = CEC_DEVICE_PLAYBACK },
    { .from = 5, .to = CEC_DEVICE_AUDIOSYSTEM },
    { .from = 6, .to = CEC_DEVICE_SWITCH },
    { .from = 7, .to = CEC_DEVICE_VIDEOPROCESSOR }
};

template <size_t N>
int Convert(const conversion_entry (&table)[N], const int from, const int ifnotfound)
{
    uint16_t index = 0;
    while ((index < N) && (from != table[index].from)) {
        index++;
    }
    return (index < N ? table[index].to : ifnotfound);
}

class App {
private:
    char GetUserInput()
    {
        char x;
        std::cin >> x;
        return x;
    }

    cec_adapter_role_t SelectRole()
    {
        int character(CEC_LOGICAL_ADDRESS_UNREGISTERED);

        std::cout << "Choose device type:\n"
                     "  1 : TV\n"
                     "  2 : Recorder\n"
                     "  3 : Tuner\n"
                     "  4 : Playback\n"
                     "  5 : Audiosystem\n"
                     "  6 : Backup\n"
                     "  7 : Specific\n"
                  << std::endl
                  << std::flush;

        character = toupper(GetUserInput());

        uint8_t dev = static_cast<uint8_t>(strtol((char*)&character, NULL, 10));

        return static_cast<cec_adapter_role_t>(Convert(_tableDevType, dev, CEC_DEVICE_UNKNOWN));
    }

    void ShowMenu()
    {
        std::cout << "Enter\n"
                     "  C : Claim Logical address\n"
                     "  R : Release logical address\n"
                     "  P : Poll all logical addresses\n"
                     "  ? : Help\n"
                     "  Q : Quit\n"
                  << std::endl
                  << std::flush;
    }

public:
    App() = delete;
    App(App&) = delete;
    App& operator=(const App&) = delete;

    App(const string& device)
        : _adapter(cec_adapter_create(device.c_str()))
        , _roles()
        , _name("Metrological")
        , _language("dut")
        , _callbacks()
    {

        _callbacks.received = onReceived;
        _callbacks.claimed_roles = onClaimedRoles;

        int res = cec_adapter_receive(_adapter, &_callbacks, this);
        TRACE(Trace::Information, ("cec_adapter_receive result: 0x%02X", res));
    }

    void Run()
    {
        ShowMenu();

        int character;
        do {
            int res;
            character = toupper(GetUserInput());

            switch (character) {
            case 'C': {
                cec_adapter_role_t role = SelectRole();
                res = cec_adapter_claim_role(_adapter, role);
                break;
            }
            case 'R': {
                cec_adapter_role_t role = SelectRole();
                res = cec_adapter_release_role(_adapter, role);
                break;
            }
            case 'P': {
                cec_adapter_role_t role = SelectRole();
                for (uint8_t dest = CEC_LOGICAL_ADDRESS_TV; dest <= CEC_LOGICAL_ADDRESS_SPECIFIC; dest++) {
                    res = cec_adapter_transmit(_adapter, role, static_cast<logical_address_t>(dest), 0, nullptr);
                    TRACE(Trace::Information, ("Polled device 0x%02x, result %d", dest, res));
                }
                break;
            }
            case '?': {
                ShowMenu();
                break;
            }
            default:
                break;
            }
        } while (character != 'Q');
    }

    ~App()
    {
        int res = cec_adapter_destroy(_adapter);
        TRACE(Trace::Information, ("cec_adapter_destroy %d", res));
    }

    static void onReceived(void* cb_data, const cec_adapter_role_t follower, const uint8_t initiator, const uint8_t length, const uint8_t data[])
    {
        App* implementation = reinterpret_cast<App*>(cb_data);

        if (implementation != nullptr) {
            implementation->Received(follower, initiator, length, data);
        }
    }

    void Received(const cec_adapter_role_t follower, const uint8_t initiator, const uint8_t length, const uint8_t payload[])
    {
        string sdata;
        Core::ToHexString(payload, length, sdata);
        TRACE(Trace::Information, ("Data received from 0x%02X, for 0x%02X: %s", initiator, follower, sdata.c_str()));

        Parse(follower, initiator, length, payload);
    }

    static void onClaimedRoles(void* cb_data, const uint8_t roles)
    {
        App* implementation = reinterpret_cast<App*>(cb_data);

        if (implementation != nullptr) {
            implementation->ClaimedRoles(roles);
        }
    }

    void ClaimedRoles(const uint8_t roles)
    {
        TRACE(Trace::Information, ("Claimed roles 0x%02X", roles))
    }

private:
    int Transmit(const cec_adapter_role_t role, const uint8_t follower, const uint8_t length, const uint8_t payload[])
    {
        TRACE(Trace::Information, ("Transmitting to 0x%02X on the behave of 0x%02X", follower, role));

        int result = cec_adapter_transmit(_adapter, role, follower, length, payload);

        return result;
    }

    void Parse(const cec_adapter_role_t role, const uint8_t initiator, const uint8_t length, const uint8_t payload[])
    {
        uint8_t replyData[14] = { 0 };
        uint8_t relayLength(0);

        bool broadcast(false);

        if ((length > 0) && (payload[0] != NO_OPCODE)) {
            switch (payload[0]) {
            case GET_CEC_VERSION: {
                uint8_t parameters[] = { CEC_VERSION, CEC_VERSION_2_0 };
                memcpy(&replyData, &parameters, sizeof(parameters));
                relayLength = sizeof(parameters);
                break;
            }
            case GIVE_PHYSICAL_ADDR: {
                uint8_t parameters[] = { REPORT_PHYSICAL_ADDR, 0x00, 0x00, 0x00 };
                broadcast = true;
                memcpy(&replyData, &parameters, sizeof(parameters));
                relayLength = sizeof(parameters);
                break;
            }
            case GIVE_DEVICE_VENDOR_ID: {
                uint8_t parameters[] = { DEVICE_VENDOR_ID, 0x00, 0x0C, 0x03 };
                broadcast = true;
                memcpy(&replyData, &parameters, sizeof(parameters));
                relayLength = sizeof(parameters);
                break;
            }
            case GIVE_OSD_NAME: {
                replyData[0] = SET_OSD_NAME;
                memcpy(&replyData[1], _name.c_str(), _name.size());
                relayLength = _name.size() + 1;
                break;
            }
            case GET_MENU_LANGUAGE: {
                replyData[0] = SET_MENU_LANGUAGE;
                memcpy(&replyData[1], _language.c_str(), _language.size());
                relayLength = _language.size() + 1;
                break;
            }
            case GIVE_DEVICE_POWER_STATUS: {
                uint8_t parameters[] = { REPORT_POWER_STATUS, CEC_OP_POWER_STATUS_ON };
                memcpy(&replyData, &parameters, sizeof(parameters));
                relayLength = sizeof(parameters);
                break;
            }
            case GIVE_FEATURES: {
                uint8_t parameters[] = { REPORT_FEATURES, CEC_VERSION_2_0, 0x00, 0x00, 0x00 };
                memcpy(&replyData, &parameters, sizeof(parameters));
                relayLength = sizeof(parameters);
                break;
            }
            default:
                uint8_t parameters[] = { FEATURE_ABORT, CEC_OP_ABORT_INVALID_OP };
                memcpy(&replyData, &parameters, sizeof(parameters));
                relayLength = sizeof(parameters);
                break;
            }

            Transmit(role, broadcast ? 0x0F : initiator, relayLength, replyData);
        } else {
            TRACE(Trace::Information, ("Received poll message"));
        }
    }

private:
    mutable Core::CriticalSection _lock;

    cec_adapter_handle_t _adapter;
    std::unordered_map<cec_adapter_role_t, logical_address_t> _roles;
    std::string _name;
    std::string _language;
    cec_adapter_callbacks_t _callbacks;
};
}

int main(int /*argc*/, const char* argv[])
{
    std::string device("/dev/cec1");

    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();
    Messaging::ConsolePrinter printer(false);

    tracer.Callback(&printer);
    tracer.EnableMessage("CECDeviceAdapter", "Information", true);
    tracer.EnableMessage("CECDeviceAdapter", "Error", true);
    tracer.EnableMessage("CECMessageProcessor", "Information", true);
    tracer.EnableMessage("CECMessageProcessor", "Error", true);
    tracer.EnableMessage("CecDeviceAdapterTest", "Information", true);

    TRACE(Trace::Information, ("%s - build: %s", Core::FileNameOnly(argv[0]), __TIMESTAMP__));

    App app(device);

    app.Run();

    tracer.Close();

    WPEFramework::Core::Singleton::Dispose();

    return 0;
}