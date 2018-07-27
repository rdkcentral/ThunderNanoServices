#include "SystemdConnector.h"

#include <systemd/sd-daemon.h>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(SystemdConnector, 1, 0);

    static void Startup () {

        TRACE_L1("Notify systemd that the Platform is up and running.\n");

        int rc = sd_notifyf(0,
                "READY=1\n"
                "STATUS=Platform Server is Ready (from WPE Framework Compositor Plugin)\n"
                "MAINPID=%lu",
                ::getpid());
        if (rc) {
            TRACE_L1("Notify Nexus Server Ready to systemd: FAILED (%d)\n", rc);
        }
        else {
            TRACE_L1("Notify Nexus Server Ready to systemd: OK\n");
        }
    }

    static void Shutdown () {

        TRACE_L1("Notify systemd that the Platform is nolonger available.\n");

        int rc = sd_notifyf(0,
                "READY=0\n"
                "STATUS=Platform Server is Down (from WPE Framework Compositor Plugin)\n"
                "MAINPID=%lu",
                ::getpid());
        if (rc) {
            TRACE_L1("Notify Nexus Server Ready to systemd: FAILED (%d)\n", rc);
        }
        else {
            TRACE_L1("Notify Nexus Server Ready to systemd: OK\n");
        }
    }

    SystemdConnector::SystemdConnector()
    {
    }

    /* virtual */ SystemdConnector::~SystemdConnector()
    {
    }

    /* virtual */ const string SystemdConnector::Initialize(PluginHost::IShell* /* service */)
    {
	StartUp();
        // On success return empty, to indicate there is no error text.
        return (string());
    }

    /* virtual */ void SystemdConnector::Deinitialize(PluginHost::IShell* /* servicei */)
    {
        ShutDown();
    }

    /* virtual */ string SystemdConnector::Information() const
    {
        // No additional info to report.
        return (string());
    }

} // namespace Plugin
} // namespace WPEFramework
