
// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#include <interfaces/json/JsonData_SecureShellServer.h>
#include "SecureShellServer.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::SecureShellServer;

    // Registration
    //

    void SecureShellServer::RegisterAll()
    {
        Register<Core::JSON::String,void>(_T("startservice"), &SecureShellServer::endpoint_startservice, this);
        Register<void,void>(_T("stopservice"), &SecureShellServer::endpoint_stopservice, this);
	Register<void, Core::JSON::DecUInt32>(_T("getactivesessionscount"), &SecureShellServer::endpoint_getactivesessionscount, this);
	Register<void, Core::JSON::ArrayType<SessioninfoResultData>>(_T("getactivesessionsinfo"), &SecureShellServer::endpoint_getactivesessionsinfo, this);
	Register<CloseclientsessionParamsData,void>(_T("closeclientsession"), &SecureShellServer::endpoint_closeclientsession, this);
    }

    void SecureShellServer::UnregisterAll()
    {
        Unregister(_T("closeclientsession"));
        Unregister(_T("getactivesessionsinfo"));
        Unregister(_T("getactivesessionscount"));
        Unregister(_T("stopservice"));
        Unregister(_T("startservice"));
    }

    // API implementation
    //
    
    // Property: 
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Start Dropbear Service.
    uint32_t SecureShellServer::endpoint_startservice(const Core::JSON::String& params)
    {
        uint32_t result = Core::ERROR_NONE;
	std::string& argv = params.Value();

        if(params.Port.IsSet() == true) {
            result = StartService(argv);
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        return result;
    }

    // Property: 
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Stop Dropbear Service.
    uint32_t SecureShellServer::endpoint_stopservice()
    {
        uint32_t result = Core::ERROR_NONE;

        result = StopService();

        return result;
    }

    // Property: 
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Get total active SSH client sessions managed by Dropbear Service.
    uint32_t SecureShellServer::endpoint_getactivesessionscount(Core::JSON::DecUInt32& response)
    {
        uint32_t result = Core::ERROR_NONE;

        response = GetSessionsCount();

        return result;
    }

    // Property: 
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Get details of each active SSH client sessions managed by Dropbear Service.
    uint32_t SecureShellServer::endpoint_getactivesessionsinfo(Core::JSON::ArrayType<SessioninfoResultData>& response)
    {
        uint32_t result = Core::ERROR_NONE;

        result = GetSessionsInfo(response);

        return result;
    }

    // Property: 
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Close a SSH client session.
    uint32_t SecureShellServer::endpoint_closeclientsession(const JsonData::SecureShellServer::CloseclientsessionParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if(params.ClientPid.IsSet() == true) {
            result = CloseClientSession(params.ClientPid.Value());
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        TRACE(Trace::Information, (_T("closing client session with pid: %s"), params.ClientPid.Value()));

        return result;
    }


} // namespace Plugin

} // namespace WPEFramework

