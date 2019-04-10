#include "SecurityOfficer.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(SecurityOfficer, 1, 0);

    static Core::ProxyPoolType<Web::Response> responseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<SecurityOfficer::Data<>>> jsonResponseFactory(4);

    static const uint16_t NTPPort = 123;
    static constexpr auto* kTimeMethodName = _T("time");
    static constexpr auto* kSyncMethodName = _T("synchronize");

#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
    SecurityOfficer::SecurityOfficer()
        : _skipURL(0)
        , _periodicity(0)
        , _sink(this)
    {
        Register<void, Data<Core::JSON::DecUInt64>>(kTimeMethodName, &SecurityOfficer::time, this);
        Register<void, void>(kSyncMethodName, &SecurityOfficer::synchronize, this);
    }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

    /* virtual */ SecurityOfficer::~SecurityOfficer()
    {
        Unregister(kTimeMethodName);
        Unregister(kSyncMethodName);
    }

    /* virtual */ const string SecurityOfficer::Initialize(PluginHost::IShell* service)
    {
        Config config;
        config.FromString(service->ConfigLine());
        string version = service->Version();
        _skipURL = static_cast<uint16_t>(service->WebPrefix().length());
        _periodicity = config.Periodicity.Value() * 60 /* minutes */ * 60 /* seconds */ * 1000 /* milliSeconds */;

        _sink.Initialize(service, _client);

        // On success return empty, to indicate there is no error text.
        return _T("");
    }

    /* virtual */ void SecurityOfficer::Deinitialize(PluginHost::IShell* service)
    {
        _sink.Deinitialize();
    }

    /* virtual */ string SecurityOfficer::Information() const
    {
        // No additional info to report.
        return (string());
    }

    uint32_t SecurityOfficer::time(Data<Core::JSON::DecUInt64>& response)
    {
        response.TimeSource = _client->Source();
        response.SyncTime = _client->SyncTime();
        return Core::ERROR_NONE;
    }

    uint32_t SecurityOfficer::synchronize()
    {
        return _client->Synchronize();
    }

} // namespace Plugin
} // namespace WPEFramework
