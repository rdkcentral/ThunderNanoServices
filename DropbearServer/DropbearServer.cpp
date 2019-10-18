#include "DropbearServer.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DropbearServer, 1, 0);

    const string DropbearServer::Initialize(PluginHost::IShell* service)
    {
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        return string();
    }

    void DropbearServer::Deinitialize(PluginHost::IShell* service)
    {
    }

    string DropbearServer::Information() const
    {
        return string();
    }

    void DropbearServer::Inbound(Web::Request& request)
    {
    }

    Core::ProxyType<Web::Response> DropbearServer::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
        index.Next();

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported request for the [DropbearServer] service9.");

        if ((request.Verb == Web::Request::HTTP_PUT || request.Verb == Web::Request::HTTP_POST) && index.Next()) {

            if (index.Current().Text() == "StartService") {
                string argv;
                Core::URL::KeyValue options(request.Query.Value());
                if (options.Exists(_T("port"), true) == true) {
                    constexpr int kPortNameMaxLength = 255;
                    std::array<char, kPortNameMaxLength> portNumber {0};
                    argv = options[_T("port")].Text();
                    Core::URL::Decode(argv.c_str(), argv.length(), portNumber.data(), portNumber.size());
                    argv = portNumber.data();
                }
                if (!argv.empty()) {
        	    TRACE(Trace::Information, (_T("Entered into StartService method")));
                    uint32_t status = StartService(argv);
                    if (status != Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        result->Message = _T("Dropbear StartService failed for ") + argv;
                    } else {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = "OK";
                    }
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("No port number given to Dropbear StartService.");
                }
            }
	    else if (index.Current().Text() == "StopService") {
        	TRACE(Trace::Information, (_T("Entered into StopService method")));
                uint32_t status = StopService();
                if (status != Core::ERROR_NONE) {
                    result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                    result->Message = _T("Dropbear StopService failed for ");
                } else {
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = "OK";
                }
            }
	    else {
                result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                result->Message = _T("Unavailable method");
            }
        }

        return result;
    }

    uint32_t DropbearServer::StartService(const std::string& port)
    {
        return _implemetation.StartService(port);
    }

    uint32_t DropbearServer::StopService()
    {
        return _implemetation.StopService();
    }

} // namespace Plugin
} // namespace WPEFramework
