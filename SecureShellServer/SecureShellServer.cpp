#include "SecureShellServer.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(SecureShellServer, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<SecureShellServer::Data>> jsonBodyDataFactory(2);

    const string SecureShellServer::Initialize(PluginHost::IShell* service)
    {
        //ASSERT(service != nullptr);

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        Config config;
        config.FromString(service->ConfigLine());
        _InputParameters = config.InputParameters.Value();

        fprintf(stderr, "Input=%s\n", _InputParameters.c_str());
        _implementation.StartService(_InputParameters);

        return string();
    }

    void SecureShellServer::Deinitialize(PluginHost::IShell* service)
    {
        //ASSERT(_service == service);
        // Deinitialize what we initialized..

        _implementation.StopService();
	//_implementation = nullptr;
    }

    string SecureShellServer::Information() const
    {
	// No additional info to report.
        return string();
    }

    void SecureShellServer::Inbound(Web::Request& request)
    {
    }

    // GET	<- GetSessionsInfo
    // GET	<- GetSessionsCount
    // DELETE	<-CloseClientSession

    Core::ProxyType<Web::Response> SecureShellServer::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::ProxyType<Web::JSONBodyType<SecureShellServer::Data>> response(jsonBodyDataFactory.Element());

        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported request for the [SecureShellServer] service.");

        if ((request.Verb == Web::Request::HTTP_GET && index.Next())) {
		
		if (index.Current().Text() == "GetSessionsCount") {
			// GET	<- GetSessionsInfo
    			response->ActiveCount = SecureShellServer::GetSessionsCount();
			result->ErrorCode = Web::STATUS_OK;
			result->ContentType = Web::MIMETypes::MIME_JSON;
			result->Body(response);
		} else if (index.Current().Text() == "GetSessionsInfo") {
			// GET	<- GetSessionsCount
    			uint32_t status = SecureShellServer::GetSessionsInfo(response->SessionInfo);
                	if (status != Core::ERROR_NONE) {
	                    result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
        	            result->Message = _T("Dropbear GetSessionsInfo failed for ");
                	} else {
				result->ErrorCode = Web::STATUS_OK;
				result->ContentType = Web::MIMETypes::MIME_JSON;
				result->Body(response);
			}
		} else {
			result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
			result->Message = _T("Unavailable method");
		}

	} else if ((request.Verb == Web::Request::HTTP_DELETE && index.Next())) {
		
		if (index.Current().Text() == "CloseClientSession") {
	    		// DELETE	<-CloseClientSession
			std::string clientpid;

	                Core::URL::KeyValue options(request.Query.Value());
	
        	        if (options.Exists(_T("clientpid"), true) == true) {
		                clientpid = options[_T("clientpid")].Text();
    				uint32_t status = SecureShellServer::CloseClientSession(clientpid);
	                	if (status != Core::ERROR_NONE) {
		                    result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
        		            result->Message = _T("Dropbear CloseClientSession failed for ");
                		} else {
					result->ErrorCode = Web::STATUS_OK;
					result->ContentType = Web::MIMETypes::MIME_JSON;
					result->Body(response);
				}
			}
		} else {
			result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
			result->Message = _T("Unavailable method");
		}
	} else {
		// Not supported HTTP Method
		result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
		result->Message = _T("Unavailable method");
	}
		
        return result;
    }

    uint32_t SecureShellServer::GetSessionsInfo(Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoResultData>& sessioninfo)
    {
        return _implementation.GetSessionsInfo(sessioninfo);
    }

    uint32_t SecureShellServer::GetSessionsCount()
    {
        return _implementation.GetSessionsCount();
    }

    uint32_t SecureShellServer::CloseClientSession(const std::string& clientpid)
    {
        return _implementation.CloseClientSession(clientpid);
    }

} // namespace Plugin
} // namespace WPEFramework
