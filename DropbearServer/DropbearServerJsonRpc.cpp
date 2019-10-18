
// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#include <interfaces/json/JsonData_DropbearServer.h>
#include "DropbearServer.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::DropbearServer;

    // Registration
    //

    void DropbearServer::RegisterAll()
    {
        Register<StartserviceParamsData,void>(_T("startservice"), &DropbearServer::endpoint_startservice, this);
        Register<void,void>(_T("stopservice"), &DropbearServer::endpoint_stopservice, this);
    }

    void DropbearServer::UnregisterAll()
    {
        Unregister(_T("stopservice"));
        Unregister(_T("startservice"));
    }

    // API implementation
    //

    // Start Dropbear Service.
    uint32_t DropbearServer::endpoint_startservice(const StartserviceParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if(params.Port.IsSet() == true) {
            result = StartService(params.Port.Value());
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        return result;
    }

    // Stop Dropbear Service.
    uint32_t DropbearServer::endpoint_stopservice()
    {
        uint32_t result = Core::ERROR_NONE;

        result = StopService();

        return result;
    }

} // namespace Plugin

} // namespace WPEFramework

