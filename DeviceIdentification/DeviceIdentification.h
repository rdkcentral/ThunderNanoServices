#ifndef DeviceIdentification_DeviceIdentification_H
#define DeviceIdentification_DeviceIdentification_H

#include "Module.h"
#include <IdentityProvider.h>

namespace WPEFramework {
namespace Plugin {

    class DeviceIdentification : public PluginHost::IPlugin {
    public:
        DeviceIdentification()
            :  _idProvider(nullptr)
        {
        }

        virtual ~DeviceIdentification()
        {
        }

        BEGIN_INTERFACE_MAP(DeviceIdentification)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

    private:
        IdentityProvider* _idProvider;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // DeviceIdentification_DeviceIdentification_H
