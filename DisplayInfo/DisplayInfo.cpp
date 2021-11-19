/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "DisplayInfo.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DisplayInfo, 1, 0);

    static Core::ProxyPoolType<Web::Response> responseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<JsonData::DisplayInfo::DisplayinfoData>> jsonResponseFactory(4);

    /* virtual */ const string DisplayInfo::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_connectionProperties == nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_graphicsProperties == nullptr);
        ASSERT(_hdrProperties == nullptr);


        string message;
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        _connectionProperties = service->Root<Exchange::IConnectionProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        if (_connectionProperties != nullptr) {

            Exchange::IConfiguration* configConnection = _connectionProperties->QueryInterface<Exchange::IConfiguration>();
            if (configConnection != nullptr) {
                configConnection->Configure(service);
                configConnection->Release();
            }

            _graphicsProperties = _connectionProperties->QueryInterface<Exchange::IGraphicsProperties>();
            if (_graphicsProperties == nullptr) {
                message = _T("DisplayInfo could not be instantiated. Could not aquire GraphicsProperties interface");
            } else {
                _hdrProperties = _connectionProperties->QueryInterface<Exchange::IHDRProperties>();
                if (_hdrProperties == nullptr) {
                    message = _T("DisplayInfo could not be instantiated. Could not aquire HDRProperties interface");
                } else {
                    _notification.Initialize(_connectionProperties);
                    Exchange::JGraphicsProperties::Register(*this, _graphicsProperties);
                    Exchange::JConnectionProperties::Register(*this, _connectionProperties);
                    Exchange::JHDRProperties::Register(*this, _hdrProperties);
                }
            }
        } else {
            message = _T("DisplayInfo could not be instantiated. Could not aquire ConnectionProperties interface");
        }

        if (message.length() != 0) {
            Deinitialize(service);
        }

        return message;
    }

    void DisplayInfo::Deinitialize(PluginHost::IShell* service) /* override */
    {
        ASSERT(service != nullptr);

        if (_hdrProperties != nullptr) {

            Exchange::JHDRProperties::Unregister(*this);
            Exchange::JConnectionProperties::Unregister(*this);
            Exchange::JGraphicsProperties::Unregister(*this);
            _notification.Deinitialize();

            _hdrProperties->Release();
            _hdrProperties = nullptr;
        }

        if (_graphicsProperties != nullptr) {
            _graphicsProperties->Release();
            _graphicsProperties = nullptr;
        }

        if(_connectionProperties != nullptr) {
            // Stop processing:
            RPC::IRemoteConnection* connection = service->RemoteConnection(_connectionId);

            VARIABLE_IS_NOT_USED uint32_t result = _connectionProperties->Release();
            _connectionProperties = nullptr;

            // It should have been the last reference we are releasing, 
            // so it should endup in a DESTRUCTION_SUCCEEDED, if not we
            // are leaking...
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

            // If this was running in a (container) process...
            if (connection != nullptr) {
                // Lets trigger the cleanup sequence for 
                // out-of-process code. Which will guard 
                // that unwilling processes, get shot if
                // not stopped friendly :-)
                connection->Terminate();
                connection->Release();
            }
        }

        _connectionId = 0;
    }

    string DisplayInfo::Information() const /* override */
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void DisplayInfo::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> DisplayInfo::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        // <GET> - currently, only the GET command is supported, returning system info
        if (request.Verb == Web::Request::HTTP_GET) {

            Core::ProxyType<Web::JSONBodyType<JsonData::DisplayInfo::DisplayinfoData>> response(jsonResponseFactory.Element());

            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

            // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
            index.Next();

            Info(*response);
            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::ProxyType<Web::IBody>(response));
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Unsupported request for the [DisplayInfo] service.");
        }

        return result;
    }

    void DisplayInfo::Info(JsonData::DisplayInfo::DisplayinfoData& displayInfo) const
    {
        uint64_t ram = 0;
        if (_graphicsProperties->TotalGpuRam(ram) == Core::ERROR_NONE) {
            displayInfo.Totalgpuram = ram;
        }
        ram = 0;
        if (_graphicsProperties->FreeGpuRam(ram) == Core::ERROR_NONE) {
            displayInfo.Freegpuram = ram;
        }

        bool status = false;
        if (_connectionProperties->IsAudioPassthrough(status) == Core::ERROR_NONE) {
            displayInfo.Audiopassthrough = status;
        }
        status = false;
        if (_connectionProperties->Connected(status) == Core::ERROR_NONE) {
            displayInfo.Connected = status;
        }

        uint32_t value = 0;
        if (_connectionProperties->Width(value) == Core::ERROR_NONE) {
            displayInfo.Width = value;
        }
        value = 0;
        if (_connectionProperties->Height(value) == Core::ERROR_NONE) {
            displayInfo.Height = value;
        }

        Exchange::IConnectionProperties::HDCPProtectionType hdcpProtection(Exchange::IConnectionProperties::HDCPProtectionType::HDCP_Unencrypted);
        if ((const_cast<const Exchange::IConnectionProperties*>(_connectionProperties))->HDCPProtection(hdcpProtection) == Core::ERROR_NONE) {
            displayInfo.Hdcpprotection = static_cast<JsonData::DisplayInfo::DisplayinfoData::HdcpprotectionType>(hdcpProtection);
        }

        Exchange::IHDRProperties::HDRType hdrType(Exchange::IHDRProperties::HDRType::HDR_OFF);
        if (_hdrProperties->HDRSetting(hdrType) == Core::ERROR_NONE) {
            displayInfo.Hdrtype = static_cast<JsonData::DisplayInfo::DisplayinfoData::HdrtypeType>(hdrType);
        }
    }

} // namespace Plugin
} // namespace WPEFramework
