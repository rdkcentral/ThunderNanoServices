#include "TimeSync.h"
#include "NTPClient.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(TimeSync, 1, 0);

    static Core::ProxyPoolType<Web::Response> responseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<TimeSync::Data> > jsonResponseFactory(4);

    static const uint16_t NTPPort = 123;

#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
    TimeSync::TimeSync()
        : _skipURL(0)
        , _periodicity(0)
        , _client(Core::Service<NTPClient>::Create<Exchange::ITimeSync>())
        , _activity(Core::ProxyType<PeriodicSync>::Create(_client))
        , _sink(this)
    {
    }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

    /* virtual */ TimeSync::~TimeSync()
    {
        _client->Release();
    }

    /* virtual */ const string TimeSync::Initialize(PluginHost::IShell* service)
    {
        Config config;
        config.FromString(service->ConfigLine());
        string version = service->Version();
        _skipURL = static_cast<uint16_t>(service->WebPrefix().length());
        _periodicity = config.Periodicity.Value() * 60 /* minutes */ * 60 /* seconds */ * 1000 /* milliSeconds */;

        NTPClient::SourceIterator index(config.Sources.Elements());

        static_cast<NTPClient*>(_client)->Initialize(index, config.Retries.Value(), config.Interval.Value());

        _sink.Initialize(service, _client);

        // On success return empty, to indicate there is no error text.
        return _T("");
    }

    /* virtual */ void TimeSync::Deinitialize(PluginHost::IShell* service)
    {
        PluginHost::WorkerPool::Instance().Revoke(_activity);
        _sink.Deinitialize();
    }

    /* virtual */ string TimeSync::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void TimeSync::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response>
    TimeSync::Process(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(
            Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL),
            false,
            '/');

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        if (request.Verb == Web::Request::HTTP_GET) {
            Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
            uint64_t syncTime(_client->SyncTime());

            response->TimeSource = _client->Source();
            response->SyncTime = (syncTime == 0 ? _T("invalid time") : Core::Time(syncTime).ToRFC1123(true));

            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::proxy_cast<Web::IBody>(response));
        } else if (request.Verb == Web::Request::HTTP_POST) {

            if (index.Next()) {
                if (index.Current() == "Sync") {
                    _client->Synchronize();
                }
            }
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Unsupported request for the [TimeSync] service.");
        }

        return result;
    }

    void TimeSync::SyncedTime(const uint64_t time)
    {
        Core::Time newTime(time);

        TRACE(Trace::Information, (_T("Syncing time to %s."), newTime.ToRFC1123(false).c_str()));

        Core::SystemInfo::Instance().SetTime(newTime);

        if (_periodicity != 0) {
            Core::Time newSyncTime(Core::Time::Now());

            newSyncTime.Add(_periodicity);

            // Seems we are synchronised with the time. Schedule the next timesync.
            TRACE_L1("Waking up again at %s.", newSyncTime.ToRFC1123(false).c_str());
            PluginHost::WorkerPool::Instance().Schedule(newSyncTime, _activity);
        }
    }
} // namespace Plugin
} // namespace WPEFramework
