#include "Monitor.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(Monitor, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<Monitor::Data> > > jsonBodyDataFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<Monitor::Data::MetaData> > jsonMemoryBodyDataFactory(2);

    /* virtual */ const string Monitor::Initialize(PluginHost::IShell* service)
    {

        _config.FromString(service->ConfigLine());

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        Core::JSON::ArrayType<Config::Entry>::Iterator index(_config.Observables.Elements());

        // Create a list of plugins to monitor..
        _monitor->Open(service, index);

        // During the registartion, all Plugins, currently active are reported to the sink.
        service->Register(_monitor);

        // On succes return a name as a Callsign to be used in the URL, after the "service"prefix
        return (_T(""));
    }

    /* virtual */ void Monitor::Deinitialize(PluginHost::IShell* service)
    {

        _monitor->Close();

        service->Unregister(_monitor);
    }

    /* virtual */ string Monitor::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }

    /* virtual */ void Monitor::Inbound(Web::Request& /* request */)
    {
        // There are no requests coming in with a body that require a JSON body. So continue without action here.
    }

    // <GET> ../				Get all Memory Measurments
    // <GET> ../<Callsign>		Get the Memory Measurements for Callsign
    // <PUT> ../<Callsign>		Reset the Memory measurements for Callsign
    /* virtual */ Core::ProxyType<Web::Response> Monitor::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        // If there is an entry, the first one will alwys be a '/', skip this one..
        index.Next();

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        if (request.Verb == Web::Request::HTTP_GET) {
            // Let's list them all....
            if (index.Next() == false) {
                if (_monitor->Length() > 0) {
                    Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<Monitor::Data> > > response(jsonBodyDataFactory.Element());

                    _monitor->Snapshot(*response);

                    result->Body(Core::proxy_cast<Web::IBody>(response));
                }
            }
            else {
                MetaData memoryInfo;

                // Seems we only want 1 name
                if (_monitor->Snapshot(index.Current().Text(), memoryInfo) == true) {
                    Core::ProxyType<Web::JSONBodyType<Monitor::Data::MetaData> > response(jsonMemoryBodyDataFactory.Element());

                    *response = memoryInfo;

                    result->Body(Core::proxy_cast<Web::IBody>(response));
                }
            }

            result->ContentType = Web::MIME_JSON;
        }
        else if ((request.Verb == Web::Request::HTTP_PUT) && (index.Next() == true)) {
            MetaData memoryInfo;

            // Seems we only want 1 name
            if (_monitor->Reset(index.Current().Text(), memoryInfo) == true) {
                Core::ProxyType<Web::JSONBodyType<Monitor::Data::MetaData> > response(jsonMemoryBodyDataFactory.Element());

                *response = memoryInfo;

                result->Body(Core::proxy_cast<Web::IBody>(response));
            }

            result->ContentType = Web::MIME_JSON;
        }
        else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T(" could not handle your request.");
        }

        return (result);
    }
}
}
