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

#include "RtspClient.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(RtspClient, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<RtspClient::Data>> jsonDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<RtspClient::Data>> jsonBodyDataFactory(2);

    /* virtual */ const string RtspClient::Initialize(PluginHost::IShell* service)
    {
        Config config;
        string message;

        ASSERT(_service == nullptr);
        ASSERT(_implementation == nullptr);

        _connectionId = 0;
        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        config.FromString(_service->ConfigLine());

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notification);

        _implementation = _service->Root<Exchange::IRtspClient>(_connectionId, 2000, _T("RtspClientImplementation"));

        if (_implementation == nullptr) {
            message = _T("RtspClient could not be instantiated.");
            _service->Unregister(&_notification);
            _service = nullptr;
        } else {
            _implementation->Configure(_service);
            TRACE(Trace::Information, (_T("RtspClient Plugin initialized %p"), _implementation));
        }

        return message;
    }

    /*virtual*/ void RtspClient::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_implementation != nullptr);

        _service->Unregister(&_notification);
        
        _implementation->Release(); 

        if(_connectionId != 0){
            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            // The process can disappear in the meantime...
            if (connection != nullptr) {
                // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                connection->Terminate();
                connection->Release();
            }
        }

        // Deinitialize what we initialized..
        _implementation = nullptr;
        _service = nullptr;
    }

    /* virtual */ string RtspClient::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }

    /* virtual */ void RtspClient::Inbound(WPEFramework::Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_POST)
            request.Body(jsonBodyDataFactory.Element());
    }

    /* virtual */ Core::ProxyType<Web::Response> RtspClient::Process(const WPEFramework::Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (_T("Received RtspClient request")));

        uint32_t rc = 0;
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
        result->ErrorCode = Web::STATUS_BAD_REQUEST;

        if ((request.Verb == Web::Request::HTTP_GET) && ((index.Next()) && (index.Next()))) {
            if (index.Current().Text() == _T("Get")) {
                Core::ProxyType<Web::JSONBodyType<Data>> data(jsonDataFactory.Element());
                data->Str = _implementation->Get("");
                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(data);
                result->ErrorCode = Web::STATUS_OK;
            }
        } else if ((request.Verb == Web::Request::HTTP_POST) && ((index.Next()) && (index.Next()))) {
            if (index.Current().Text() == _T("Setup")) {
                string assetId = request.Body<const Data>()->AssetId.Value();
                uint32_t position = request.Body<const Data>()->Position.Value();
                rc = _implementation->Setup(assetId, position);
                result->ErrorCode = (rc == 0) ? Web::STATUS_OK : Web::STATUS_INTERNAL_SERVER_ERROR;
            } else if (index.Current().Text() == _T("Teardown")) {
                rc = _implementation->Teardown();
                result->ErrorCode = (rc == 0) ? Web::STATUS_OK : Web::STATUS_INTERNAL_SERVER_ERROR;
            } else if (index.Current().Text() == _T("Play")) {
                int32_t scale = request.Body<const Data>()->Scale.Value();
                uint32_t position = request.Body<const Data>()->Position.Value();
                rc = _implementation->Play(scale, position);
                result->ErrorCode = (rc == 0) ? Web::STATUS_OK : Web::STATUS_INTERNAL_SERVER_ERROR;
            } else if (index.Current().Text() == _T("Set")) {
                std::string str = request.Body<const Data>()->Str.Value();
                _implementation->Set(str, "");
                result->ErrorCode = Web::STATUS_OK;
            }
        }
        return result;
    }

    void RtspClient::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {

            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }
}
} //namespace WPEFramework::Plugin
