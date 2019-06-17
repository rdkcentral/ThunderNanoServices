
#include "Module.h"
#include "DIALServer.h"
#include <interfaces/json/JsonData_DIALServer.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::DIALServer;

    // API implementation
    //

    // Event: start - Signals that application start was requested over DIAL *(passive mode only)*
    void DIALServer::event_start(const string& application, const string& parameters)
    {
        StartParamsInfo params;
        params.Application = application;
        params.Parameters = parameters;

        Notify(_T("start"), params);
    }

    // Event: stop - Signals that application stop was requested over DIAL *(passive mode only)*
    void DIALServer::event_stop(const string& application, const string& parameters)
    {
        StartParamsInfo params;
        params.Application = application;
        params.Parameters = parameters;

        Notify(_T("stop"), params);
    }

} // namespace Plugin

}

