#pragma once

#include "Module.h"
#include <atomic>
#include <map>

namespace Thunder {
namespace Plugin {
    class JsonRpcMuxer :
        public PluginHost::IPlugin,
        public PluginHost::JSONRPC {
    private:
        class Config : public Core::JSON::Container {
        public:
            Config(Config&&) = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , TimeOut(Core::infinite)
                , MaxBatchSize(10)
                , MaxBatches(5)
            {
                Add(_T("timeout"), &TimeOut);
                Add(_T("maxbatchsize"), &MaxBatchSize);
                Add(_T("maxbatches"), &MaxBatches);
            }
            ~Config() override = default;

        public:
            Core::JSON::DecUInt32 TimeOut;
            Core::JSON::DecUInt8 MaxBatchSize;
            Core::JSON::DecUInt8 MaxBatches;
        };

        class Response : public Core::JSON::Container {
        public:
            ~Response() override = default;

            Response()
                : Core::JSON::Container()
                , Id(~0)
                , Result(false)
                , Error()
            {
                Add(_T("id"), &Id);
                Add(_T("result"), &Result);
                Add(_T("error"), &Error);
                Clear();
            }

            Response(const Response& copy)
                : Core::JSON::Container()
                , Id(copy.Id)
                , Result(copy.Result)
                , Error(copy.Error)
            {
                Add(_T("id"), &Id);
                Add(_T("result"), &Result);
                Add(_T("error"), &Error);
            }

            Response(Response&& move) noexcept
                : Core::JSON::Container()
                , Id(std::move(move.Id))
                , Result(std::move(move.Result))
                , Error(std::move(move.Error))
            {
                Add(_T("id"), &Id);
                Add(_T("result"), &Result);
                Add(_T("error"), &Error);
            }

            Core::JSON::DecUInt32 Id;
            Core::JSON::String Result;
            Core::JSONRPC::Message::Info Error;
        };

        class Batch {
        private:
            class Job : public Core::IDispatch {
            public:
                Job(Batch* batch, uint32_t index)
                    : _batch(batch)
                    , _index(index)
                {
                }

                Job(const Job&) = delete;
                Job(Job&&) = delete;
                Job& operator=(const Job&) = delete;
                Job& operator=(Job&&) = delete;

                void Dispatch() override
                {
                    _batch->Run(_index);
                }

            private:
                Batch* _batch;
                const uint32_t _index;
            };

        public:
            Batch(
                Core::ProxyType<JsonRpcMuxer> parent,
                uint32_t batchId,
                bool parallel,
                const Core::JSON::ArrayType<Core::JSONRPC::Message>& messages,
                uint32_t channelId,
                uint32_t responseId,
                const string& token,
                uint32_t timeoutMs)
                : _parent(parent)
                , _batchId(batchId)
                , _parallel(parallel)
                , _pendingCount(messages.Length())
                , _messages(messages)
                , _responseLock()
                , _responseArray()
                , _channelId(channelId)
                , _responseId(responseId)
                , _token(token)
                , _timeoutMs(timeoutMs)
                , _startTime(Core::Time::Now().Ticks())
                , _cancelled(false)
            {
                for (uint32_t i = 0; i < _pendingCount; ++i) {
                    _responseArray.Add();
                }
            }

            Batch(const Batch&) = delete;
            Batch(Batch&&) = delete;
            Batch& operator=(const Batch&) = delete;
            Batch& operator=(Batch&&) = delete;
            ~Batch() = default;

            bool IsActive() const
            {
                return !_cancelled.load() && !_parent->_shuttingDown.load() && !IsTimedOut();
            }

            bool IsForChannel(uint32_t channelId) const
            {
                return (_channelId == channelId);
            }

            void Start()
            {
                if (_parallel) {
                    for (uint32_t i = 0; i < _messages.Length(); ++i) {
                        SubmitJob(i);
                    }
                } else if (_messages.Length() > 0) {
                    SubmitJob(0);
                }
            }

            void Cancel() { _cancelled.store(true); }

        private:
            void Run(uint32_t index)
            {
                ASSERT(index < _messages.Length());

                if (IsActive() == false) {
                    _responseLock.Lock();
                    bool isComplete = (--_pendingCount == 0);
                    _responseLock.Unlock();

                    if (isComplete) {
                        FinishBatch();
                    }
                } else {
                    const Core::JSONRPC::Message& message = _messages[index];

                    string output;
                    Core::hresult result = _parent->_dispatch->Invoke(
                        _channelId, message.Id.Value(), _token,
                        message.Designator.Value(), message.Parameters.Value(),
                        output);

                    bool isActive = IsActive();

                    _responseLock.Lock();

                    if (isActive && ((result != static_cast<uint32_t>(~0)) && ((message.Id.IsSet()) || (result != Core::ERROR_NONE)))) {
                        Response& response = _responseArray[index];

                        if (message.Id.IsSet()) {
                            response.Id = message.Id.Value();
                        }

                        if (result == Core::ERROR_NONE) {
                            if (output.empty()) {
                                response.Result.Null(true);
                            } else {
                                response.Result = output;
                            }
                        } else {
                            response.Error.SetError(result);
                            if (!output.empty()) {
                                response.Error.Text = output;
                            }
                        }
                    }

                    bool isComplete = (--_pendingCount == 0);
                    uint32_t nextIndex = index + 1;

                    _responseLock.Unlock();

                    // Handle completion or chain next job
                    if (isComplete) {
                        FinishBatch();
                    } else if (!_parallel && isActive) {
                        // Sequential: only chain next if this job completed successfully
                        SubmitJob(nextIndex);
                    }
                }
            }

            bool IsTimedOut() const
            {
                if (_timeoutMs == Core::infinite)
                    return false;
                uint64_t elapsedMs = (Core::Time::Now().Ticks() - _startTime) / Core::Time::TicksPerMillisecond;
                return elapsedMs > _timeoutMs;
            }

            void SubmitJob(uint32_t index)
            {
                if (index >= _messages.Length()) {
                    return;
                }
                Core::ProxyType<Job> job = Core::ProxyType<Job>::Create(this, index);
                Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(job));
            }

            void FinishBatch()
            {
                if (_parent->IsChannelValid(_channelId)) {
                    string response;
                    _responseArray.ToString(response);
                    Core::ProxyType<Core::JSONRPC::Message> message(PluginHost::IFactories::Instance().JSONRPC());
                    if (message.IsValid()) {
                        message->Id = _responseId;
                        message->Result = response;
                        _parent->SendResponse(_channelId, Core::ProxyType<Core::JSON::IElement>(message));
                    }
                }
                _parent->RemoveBatchFromRegistry(_batchId);
            }

        private:
            Core::ProxyType<JsonRpcMuxer> _parent;
            uint32_t _batchId;
            bool _parallel;
            std::atomic<uint32_t> _pendingCount;
            Core::JSON::ArrayType<Core::JSONRPC::Message> _messages;
            Core::CriticalSection _responseLock;
            Core::JSON::ArrayType<Response> _responseArray;
            uint32_t _channelId;
            uint32_t _responseId;
            string _token;
            uint32_t _timeoutMs;
            uint64_t _startTime;
            std::atomic<bool> _cancelled;
        };

