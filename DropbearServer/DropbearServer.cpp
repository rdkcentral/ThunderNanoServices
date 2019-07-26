#include "DropbearServer.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DropbearServer, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<DropbearServer::Data>> jsonBodyDataFactory(2);

    const string DropbearServer::Initialize(PluginHost::IShell* service)
    {
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        return string();
    }

    void DropbearServer::Deinitialize(PluginHost::IShell* service)
    {
    }

    string DropbearServer::Information() const
    {
	// No additional info to report.
        return string();
    }

    void DropbearServer::Inbound(Web::Request& request)
    {
    }

    // POST/PUT	<- StartService
    // POST/PUT	<- StopService
    // GET	<- GetTotalSessions
    // GET	<- GetSessionsInfo
    // GET	<- GetSessionsCount
    // DELETE	<-CloseClientSession

    Core::ProxyType<Web::Response> DropbearServer::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::ProxyType<Web::JSONBodyType<DropbearServer::Data>> response(jsonBodyDataFactory.Element());

        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported request for the [DropbearServer] service.");

        if ((request.Verb == Web::Request::HTTP_PUT || (request.Verb == Web::Request::HTTP_POST && index.Next()))) {

            if (index.Current().Text() == "StartService") {
                string sshport;
                string hostkeys;
                string portflag;

                Core::URL::KeyValue options(request.Query.Value());

                if (options.Exists(_T("port"), true) == true) {
                    constexpr int kPortNameMaxLength = 255;
                    std::array<char, kPortNameMaxLength> portNumber {0};
		    std::string hostKeys;
		    std::string portFlag;

                    sshport = options[_T("port")].Text();
                    hostkeys = options[_T("hostkeys")].Text();
                    portflag = options[_T("portflag")].Text();

                    Core::URL::Decode(sshport.c_str(), sshport.length(), const_cast<char*>(portNumber.data()), portNumber.size());
                    Core::URL::Decode(hostkeys.c_str(), hostkeys.length(), const_cast<char*>(hostKeys.data()), hostKeys.size());
                    Core::URL::Decode(portflag.c_str(), portflag.length(), const_cast<char*>(portFlag.data()), portFlag.size());

                    sshport = portNumber.data();
                    hostkeys = hostKeys.data();
                    portflag = portFlag.data();
                }
                if (!sshport.empty()) {
        	    TRACE(Trace::Information, (_T("Entered into StartService method")));
                    uint32_t status = StartService(hostkeys, portflag, sshport);
                    if (status != Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        result->Message = _T("Dropbear StartService failed for:") + sshport + _T("hostkeys:") + hostkeys + _T("portflag:") + portflag;
                    } else {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = "OK";
                    }
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("No port number given to Dropbear StartService.");
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
		
		if (index.Current().Text() == "GetTotalSessions") {
			// GET	<- GetTotalSessions
    			response->TotalCount = DropbearServer::GetTotalSessions();
			result->ErrorCode = Web::STATUS_OK;
			result->ContentType = Web::MIMETypes::MIME_JSON;
			result->Body(response);
		} else if (index.Current().Text() == "GetSessionsCount") {
			// GET	<- GetSessionsInfo
    			response->ActiveCount = DropbearServer::GetSessionsCount();
			result->ErrorCode = Web::STATUS_OK;
			result->ContentType = Web::MIMETypes::MIME_JSON;
			result->Body(response);
		} else if (index.Current().Text() == "GetSessionsInfo") {
			// GET	<- GetSessionsCount
    			uint32_t status = DropbearServer::GetSessionsInfo(response->SessionInfo);
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
    				uint32_t status = DropbearServer::CloseClientSession(clientpid);
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

    uint32_t DropbearServer::StartService(const std::string& hostkeys, const std::string& portflag, const std::string& port)
    {
        return _implemetation.StartService(hostkeys, portflag, port);
    }

    uint32_t DropbearServer::StopService()
    {
        return _implemetation.StopService();
    }

    uint32_t DropbearServer::GetTotalSessions()
    {
        return _implemetation.GetTotalSessions();
    }

    uint32_t DropbearServer::GetSessionsInfo(Core::JSON::ArrayType<JsonData::DropbearServer::SessioninfoResultData>& sessioninfo)
    {
        return _implemetation.GetSessionsInfo(sessioninfo);
    }

    uint32_t DropbearServer::GetSessionsCount()
    {
        return _implemetation.GetSessionsCount();
    }

    uint32_t DropbearServer::CloseClientSession(const std::string& clientpid)
    {
        return _implemetation.CloseClientSession(clientpid);
    }

} // namespace Plugin
} // namespace WPEFramework
