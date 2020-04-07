/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
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

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::ProxyType<Web::JSONBodyType<SecureShellServer::Data>> response(jsonBodyDataFactory.Element());

        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported request for the [SecureShellServer] service.");

        if ((request.Verb == Web::Request::HTTP_GET && index.Next())) {

                if (index.Current().Text() == "GetSessionsCount") {
                        // GET  <- GetSessionsCount
			Exchange::ISecureShellServer::IClient::IIterator* client_iter = SessionsInfo();
                        response->ActiveCount = SecureShellServer::GetSessionsCount(client_iter);
                        result->ErrorCode = Web::STATUS_OK;
                        result->ContentType = Web::MIMETypes::MIME_JSON;
                        result->Message = _T("Success");
                        result->Body(response);
                } else if (index.Current().Text() == "GetSessionsInfo") {
                        // GET  <- GetSessionsInfo
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
		        ISecureShellServer::IClient* client = Core::Service<ClientImpl>::Create<ISecureShellServer::IClient>(
							request.Body<const JsonData::SecureShellServer::SessioninfoResultData>()->IpAddress.Value(),
							request.Body<const JsonData::SecureShellServer::SessioninfoResultData>()->TimeStamp.Value(),
							request.Body<const JsonData::SecureShellServer::SessioninfoResultData>()->Pid.Value());
                        uint32_t status = SecureShellServer::CloseClientSession(client);
                        if (status != Core::ERROR_NONE) {
                               result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                               result->Message = _T("Dropbear CloseClientSession failed for ");
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
        } else {
                // Not supported HTTP Method
                result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                result->Message = _T("Unavailable method");
        }

        return result;
    }

    Exchange::ISecureShellServer::IClient::IIterator* SecureShellServer::SessionsInfo()
    {
        std::list<ClientImpl*> local_clients;
	Exchange::ISecureShellServer::IClient::IIterator* iter=nullptr;

        int32_t count = get_active_sessions_count();
        TRACE(Trace::Information, (_T("Get details of (%d)active SSH client sessions managed by Dropbear service"), count));

	if (count>0)
	{
            struct client_info *info = static_cast<struct client_info*>(::malloc(sizeof(struct client_info) * count));

            get_active_sessions_info(info, count);

            for(int32_t i=0; i<count; i++)
            {
                TRACE(Trace::Information, (_T("Count: %d index: %d pid: %d IP: %s Timestamp: %s"),
                                            count, i, info[i].pid, info[i].ipaddress, info[i].timestamp));

                local_clients.push_back(Core::Service<SecureShellServer::ClientImpl>::Create<ClientImpl>(info[i].ipaddress,
                                        info[i].timestamp, std::to_string(info[i].pid)));
            }
            ::free(info);

	    if (local_clients.empty() == false) {
                iter = Core::Service<ClientImpl::IteratorImpl>::Create<ISecureShellServer::IClient::IIterator>(local_clients);
                TRACE(Trace::Information, (_T("Currently total %d sessions are active"), iter->Count()));
	    }
            local_clients.clear(); // Clear all elements before we re-populate it with new elements
	}

	return iter;
     }

    uint32_t SecureShellServer::GetSessionsInfo(Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoResultData>& sessioninfo)
    {
        uint32_t result = Core::ERROR_NONE;
	Exchange::ISecureShellServer::IClient::IIterator* iter = SessionsInfo();

	if (iter != nullptr)
	{
	    uint32_t index = 0;

	    iter->Reset();
            while(iter->Next())
            {
                TRACE(Trace::Information, (_T("Count: %d index:%d pid: %s IP: %s Timestamp: %s"),
                                         iter->Count(), index++, iter->Current()->RemoteId().c_str(), iter->Current()->IpAddress().c_str(),
					 iter->Current()->TimeStamp().c_str()));

                JsonData::SecureShellServer::SessioninfoResultData newElement;

                newElement.IpAddress = iter->Current()->IpAddress();
                newElement.Pid = iter->Current()->RemoteId();
                newElement.TimeStamp = iter->Current()->TimeStamp();

                JsonData::SecureShellServer::SessioninfoResultData& element(sessioninfo.Add(newElement));
            }
	    iter->Release();
	}
        return result;
    }

    uint32_t SecureShellServer::GetSessionsCount(Exchange::ISecureShellServer::IClient::IIterator* iter)
    {
        uint32_t count = iter->Count();
        TRACE(Trace::Information, (_T("Get total number of active SSH client sessions managed by Dropbear service: %d"), count));

        return count;
    }

    uint32_t SecureShellServer::CloseClientSession(Exchange::ISecureShellServer::IClient* client)
    {
        uint32_t result = Core::ERROR_NONE;

        TRACE(Trace::Information, (_T("closing client session with PID1: %s"), client->RemoteId().c_str()));

	client->Close();

        return result;
    }

    /*virtual*/ Exchange::ISecureShellServer::IClient::IIterator* SecureShellServer::Clients()
    {
        return SecureShellServer::SessionsInfo();
    }

} // namespace Plugin
} // namespace WPEFramework
