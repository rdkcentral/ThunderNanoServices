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

        // Signal shutdown and cancel all batches
        _shuttingDown.store(true);
        CancelAllBatches();

        if (WaitForBatchCompletion() == Core::ERROR_TIMEDOUT) {
            TRACE(Trace::Warning, (_T("Timed out waiting for batches to complete during shutdown")));
        } else {
            TRACE(Trace::Information, (_T("All batches completed successfully")));
        }

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

    void JsonRpcMuxer::Process(uint32_t channelId, uint32_t responseId, const string& token,
        Core::JSON::ArrayType<Core::JSONRPC::Message>& messages, bool parallel)
    {
        uint32_t batchId = ++_batchCounter;

        TRACE(Trace::Information, (_T("Processing batch, batchId=%u, count=%u, mode=%s"), batchId, messages.Length(), parallel ? _T("parallel") : _T("sequential")));

        Core::ProxyType<Batch> batch = Core::ProxyType<Batch>::Create(
            Core::ProxyType<JsonRpcMuxer>(*this, *this), batchId, parallel, messages,
            channelId, responseId, token, _timeout);

        if (batch.IsValid() == false) {
            TRACE(Trace::Error, (_T("Failed to create batch object, out of memory?")));
        } else {
            _batchesLock.Lock();
            _activeBatches.emplace(batchId, batch);
            _batchesLock.Unlock();

            batch->Start();
        }
    }

    bool JsonRpcMuxer::TryClaimBatchSlot()
    {
        uint8_t current = _activeBatchCount.fetch_add(1);

        if (current >= _maxBatches) {
            // Oops, we exceeded the limit, give it back
            --_activeBatchCount;
            return false;
        }

        return true;
    }

    uint32_t JsonRpcMuxer::SendResponse(const uint32_t Id, const Core::ProxyType<Core::JSON::IElement>& response)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_service) {
            result = _service->Submit(Id, response);
        }

        return result;
    }

    uint32_t JsonRpcMuxer::WaitForBatchCompletion(const uint32_t maxWaitMs)
    {
        _batchesLock.Lock();
        bool allDone = _activeBatches.empty();
        _batchesLock.Unlock();

        uint32_t waitResult = Core::ERROR_NONE;

        if (!allDone) {
            _batchCompletionEvent.ResetEvent();
            waitResult = _batchCompletionEvent.Lock(maxWaitMs);
        }

        return waitResult;
    }

    void JsonRpcMuxer::RemoveBatchFromRegistry(uint32_t batchId)
    {
        _batchesLock.Lock();
        _activeBatches.erase(batchId);
        --_activeBatchCount;
        bool allDone = _activeBatches.empty();
        _batchesLock.Unlock();

        if (allDone) {
            _batchCompletionEvent.SetEvent();
        }
    }

    void JsonRpcMuxer::CancelBatchesForChannel(uint32_t channelId)
    {
        _batchesLock.Lock();

        for (auto& pair : _activeBatches) {
            if (pair.second->ChannelId() == channelId) {
                pair.second->Cancel();
                TRACE(Trace::Information, (_T("Cancelled batch batchId=%u for channelId=%u"), pair.first, channelId));
            }
        }

        _batchesLock.Unlock();
    }

    void JsonRpcMuxer::CancelAllBatches()
    {
        _batchesLock.Lock();

        for (auto& pair : _activeBatches) {
            pair.second->Cancel();
            TRACE(Trace::Information, (_T("Cancelled batch batchId=%u"), pair.first));
        }

        _batchesLock.Unlock();
    }

    bool JsonRpcMuxer::IsChannelValid(uint32_t channelId) const
    {
        if (_activeWebSocket != 0) {
            return (channelId == _activeWebSocket);
        }

        return true;
    }

    uint32_t JsonRpcMuxer::ValidateBatch(const Core::JSON::ArrayType<Core::JSONRPC::Message>& messages, string& errorMessage)
    {
        // Empty batch check
        if (messages.Length() == 0) {
            errorMessage = _T("Empty message array");
            TRACE(Trace::Warning, (_T("Empty message array received")));
            return Core::ERROR_BAD_REQUEST;
        }

        // Size limit check
        if (messages.Length() > _maxBatchSize) {
            errorMessage = _T("Batch size exceeds maximum allowed (") + Core::NumberType<uint16_t>(_maxBatchSize).Text() + _T(")");
            TRACE(Trace::Warning, (_T("Batch size %u exceeds maximum %u"), messages.Length(), _maxBatchSize));
            return Core::ERROR_BAD_REQUEST;
        }

        // Concurrency limit check
        if (!TryClaimBatchSlot()) {
            errorMessage = _T("Too many concurrent batches, try again later");
            TRACE(Trace::Warning, (_T("Rejected batch, activeBatches=%u, maxBatches=%u"), _activeBatchCount.load(), _maxBatches));
            return Core::ERROR_UNAVAILABLE;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult JsonRpcMuxer::Invoke(const uint32_t channelId, const uint32_t id, const string& token,
        const string& designator, const string& parameters, string& response)
    {
        const string method = Core::JSONRPC::Message::Method(designator);

        uint32_t result(Core::ERROR_NONE);

        if (_dispatch == nullptr) {
            result = Core::ERROR_UNAVAILABLE;
            response = _T("Dispatch is offline");
            TRACE(Trace::Error, (_T("Invoke failed: Dispatch unavailable")));
        } else if (method != _T("parallel") && method != _T("sequential")) {
            result = Core::ERROR_BAD_REQUEST;
            response = _T("Invalid method, must be 'parallel' or 'sequential'");
            TRACE(Trace::Warning, (_T("Invalid method: %s"), method.c_str()));
        } else {
            Core::JSON::ArrayType<Core::JSONRPC::Message> messages;
            Core::OptionalType<Core::JSON::Error> parseError;

            if (!messages.FromString(parameters, parseError) || parseError.IsSet()) {
                result = Core::ERROR_PARSE_FAILURE;
                response = parseError.IsSet() ? parseError.Value().Message() : _T("Failed to parse message array");
                TRACE(Trace::Error, (_T("Parse failure: %s"), response.c_str()));
            } else {
                uint32_t validationResult = ValidateBatch(messages, response);

                if (validationResult != Core::ERROR_NONE) {
                    result = validationResult;
                } else {
                    result = ~0; // Keep channel open
                    Process(channelId, id, token, messages, (method == _T("parallel")));
                }
            }
        }

        return result;
    }

#ifdef ENABLE_WEBSOCKET_CONNECTION
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
            CancelBatchesForChannel(channel.Id());
            _activeWebSocket = 0;
        }
    }

    Core::ProxyType<Core::JSON::IElement> JsonRpcMuxer::Inbound(const string& /* identifier */)
    {
        return Core::ProxyType<Core::JSON::IElement>(PluginHost::IFactories::Instance().JSONRPC());
    }

    Core::ProxyType<Core::JSON::IElement> JsonRpcMuxer::Inbound(const uint32_t channelId,
        const Core::ProxyType<Core::JSON::IElement>& element)
    {
        if (_activeWebSocket == channelId) {
            Core::ProxyType<Core::JSONRPC::Message> message(element);

            if (message.IsValid()) {
                if (message->Method() != _T("parallel") && message->Method() != _T("sequential")) {
                    SendErrorResponse(channelId, message->Id.Value(), Core::ERROR_BAD_REQUEST, _T("Invalid method, must be 'parallel' or 'sequential'"));
                } else {
                    Core::JSON::ArrayType<Core::JSONRPC::Message> messages;
                    Core::OptionalType<Core::JSON::Error> parseError;

                    if (!messages.FromString(message->Parameters.Value(), parseError)) {
                        TRACE(Trace::Error, (_T("WebSocket inbound parse failure on channelId=%u: %s"), channelId, parseError.IsSet() ? parseError.Value().Message().c_str() : _T("Unknown error")));
                        SendErrorResponse(channelId, message->Id.Value(), Core::ERROR_PARSE_FAILURE, parseError.IsSet() ? parseError.Value().Message() : _T("Failed to parse message array"));
                    } else {
                        string errorMsg;
                        uint32_t validationResult = ValidateBatch(messages, errorMsg);

                        if (validationResult != Core::ERROR_NONE) {
                            SendErrorResponse(channelId, message->Id.Value(), validationResult, errorMsg);
                        } else {
                            Process(channelId, message->Id.Value(), "", messages, (message->Method() == _T("parallel")));
                        }
                    }
                }
            } else {
                TRACE(Trace::Warning, (_T("Invalid JSONRPC message received on channelId=%u"), channelId));
            }
        } else {
            TRACE(Trace::Warning, (_T("Received message from inactive WebSocket, channelId=%u"), channelId));
        }

        return Core::ProxyType<Core::JSON::IElement>();
    }
#endif
}
}