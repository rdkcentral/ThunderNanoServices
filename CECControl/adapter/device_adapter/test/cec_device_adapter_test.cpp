#ifndef MODULE_NAME
#define MODULE_NAME CecDeviceAdapterTest
#endif

#include <cec_device_adapter.h>

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <thread>

#include <CECTypes.h>
#include <core/core.h>
#include <messaging/messaging.h>

#include <localtracer/localtracer.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder::CEC;
using namespace Thunder;

/* Power Status Feature */
#define CEC_MESSAGE_GIVE_DEVICE_POWER_STATUS 0x8f
#define CEC_MESSAGE_REPORT_POWER_STATUS 0x90
/* Power Status Operand (pwr_state) */
#define CEC_PARAMETER_POWER_STATUS_ON 0
#define CEC_PARAMETER_POWER_STATUS_STANDBY 1
#define CEC_PARAMETER_POWER_STATUS_TO_ON 2
#define CEC_PARAMETER_POWER_STATUS_TO_STANDBY 3

/* General Protocol Messages */
#define CEC_MESSAGE_FEATURE_ABORT 0x00
/* Abort Reason Operand (reason) */
#define CEC_PARAMETER_ABORT_UNRECOGNIZED_OP 0
#define CEC_PARAMETER_ABORT_INCORRECT_MODE 1
#define CEC_PARAMETER_ABORT_NO_SOURCE 2
#define CEC_PARAMETER_ABORT_INVALID_OP 3
#define CEC_PARAMETER_ABORT_REFUSED 4
#define CEC_PARAMETER_ABORT_UNDETERMINED 5

#define CEC_MESSAGE_ABORT 0xff

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
    string GetUserInput(bool capitalize)
    {
        std::string input;
        std::getline(std::cin, input); // get input until enter key is pressed

        if (capitalize) {
            for (uint8_t i = 0; i < input.size(); i++) {
                input.at(i) = toupper(input.at(i));
            }
        }

        return input;
    }

    cec_adapter_role_t SelectRole()
    {
        string uInput;
        uint8_t dev(~0);

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

        uInput = GetUserInput(true);

        if (uInput.size() > 0) {
            dev = static_cast<uint8_t>(strtol(uInput.c_str(), NULL, 10));
        }

        return static_cast<cec_adapter_role_t>(Convert(_tableDevType, dev, CEC_DEVICE_UNKNOWN));
    }

    logical_address_t SelectAddress()
    {
        string uInput;
        logical_address_t adress(CEC_LOGICAL_ADDRESS_INVALID);

        std::cout << "To whom?\n"
                     "  (0-15): for direct\n"
                     "  A: for all devices\n"
                  << std::endl
                  << std::flush;

        uInput = GetUserInput(true);

        if (uInput.size() > 0) {
            if (uInput == "A") {
                adress = CEC_LOGICAL_ADDRESS_INVALID;
            } else {
                adress = static_cast<logical_address_t>(strtol(uInput.c_str(), NULL, 10));
            }
        }

        return adress;
    }

    uint8_t SelectNumber()
    {
        string uInput;
        uint8_t number(1);

        std::cout << "How many times? (0-255)\n"
                  << std::endl
                  << std::flush;

        uInput = GetUserInput(true);

        if (uInput.size() > 0) {
            long int n = (strtol(uInput.c_str(), NULL, 10));
            number = (n <= 0) ? 1 : ((n > 255) ? 255 : static_cast<uint8_t>(n));
        }

        return number;
    }

    void ShowMenu()
    {
        std::cout << "Enter\n"
                     "  C : Claim Logical address\n"
                     "  R : Release logical address\n"
                     "  P : Poll logical address(es)\n"
                     "  V : Get VendorID(s)\n"
                     "----------------------------------------\n"
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
        : _lock()
        , _adapter(cec_adapter_create(device.c_str()))
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

        char character;
        do {
            int res;
            character = '\0';

            string uInput = GetUserInput(true);

            if (uInput.size() > 0) {
                character = uInput.at(0);
            }

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
                logical_address_t address = SelectAddress();

                if (address == CEC_LOGICAL_ADDRESS_INVALID) {
                    for (uint8_t dest = CEC_LOGICAL_ADDRESS_TV; dest <= CEC_LOGICAL_ADDRESS_SPECIFIC; dest++) {
                        res = cec_adapter_transmit(_adapter, role, static_cast<logical_address_t>(dest), 0, nullptr);
                        TRACE(Trace::Information, ("Polled device 0x%02x, result %d", dest, res));
                    }
                } else {
                    uint8_t n = SelectNumber();
                    for (uint8_t i = 0; i < n; i++) {
                        res = cec_adapter_transmit(_adapter, role, address, 0, nullptr);
                        TRACE(Trace::Information, ("Polled device no. %d 0x%02x, result %d", i + 1, address, res));
                    }
                }
                break;
            }
            case 'V': {
                cec_adapter_role_t role = SelectRole();
                logical_address_t address = SelectAddress();
                uint8_t getVendor(GIVE_DEVICE_VENDOR_ID);

                if (address == CEC_LOGICAL_ADDRESS_INVALID) {
                    for (uint8_t dest = CEC_LOGICAL_ADDRESS_TV; dest <= CEC_LOGICAL_ADDRESS_SPECIFIC; dest++) {
                        res = cec_adapter_transmit(_adapter, role, static_cast<logical_address_t>(dest), sizeof(getVendor), &getVendor);
                        TRACE(Trace::Information, ("Request VendorId of device 0x%02x, result %d", dest, res));
                    }
                } else {
                    res = cec_adapter_transmit(_adapter, role, address, sizeof(getVendor), &getVendor);
                    TRACE(Trace::Information, ("Request VendorId of device 0x%02x, result %d", address, res));
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

        if (payload) {
            Core::ToHexString(payload, length, sdata);
            TRACE(Trace::Information, ("Data received from 0x%02X, for 0x%02X: %s", initiator, follower, sdata.c_str()));
        }

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
        TRACE(Trace::Information, ("Transmitting to 0x%02X via role 0x%02X", follower, role));

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
                uint8_t parameters[] = { REPORT_POWER_STATUS, CEC_PARAMETER_POWER_STATUS_ON };
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
                uint8_t parameters[] = { FEATURE_ABORT, CEC_PARAMETER_ABORT_INVALID_OP };
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

int main(int argc, const char* argv[])
{
    std::string device = "/dev/cec";

    if (argc > 1) {
        device = argv[1];
    }

    {
        Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();
        Messaging::ConsolePrinter printer(false);

        tracer.Callback(&printer);
        tracer.EnableMessage("CECAdapter", "Information", true);
        tracer.EnableMessage("CECAdapter", "Error", true);
        tracer.EnableMessage("CecDeviceAdapterTest", "Information", true);

        TRACE(Trace::Information, ("%s - build: %s", Core::FileNameOnly(argv[0]), __TIMESTAMP__));

        App app(device);
        app.Run();
        tracer.Close();
    }

    Thunder::Core::Singleton::Dispose();

    return 0;
}