    public:
        JsonRpcMuxer(JsonRpcMuxer&&) = delete;
        JsonRpcMuxer(const JsonRpcMuxer&) = delete;
        JsonRpcMuxer& operator=(const JsonRpcMuxer&) = delete;

        JsonRpcMuxer()
            : _service(nullptr)
            , _dispatch(nullptr)
            , _activeWebSocket(0)
            , _batchCounter(0)
            , _maxBatchSize(10)
            , _activeBatchCount(0)
            , _maxBatches(5)
            , _timeout(Core::infinite)
            , _shuttingDown(false)
            , _batchesLock()
            , _activeBatches()
            , _batchCompletionEvent(false, true)
        {
        }

        ~JsonRpcMuxer() override = default;

        BEGIN_INTERFACE_MAP(JsonRpcMuxer)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        Core::hresult Invoke(
            const uint32_t channelId, const uint32_t id, const string& token,
            const string& designator, const string& parameters, string& response) override;

    private:
        void Process(uint32_t channelId, uint32_t responseId, const string& token, Core::JSON::ArrayType<Core::JSONRPC::Message>& messages, bool parallel);
        void SendErrorResponse(uint32_t channelId, uint32_t responseId, uint32_t errorCode, const string& errorMessage);
        void SendTimeoutResponse(uint32_t channelId, uint32_t responseId, uint32_t batchId);
        bool TryClaimBatchSlot();
        uint32_t ValidateBatch(const Core::JSON::ArrayType<Core::JSONRPC::Message>& messages, string& errorMessage);
        uint32_t WaitForBatchCompletion(const uint32_t maxWaitMs = Core::infinite);
        void RemoveBatchFromRegistry(uint32_t batchId);
        void CancelBatchesForChannel(uint32_t channelId);
        void CancelAllBatches();
        bool IsChannelValid(uint32_t channelId) const;
        uint32_t SendResponse(const uint32_t Id, const Core::ProxyType<Core::JSON::IElement>& response);

    private:
        friend class Batch;

        PluginHost::IShell* _service;
        PluginHost::IDispatcher* _dispatch;
        uint32_t _activeWebSocket;
        std::atomic<uint32_t> _batchCounter;
        uint8_t _maxBatchSize;
        std::atomic<uint8_t> _activeBatchCount;
        uint8_t _maxBatches;
        uint32_t _timeout;
        std::atomic<bool> _shuttingDown;
        Core::CriticalSection _batchesLock;
        std::map<uint32_t, Core::ProxyType<Batch>> _activeBatches;
        mutable Core::Event _batchCompletionEvent;
    };
}
}