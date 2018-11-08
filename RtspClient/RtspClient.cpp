#include "RtspClient.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(RtspClient, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<RtspClient::Data> > jsonDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<RtspClient::Data> > jsonBodyDataFactory(2);

    /* virtual */ const string RtspClient::Initialize(PluginHost::IShell* service)
    {
        Config config;
        string message;

        ASSERT(_service == nullptr);
        ASSERT(_implementation == nullptr);

        _pid = 0;
        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        config.FromString(_service->ConfigLine());

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notification);

        _implementation = _service->Root<Exchange::IRtspClient>(_pid, 2000, _T("RtspClientImplementation"));

        if (_implementation == nullptr) {
            message = _T("RtspClient could not be instantiated.");
            _service->Unregister(&_notification);
            _service = nullptr;
        }
        else {
            _implementation->Configure(_service);
            TRACE_L1("RtspClient Plugin initialized %p", _implementation);
       }

        return message;
    }

    /*virtual*/ void RtspClient::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_implementation != nullptr);

        _service->Unregister(&_notification);

        if (_implementation->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

            ASSERT(_pid != 0);

            TRACE_L1("RtspClient Plugin is not properly destructed. %d", _pid);

            RPC::IRemoteProcess* process(_service->RemoteProcess(_pid));

            // The process can disappear in the meantime...
            if (process != nullptr) {

                // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                process->Terminate();
                process->Release();
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
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
        result->ErrorCode = Web::STATUS_BAD_REQUEST;

        if ((request.Verb == Web::Request::HTTP_GET) && ((index.Next()) && (index.Next()))) {
            if (index.Current().Text() == _T("Get")) {
                Core::ProxyType<Web::JSONBodyType<Data> > data (jsonDataFactory.Element());
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

    void RtspClient::Deactivated(RPC::IRemoteProcess* process)
    {
        if (process->Id() == _pid) {

            ASSERT(_service != nullptr);

            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }
}
} //namespace WPEFramework::Plugin
