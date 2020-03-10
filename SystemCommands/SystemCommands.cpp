/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
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

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
        index.Next();

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported request for the [SystemCommands] service.");

        if ((request.Verb == Web::Request::HTTP_PUT || request.Verb == Web::Request::HTTP_POST) && index.Next()) {

            if (index.Current().Text() == "USBReset") {
                string device;
                Core::URL::KeyValue options(request.Query.Value());
                if (options.Exists(_T("device"), true) == true) {
                    constexpr int kDeviceNameMaxLength = 255;
                    std::array<char, kDeviceNameMaxLength> deviceName {0};
                    device = options[_T("device")].Text();
                    Core::URL::Decode(device.c_str(), device.length(), deviceName.data(), deviceName.size());
                    device = deviceName.data();
                }
                if (!device.empty()) {
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
