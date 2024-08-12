/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
 
#include "OutOfProcessPlugin.h"

#ifdef __CORE_EXCEPTION_CATCHING__
#include <stdexcept>
#endif

namespace Thunder {

namespace OutOfProcessPlugin {

    // An implementation file needs to implement this method to return an operational browser, wherever that would be :-)
    extern Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection);
}

namespace Plugin {

    namespace {

        static Metadata<OutOfProcessPlugin> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    static Core::ProxyPoolType<OutOfProcessPlugin::Data> jsonDataFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<OutOfProcessPlugin::Data>> jsonBodyDataFactory(4);
    static Core::ProxyPoolType<Web::TextBody> textFactory(4);

    static const char SampleData[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789=!";

    const string OutOfProcessPlugin::Initialize(PluginHost::IShell* service) /* override */
    {
        ASSERT(service != nullptr);
        ASSERT(_browser == nullptr);
        ASSERT(_browserresources == nullptr);
        ASSERT(_memory == nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_state == nullptr);

        string message;

        _service = service;
        _service->AddRef();

        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

        _service->EnableWebServer(_T("UI"), EMPTY_STRING);
        _service->Register(static_cast<RPC::IRemoteConnection::INotification*>(_notification));
        _service->Register(static_cast<PluginHost::IPlugin::INotification*>(_notification));

        _browser = service->Root<Exchange::IBrowser>(_connectionId, Core::infinite, _T("OutOfProcessImplementation"));

        if (_browser == nullptr) {
            message = _T("OutOfProcessPlugin could not be instantiated.");
        } else {
            _browserresources = _browser->QueryInterface<Exchange::IBrowserResources>();
            if( _browserresources != nullptr) {
                Exchange::JBrowserResources::Register(*this, _browserresources);
                Register("bigupdate", [this](const Core::JSONRPC::Context&, const string& params, Core::OptionalType<Core::JSON::Error>&) {
                    uint32_t updates = 5000;
                    string sleep("100");
                    if(params.empty() == false) {
                        class Params : public Core::JSON::Container {
                        public:
                            Params(const Params&) = delete;
                            Params& operator=(const Params&) = delete;

                            Params()
                                : Core::JSON::Container()
                                , Updates(5000)
                                , Sleep("100")
                            {
                                Add(_T("updates"), &Updates);
                                Add(_T("sleep"), &Sleep);
                            }
                            ~Params() override = default;

                        public:
                            Core::JSON::DecUInt32 Updates;
                            Core::JSON::String Sleep;
                        } paramscontainer;
                        paramscontainer.FromString(params);
                        updates = paramscontainer.Updates.Value();
                        sleep = paramscontainer.Sleep.Value();
                    }

                    std::list<string> _elements;
                    for(uint32_t i = 0; i<updates; ++i) {
                        string s("UserScripts_Updated_");
                        s += std::to_string(i);
                        if( i == 0 ) {
                            s = sleep;
                        }
                        _elements.push_back(s);
                    }
                    RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>* _params{Core::ServiceType<RPC::IteratorType<RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>>>::Create<RPC::IIteratorType<string, RPC::ID_STRINGITERATOR>>(_elements)};
                    if ((_params != nullptr)) {
                        _browserresources->UserScripts(_params);
                        _params->Release();
                    }
                }); 
            }

            _browser->Register(_notification);

            PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());

            if (stateControl != nullptr) {

                _state = stateControl;

                _state->Configure(_service);
                _state->Register(_notification);

                PluginHost::IPlugin::INotification* sink = _browser->QueryInterface<PluginHost::IPlugin::INotification>();

                if (sink != nullptr) {
                    _service->Register(sink);
                    sink->Release();

                    RPC::IRemoteConnection* remoteConnection = _service->RemoteConnection(_connectionId);
                    if (remoteConnection != nullptr) {
                        _memory = Thunder::OutOfProcessPlugin::MemoryObserver(remoteConnection);
                        ASSERT(_memory != nullptr);
                        remoteConnection->Release();
                    }
                } else {
                    message = _T("Failed to register Notification sink.");
                }
            }
            else {
                message = _T("OutOfProcessPlugin could not obtain state control.");
            }
        }
        
        return message;
    }

