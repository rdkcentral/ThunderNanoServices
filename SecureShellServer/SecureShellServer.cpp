#include "SecureShellServer.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(SecureShellServer, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<SecureShellServer::Data>> jsonBodyDataFactory(2);

    const string SecureShellServer::Initialize(PluginHost::IShell* service)
    {
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        Config config;
        config.FromString(service->ConfigLine());
        _InputParameters = config.InputParameters.Value();

        fprintf(stderr, "Input=%s\n", _InputParameters.c_str());

        TRACE(Trace::Information, (_T("Starting Dropbear Service with options as: %s"), _InputParameters.c_str()));
        // TODO: Check the return value and based on that change result
        activate_dropbear(const_cast<char*>(_InputParameters.c_str()));

        return string();
    }

    void SecureShellServer::Deinitialize(PluginHost::IShell* service)
    {
        // Deinitialize what we initialized..
        TRACE(Trace::Information, (_T("Stoping Dropbear Service")));
        deactivate_dropbear(); //TODO: Check the return value and based on that change result
    }

    string SecureShellServer::Information() const
    {
        // No additional info to report.
        return string();
    }

    void SecureShellServer::Inbound(Web::Request& request)
    {
    }

    // GET      <- GetSessionsInfo
    // GET      <- GetSessionsCount
    // DELETE   <-CloseClientSession
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
                        // GET  <- GetSessionsInfo
                        response->ActiveCount = SecureShellServer::GetSessionsCount();
                        result->ErrorCode = Web::STATUS_OK;
                        result->ContentType = Web::MIMETypes::MIME_JSON;
                        result->Message = _T("Success");
                        result->Body(response);
                } else if (index.Current().Text() == "GetSessionsInfo") {
                        // GET  <- GetSessionsCount
                        uint32_t status = SecureShellServer::GetSessionsInfo(response->SessionInfo);
                        if (status != Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                            result->Message = _T("Dropbear GetSessionsInfo failed for ");
                        } else {
                                result->ErrorCode = Web::STATUS_OK;
                                result->ContentType = Web::MIMETypes::MIME_JSON;
                                result->Message = _T("Success");
                                result->Body(response);
                        }
                } else {
                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        result->Message = _T("Unavailable method");
                }

        } else if ((request.Verb == Web::Request::HTTP_DELETE && index.Next())) {

                if (index.Current().Text() == "CloseClientSession") {
                        // DELETE       <-CloseClientSession
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
                                        result->Message = _T("Success");
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
            uint32_t result = Core::ERROR_NONE;

            _adminLock.Lock();
            int32_t count = get_active_sessions_count();
            TRACE(Trace::Information, (_T("Get details of (%d)active SSH client sessions managed by Dropbear service"), count));

            struct client_info *info = static_cast<struct client_info*>(::malloc(sizeof(struct client_info) * count));

            get_active_sessions_info(info, count);

            _clients.clear(); // Clear all elements before we re-populate it with new elements

            for(int32_t i=0; i<count; i++)
            {
                TRACE(Trace::Information, (_T("Count: %d index: %d pid: %d IP: %s Timestamp: %s"),
                                            count, i, info[i].pid, info[i].ipaddress, info[i].timestamp));

                JsonData::SecureShellServer::SessioninfoResultData newElement;

                newElement.IpAddress = info[i].ipaddress;
                newElement.Pid = std::to_string(info[i].pid);
                newElement.TimeStamp = info[i].timestamp;

                JsonData::SecureShellServer::SessioninfoResultData& element(sessioninfo.Add(newElement));

                _clients.push_back(Core::Service<SecureShellServer::ClientImpl>::Create<ClientImpl>(info[i].ipaddress,
                                        info[i].timestamp, std::to_string(info[i].pid)));
            }
            ::free(info);
            _adminLock.Unlock();

            return result;
    }

    uint32_t SecureShellServer::GetSessionsCount()
    {
        uint32_t count = get_active_sessions_count();
        TRACE(Trace::Information, (_T("Get total number of active SSH client sessions managed by Dropbear service: %d"), count));

        return count;
    }

    uint32_t SecureShellServer::CloseClientSession(const std::string& clientpid)
    {
        uint32_t result = Core::ERROR_NONE;

        TRACE(Trace::Information, (_T("closing client session with PID1: %s"), clientpid.c_str()));

        _adminLock.Lock();

        std::list<ClientImpl*>::iterator index = _clients.begin();
        std::list<ClientImpl*>::iterator end = _clients.end();

        for (; index != end; ++index)
        {
            TRACE(Trace::Information, (_T("closing client session with PID2: %s"), (*index)->RemoteId().c_str()));
            if((*index)->RemoteId() == clientpid)
            {
                TRACE(Trace::Information, (_T("closing client session with PID3: %s"), clientpid.c_str()));
                result = close_client_session(std::stoi(clientpid));
                (*index)->Release();
                _clients.erase(index);
                 break;
            }
         }

         _adminLock.Unlock();

         return result;
    }

} // namespace Plugin
} // namespace WPEFramework
