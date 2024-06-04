/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#ifndef MODULE_NAME
#define MODULE_NAME CECAccessorTest
#endif

#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdarg.h>
#include <thread>
#include <time.h>

#include <CECAdapter.h>
#include <CECIAccessor.h>
#include <CECProcessor.h>
#include <CECTypes.h>
#include <Messages.h>

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

#include "../CECAccessor.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder::CEC;
using namespace Thunder;

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

class Config : public Core::JSON::Container {
public:
    class Interface : public Core::JSON::Container {
    public:
        Interface(const Interface& copy)
            : Name(copy.Name)
            , Config(copy.Config)
        {
        }

        Interface& operator=(const Interface& rhs)
        {
            Name = rhs.Name;
            Config = rhs.Config;
            return (*this);
        }

        Interface()
            : Name()
            , Config()
        {
            Add(_T("name"), &Name);
            Add(_T("configuration"), &Config);
        }

        ~Interface() = default;

    public:
        Core::JSON::String Name;
        Core::JSON::String Config;
    };

    class InterfaceConfig : public Core::JSON::Container {
    public:
        InterfaceConfig(const InterfaceConfig&) = delete;
        InterfaceConfig& operator=(const InterfaceConfig&) = delete;

        InterfaceConfig()
            : Node()
            , Roles()
        {
            Add(_T("node"), &Node);
            Add(_T("roles"), &Roles);
        }

        ~InterfaceConfig() = default;

    public:
        Core::JSON::String Node;
        Core::JSON::ArrayType<Core::JSON::EnumType<cec_adapter_role_t>> Roles;
    };

public:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config()
        : Interfaces()
    {
        Add(_T("interfaces"), &Interfaces);
    }
    ~Config()
    {
    }

public:
    Core::JSON::ArrayType<Core::JSON::String> Interfaces;
};

class App {
private:
public:
    App(const string& filename)
    {
        TRACE(Trace::Information, ("Start %s(%p) - config file: %s", __FUNCTION__, this, filename.c_str()));

        Thunder::Core::File configFile(filename.c_str());

        if (configFile.Exists() && configFile.Open(true)) {
            _config.IElement::FromFile(configFile);
            configFile.Close();
        } else {
            std::cerr << "Failed to open " << filename << std::endl
                      << std::flush;
        }

        auto index(_config.Interfaces.Elements());

        while (index.Next() == true) {
            Config::Interface iface;
            iface.FromString(index.Current().Value());

            string name = iface.Name.Value();
            string config = iface.Config.Value();
            Thunder::CEC::IAccessor::Instance()->Announce(name, config);

            Config::InterfaceConfig ifaceconfig;
            ifaceconfig.FromString(config);

            InterfaceMap::iterator index = _interfaces.find(name);

            if (index == _interfaces.end()) {
                index = _interfaces.emplace(
                                       std::piecewise_construct,
                                       std::forward_as_tuple(name),
                                       std::forward_as_tuple(device_list_t()))
                            .first;
            }

            if (index != _interfaces.end()) {
                TRACE(Trace::Information, ("Register Roles for %s", (index->first).c_str()));

                auto roles(ifaceconfig.Roles.Elements());

                while (roles.Next() == true) {
                    TRACE(Trace::Information, ("Register Role 0x%02X ", roles.Current().Value()));
                    (index->second).push_back(roles.Current().Value());
                }
            }
        }

        ShowMenu();
    }

    ~App()
    {
        TRACE(Trace::Information, ("Exit %s", __FUNCTION__));

        auto index(_config.Interfaces.Elements());

        while (index.Next() == true) {
            Config::Interface iface;
            iface.FromString(index.Current().Value());

            string name = iface.Name.Value();
            Thunder::CEC::IAccessor::Instance()->Revoke(name);
        }

        TRACE(Trace::Information, ("Exit %s done...", __FUNCTION__));
    }

