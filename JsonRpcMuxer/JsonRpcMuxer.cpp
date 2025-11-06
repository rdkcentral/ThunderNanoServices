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
        _adminLock.Lock();
        _shuttingDown = true;
        _adminLock.Unlock();
        
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
        _adminLock.Lock();
        uint32_t batchId = _batchCounter++;
        _adminLock.Unlock();

        TRACE(Trace::Information, (_T("Processing batch, batchId=%u, count=%u, mode=%s"), batchId, messages.Length(), parallel ? _T("parallel") : _T("sequential")));

        Core::ProxyType<Batch> batch = Core::ProxyType<Batch>::Create(
            Core::ProxyType<JsonRpcMuxer>(*this, *this), batchId, parallel, messages,
            channelId, responseId, token, _timeout);

        if (batch.IsValid() == false) {
            TRACE(Trace::Error, (_T("Failed to create batch object, out of memory?")));
            ReleaseBatchSlot();
        } else {
            _adminLock.Lock();
            _activeBatches.emplace(batchId, batch);
            _adminLock.Unlock();

            batch->Start();
        }
    }

    bool JsonRpcMuxer::TryClaimBatchSlot()
    {
        _adminLock.Lock();
        
        bool success = false;
        if (_activeBatchCount < _maxBatches) {
            ++_activeBatchCount;
            success = true;
        }
        
        _adminLock.Unlock();
        
        return success;
    }

    void JsonRpcMuxer::ReleaseBatchSlot()
    {
        _adminLock.Lock();
        ASSERT(_activeBatchCount > 0);
        --_activeBatchCount;
        _adminLock.Unlock();
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
        _adminLock.Lock();
        bool allDone = _activeBatches.empty();
        _adminLock.Unlock();

        uint32_t waitResult = Core::ERROR_NONE;

        if (!allDone) {
            _batchCompletionEvent.ResetEvent();
            waitResult = _batchCompletionEvent.Lock(maxWaitMs);
        }

        return waitResult;
    }

    void JsonRpcMuxer::RemoveBatchFromRegistry(uint32_t batchId)
    {
        _adminLock.Lock();
        _activeBatches.erase(batchId);
        ASSERT(_activeBatchCount > 0);
        --_activeBatchCount;
        bool allDone = _activeBatches.empty();
        _adminLock.Unlock();

        if (allDone) {
            _batchCompletionEvent.SetEvent();
        }
    }

    void JsonRpcMuxer::CancelBatchesForChannel(uint32_t channelId)
    {
        _adminLock.Lock();

        for (auto& pair : _activeBatches) {
            if (pair.second->IsForChannel(channelId)) {
                pair.second->Cancel();
                TRACE(Trace::Information, (_T("Cancelled batch batchId=%u for channelId=%u"), pair.first, channelId));
            }
        }

        _adminLock.Unlock();
    }

    void JsonRpcMuxer::CancelAllBatches()
    {
        _adminLock.Lock();

        for (auto& pair : _activeBatches) {
            pair.second->Cancel();
            TRACE(Trace::Information, (_T("Cancelled batch batchId=%u"), pair.first));
        }

        _adminLock.Unlock();
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
            
            _adminLock.Lock();
            uint8_t currentCount = _activeBatchCount;
            _adminLock.Unlock();
            
            TRACE(Trace::Warning, (_T("Rejected batch, activeBatches=%u, maxBatches=%u"), currentCount, _maxBatches));
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
}
}