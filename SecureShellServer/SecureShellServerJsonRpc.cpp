// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#include <interfaces/json/JsonData_SecureShellServer.h>
#include "SecureShellServer.h"
#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    using namespace JsonData::SecureShellServer;

    // Registration
    void SecureShellServer::RegisterAll()
    {
	Register<void, Core::JSON::DecUInt32>(_T("getactivesessionscount"), &SecureShellServer::endpoint_getactivesessionscount, this);
	Register<void, Core::JSON::ArrayType<SessioninfoResultData>>(_T("getactivesessionsinfo"), &SecureShellServer::endpoint_getactivesessionsinfo, this);
	Register<SessioninfoResultData,void>(_T("closeclientsession"), &SecureShellServer::endpoint_closeclientsession, this);
    }

    void SecureShellServer::UnregisterAll()
    {
        Unregister(_T("closeclientsession"));
        Unregister(_T("getactivesessionsinfo"));
        Unregister(_T("getactivesessionscount"));
    }

    // API implementation
    
    // Property: 
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Get total active SSH client sessions managed by Dropbear Service.
    uint32_t SecureShellServer::endpoint_getactivesessionscount(Core::JSON::DecUInt32& response)
    {
        uint32_t result = Core::ERROR_NONE;

	Exchange::ISecureShellServer::IClient::IIterator* iter = SessionsInfo();
	if (iter != nullptr)
	{
            response = GetSessionsCount(iter);

            iter->Reset();
            iter->Next();
            iter->Release();
	}

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

        result = SecureShellServer::GetSessionsInfo(response);

        return result;
    }

    // Property: 
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    // Close a SSH client session.
    uint32_t SecureShellServer::endpoint_closeclientsession(const JsonData::SecureShellServer::SessioninfoResultData& params)
    {
        uint32_t result = Core::ERROR_NONE;
	ClientImpl* client =
		Core::Service<SecureShellServer::ClientImpl>::Create<ClientImpl>(params.IpAddress.Value(), params.TimeStamp.Value(), params.Pid.Value());

        if(params.Pid.IsSet() == true) {
            TRACE(Trace::Information, (_T("closing client session with pid: %s"), params.Pid.Value()));
            result = CloseClientSession(client);
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }


        return result;
    }

} // namespace Plugin
} // namespace WPEFramework

