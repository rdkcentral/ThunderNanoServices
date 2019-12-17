#include "WebServer.h"

namespace WPEFramework {

namespace WebServer {

    // An implementation file needs to implement this method to return an operational webserver, wherever that would be :-)
    extern Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection);
}

namespace Plugin {

    SERVICE_REGISTRATION(WebServer, 1, 0);

    /* virtual */ const string WebServer::Initialize(PluginHost::IShell* service)
    {
        string message;
        Config config;

        ASSERT(_server == nullptr);
        ASSERT(_memory == nullptr);
        ASSERT(_service == nullptr);

        // Setup skip URL for right offset.
        _connectionId = 0;
        _service = service;
        _skipURL = static_cast<uint32_t>(_service->WebPrefix().length());

        config.FromString(_service->ConfigLine());

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notification);

        _server = _service->Root<Exchange::IWebServer>(_connectionId, 2000, _T("WebServerImplementation"));

        if (_server != nullptr) {
            PluginHost::IStateControl* stateControl(_server->QueryInterface<PluginHost::IStateControl>());

            // We see that sometimes the implementation crashes before it reaches this point, than there is
            // no StateControl. Cope with this situation.
            if (stateControl == nullptr) {
                _server->Release();
                _server = nullptr;

            } else {
                stateControl->Configure(_service);
                stateControl->Release();

                RPC::IRemoteConnection* connection = _service->RemoteConnection(_connectionId);

                ASSERT(connection != nullptr);

                if(connection != nullptr) {
                    _memory = WPEFramework::WebServer::MemoryObserver(connection);
                    connection->Release();
                }

                ASSERT(_memory != nullptr);

                _memory->Observe(true);

                PluginHost::ISubSystem* subSystem = service->SubSystems();

                if (subSystem != nullptr) {
                    if (subSystem->IsActive(PluginHost::ISubSystem::WEBSOURCE) == true) {
                        SYSLOG(Logging::Startup, (_T("WebSource is not defined as External !!")));
                    } else {
                        subSystem->Set(PluginHost::ISubSystem::WEBSOURCE, nullptr);
                    }
                    subSystem->Release();
                }
            }
        }

        if (_server == nullptr) {
            _service->Unregister(&_notification);
            _service = nullptr;
            message = _T("WebServer could not be instantiated.");
        }

        return message;
    }

    /* virtual */ void WebServer::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == _service);
        ASSERT(_server != nullptr);
        ASSERT(_memory != nullptr);

        _service->Unregister(&_notification);
        _memory->Release();

        // Stop processing of the browser:
        if (_server->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

            ASSERT(_connectionId != 0);

            TRACE_L1("OutOfProcess Plugin is not properly destructed. PID: %d", _connectionId);

            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            // The process can disappear in the meantime...
            if (connection != nullptr) {

                // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                connection->Terminate();
                connection->Release();
            }
        }

        PluginHost::ISubSystem* subSystem = service->SubSystems();

        if (subSystem != nullptr) {
            ASSERT(subSystem->IsActive(PluginHost::ISubSystem::WEBSOURCE) == true);
            subSystem->Set(PluginHost::ISubSystem::NOT_WEBSOURCE, nullptr);
            subSystem->Release();
        }

        _memory = nullptr;
        _server = nullptr;
        _service = nullptr;
    }

    /* virtual */ string WebServer::Information() const
    {
        // No additional info to report.
        return (string());
    }

    void WebServer::Deactivated(RPC::IRemoteConnection* connection)
    {
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (_connectionId == connection->Id()) {

            ASSERT(_service != nullptr);

            Core::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
}
}