    /* virtual */ void OutOfProcessPlugin::Deinitialize(PluginHost::IShell* service)
    {
        if (_service != nullptr) {
	    ASSERT(_service == service);

            _service->Unregister(static_cast<RPC::IRemoteConnection::INotification*>(_notification));
            _service->Unregister(static_cast<PluginHost::IPlugin::INotification*>(_notification));
            _service->DisableWebServer();

            if (_browser != nullptr) {
                if (_browserresources != nullptr) {
                    Exchange::JBrowserResources::Unregister(*this);
                    _browserresources->Release();
                    _browserresources = nullptr;
                }
                _browser->Unregister(_notification);

                if (_memory != nullptr) {
                    _memory->Release();
                    _memory = nullptr;
                }

                if (_state != nullptr) {
                    PluginHost::IPlugin::INotification* sink = _browser->QueryInterface<PluginHost::IPlugin::INotification>();
                    if (sink != nullptr) {
                        _service->Unregister(sink);
                        sink->Release();
                    }
                    _state->Unregister(_notification);
                    _state->Release();
                    _state = nullptr;
                }

                // Stop processing:
                RPC::IRemoteConnection* connection = service->RemoteConnection(_connectionId);
                VARIABLE_IS_NOT_USED uint32_t result = _browser->Release();
                _browser = nullptr;

                // It should have been the last reference we are releasing, 
                // so it should endup in a DESTRUCTION_SUCCEEDED, if not we
                // are leaking...
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                // If this was running in a (container) process...
                if (connection != nullptr) {
                    // Lets trigger the cleanup sequence for 
                    // out-of-process code. Which will guard 
                    // that unwilling processes, get shot if
                    // not stopped friendly :-)
                    connection->Terminate();
                    connection->Release();
                }
            }

            _connectionId = 0;
            _service->Release();
            _service = nullptr;
        }
    }

    /* static */ const char* OutOfProcessPlugin::PluginStateStr(const PluginHost::IShell::state state)
    {
        switch (state) {
        case PluginHost::IShell::DEACTIVATED:
            return "DEACTIVATED";
        case PluginHost::IShell::DEACTIVATION:
            return "DEACTIVATION";
        case PluginHost::IShell::ACTIVATED:
            return "ACTIVATED";
        case PluginHost::IShell::ACTIVATION:
            return "ACTIVATION";
        case PluginHost::IShell::PRECONDITION:
            return "PRECONDITION";
        case PluginHost::IShell::DESTROYED:
            return "DESTROYED";
        default:
            break;
        }

        return "### UNKNOWN ###";
    }

    void OutOfProcessPlugin::Activated(const string& callsign, PluginHost::IShell* plugin)
    {
        ASSERT(plugin != nullptr);

        const PluginHost::IShell::state state = plugin->State();
        const char* stateStr = PluginStateStr(state);

        TRACE(Trace::Information, (_T("OutOfProcessPlugin::PluginStateChanged: Got [%s] plugin [%s] event..."), callsign.c_str(), stateStr));
    }

    void OutOfProcessPlugin::Deactivated(const string& callsign, PluginHost::IShell* plugin)
    {
        ASSERT(plugin != nullptr);

        const PluginHost::IShell::state state = plugin->State();
        const char* stateStr = PluginStateStr(state);

        TRACE(Trace::Information, (_T("OutOfProcessPlugin::PluginStateChanged: Got [%s] plugin [%s] event..."), callsign.c_str(), stateStr));

        if (callsign == "OutOfProcessPlugin") {
            if(_state != nullptr) {
                TRACE(Trace::Information, (_T("OutOfProcessPlugin::PluginStateChanged: Doing RPC call with IShell * ptr...")));
                _state->Configure(plugin); //note this indeed does not make sense but is used to trigger an IShell* call to the OOP side to reproduce an issue there
                TRACE(Trace::Information, (_T("OutOfProcessPlugin::PluginStateChanged: Doing RPC call with IShell * ptr...DONE")));
            }
        }

        TRACE(Trace::Information, (_T("OutOfProcessPlugin::PluginStateChanged: Got [%s] plugin [%s] event...DONE"), callsign.c_str(), stateStr));
    }

    /* virtual */ string OutOfProcessPlugin::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }
    /* virtual */ bool OutOfProcessPlugin::Attach(PluginHost::Channel& channel)
    {
        if (_subscriber == nullptr) {
            _subscriber = &channel;
            return (true);
        }
        return (false);
    }

