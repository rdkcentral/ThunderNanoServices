
// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#include <interfaces/json/LocationSync_JsonData.h>
#include "LocationSync.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::LocationSync;

    // Registration
    //

    void LocationSync::RegisterAll()
    {
        Register<void,LocationResultData>(_T("location"), &LocationSync::endpoint_location, this);
        Register<void,void>(_T("sync"), &LocationSync::endpoint_sync, this);
    }

    void LocationSync::UnregisterAll()
    {
        Unregister(_T("sync"));
        Unregister(_T("location"));
    }

    // API implementation
    //

    // Retrieves location information.
    uint32_t LocationSync::endpoint_location(LocationResultData& response)
    {
        uint32_t result = Core::ERROR_NONE;

        PluginHost::ISubSystem* subSystem = _service->SubSystems();

        ASSERT(subSystem != nullptr);

        const PluginHost::ISubSystem::IInternet* internet(subSystem->Get<PluginHost::ISubSystem::IInternet>());
        const PluginHost::ISubSystem::ILocation* location(subSystem->Get<PluginHost::ISubSystem::ILocation>());

        ASSERT(internet != nullptr);
        ASSERT(location != nullptr);

        response.Publicip = internet->PublicIPAddress();
        response.Timezone = location->TimeZone();
        response.Region = location->Region();
        response.Country = location->Country();
        response.City = location->City();

        return result;
    }

    // Runs sync command.
    uint32_t LocationSync::endpoint_sync()
    {
        uint32_t result = Core::ERROR_NONE;

        if (_source.empty() == false) {
            result = _sink.Probe(_source, 1, 1);
        } else {
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

} // namespace Plugin

} // namespace WPEFramework

