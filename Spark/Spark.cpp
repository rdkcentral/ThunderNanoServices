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
 
#include "Spark.h"

namespace Thunder {

namespace Spark {

    extern Exchange::IMemory* MemoryObserver(const uint32_t pid);
}

namespace Plugin {

    namespace {

        static Metadata<Spark> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::INTERNET, subsystem::GRAPHICS },
            // Terminations
            {},
            // Controls
            {}
        );
    }

    static Core::ProxyPoolType<Web::TextBody> _textBodies(2);
    static Core::ProxyPoolType<Web::JSONBodyType<Spark::Data>> jsonBodyDataFactory(2);

    /* encapsulated class Thread  */
    /* virtual */ const string Spark::Initialize(PluginHost::IShell* service)
    {
        Config config;
        string message;

        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_spark == nullptr);
        ASSERT(_memory == nullptr);

        config.FromString(service->ConfigLine());

        _service = service;
        _service->AddRef();

        _skipURL = _service->WebPrefix().length();

        // Register the Connection::Notification stuff. The Remote process might die
        // before we get a
        // change to "register" the sink for these events !!! So do it ahead of
        // instantiation.
        _service->Register(&_notification);

        _spark = _service->Root<Exchange::IBrowser>(_connectionId, 3000, _T("SparkImplementation"));
        if (_spark == nullptr) {
            message = _T("Spark could not be instantiated. Could not load SparkImplementation");
        } else {
            RegisterAll();
            _spark->Register(&_notification);
            PluginHost::IStateControl* stateControl(_spark->QueryInterface<PluginHost::IStateControl>());
            if (stateControl == nullptr) {
                message = _T("Spark could not be instantiated.");
            } else {
                stateControl->Register(&_notification);
                stateControl->Configure(_service);
                stateControl->Release();

                const RPC::IRemoteConnection *connection = _service->RemoteConnection(_connectionId);
                if (connection != nullptr) {
                    _memory = Thunder::Spark::MemoryObserver(connection->RemoteId());
                    ASSERT(_memory != nullptr);
                    connection->Release();
                }
            }
        }

        return message;
    }

    /* virtual */ void Spark::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            // Make sure the Activated and Deactivated are no longer called before we
            // start cleaning up..
            _service->Unregister(&_notification);

            if (_spark != nullptr) {
                UnregisterAll();
                _spark->Unregister(&_notification);

                PluginHost::IStateControl* stateControl(_spark->QueryInterface<PluginHost::IStateControl>());
                // In case Spark crashed, there is no access to the statecontrol interface,
                // check it !!
                if (stateControl != nullptr) {
                    stateControl->Unregister(&_notification);
                    stateControl->Release();
                } else {
                    // On behalf of the crashed process, we will release the notification sink.
                    _notification.Release();
                }

                if (_memory != nullptr) {
                    _memory->Release();
                    _memory = nullptr;
                }

                RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
                VARIABLE_IS_NOT_USED uint32_t result = _spark->Release();
                _spark = nullptr;
                // It should have been the last reference we are releasing,
                // so it should endup in a DESTRUCTION_SUCCEEDED, if not we
                // are leaking...
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                // The process can disappear in the meantime...
                if (connection != nullptr) {
                    connection->Terminate();
                    connection->Release();
                }
            }

            // Deinitialize what we initialized..
            _service->Release();
            _service = nullptr;
            _connectionId = 0;
        }
    }

    /* virtual */ string Spark::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void Spark::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_POST) {
            // This might be a "launch" application thingy, make sure we receive the
            // proper info.
            request.Body(jsonBodyDataFactory.Element());
            //request.Body(_textBodies.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response>
    Spark::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (string(_T("Received spark request"))));

        Core::ProxyType<Web::Response> result(
            PluginHost::IFactories::Instance().Response());

        Core::TextSegmentIterator index(
            Core::TextFragment(request.Path, _skipURL,
                request.Path.length() - _skipURL),
            false, '/');

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = "Unknown error";


        if (request.Verb == Web::Request::HTTP_POST) {
            // We might be receiving a plugin download request.
            if ((index.Next() == true) && (index.Next() == true) && (_spark != nullptr)) {
                PluginHost::IStateControl* stateControl(_spark->QueryInterface<PluginHost::IStateControl>());
                if (stateControl != nullptr) {
                    if (index.Remainder() == _T("Suspend")) {
                        stateControl->Request(PluginHost::IStateControl::SUSPEND);
                    }
                    else if (index.Remainder() == _T("Resume")) {
                        stateControl->Request(PluginHost::IStateControl::RESUME);
                    }
                    else if ((index.Remainder() == _T("URL")) && (request.HasBody() == true) && (request.Body<const Data>()->URL.Value().empty() == false)) {
                        _spark->SetURL(request.Body<const Data>()->URL.Value());
                    }
                    stateControl->Release();
                }
            }
        } else if (request.Verb == Web::Request::HTTP_GET) {
        }

        return result;
    }

    /* ---------------------------------------------------------
    void Spark::StateChange(const Exchange::ISpark::state state)
    {
        switch (state) {
        case Exchange::ISpark::STOPPED:
            TRACE(Trace::Information,
                (string(_T("StateChange: { \"playing\":false }"))));
            _service->Notify("{ \"playing\":false }");
            break;
        case Exchange::ISpark::PLAYING:
            TRACE(Trace::Information,
                (string(_T("StateChange: { \"playing\":true }"))));
            _service->Notify("{ \"playing\":true }");
            break;
        case Exchange::ISpark::SUSPENDING:
            break;
        default:
            ASSERT(false);
            break;
        }
    }
    ------------------------------------------------------------- */

    void Spark::LoadFinished(const string& URL)
    {
        string message(string("{ \"url\": \"") + URL + string("\", \"loaded\":true }"));
        TRACE(Trace::Information, (_T("LoadFinished: %s"), message.c_str()));
        _service->Notify(message);

        event_urlchange(URL, true);
    }
    void Spark::URLChanged(const string& URL)
    {
        string message(string("{ \"url\": \"") + URL + string("\" }"));
        TRACE(Trace::Information, (_T("URLChanged: %s"), message.c_str()));
        _service->Notify(message);

        event_urlchange(URL, false);
    }
    void Spark::Hidden(const bool hidden)
    {
        TRACE(Trace::Information, (_T("Hidden: %s }"), (hidden ? "true" : "false")));
        string message(string("{ \"hidden\": ") + (hidden ? _T("true") : _T("false")) + string("}"));
        _hidden = hidden;
        _service->Notify(message);

        event_visibilitychange(hidden);
    }
    void Spark::StateChange(const PluginHost::IStateControl::state state)
    {
        switch (state) {
        case PluginHost::IStateControl::RESUMED:
            TRACE(Trace::Information,
                (string(_T("StateChange: { \"suspend\":false }"))));
            _service->Notify("{ \"suspended\":false }");
            event_statechange(false);
            break;
        case PluginHost::IStateControl::SUSPENDED:
            TRACE(Trace::Information,
                (string(_T("StateChange: { \"suspend\":true }"))));
            _service->Notify("{ \"suspended\":true }");
            event_statechange(true);
            break;
        case PluginHost::IStateControl::EXITED:
            // Exited by Spark app
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(
                _service, PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::REQUESTED));
            break;
        case PluginHost::IStateControl::UNINITIALIZED:
            break;
        default:
            ASSERT(false);
            break;
        }
    }
    void Spark::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {

            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(
                _service, PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }
}
} // namespace
