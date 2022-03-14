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
 
#include "Cobalt.h"

namespace WPEFramework {
namespace Cobalt {

extern Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection);
}

namespace Plugin {

SERVICE_REGISTRATION(Cobalt, 1, 0);

static Core::ProxyPoolType<Web::TextBody> _textBodies(2);
static Core::ProxyPoolType<Web::JSONBodyType<Cobalt::Data>> jsonBodyDataFactory(2);

const string Cobalt::Initialize(PluginHost::IShell *service)
{
    Config config;
    string message(EMPTY_STRING);

    ASSERT(service != nullptr);
    ASSERT(_service == nullptr);
    ASSERT(_connectionId == 0);
    ASSERT(_cobalt == nullptr);
    ASSERT(_application == nullptr);
    ASSERT(_memory == nullptr);

    config.FromString(service->ConfigLine());
    _persistentStoragePath = service->PersistentPath();

    _service = service;
    _service->AddRef();
    _skipURL = _service->WebPrefix().length();

    // Register the Connection::Notification stuff. The Remote process might die
    // before we get a
    // change to "register" the sink for these events !!! So do it ahead of
    // instantiation.
    _service->Register(&_notification);
    _cobalt = _service->Root < Exchange::IBrowser
            > (_connectionId, 2000, _T("CobaltImplementation"));

    if (_cobalt != nullptr) {
        PluginHost::IStateControl *stateControl(
                _cobalt->QueryInterface<PluginHost::IStateControl>());
        if (stateControl == nullptr) {
            message = _T("Cobalt StateControl could not be Obtained.");
        } else {
            _application = _cobalt->QueryInterface<Exchange::IApplication>();
            if (_application != nullptr) {
                RegisterAll();
                Exchange::JApplication::Register(*this, _application);
                RPC::IRemoteConnection* remoteConnection = _service->RemoteConnection(_connectionId);
                if(remoteConnection != nullptr) {
                    _memory = WPEFramework::Cobalt::MemoryObserver(remoteConnection);
                    if(_memory != nullptr) {
                        _cobalt->Register(&_notification);
                        stateControl->Register(&_notification);
                        stateControl->Configure(_service);
                    } else {
                        message = _T("Cobalt Memory Obesever could not be Obtained.");
                    }
                    remoteConnection->Release();
                } else {
                    message = _T("Cobalt Remote Connection could not be Obtained.");
                }
            } else {
                message = _T("Cobalt IApplication could not be Obtained.");
            }
            stateControl->Release();

        }
    } else {
        message = _T("Cobalt could not be instantiated.");
    }

    if (message.length() != 0) {
       Deinitialize(service);
    }

    return message;
}

void Cobalt::Deinitialize(PluginHost::IShell *service)
{
    ASSERT(_service == service);
    ASSERT(_cobalt != nullptr);
    ASSERT(_application != nullptr);
    ASSERT(_memory != nullptr);

    ASSERT(_service == service);
    _service->Unregister(&_notification);

    if(_connectionId != 0) {
        ASSERT(_cobalt != nullptr);

        PluginHost::IStateControl *stateControl(_cobalt->QueryInterface<PluginHost::IStateControl>());
        // Make sure the Activated and Deactivated are no longer called before we
        // start cleaning up..
        // In case Cobalt crashed, there is no access to the statecontrol interface,
        // check it !!
        if (stateControl != nullptr) {
            stateControl->Unregister(&_notification);
            stateControl->Release();
        } else {
            // On behalf of the crashed process, we will release the notification sink.
            _notification.Release();
        }
        if(_memory != nullptr) {
            _cobalt->Unregister(&_notification);
            _memory->Release();
            _memory = nullptr;
        }
        if(_application != nullptr) {
            Exchange::JApplication::Unregister(*this);
            UnregisterAll();
            _application->Release();
            _application = nullptr;
        }

        RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
        VARIABLE_IS_NOT_USED uint32_t result = _cobalt->Release();
        _cobalt = nullptr;
        ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

        // The connection can disappear in the meantime...
        if (connection != nullptr) {
            // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
            connection->Terminate();
            connection->Release();
        }
    }
    _service->Release();
    _service = nullptr;
    _connectionId = 0;
}

string Cobalt::Information() const
{
    // No additional info to report.
    return (string());
}

void Cobalt::Inbound(Web::Request &request)
{
    if (request.Verb == Web::Request::HTTP_POST) {
        // This might be a "launch" application thingy, make sure we receive the
        // proper info.
        request.Body(jsonBodyDataFactory.Element());
        //request.Body(_textBodies.Element());
    }
}

Core::ProxyType<Web::Response> Cobalt::Process(const Web::Request &request)
{
    ASSERT(_skipURL <= request.Path.length());
    TRACE(Trace::Information, (string(_T("Received cobalt request"))));

    Core::ProxyType < Web::Response
            > result(PluginHost::IFactories::Instance().Response());

    Core::TextSegmentIterator index(
            Core::TextFragment(request.Path, _skipURL,
                    request.Path.length() - _skipURL), false, '/');

    result->ErrorCode = Web::STATUS_BAD_REQUEST;
    result->Message = "Unknown error";

    if (request.Verb == Web::Request::HTTP_POST) {
        // We might be receiving a plugin download request.
        if ((index.Next() == true) && (index.Next() == true)
                && (_cobalt != nullptr)) {
            PluginHost::IStateControl *stateControl(
                    _cobalt->QueryInterface<PluginHost::IStateControl>());
            if (stateControl != nullptr) {
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";
                if (index.Remainder() == _T("Suspend")) {
                    stateControl->Request(PluginHost::IStateControl::SUSPEND);
                } else if (index.Remainder() == _T("Resume")) {
                    stateControl->Request(PluginHost::IStateControl::RESUME);
                } else if ((index.Remainder() == _T("URL"))
                        && (request.HasBody() == true)
                        && (request.Body<const Data>()->URL.Value().empty()
                                == false)) {
                    _cobalt->SetURL(request.Body<const Data>()->URL.Value());
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = "Unknown error";
                }
                stateControl->Release();
            }
        }
    } else if (request.Verb == Web::Request::HTTP_GET) {
    }
    return result;
}

void Cobalt::LoadFinished(const string &URL)
{
    string message(
            string("{ \"url\": \"") + URL + string("\", \"loaded\":true }"));
    TRACE(Trace::Information, (_T("LoadFinished: %s"), message.c_str()));
    _service->Notify(message);

    event_urlchange(URL, true);
}

void Cobalt::URLChanged(const string &URL)
{
    string message(string("{ \"url\": \"") + URL + string("\" }"));
    TRACE(Trace::Information, (_T("URLChanged: %s"), message.c_str()));
    _service->Notify(message);

    event_urlchange(URL, false);
}

void Cobalt::Hidden(const bool hidden)
{
    TRACE(Trace::Information,
            (_T("Hidden: %s }"), (hidden ? "true" : "false")));
    string message(
            string("{ \"hidden\": ") + (hidden ? _T("true") : _T("false"))
                    + string("}"));
    _hidden = hidden;
    _service->Notify(message);

    event_visibilitychange(hidden);
}

uint32_t Cobalt::DeleteDir(const string& path)
{
    uint32_t result = Core::ERROR_NONE;
    if (path.empty() == false) {
        string fullPath = _persistentStoragePath + path;
        Core::Directory dir(fullPath.c_str());
        if (!dir.Destroy()) {
            TRACE(Trace::Error, (_T("Failed to delete %s\n"), fullPath.c_str()));
            result = Core::ERROR_UNKNOWN_KEY;
        }
    }

    return result;
}

void Cobalt::StateChange(const PluginHost::IStateControl::state state)
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
        // Exited by Cobalt app
        Core::IWorkerPool::Instance().Submit(
                PluginHost::IShell::Job::Create(_service,
                        PluginHost::IShell::DEACTIVATED,
                        PluginHost::IShell::REQUESTED));
        break;
    case PluginHost::IStateControl::UNINITIALIZED:
        break;
    default:
        ASSERT(false);
        break;
    }
}

void Cobalt::Deactivated(RPC::IRemoteConnection *connection)
{
    if (connection->Id() == _connectionId) {

        ASSERT(_service != nullptr);
        Core::IWorkerPool::Instance().Submit(
                PluginHost::IShell::Job::Create(_service,
                        PluginHost::IShell::DEACTIVATED,
                        PluginHost::IShell::FAILURE));
    }
}
}
} // namespace
