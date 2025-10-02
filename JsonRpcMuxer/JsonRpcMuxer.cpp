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

        TRACE(Trace::Information, (_T("Initialized successfully")));
        return string();
    }

    void JsonRpcMuxer::Deinitialize(PluginHost::IShell* /*service*/)
    {
        TRACE(Trace::Information, (_T("Deinitializing...")));

        if (_dispatch != nullptr) {
            _dispatch->Release();
            _dispatch = nullptr;
        }

        if (_service != nullptr) {
            _service->Release();
            _service = nullptr;
        }

        TRACE(Trace::Information, (_T("Deinitialized")));
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
            Core::JSON::ArrayType<Core::JSONRPC::Message> messages;
            Core::OptionalType<Core::JSON::Error> parseError;

            if (!messages.FromString(parameters, parseError) || parseError.IsSet()) {
                result = Core::ERROR_PARSE_FAILURE;
                response = parseError.IsSet() ? parseError.Value().Message() : _T("Failed to parse message array");
            } else if (messages.Length() == 0) {
                result = Core::ERROR_BAD_REQUEST;
                response = _T("Empty message array");
            } else {
                result = ~0; // Keep channel open
                Process(channelId, id, token, messages);
            }
        }

        return result;
    }

    void JsonRpcMuxer::Process(
        uint32_t channelId,
        uint32_t responseId,
        const string& token,
        Core::JSON::ArrayType<Core::JSONRPC::Message>& messages)
    {
        uint32_t batchId = ++_batchCounter;
        uint32_t messageCount = messages.Length();

        TRACE(Trace::Information, (_T("Processing batch, batchId=%u, count=%u"), batchId, messageCount));

        Core::ProxyType<Job::State> state = Core::ProxyType<Job::State>::Create(messageCount, channelId, responseId, batchId);

        uint32_t index = 0;
        Core::JSON::ArrayType<Core::JSONRPC::Message>::Iterator it = messages.Elements();

        while (it.Next()) {
            Core::ProxyType<Job> job = Core::ProxyType<Job>::Create(*this, token, it.Current(), state, index++);
            Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(job));
        }

        TRACE(Trace::Information, (_T("Batch queued to worker pool, batchId=%u, jobs=%u"), batchId, messageCount));
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
                Core::JSON::ArrayType<Core::JSONRPC::Message> messages;
                Core::OptionalType<Core::JSON::Error> parseError;

                if (!messages.FromString(message->Parameters.Value(), parseError)) {
                    TRACE(Trace::Error, (_T("WebSocket inbound parse failure on channelId=%u: %s"), channelId, parseError.IsSet() ? parseError.Value().Message().c_str() : _T("Unknown error")));

                    // Send error response back to client
                    Core::ProxyType<Core::JSONRPC::Message> errorResponse(PluginHost::IFactories::Instance().JSONRPC());
                    
                    if (errorResponse.IsValid()) {
                        errorResponse->Id = message->Id.Value();
                        errorResponse->Error.SetError(Core::ERROR_PARSE_FAILURE);
                        errorResponse->Error.Text = parseError.IsSet() ? parseError.Value().Message() : _T("Failed to parse message array");
                        _service->Submit(channelId, Core::ProxyType<Core::JSON::IElement>(errorResponse));
                    }
                } else if (messages.Length() == 0) {
                    TRACE(Trace::Error, (_T("WebSocket received empty message array on channelId=%u"), channelId));

                    Core::ProxyType<Core::JSONRPC::Message> errorResponse(PluginHost::IFactories::Instance().JSONRPC());

                    if (errorResponse.IsValid()) {
                        errorResponse->Id = message->Id.Value();
                        errorResponse->Error.SetError(Core::ERROR_BAD_REQUEST);
                        errorResponse->Error.Text = _T("Empty message array");
                        _service->Submit(channelId, Core::ProxyType<Core::JSON::IElement>(errorResponse));
                    }
                } else {
                    Process(channelId, message->Id.Value(), "", messages);
                }
            } else {
                TRACE(Trace::Error, (_T("Invalid JSONRPC message received on channelId=%u"), channelId));
            }
        } else {
            TRACE(Trace::Error, (_T("Received message from inactive WebSocket, channelId=%u"), channelId));
        }

        return Core::ProxyType<Core::JSON::IElement>();
    }
}
}