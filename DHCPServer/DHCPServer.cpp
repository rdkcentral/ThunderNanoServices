#include "DHCPServer.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DHCPServer, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<DHCPServer::Data> >         jsonDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<DHCPServer::Data::Server> > jsonServerDataFactory(1);

    #ifdef __WIN32__
    #pragma warning( disable : 4355 )
    #endif
    DHCPServer::DHCPServer()
        : _skipURL(0)
        , _servers() {
    }
    #ifdef __WIN32__
    #pragma warning( default : 4355 )
    #endif

    /* virtual */ DHCPServer::~DHCPServer() {
    }

    /* virtual */ const string DHCPServer::Initialize(PluginHost::IShell* service)
    {
        string result;
        Config config;
        config.FromString(service->ConfigLine());

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

	Core::NodeId dns (config.DNS.Value().c_str());
        Core::JSON::ArrayType<Config::Server>::Iterator index (config.Servers.Elements());

        while (index.Next() == true) {
            if (index.Current().Interface.IsSet() == true) {
                _servers.emplace(std::piecewise_construct, 
                    std::make_tuple(index.Current().Interface.Value()), 
                    std::make_tuple(
                        config.Name.Value(),
                        index.Current().Interface.Value(),
                        index.Current().PoolStart.Value(),
                        index.Current().PoolSize.Value(),
			index.Current().Router.Value(),
			dns));
            }
        }

        // On success return empty, to indicate there is no error text.
        return (result);
    }

    /* virtual */ void DHCPServer::Deinitialize(PluginHost::IShell* service)
    {
	std::map<const string, DHCPServerImplementation>::iterator index( _servers.begin());

        while (index != _servers.end()) {
            index->second.Close();
            index++;
        }

        _servers.clear();
    }

    /* virtual */ string DHCPServer::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void DHCPServer::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response>
    DHCPServer::Process(const Web::Request& request) {

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(
            Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL),
            false,
            '/');

        // By definition skip the first slash.
        index.Next();

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported request for the [DHCPServer] service.");

        if (request.Verb == Web::Request::HTTP_GET) {


            if (index.Next() == true) {
	        std::map<const string, DHCPServerImplementation>::iterator server (_servers.find(index.Current().Text()));

                if (server != _servers.end()) {
                    Core::ProxyType<Data::Server> info (jsonDataFactory.Element());

                    info->Set (server->second);
                    result->Body(info);
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = "OK";
                }
            }
            else {
                Core::ProxyType<Data> info (jsonDataFactory.Element());
	        std::map<const string, DHCPServerImplementation>::iterator servers (_servers.begin());

                while (servers != _servers.end()) {

                    info->Servers.Add().Set(servers->second);                    
                    servers++;
                }

                result->Body(info);
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";
            }
        }
        else if (request.Verb == Web::Request::HTTP_PUT) {

            if (index.Next() == true) {
	        std::map<const string, DHCPServerImplementation>::iterator server (_servers.find(index.Current().Text()));

                if ( (server != _servers.end()) && (index.Next() == true) )  {

                    if (index.Current() == _T("Activate")) {

                        if (server->second.Open() == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = "OK";
                        }
                        else {
                            result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                            result->Message = "Could not activate the server";
                        }
                    }
                    else if (index.Current() == _T("Deactivate")) {

                        if (server->second.Close() == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = "OK";
                        }
                        else {
                            result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                            result->Message = "Could not activate the server";
                        }
                    }
                }
            }
        }

        return result;
    }

} // namespace Plugin
} // namespace WPEFramework
