#include "SystemCommands.h"
#include "SystemCommandsImplementation.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(SystemCommands, 1, 0);

    const string SystemCommands::Initialize(PluginHost::IShell* service)
    {
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        return string();
    }

    void SystemCommands::Deinitialize(PluginHost::IShell* service)
    {
    }

    string SystemCommands::Information() const
    {
        return string();
    }

    void SystemCommands::Inbound(Web::Request& request)
    {
    }

    Core::ProxyType<Web::Response> SystemCommands::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
        index.Next();

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported request for the [SystemCommands] service.");

        if ((request.Verb == Web::Request::HTTP_PUT || request.Verb == Web::Request::HTTP_POST) && index.Next()) {

            if (index.Current().Text() == "USBReset") {
                if (index.Next()) {
                    string device = index.Current().Text();
                    uint32_t status = USBReset(device);
                    if (status != Core::ERROR_NONE) {
                        result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        result->Message = _T("ResetUSB failed for ") + device;
                    } else {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = "OK";
                    }
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("No device name given to USBReset system command.");
                }
            }
        }

        return result;
    }

    uint32_t SystemCommands::USBReset(const std::string& device)
    {
        return _implemetation.USBReset(device);
    }

} // namespace Plugin
} // namespace WPEFramework