    void Run()
    {
        int character;

        do {
            character = toupper(GetUserInput());

            switch (character) {
            case 'V': {
                cec_adapter_role_t role;
                string interface;
                GetAdapter(interface, role);

                Thunder::CEC::Adapter adapter = Thunder::CEC::IAccessor::Instance()->GetAdapter(interface, role);

                if (adapter.IsValid()) {
                    for (uint8_t dest = CEC_LOGICAL_ADDRESS_TV; dest <= CEC_LOGICAL_ADDRESS_SPECIFIC; dest++) {
                        Message::Version version;
                        uint32_t r = adapter.Exchange(static_cast<logical_address_t>(dest), version);

                        if (r == Core::ERROR_NONE) {
                            if (version.Value() == VERSION_CEC_2_0) {
                                TRACE(Trace::Information, ("Device 0x%02X supports bleeding edge CEC!!!!", dest));
                            } else {
                                TRACE(Trace::Information, ("Device 0x%02X supports 0x%02X CEC :-(", dest, version.Value()));
                            }
                        } else {
                            TRACE(Trace::Error, ("Error 0x%02X for device 0x%02X....", r, dest));
                        }
                    }
                } else {
                    TRACE(Trace::Error, ("Could not find valid adapter"));
                }

                break;
            }
            case 'P': {
                cec_adapter_role_t role;
                string interface;
                GetAdapter(interface, role);

                Thunder::CEC::Adapter adapter = Thunder::CEC::IAccessor::Instance()->GetAdapter(interface, role);

                if (adapter.IsValid()) {
                    for (uint8_t dest = CEC_LOGICAL_ADDRESS_TV; dest <= CEC_LOGICAL_ADDRESS_SPECIFIC; dest++) {
                        Message::Poll poll;
                        uint32_t r = adapter.Exchange(static_cast<logical_address_t>(dest), poll);
                        TRACE(Trace::Information, ("Exchange[0x%04X]: Device 0x%02X %s", r, dest, (r == Core::ERROR_NONE) ? "available" : "unavailable"));
                    }
                } else {
                    TRACE(Trace::Error, ("Could not find valid adapter"));
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

private:
    const string ToString(cec_adapter_role_t role)
    {
        string result;

        switch (role) {
        case CEC_DEVICE_TV: {
            result = "TV";
            break;
        }
        case CEC_DEVICE_RECORDER: {
            result = "Recorder";
            break;
        }
        case CEC_DEVICE_PLAYBACK: {
            result = "Playback";
            break;
        }
        case CEC_DEVICE_AUDIOSYSTEM: {
            result = "AudioSystem";
            break;
        }
        case CEC_DEVICE_SWITCH: {
            result = "Switch";
            break;
        }
        case CEC_DEVICE_VIDEOPROCESSOR: {
            result = "VideoProcessor";
            break;
        }
        default:
            result = "Unknown";
        }

        return result;
    }

    void GetAdapter(string& uIface, cec_adapter_role_t& uRole)
    {
        if (_interfaces.empty() == false) {
            int character(CEC_LOGICAL_ADDRESS_UNREGISTERED);
            uint8_t idx = 1;

            InterfaceMap::const_iterator interfaces = _interfaces.cbegin();

            if (_interfaces.size() == 1) {
                std::cout << interfaces->first << " was auto selected for the adapter" << std::endl
                          << std::flush;
            } else {
                std::cout << "Choose device:" << std::endl
                          << std::flush;

                while (interfaces != _interfaces.cend()) {
                    std::cout << " " << unsigned(idx++) << ". : " << interfaces->first << std::endl;
                    interfaces++;
                }
                uint8_t interface(0);

                character = toupper(GetUserInput());
                interface = static_cast<uint8_t>(strtol((char*)&character, NULL, 10)) - 1;

                interfaces = _interfaces.cbegin();
                std::advance(interfaces, interface);
            }

            uIface = interfaces->first;

            if ((interfaces->second).empty() == false) {
                device_list_t::const_iterator roles = (interfaces->second).cbegin();

                if ((interfaces->second).size() == 1) {
                    std::cout << ToString((*roles)) << " was auto selected for the role." << std::endl
                              << std::flush;
                } else {
                    idx = 1;
                    std::cout << "Choose role:" << std::endl
                              << std::flush;

                    while (roles != (interfaces->second).cend()) {
                        std::cout << " " << unsigned(idx++) << ". : " << ToString((*roles)) << std::endl;
                        roles++;
                    }

                    character = toupper(GetUserInput());
                    uint8_t role = static_cast<uint8_t>(strtol((char*)&character, NULL, 10)) - 1;

                    roles = (interfaces->second).cbegin();
                    std::advance(roles, role);
                }

                uRole = *roles;
            } else {
                std::cout << "No roles registered on " << interfaces->first << "!!!" << std::endl
                          << std::flush;
            }
        }
    }

    cec_adapter_role_t SelectRole()
    {
        int character(CEC_LOGICAL_ADDRESS_UNREGISTERED);

        std::cout << "Choose role:\n"
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
                     "  P : Poll all logical addresses\n"
                     "  V : Scan for CEC versions\n"
                     "  ? : Help\n"
                     "  Q : Quit\n"
                  << std::endl;
    }

    char GetUserInput()
    {
        char x;
        std::cin >> x;
        return x;
    }

    uint8_t SelectDevice()
    {
        int character(CEC_LOGICAL_ADDRESS_UNREGISTERED);

        std::cout << "Choose device type:\n"
                     "  1 : TV\n"
                     "  2 : Record\n"
                     "  3 : Tuner\n"
                     "  4 : Playback\n"
                     "  5 : Audiosystem\n"
                     "  6 : Other\n"
                  << std::endl;

        character = toupper(GetUserInput());

        return strtol((char*)&character, NULL, 16) & 0x0F;
    }

    typedef std::map<string, device_list_t> InterfaceMap;
private:
    Config _config;
    InterfaceMap _interfaces;
}; // class App
} // namespace

int main(int argc, const char* argv[])
{
    {
        std::string filename;

        if (argc == 1) {
            filename = "cec_config.json";
        } else {
            filename = argv[1];
        }

        {
            Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();
            Messaging::ConsolePrinter printer(false);

            tracer.Callback(&printer);
            tracer.EnableMessage("CECAdapter", "Information", true);
            tracer.EnableMessage("CECAdapter", "Error", true);
            tracer.EnableMessage("CECAccessorTest", "Information", true);
            tracer.EnableMessage("CECAccessorTest", "Error", true);

            TRACE(Trace::Information, ("%s - build: %s", Core::FileNameOnly(argv[0]), __TIMESTAMP__));

            App app(filename);

            app.Run();

            tracer.Close();
        }

        std::cout << "Dispose...." << std::endl
                  << std::flush;

        Thunder::Core::Singleton::Dispose();
    }

    std::cout << "Ok, bye! " << std::endl
              << std::flush;

    return 0;
}