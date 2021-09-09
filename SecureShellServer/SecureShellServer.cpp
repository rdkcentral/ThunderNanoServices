/*
 *If not stated otherwise in this file or this component's LICENSE file the
 *following copyright and licenses apply:
 *
 *Copyright 2020 Metrological
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */
#include "SecureShellServer.h"

namespace WPEFramework
{
namespace Plugin
{

        SERVICE_REGISTRATION(SecureShellServer, 1, 0);

        static Core::ProxyPoolType<Web::JSONBodyType<SecureShellServer::Data>> jsonBodyDataFactory(2);

        const string SecureShellServer::Initialize(PluginHost::IShell *service)
        {
                ASSERT(service != nullptr);
                ASSERT(_secureShellServer == nullptr);

                _skipURL = static_cast<uint8_t> (service->WebPrefix().length());
                string message;
                Config config;
                config.FromString(service->ConfigLine());
                _InputParameters = config.InputParameters.Value();

                _secureShellServer = service->Root<Exchange::ISecureShellServer > (_connectionId, 2000, _T("SecureShellImplementation"));

                TRACE(Trace::Information, (_T("Starting SecureShellServer Service with options as: %s"), _InputParameters.c_str()));

                if (_secureShellServer != nullptr)
                {
                        if (_secureShellServer->Activate_dropbear(const_cast<char*> (_InputParameters.c_str())) != 0)
                        {
                                message = _T("SecureShellServer::Initialize failed to start SecureShellServer \n");
                        } else {
                                TRACE(Trace::Information, (_T("SecureShellServer successfully started")));
                        }
                } else {
                        _secureShellServer = nullptr;
                        SYSLOG(Logging::Startup, (_T("SecureShellServer could not be instantiated")));
                }

                return message;
        }

        void SecureShellServer::Deinitialize(PluginHost::IShell *service)
        {
                //TODO make sure drop bear closes all open sessions
                ASSERT(service != nullptr);
                ASSERT(_secureShellServer != nullptr);

                if (_secureShellServer != nullptr)
                {
                        if (_secureShellServer->Deactivate_dropbear() == 0)
                        {
                                TRACE(Trace::Information, (_T("SecureShellServer successfully stopped")));
                        } else  {
                                TRACE(Trace::Information, (_T("SecureShellServer COULD NOT BE stopped!")));
                        }
                        _secureShellServer->Release();
                }

                TRACE(Trace::Information, (_T("SecureShellServer Deinitialized")));
        }

        string SecureShellServer::Information() const
        {
                // No additional info to report.
                return string();
        }

        void SecureShellServer::Inbound(Web::Request &request)
        {
                //TODO finish its implementation
        }

        // GET     <- GetSessionsInfo
        // GET     <- GetSessionsCount
        // DELETE  <-CloseClientSession
        Core::ProxyType<Web::Response > SecureShellServer::Process(const Web::Request &request)
        {
                ASSERT(_skipURL <= request.Path.length());
                ASSERT(_secureShellServer != nullptr);

                Core::ProxyType<Web::Response > result(PluginHost::IFactories::Instance().Response());
                Core::ProxyType<Web::JSONBodyType<SecureShellServer::Data>> response(jsonBodyDataFactory.Element());

                Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
                
                // Skip the first item, which will be a '/'
                index.Next();

                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Unsupported request or syntax error for SecureShellServer service.");

                if ((request.Verb == Web::Request::HTTP_GET && index.Next()))
                {
                        if (index.Current().Text() == _T("SessionsCount"))
                        {   
                                // GET <- GetSessionsCount
                                TRACE(Trace::Information, (_T("SecureShellServer received SessionsCount HTTP Get request ")));
                                response->ActiveCount = _secureShellServer->GetActiveSessionsCount();
                                result->ErrorCode = Web::STATUS_OK;
                                result->ContentType = Web::MIMETypes::MIME_JSON;
                                result->Message = _T("Success");
                                result->Body(response);
                        }
                        else if (index.Current().Text() == _T("SessionsInfo"))
                        {
                                // GET <- GetSessionsInfo
                                TRACE(Trace::Information, (_T("SecureShellServer received SessionsInfo HTTP Get request ")));
                                std::list<JsonData::SecureShellServer::SessioninfoData> allSessionsInfo;
                                uint32_t status = GetActiveSessionsInfo(allSessionsInfo);
                                if (status != Core::ERROR_NONE)
                                {
                                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                                        result->Message = _T("SecureShellServer HTTP Get request SessionsInfo failed! ");
                                }
                                else
                                {
                                        //Fill in the response
                                        for(auto& elementSession:allSessionsInfo ){
                                                response->SessionInfo.Add(elementSession);

                                        }
                                        result->ErrorCode = Web::STATUS_OK;
                                        result->ContentType = Web::MIMETypes::MIME_JSON;
                                        result->Message = _T("Success");
                                        result->Body(response);
                                }
                        }
                        else
                        {
                                result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                                result->Message = _T("SecureShellServer Unavailable method");
                        }
                }
                else if ((request.Verb == Web::Request::HTTP_DELETE ))
                {
                        if(index.IsValid() == true && index.Next() == true)
                        {
                                // DELETE      <-CloseClientSessio	
                                //example DELETE http://192.168.2.2:80/Service/SecureShellServer/454	
                                TRACE(Trace::Information, (_T("SecureShellServer received CloseClientSession HTTP Delete request ")));
                                uint32_t pidUint;
                                Core::FromString(index.Current().Text(),pidUint);

                                uint32_t status = _secureShellServer->CloseClientSession(pidUint);
                                if (status != Core::ERROR_NONE)
                                {
                                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                                        result->Message = _T("SecureShellServer CloseClientSession failed ! ");
                                }
                                else
                                {
                                        result->ErrorCode = Web::STATUS_OK;
                                        result->ContentType = Web::MIMETypes::MIME_JSON;
                                        result->Message = _T("Success");
                                        result->Body(response);
                                }
                        }
                        else
                        {
                                result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                                result->Message = _T("SecureShellServer Unavailable method");
                        }
                }
                else
                {
                        // Not supported HTTP Method
                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        result->Message = _T("SecureShellServer Unavailable method");
                }

                return result;
        }
        uint32_t SecureShellServer::GetActiveSessionsInfo(std::list<JsonData::SecureShellServer::SessioninfoData>& activesSessions)
        {
                ASSERT(_secureShellServer != nullptr);
                uint32_t result = Core::ERROR_NONE;

               if(_secureShellServer!= nullptr) {
 
                        auto iter = _secureShellServer->ActiveClientsIter();
                        
                        if (iter != nullptr)
                        {
                                uint32_t index = 0;

                                iter->Reset();
                                while(iter->Next())
                                {
                                        TRACE(Trace::Information, (_T("Count: %d index:%d pid: %u IP: %s Timestamp: %s"),
                                                                iter->Count(), index++, iter->Current()->RemoteId(), iter->Current()->IpAddress().c_str(),
                                                                iter->Current()->TimeStamp().c_str()));

                                        JsonData::SecureShellServer::SessioninfoData newElement;

                                        newElement.Ipaddress = iter->Current()->IpAddress();
                                        newElement.Pid = iter->Current()->RemoteId();
                                        newElement.Timestamp = iter->Current()->TimeStamp();

                                        activesSessions.push_back(newElement);

                                }
                                iter->Release();
                        }

               } else {
                       result = Core::ERROR_GENERAL;
               }


                return result;
        }

}	// namespace Plugin
}	// namespace WPEFramework