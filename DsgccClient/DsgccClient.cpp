#include "DsgccClient.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Exchange::IDsgccClient::state)

    { Exchange::IDsgccClient::Unknown,  _TXT("Unknown") },
    { Exchange::IDsgccClient::Ready,    _TXT("Ready")   },
    { Exchange::IDsgccClient::Changed,  _TXT("Changed")  },
    { Exchange::IDsgccClient::Error,    _TXT("Error")    },

ENUM_CONVERSION_END(Exchange::IDsgccClient::state)

namespace Plugin {

    SERVICE_REGISTRATION(DsgccClient, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<DsgccClient::Data>> jsonDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<DsgccClient::Data>> jsonBodyDataFactory(2);

    /* virtual */ const string DsgccClient::Initialize(PluginHost::IShell* service)
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

        _implementation = _service->Root<Exchange::IDsgccClient>(_connectionId, 2000, _T("DsgccClientImplementation"));

        if (_implementation == nullptr) {
            message = _T("DsgccClient could not be instantiated.");
            _service->Unregister(&_notification);
            _service = nullptr;
        } else {
            _implementation->Callback(&_sink);
            _implementation->Configure(_service);
        }

        return message;
    }

    /*virtual*/ void DsgccClient::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_implementation != nullptr);

        _service->Unregister(&_notification);
        _implementation->Callback(nullptr);

        if (_implementation->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

            ASSERT(_connectionId != 0);

            TRACE_L1("DsgccClient Plugin is not properly destructed. %d", _connectionId);

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

    /* virtual */ string DsgccClient::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }

    /* virtual */ void DsgccClient::Inbound(WPEFramework::Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_POST)
            request.Body(jsonBodyDataFactory.Element());
    }

    /* virtual */ Core::ProxyType<Web::Response> DsgccClient::Process(const WPEFramework::Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (string(_T("Received DsgccClient request"))));

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        if ((request.Verb == Web::Request::HTTP_GET) && ((index.Next()) && (index.Next()))) {
            result->ErrorCode = Web::STATUS_OK;
            result->Message = "OK";

            if (index.Current().Text() == _T("GetChannels")) {
                Core::ProxyType<Web::JSONBodyType<Data> > data (jsonDataFactory.Element());
                string strChannels = _implementation->GetChannels();
                data->Channels.FromString(strChannels, data->Channels);

                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(data);
            } else if (index.Current().Text() == _T("State")) {
                Core::ProxyType<Web::JSONBodyType<Data> > data (jsonDataFactory.Element());
                data->State = _implementation->State();

                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(data);
            } else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Unsupported GET Request");
            }
        } else if ((request.Verb == Web::Request::HTTP_POST) && ((index.Next()) && (index.Next()))) {

            if (index.Current().Text() == _T("Restart")) {
                _implementation->Restart();
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";
            } else if (index.Current().Text() == _T("Test")) {
                std::string str = request.Body<const Data>()->Str.Value();

                _implementation->DsgccClientSet(str);

                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";
            } else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Unsupported POST Request");
            }
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Unsupported Method.");
        }
        return result;
    }

    void DsgccClient::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {

            ASSERT(_service != nullptr);

            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }

    void DsgccClient::StateChange(Exchange::IDsgccClient::state state)
    {
        Data data;
        data.State = Core::EnumerateType<Exchange::IDsgccClient::state>(state).Data();
        string message;
        data.ToString(message);
        TRACE_L1("%s: %s", __FUNCTION__, message.c_str());
        _service->Notify(message);
    }


}
} //namespace WPEFramework::Plugin
