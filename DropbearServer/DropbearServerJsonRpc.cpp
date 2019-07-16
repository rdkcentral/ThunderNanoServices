
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
	Register<Core::JSON::DecUInt32>(_T("gettotalsessions"), &DropbearServer::endpoint_gettotalsessions, this);
	Register<Core::JSON::DecUInt32>(_T("getactivesessionscount"), &DropbearServer::endpoint_getactivesessionscount, this);
	Register<Core::JSON::ArrayType<SessioninfoResultData>>(_T("getactivesessionsinfo"), &DropbearServer::endpoint_getactivesessionsinfo, this);
	Register<Core::JSON::DecUInt32, void>(_T("closeclientsession"), &DropbearServer::endpoint_closeclientsession, this);
    }

    void DropbearServer::UnregisterAll()
    {
        Unregister(_T("closeclientsession"));
        Unregister(_T("getactivesessionsinfo"));
        Unregister(_T("getactivesessionscount"));
        Unregister(_T("gettotalsessions"));
        Unregister(_T("stopservice"));
        Unregister(_T("startservice"));
    }

    // API implementation
    //
    
    // Property: url - URL loaded in the browser
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Start Dropbear Service.
    uint32_t DropbearServer::endpoint_startservice(const JsonData::DropbearServer::StartserviceParamsData& params);
    {
        uint32_t result = Core::ERROR_NONE;
	TRACE(Trace::Information, (_T("BackgroundFlag: %s PortFlag: %s Port: %s"), params.BackgroundFlag.Value(), params.PortFlag.Value(), params.Port.Value()));

        if(params.Port.IsSet() == true) {
            result = StartService(params.Port.Value(), params.PortFlag.Value(), params.BackgroundFlag.Value());
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        return result;
    }

    // Property: url - URL loaded in the browser
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Stop Dropbear Service.
    uint32_t DropbearServer::endpoint_stopservice()
    {
        uint32_t result = Core::ERROR_NONE;

        result = StopService();

        return result;
    }

    // Property: url - URL loaded in the browser
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Get total SSH client sessions served by Dropbear Service.
    uint32_t DropbearServer::endpoint_gettotalsessions(Core::JSON::DecUInt32& response)
    {
        uint32_t result = Core::ERROR_NONE;

        response = GetTotalSessions();

        return result;
    }

    // Property: url - URL loaded in the browser
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Get total active SSH client sessions managed by Dropbear Service.
    uint32_t DropbearServer::endpoint_getactivesessionscount(Core::JSON::DecUInt32& response)
    {
        uint32_t result = Core::ERROR_NONE;

        response = GetSessionsCount();

        return result;
    }

    // Property: url - URL loaded in the browser
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Get details of each active SSH client sessions managed by Dropbear Service.
    uint32_t DropbearServer::endpoint_getactivesessionsinfo(Core::JSON::ArrayType<SessioninfoResultData>& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        result = GetSessionsInfo(response);

        return result;
    }

    // Property: url - URL loaded in the browser
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Close a SSH client session.
    uint32_t DropbearServer::endpoint_closeclientsession(Core::JSON::DecUInt32& pid) const
    {
        uint32_t result = Core::ERROR_NONE;
	const uint32_t& client_pid = pid.Value();

        result = CloseClientSession(client_pid);

        return result;
    }

} // namespace Plugin

} // namespace WPEFramework