    /* virtual */ void OutOfProcessPlugin::Detach(PluginHost::Channel& channel)
    {
        if ((_subscriber != nullptr) && (_subscriber == &channel)) {
            _subscriber = nullptr;
        }
    }

    /* virtual */ void OutOfProcessPlugin::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_PUT) {
            request.Body(jsonBodyDataFactory.Element());
        } else if (request.Verb == Web::Request::HTTP_POST) {
            request.Body(textFactory.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> OutOfProcessPlugin::Process(const Web::Request& request)
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
                Core::ProxyType<Web::JSONBodyType<OutOfProcessPlugin::Data>> body(jsonBodyDataFactory.Element());
                body->URL = _browser->GetURL();
                body->FPS = _browser->GetFPS();
                body->Suspended = (currentState == PluginHost::IStateControl::SUSPENDED);
                body->Hidden = _hidden;
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";
                result->Body<Web::JSONBodyType<OutOfProcessPlugin::Data>>(body);
                
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

                // Test IDispatcher with given data size
                } else if (index.Remainder() == _T("TestValidator")) {
                    uint32_t stringSize = 32;
                    if (request.HasBody() == true) {
                        stringSize = std::stoi(*(request.Body<Web::TextBody>()));
                        if (stringSize > 128) {
                            TRACE(Trace::Information, (_T("%u is not allowed. Size automatically set to maximum: 128K"), stringSize));
                            stringSize = 128;
                        }
                    }
                } else if (index.Remainder() == _T("Notify4K")) {
                    string message;
                    for (uint32_t teller = 0; teller < ((4 * 1024) + 64); teller++) {
                        message += SampleData[teller % 64];
                    }
                    _service->Notify(message);
                } else if ((index.Remainder() == _T("JSONPop")) && (_subscriber != nullptr)) {
                    Core::ProxyType<Data> info(jsonDataFactory.Element());

                    info->URL = _T("ws://<My IP>/JSONPop");
                    _subscriber->Submit(Core::ProxyType<Core::JSON::IElement>(info));
                } else if ((index.Remainder() == _T("URL")) && (request.HasBody() == true) && (request.Body<Web::TextBody>()->empty() == false)) {
                    _browser->SetURL(*(request.Body<Web::TextBody>()));
                #ifdef __CORE_EXCEPTION_CATCHING__
                } else if (index.Remainder() == _T("ThunderException")) {
                    throw(std::domain_error(_T("TestException")));
                } else if (index.Current() == _T("ProcessException")) {
                    string message(_T("ThrowException"));
                    if (index.Next() == true) {
                        message = message + ':' + index.Remainder().Text();
                    }
                    _browser->SetURL(message);
		#endif
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = "Unknown error";
                }
            }
            stateControl->Release();
        }

        return result;
    }

    void OutOfProcessPlugin::LoadFinished(const string& URL)
    {
        Data info(URL);
        string message;
        info.ToString(message);

        _service->Notify(message);
    }

    void OutOfProcessPlugin::URLChanged(const string& URL)
    {

        Data info(URL);
        string message;
        info.ToString(message);
        TRACE(Trace::Information, (_T("Received a new URL: %s"), message.c_str()));
        TRACE(Trace::Information, (_T("URL length: %u"), static_cast<uint32_t>(message.length())));
        TRACE(Trace::Information, (_T("Parent process recceived a changed URL: [%s]"), URL.c_str()));

        _service->Notify(message);
    }

    void OutOfProcessPlugin::Hidden(const bool hidden)
    {
        TRACE(Trace::Information, (_T("Parent process recceived a changed visibility: [%s]"), hidden ? _T("Hidden") : _T("Shown")));
        string message = "{ \"hidden\": \"" + string(hidden ? _T("true") : _T("false")) + "\" }";

        _service->Notify(message);
    }

    void OutOfProcessPlugin::StateChange(const PluginHost::IStateControl::state value)
    {
        TRACE(Trace::Information, (_T("Parent process recceived a changed state: [%s]"), value == PluginHost::IStateControl::state::RESUMED ? _T("Resumed") : _T("Suspended")));
        string message = "{ \"state\": \"test\" }";

        _service->Notify(message);
    }

    void OutOfProcessPlugin::Deactivated(RPC::IRemoteConnection* connection)
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
