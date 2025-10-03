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
        string result;

        ASSERT(service != nullptr);

        _service = service;
        _service->AddRef();

        Config config;
        config.FromString(_service->ConfigLine());

        _maxBatches = config.MaxBatches.Value();
        _maxBatchSize = config.MaxBatchSize.Value();
        _timeout = config.TimeOut.Value();

        TRACE(Trace::Information, (_T("Configuration loaded: maxBatches=%u, maxBatchSize=%u, timeout=%u"), _maxBatches, _maxBatchSize, _timeout));

        _dispatch = service->QueryInterfaceByCallsign<PluginHost::IDispatcher>("Controller");

        if (_dispatch == nullptr) {
            TRACE(Trace::Error, (_T("Failed to acquire Controller dispatcher")));
            result = _T("Failed to acquire Controller dispatcher");
        } else {
            _dispatch->AddRef();
            TRACE(Trace::Information, (_T("Initialized successfully")));
        }

        return result;
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
                TRACE(Trace::Information, (_T("WebSocket connected, channelId=%u"), channel.Id()));
                return true;
            }
            TRACE(Trace::Warning, (_T("WebSocket rejected, already connected on channelId=%u"), _activeWebSocket));
            return false;
        }
        return false;
    }

    void JsonRpcMuxer::Detach(PluginHost::Channel& channel)
    {
        if (_activeWebSocket == channel.Id()) {
            TRACE(Trace::Information, (_T("WebSocket disconnected, channelId=%u"), channel.Id()));
            _activeWebSocket = 0;
        }
    }

    Core::hresult JsonRpcMuxer::Invoke(const uint32_t channelId, const uint32_t id, const string& token,
        const string& method, const string& parameters, string& response)
    {
        uint32_t result(Core::ERROR_NONE);

        if (_dispatch == nullptr) {
            result = Core::ERROR_UNAVAILABLE;
            response = _T("Dispatch is offline");
            TRACE(Trace::Error, (_T("Invoke failed: Dispatch unavailable")));
        } else {
            Core::JSON::ArrayType<Core::JSONRPC::Message> messages;
            Core::OptionalType<Core::JSON::Error> parseError;

            if (!messages.FromString(parameters, parseError) || parseError.IsSet()) {
                result = Core::ERROR_PARSE_FAILURE;
                response = parseError.IsSet() ? parseError.Value().Message() : _T("Failed to parse message array");
                TRACE(Trace::Error, (_T("Parse failure: %s"), response.c_str()));
            } else if (messages.Length() == 0) {
                result = Core::ERROR_BAD_REQUEST;
                response = _T("Empty message array");
                TRACE(Trace::Warning, (_T("Empty message array received")));
            } else if (messages.Length() > _maxBatchSize) {
                result = Core::ERROR_BAD_REQUEST;
                response = _T("Batch size exceeds maximum allowed (") + Core::NumberType<uint16_t>(_maxBatchSize).Text() + _T(")");
                TRACE(Trace::Warning, (_T("Batch size %u exceeds maximum %u"), messages.Length(), _maxBatchSize));
            } else if (TryClaimBatchSlot() == false) {
                result = Core::ERROR_UNAVAILABLE;
                response = _T("Too many concurrent batches, try again later");
                TRACE(Trace::Warning, (_T("Rejected batch, activeBatches=%u, maxBatches=%u"), _activeBatches.load(), _maxBatches));
            } else {
                result = ~0; // Keep channel open
                Process(channelId, id, token, messages);
            }
        }

        return result;
    }

    void JsonRpcMuxer::Process(uint32_t channelId, uint32_t responseId, const string& token, Core::JSON::ArrayType<Core::JSONRPC::Message>& messages)
    {
        uint32_t batchId = ++_batchCounter;
        uint32_t messageCount = messages.Length();

        TRACE(Trace::Information, (_T("Processing batch, batchId=%u, count=%u, channelId=%u, timeout=%u"), batchId, messageCount, channelId, _timeout));
        Core::ProxyType<Job::State> state = Core::ProxyType<Job::State>::Create(messageCount, channelId, responseId, batchId, _timeout);

        uint32_t index = 0;
        Core::JSON::ArrayType<Core::JSONRPC::Message>::Iterator it = messages.Elements();

        while (it.Next()) {
            Core::ProxyType<Job> job = Core::ProxyType<Job>::Create(*this, token, it.Current(), state, index++);
            Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(job));
        }

        TRACE(Trace::Information, (_T("Batch queued to worker pool, batchId=%u, jobs=%u"), batchId, messageCount));
    }

    void JsonRpcMuxer::SendErrorResponse(uint32_t channelId, uint32_t responseId, uint32_t errorCode, const string& errorMessage)
    {
        Core::ProxyType<Core::JSONRPC::Message> errorResponse(
            PluginHost::IFactories::Instance().JSONRPC());

        if (errorResponse.IsValid()) {
            errorResponse->Id = responseId;
            errorResponse->Error.SetError(errorCode);
            errorResponse->Error.Text = errorMessage;
            _service->Submit(channelId, Core::ProxyType<Core::JSON::IElement>(errorResponse));
        }
    }

    void JsonRpcMuxer::SendTimeoutResponse(uint32_t channelId, uint32_t responseId, uint32_t batchId)
    {
        TRACE(Trace::Error, (_T("Batch timeout, batchId=%u"), batchId));
        SendErrorResponse(channelId, responseId, Core::ERROR_TIMEDOUT, _T("Batch processing timed out"));
    }

    Core::ProxyType<Core::JSON::IElement> JsonRpcMuxer::Inbound(const string& /* identifier */)
    {
        return Core::ProxyType<Core::JSON::IElement>(PluginHost::IFactories::Instance().JSONRPC());
    }

    bool JsonRpcMuxer::TryClaimBatchSlot()
    {
        uint8_t current = _activeBatches.fetch_add(1);

        if (current >= _maxBatches) {
            // Oops, we exceeded the limit, give it back
            --_activeBatches;
            return false;
        }

        return true;
    }

    Core::ProxyType<Core::JSON::IElement> JsonRpcMuxer::Inbound(const uint32_t channelId,
        const Core::ProxyType<Core::JSON::IElement>& element)
    {
        if (_activeWebSocket == channelId) {
            Core::ProxyType<Core::JSONRPC::Message> message(element);
            if (message.IsValid()) {
                Core::JSON::ArrayType<Core::JSONRPC::Message> messages;
                Core::OptionalType<Core::JSON::Error> parseError;

                if (!messages.FromString(message->Parameters.Value(), parseError)) {
                    TRACE(Trace::Error, (_T("WebSocket inbound parse failure on channelId=%u: %s"), channelId, parseError.IsSet() ? parseError.Value().Message().c_str() : _T("Unknown error")));
                    SendErrorResponse(channelId, message->Id.Value(), Core::ERROR_PARSE_FAILURE, parseError.IsSet() ? parseError.Value().Message() : _T("Failed to parse message array"));
                } else if (messages.Length() == 0) {
                    TRACE(Trace::Warning, (_T("WebSocket received empty message array on channelId=%u"), channelId));
                    SendErrorResponse(channelId, message->Id.Value(), Core::ERROR_BAD_REQUEST, _T("Empty message array"));
                } else if (messages.Length() > _maxBatchSize) {
                    TRACE(Trace::Warning, (_T("WebSocket batch size %u exceeds maximum %u on channelId=%u"), messages.Length(), _maxBatchSize, channelId));
                    string errorMsg = _T("Batch size exceeds maximum allowed (") + Core::NumberType<uint16_t>(_maxBatchSize).Text() + _T(")");
                    SendErrorResponse(channelId, message->Id.Value(), Core::ERROR_BAD_REQUEST, errorMsg);
                } else if (TryClaimBatchSlot() == false) {
                    TRACE(Trace::Warning, (_T("WebSocket rejected, activeBatches=%u, maxBatches=%u, channelId=%u"), _activeBatches.load(), _maxBatches, channelId));
                    SendErrorResponse(channelId, message->Id.Value(), Core::ERROR_UNAVAILABLE, _T("Too many concurrent batches, try again later"));
                } else {
                    Process(channelId, message->Id.Value(), "", messages);
                }
            } else {
                TRACE(Trace::Warning, (_T("Invalid JSONRPC message received on channelId=%u"), channelId));
            }
        } else {
            TRACE(Trace::Warning, (_T("Received message from inactive WebSocket, channelId=%u"), channelId));
        }

        return Core::ProxyType<Core::JSON::IElement>();
    }
}
}