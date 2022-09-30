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
 
#include "WebPA.h"

namespace WPEFramework {
namespace Plugin {

namespace {

    static Metadata<WebPA> metadata(
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

/* virtual */ const string WebPA::Initialize(PluginHost::IShell* service)
{
    string message;

    ASSERT(service != nullptr);
    ASSERT(_webpa == nullptr);
    ASSERT(_service == nullptr);
    ASSERT(_connectionId == 0);

    // Setup skip URL for right offset.

    _service = service;
    _service->AddRef();
    _skipURL = _service->WebPrefix().length();

    // Register the Process::Notification stuff. The Remote connection might die before we get a
    // change to "register" the sink for these events !!! So do it ahead of instantiation.
    _service->Register(&_notification);

    _webpa =  _service->Root<Exchange::IWebPA>(_connectionId, ImplWaitTime, _T("WebPAImplementation"));

    if (nullptr != _webpa)  {
        // Configure and Launch Service
        if (_webpa->Initialize(_service) == Core::ERROR_NONE) {
            TRACE(Trace::Information, (_T("Successfully instantiated WebPA Service")));
        } else {
            TRACE(Trace::Error, (_T("WebPA Service could not be launched.")));
            message = _T("WebPA Service could not be launched.");
        }
    } else {
        TRACE(Trace::Error, (_T("WebPA Service could not be instantiated.")));
        message = _T("WebPA Service could not be instantiated.");
    }

    if(message.length() != 0) {
        Deinitialize(service);
    }


    return message;
}

/* virtual */ void WebPA::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
{
    ASSERT(_service == service);

    _service->Unregister(&_notification);

    if(_webpa != nullptr){

        _webpa->Deinitialize(_service);

        RPC::IRemoteConnection* serviceConnection(_service->RemoteConnection(_connectionId));
        VARIABLE_IS_NOT_USED uint32_t result = _webpa->Release();
        _webpa = nullptr;
        ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
        // The connection can disappear in the meantime...
        if (nullptr != serviceConnection) {
            // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
            serviceConnection->Terminate();
            serviceConnection->Release();
        }
    }
    _connectionId = 0;
    _service->Release();
    _service = nullptr;
}

/* virtual */ string WebPA::Information() const
{
    // No additional info to report.
    return (string());
}

/* virtual */ void WebPA::Inbound(Web::Request& /* request */)
{
}

/* virtual */ Core::ProxyType<Web::Response> WebPA::Process(const Web::Request& request)
{
    ASSERT(_skipURL <= request.Path.length());

    Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
    Core::TextSegmentIterator index(
        Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

    result->ErrorCode = Web::STATUS_BAD_REQUEST;
    result->Message = "Unknown error";
    if ((request.Verb == Web::Request::HTTP_POST) && (index.Next() == true) && (index.Next() == true)) {
        TRACE(Trace::Information, (string(_T("POST request"))));
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";
    }
    return result;
}

void WebPA::Deactivated(RPC::IRemoteConnection* connection)
{
    ASSERT(connection != nullptr);

    // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
    // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
    if ((_connectionId == connection->Id())) {
        ASSERT(nullptr != _service);
        Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
    }
}

} //namespace Plugin
} // namespace WPEFramework
