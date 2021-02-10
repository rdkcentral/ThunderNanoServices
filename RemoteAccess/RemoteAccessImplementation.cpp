#include "Module.h"
#include <interfaces/IRemoteAccess.h>
#include <interfaces/IMemory.h>

extern "C" {
void telnetd_main();
}

namespace WPEFramework {
namespace Plugin {

    class RemoteAccessImplementation : public Exchange::IRemoteAccess, public Core::Thread {
    private:

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
            {
            }
            ~Config()
            {
            }

        public:
        };

    private:
        RemoteAccessImplementation(const RemoteAccessImplementation&) = delete;
        RemoteAccessImplementation& operator=(const RemoteAccessImplementation&) = delete;

    public:
        RemoteAccessImplementation()
            : Core::Thread(Core::Thread::DefaultStackSize(), _T("RemoteAccessSession"))
        {
            SYSLOG(Logging::Startup, (_T("Inside %s:%d"), __FUNCTION__, __LINE__));
        }

        virtual ~RemoteAccessImplementation()
        {
            SYSLOG(Logging::Startup, (_T("Inside %s:%d"), __FUNCTION__, __LINE__));
        }

        uint32_t Configure(PluginHost::IShell* service)
        {
            ASSERT(service != nullptr);

            Config config;
            config.FromString(service->ConfigLine());
            SYSLOG(Logging::Startup, (_T("Inside %s:%d"), __FUNCTION__, __LINE__));

            uint32_t result = 0;
            Run();
            return (result);
        }

        uint32_t Enable()
        {
            return 0;
        }

        uint32_t Disable()
        {
            return 0;
        }

        uint32_t Worker()
        {
            SYSLOG(Logging::Startup, (_T("Starting telned")));
            telnetd_main();
            SYSLOG(Logging::Startup, (_T("Exited telned")));

            return 0;
        }

        BEGIN_INTERFACE_MAP(RemoteAccessImplementation)
        INTERFACE_ENTRY(Exchange::IRemoteAccess)
        END_INTERFACE_MAP

    private:
    };

    SERVICE_REGISTRATION(RemoteAccessImplementation, 1, 0);

} // namespace Plugin

} // namespace WPEFramework
