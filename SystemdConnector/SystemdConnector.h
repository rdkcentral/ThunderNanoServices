#ifndef SYSTEMDCONNECTOR_H
#define SYSTEMDCONNECTOR_H

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class SystemdConnector : public PluginHost::IPlugin {
    public:
        SystemdConnector(const SystemdConnector&) = delete;
        SystemdConnector& operator=(const SystemdConnector&) = delete;

    public:
        SystemdConnector();
        virtual ~SystemdConnector();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(SystemdConnector)
            INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // SYSTEMDCONNECTOR_H
