/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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

#include "JsonRpcMuxer.h"

namespace Thunder {
namespace Plugin {
    namespace {
        static Metadata<JsonRpcMuxer> metadata(
            PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_PATCH,
            {}, {}, {});
    }

    const string JsonRpcMuxer::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        _service = service;
        _service->AddRef();

        _dispatch = service->QueryInterfaceByCallsign<PluginHost::IDispatcher>("Controller");
        ASSERT(_dispatch != nullptr);
        _dispatch->AddRef();

        return string();
    }

    void JsonRpcMuxer::Deinitialize(PluginHost::IShell* /*service*/)
    {
        _processor.Stop();
        _processor.Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED, Core::infinite);

        if (_dispatch != nullptr) {
            _dispatch->Release();
            _dispatch = nullptr;
        }

        if (_service != nullptr) {
            _service->Release();
            _service = nullptr;
        }
    }

    string JsonRpcMuxer::Information() const
    {
        return string();
    }

    bool JsonRpcMuxer::Attach(PluginHost::Channel& channel)
    {
        Web::ProtocolsArray protocols = channel.Protocols();
        if (std::find(protocols.begin(), protocols.end(), string(_T("json"))) != protocols.end()) {
            if (_activeWebSocket == 0) {
                _activeWebSocket = channel.Id();
                return true; // Accept first connection
            }
            // Reject - already have a connection
            return false;
        }
        return false;
    }

    void JsonRpcMuxer::Detach(PluginHost::Channel& channel)
    {
        if (_activeWebSocket == channel.Id()) {
            _activeWebSocket = 0; // Clear when active connection disconnects
        }
    }

    Core::hresult JsonRpcMuxer::Invoke(const uint32_t channelId, const uint32_t id, const string& token,
        const string& method, const string& parameters, string& response)
    {
        uint32_t result(Core::ERROR_NONE);

        if (_dispatch == nullptr) {
            result = Core::ERROR_UNAVAILABLE;
            response = _T("Dispatch is offline");
        } else {
            Request request(channelId, id, token);
            Core::OptionalType<Core::JSON::Error> parseError;

            if (!request.FromString(parameters, parseError) || parseError.IsSet()) {
                result = Core::ERROR_PARSE_FAILURE;
                response = parseError.IsSet() ? parseError.Value().Message() : _T("Failed to parse message array");
            } else if (request.Length() == 0) {
                result = Core::ERROR_BAD_REQUEST;
                response = _T("Empty message array");
            } else {
                result = ~0; // Keep channel open while processing
                _processor.Add(std::move(request));
            }
        }

        return result;
    }

    uint32_t JsonRpcMuxer::Process(const Request& request)
    {
        Core::JSON::ArrayType<Core::JSONRPC::Message>::ConstIterator it = request.Elements();
        Core::JSON::ArrayType<Response> responseArray;

        while (it.Next()) {
            const Core::JSONRPC::Message& message = it.Current();
            string output;

            uint32_t invokeResult = _dispatch->Invoke(
                request.ChannelId(),
                message.Id.Value(),
                request.Token(),
                message.Designator.Value(),
                message.Parameters.Value(),
                output);

            Response& responseMsg = responseArray.Add();
            responseMsg.Id = message.Id.Value();

            if (invokeResult == Core::ERROR_NONE) {
                responseMsg.Result = output;
            } else {
                responseMsg.Error.SetError(invokeResult);
                if (!output.empty()) {
                    responseMsg.Error.Text = output;
                }
            }
        }

        string response;
        responseArray.ToString(response);

        return SendResponse(request, response);
    }

    uint32_t JsonRpcMuxer::SendResponse(const Request& request, const string& response)
    {
        if (response.empty()) {
            return Core::ERROR_UNAVAILABLE;
        }

        Core::ProxyType<Core::JSONRPC::Message> message(PluginHost::IFactories::Instance().JSONRPC());
        if (!message.IsValid()) {
            return Core::ERROR_BAD_REQUEST;
        }

        message->Id = request.ResponseId();
        message->Result = response;

        return _service->Submit(request.ChannelId(), Core::ProxyType<Core::JSON::IElement>(message));
    }

    Core::ProxyType<Core::JSON::IElement> JsonRpcMuxer::Inbound(const string& /* identifier */)
    {
        return Core::ProxyType<Core::JSON::IElement>(PluginHost::IFactories::Instance().JSONRPC());
    }

    Core::ProxyType<Core::JSON::IElement> JsonRpcMuxer::Inbound(const uint32_t channelId, const Core::ProxyType<Core::JSON::IElement>& element)
    {
        if (_activeWebSocket == channelId) {
            Core::ProxyType<Core::JSONRPC::Message> message(element);

            if (message.IsValid()) {
                Request request(channelId, message->Id.Value(), "");
                Core::OptionalType<Core::JSON::Error> parseError;

                if (request.FromString(message->Parameters.Value(), parseError) && request.Length() > 0) {
                    _processor.Add(std::move(request));
                } else {
                    SendResponse(request, parseError.IsSet() ? parseError.Value().Message() : _T("Invalid batch"));
                }
            }
        }

        return Core::ProxyType<Core::JSON::IElement>();
    }
}
}