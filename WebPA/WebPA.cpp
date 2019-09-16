#include "WebPA.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(WebPA, 1, 0);

/* virtual */ const string WebPA::Initialize(PluginHost::IShell* service)
{
    string message;

    ASSERT(_webpa == nullptr);
    ASSERT(_service == nullptr);

    // Setup skip URL for right offset.
    _connectionId = 0;
    _service = service;
    _skipURL = _service->WebPrefix().length();

    // Register the Process::Notification stuff. The Remote connection might die before we get a
    // change to "register" the sink for these events !!! So do it ahead of instantiation.
    _service->Register(&_notification);

    _webpa =  _service->Root<Exchange::IWebPA>(_connectionId, ImplWaitTime, _T("WebPAImplementation"));

    if (nullptr != _webpa)  {
        TRACE(Trace::Information, (_T("Successfully instantiated WebPA Service")));
            // Configure and Launch Service
            _webpa->Initialize(_service);

    } else {
        _service->Unregister(&_notification);
        TRACE(Trace::Error, (_T("WebPA Service could not be instantiated.")));
        message = _T("WebPA Service could not be instantiated.");
    }

    return message;
}

/* virtual */ void WebPA::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_webpa != nullptr);
    ASSERT(_service == service);

    _service->Unregister(&_notification);

    _webpa->Deinitialize(_service);
    if (_webpa->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
        ASSERT(_connectionId != 0);
        TRACE(Trace::Error, (_T("OutOfProcess Plugin is not properly destructed. PID: %d"), _connectionId));

        RPC::IRemoteConnection* serviceConnection(_service->RemoteConnection(_connectionId));

        // The connection can disappear in the meantime...
        if (nullptr != serviceConnection) {

            // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
            serviceConnection->Terminate();
            serviceConnection->Release();
        }
    }
    _webpa = nullptr;
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

    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
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
    ASSERT(connection != nullptr)

    // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
    // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
    if ((_connectionId == connection->Id())) {
        ASSERT(nullptr != _service);
        PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
    }
}

} //namespace Plugin
} // namespace WPEFramework
