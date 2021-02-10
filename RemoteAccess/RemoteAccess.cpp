#include "RemoteAccess.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(RemoteAccess, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<RemoteAccess::Data> > jsonDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<RemoteAccess::Data> > jsonBodyDataFactory(2);

    /* virtual */ const string RemoteAccess::Initialize(PluginHost::IShell* service)
    {
        Config config;
        string message;

        ASSERT(_service == nullptr);
        ASSERT(_implementation == nullptr);

        _pid = 0;
        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        config.FromString(_service->ConfigLine());
        SYSLOG(Logging::Startup, (_T("Inside %s:%d"), __FUNCTION__, __LINE__));

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notification);

        _implementation = _service->Root<Exchange::IRemoteAccess>(_pid, 2000, _T("RemoteAccessImplementation"));
        SYSLOG(Logging::Startup, (_T("_implementation=%p"), _implementation));

        if (_implementation == nullptr) {
            message = _T("RemoteAccess could not be instantiated.");
            _service->Unregister(&_notification);
            _service = nullptr;
        }
        else {
            _implementation->Configure(_service);
       }

        return message;
    }

    /*virtual*/ void RemoteAccess::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_implementation != nullptr);

        _service->Unregister(&_notification);

        if (_implementation->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

            ASSERT(_pid != 0);

            TRACE_L1("RemoteAccess Plugin is not properly destructed. %d", _pid);

            RPC::IRemoteConnection* connection(_service->RemoteConnection(_pid));

            // The connection can disappear in the meantime...
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

    /* virtual */ string RemoteAccess::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }

    /* virtual */ void RemoteAccess::Inbound(WPEFramework::Web::Request& request)
    {
    if (request.Verb == Web::Request::HTTP_POST)
        request.Body(jsonBodyDataFactory.Element());
    }

    /* virtual */ Core::ProxyType<Web::Response> RemoteAccess::Process(const WPEFramework::Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (string(_T("Received RemoteAccess request"))));

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        return result;
    }

    void RemoteAccess::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _pid) {

            ASSERT(_service != nullptr);

            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }
}
} //namespace WPEFramework::Plugin

