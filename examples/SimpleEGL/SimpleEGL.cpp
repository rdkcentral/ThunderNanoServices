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
 
#include "SimpleEGL.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(SimpleEGL, 1, 0);

    static Core::ProxyPoolType<SimpleEGL::Data> jsonDataFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<SimpleEGL::Data>> jsonBodyDataFactory(4);
    static Core::ProxyPoolType<Web::TextBody> textFactory(4);

    static const char SampleData[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789=!";

    /* encapsulated class Thread  */
    /* virtual */ const string SimpleEGL::Initialize(PluginHost::IShell* service)
    {
        string message;
        Config config;

        ASSERT(_browser == nullptr);
        ASSERT(_service == nullptr);

        _connectionId = 0;
        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        _service->EnableWebServer(_T("UI"), EMPTY_STRING);
        _service->Register(static_cast<RPC::IRemoteConnection::INotification*>(_notification));

        config.FromString(_service->ConfigLine());

        _browser = service->Root<Exchange::IBrowser>(_connectionId, Core::infinite, _T("SimpleEGLImplementation"));

        if (_browser == nullptr) {
            _service->Unregister(static_cast<RPC::IRemoteConnection::INotification*>(_notification));
            _service = nullptr;

            ConnectionTermination(_connectionId);
            message = _T("SimpleEGL could not be instantiated.");
        } else {
            _browser->Register(_notification);

            PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());

            if (stateControl != nullptr) {

                stateControl->Configure(_service);
                stateControl->Register(_notification);
                stateControl->Release();

                PluginHost::IPlugin::INotification* sink = _browser->QueryInterface<PluginHost::IPlugin::INotification>();

                if (sink != nullptr) {
                    _service->Register(sink);
                    sink->Release();

                    RPC::IRemoteConnection* remoteConnection = _service->RemoteConnection(_connectionId);
                    if (remoteConnection != nullptr) {
                        remoteConnection->Release();
                    }
                    else {
                        message = _T("Failed to instantiate the Server.");
                    }
                }
            }
        }

        return message;
    }

    /* virtual */ void SimpleEGL::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == _service);
        ASSERT(_browser != nullptr);

        fprintf(stderr, "================ Deinitializing the SimpleEGL forcefully =================\n"); fflush(stderr);

        _service->DisableWebServer();
        _service->Unregister(static_cast<RPC::IRemoteConnection::INotification*>(_notification));
        _browser->Unregister(_notification);

        PluginHost::IPlugin::INotification* sink = _browser->QueryInterface<PluginHost::IPlugin::INotification>();

        if (sink != nullptr) {
            _service->Unregister(sink);
            sink->Release();
        }

        PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());

        if (stateControl != nullptr) {

            // No longer a need for the notfications..
            stateControl->Unregister(_notification);
            stateControl->Release();
        }

        // Stop processing of the browser:
        _browser->Release();

        if(_connectionId != 0){
            ConnectionTermination(_connectionId);
        }

        _browser = nullptr;
        _service = nullptr;
        fprintf(stderr, "================ Deinitialized the SimpleEGL forcefully ==================\n"); fflush(stderr);
    }

    /* virtual */ string SimpleEGL::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }
    /* virtual */ bool SimpleEGL::Attach(PluginHost::Channel& channel)
    {
        if (_subscriber == nullptr) {
            _subscriber = &channel;
            return (true);
        }
        return (false);
    }

    /* virtual */ void SimpleEGL::Detach(PluginHost::Channel& channel)
    {
        if ((_subscriber != nullptr) && (_subscriber == &channel)) {
            _subscriber = nullptr;
        }
    }

    /* virtual */ void SimpleEGL::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_PUT) {
            request.Body(jsonBodyDataFactory.Element());
        } else if (request.Verb == Web::Request::HTTP_POST) {
            request.Body(textFactory.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> SimpleEGL::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (string(_T("Received request"))));

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length() - _skipURL)), false, '/');

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = "Unknown error";

        if (_browser != nullptr) {

            PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());

            ASSERT(stateControl != nullptr);

            if (request.Verb == Web::Request::HTTP_GET) {
                PluginHost::IStateControl::state currentState = stateControl->State();
                Core::ProxyType<Web::JSONBodyType<SimpleEGL::Data>> body(jsonBodyDataFactory.Element());
                body->URL = _browser->GetURL();
                body->FPS = _browser->GetFPS();
                body->Suspended = (currentState == PluginHost::IStateControl::SUSPENDED);
                body->Hidden = _hidden;
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";
                result->Body<Web::JSONBodyType<SimpleEGL::Data>>(body);
            } else if ((request.Verb == Web::Request::HTTP_POST) && (index.Next() == true) && (index.Next() == true)) {
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";

                // We might be receiving a plugin download request.
                if (index.Remainder() == _T("Suspend")) {
                    stateControl->Request(PluginHost::IStateControl::SUSPEND);
                } else if (index.Remainder() == _T("Resume")) {
                    stateControl->Request(PluginHost::IStateControl::RESUME);
                } else if (index.Remainder() == _T("Hide")) {
                    _browser->Hide(true);
                } else if (index.Remainder() == _T("Show")) {
                    _browser->Hide(false);
                } else if (index.Remainder() == _T("Notify4K")) {
                    string message;
                    for (uint32_t teller = 0; teller < ((4 * 1024) + 64); teller++) {
                        message += SampleData[teller % 64];
                    }
                    _service->Notify(message);
                } else if ((index.Remainder() == _T("JSONPop")) && (_subscriber != nullptr)) {
                    Core::ProxyType<Data> info(jsonDataFactory.Element());

                    info->URL = _T("ws://<My IP>/JSONPop");
                    _subscriber->Submit(Core::proxy_cast<Core::JSON::IElement>(info));
                } else if ((index.Remainder() == _T("URL")) && (request.HasBody() == true) && (request.Body<Web::TextBody>()->empty() == false)) {
                    _browser->SetURL(*(request.Body<Web::TextBody>()));
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = "Unknown error";
                }
            }
            stateControl->Release();
        }

        return result;
    }

    void SimpleEGL::LoadFinished(const string& URL)
    {
        Data info(URL);
        string message;
        info.ToString(message);

        _service->Notify(message);
    }

    void SimpleEGL::URLChanged(const string& URL)
    {

        Data info(URL);
        string message;
        info.ToString(message);
        TRACE_L1("Received a new URL: %s", message.c_str());
        TRACE_L1("URL length: %u", static_cast<uint32_t>(message.length()));
        fprintf(stderr, "================ URL Changed [%s] =================\n", URL.c_str()); fflush(stderr);

        _service->Notify(message);
    }

    void SimpleEGL::Hidden(const bool hidden)
    {
        string message = "{ \"hidden\": \"" + string(hidden ? _T("true") : _T("false")) + "\" }";

        _service->Notify(message);
    }

    void SimpleEGL::StateChange(const PluginHost::IStateControl::state value)
    {
        string message = "{ \"state\": \"test\" }";

        _service->Notify(message);
    }

    void SimpleEGL::ConnectionTermination(uint32_t connectionId)
    {
        RPC::IRemoteConnection* connection(_service->RemoteConnection(connectionId));
        if (connection != nullptr) {
            connection->Terminate();
            connection->Release();
        }
    }

    void SimpleEGL::Deactivated(RPC::IRemoteConnection* connection)
    {
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (_connectionId == connection->Id()) {

            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
}
} // namespace Plugin
