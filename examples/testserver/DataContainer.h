#ifndef __DATACONTAINER_H
#define __DATACONTAINER_H

#include <generics/generics.h>

namespace WPEFramework {
namespace DataContainer {
    typedef enum {
        ExecuteShell,
        WiFiSettings,
        FanControl,
        PlayerControl

    } CommandType;
    class Parameters : public Core::JSON::Container {
    private:
        Parameters(const Parameters&);
        Parameters& operator=(const Parameters&);

    public:
        Parameters()
        {
            Add(_T("speed"), &Speed);
            Add(_T("duration"), &Duration);
            Add(_T("command"), &Command);
            Add(_T("settings"), &Settings);
        }
        ~Parameters()
        {
        }

    public:
        Core::JSON::OctSInt16 Speed;
        Core::JSON::DecUInt16 Duration;
        Core::JSON::EnumType<CommandType> Command;
        Core::JSON::ArrayType<Core::JSON::DecUInt16> Settings;
    };

    class Command : public Core::JSON::Container {
    private:
        Command(const Command&);
        Command& operator=(const Command&);

    public:
        Command()
        {
            Add(_T("id"), &Identifier);
            Add(_T("name"), &Name);
            Add(_T("baseAddress"), &BaseAddress);
            Add(_T("trickFlag"), &TrickFlag);
            Add(_T("parameters"), &Params);
        }
        ~Command()
        {
        }

    public:
        Core::JSON::DecUInt32 Identifier;
        Core::JSON::String Name;
        Core::JSON::HexUInt32 BaseAddress;
        Core::JSON::Boolean TrickFlag;
        Parameters Params;
    };
}
}

#endif // __DATACONTAINER_H
