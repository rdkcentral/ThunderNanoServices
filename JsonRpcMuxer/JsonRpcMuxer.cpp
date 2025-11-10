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
        for (auto& entry : _batches) {
            entry.Abort();
        }
        for (auto& entry : _batches) {
            entry.WaitForIdle(Core::infinite);
        }

        _job.Revoke();

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
            result = Core::ERROR_UNKNOWN_METHOD;
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

                _batches.emplace_back(*this, parallel, messages, channelId, responseId, token, _timeout);

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
