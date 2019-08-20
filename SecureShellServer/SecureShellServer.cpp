#include "SecureShellServer.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(SecureShellServer, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<SecureShellServer::Data>> jsonBodyDataFactory(2);

    const string SecureShellServer::Initialize(PluginHost::IShell* service)
    {
         ASSERT(service != nullptr);

        _service = service;
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        Config config;
        config.FromString(_service->ConfigLine());
        _InputParameters = config.InputParameters.Value();

        _implementation.StartService(_InputParameters);

        return string();
    }

    void SecureShellServer::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        _implementation.StopService();

        // Deinitialize what we initialized..
        _service = nullptr;
    }

    string SecureShellServer::Information() const
    {
	// No additional info to report.
        return string();
    }

    void SecureShellServer::Inbound(Web::Request& request)
    {
    }

    // POST/PUT	<- StartService
    // POST/PUT	<- StopService
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

        if ((request.Verb == Web::Request::HTTP_PUT || (request.Verb == Web::Request::HTTP_POST && index.Next()))) {

            if (index.Current().Text() == "StartService") {
                string inputparameters;

                Core::URL::KeyValue options(request.Query.Value());

		    std::string inputParameters;
        	    TRACE(Trace::Information, (_T("Entered into StartService method")));

                    inputparameters = options[_T("params")].Text();

                    Core::URL::Decode(inputparameters.c_str(), inputparameters.length(), const_cast<char*>(inputParameters.data()), inputParameters.size());

                    inputparameters = inputParameters.data();

                    uint32_t status = StartService(inputparameters);

                    if (status != Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        result->Message = _T("Dropbear StartService failed for:") + inputparameters;
                    } else {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = "OK";
                    }
            }
	    else if (index.Current().Text() == "StopService") {
        	TRACE(Trace::Information, (_T("Entered into StopService method")));
                uint32_t status = StopService();
                if (status != Core::ERROR_NONE) {
                    result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                    result->Message = _T("Dropbear StopService failed for ");
                } else {
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = "OK";
                }
            }
	    else {
                result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                result->Message = _T("Unavailable method");
            }
        } else if ((request.Verb == Web::Request::HTTP_GET && index.Next())) {
		
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

    uint32_t SecureShellServer::StartService(const std::string& params)
    {
        return _implemetation.StartService(params);
    }

    uint32_t SecureShellServer::StopService()
    {
        return _implemetation.StopService();
    }

    uint32_t SecureShellServer::GetSessionsInfo(Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoResultData>& sessioninfo)
    {
        return _implemetation.GetSessionsInfo(sessioninfo);
    }

    uint32_t SecureShellServer::GetSessionsCount()
    {
        return _implemetation.GetSessionsCount();
    }

    uint32_t SecureShellServer::CloseClientSession(const std::string& clientpid)
    {
        return _implemetation.CloseClientSession(clientpid);
    }

} // namespace Plugin
} // namespace WPEFramework
