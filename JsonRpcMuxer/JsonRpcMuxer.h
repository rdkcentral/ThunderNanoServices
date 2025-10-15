#pragma once

#include "Module.h"
#include <atomic>
#include <map>

namespace Thunder {
namespace Plugin {
    class JsonRpcMuxer :
#ifdef ENABLE_WEBSOCKET_CONNECTION
        public PluginHost::IPluginExtended,
        public PluginHost::IWebSocket,
#else
        public PluginHost::IPlugin,
#endif
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
                Job() = delete;
                Job(Job&&) = delete;
                Job(const Job&) = delete;
                Job& operator=(const Job&) = delete;

                Job(Batch* batch, uint32_t index)
                    : _batch(batch)
                    , _index(index)
                {
                }

                void Dispatch() override;

                ~Job() override = default;

            private:
                Batch* _batch;
                uint32_t _index;
            };

        public:
            Batch() = delete;
            Batch(Batch&&) = delete;
            Batch(const Batch&) = delete;
            Batch& operator=(const Batch&) = delete;

            Batch(
                Core::ProxyType<JsonRpcMuxer> parent,  uint32_t batchId,bool parallel,
                const Core::JSON::ArrayType<Core::JSONRPC::Message>& messages,
                uint32_t channelId, uint32_t responseId, const string& token,  uint32_t timeoutMs)
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
                // Pre-allocate responses
                for (uint32_t i = 0; i < _pendingCount; i++) {
                    _responseArray.Add();
                }
            }

            ~Batch() = default;

            uint32_t ChannelId() const
            {
                return _channelId;
            }

            uint32_t ResponseId() const
            {
                return _responseId;
            }

            uint32_t BatchId() const
            {
                return _batchId;
            }

            Response& Acquire(const uint16_t index)
            {
                ASSERT(index < _responseArray.Elements().Count());
                _responseLock.Lock();
                return _responseArray[index];
            }

            bool ReleaseAndCheckComplete()
            {
                _responseLock.Unlock();
                return (--_pendingCount == 0);
            }

            string ToString()
            {
                string response;
                _responseArray.ToString(response);

                return response;
            }

            bool IsTimedOut() const
            {
                if (_timeoutMs == Core::infinite) {
                    return false; // No timeout configured
                }

                uint64_t currentTicks = Core::Time::Now().Ticks();
                uint64_t elapsedTicks = currentTicks - _startTime;
                uint64_t elapsedMs = elapsedTicks / Core::Time::TicksPerMillisecond;

                return (elapsedMs > _timeoutMs);
            }

            uint32_t TimeoutMs() const
            {
                return _timeoutMs;
            }

            void Cancel()
            {
                _cancelled.store(true);
            }

            bool IsCancelled() const
            {
                return _cancelled.load();
            }

            const string& Token() const
            {
                return _token;
            }

            const Core::JSONRPC::Message& GetMessage(uint32_t index) const
            {
                ASSERT(index < _messages.Length());
                return _messages[index];
            }

            void Start();
            void OnJobComplete(uint32_t completedIndex);

        private:
            void SubmitJob(uint32_t index);

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
#ifdef ENABLE_WEBSOCKET_CONNECTION
        INTERFACE_ENTRY(PluginHost::IPluginExtended)
        INTERFACE_ENTRY(PluginHost::IWebSocket)
#endif
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        Core::hresult Invoke(
            const uint32_t channelId, const uint32_t id, const string& token,
            const string& method, const string& parameters, string& response) override;

#ifdef ENABLE_WEBSOCKET_CONNECTION
        bool Attach(PluginHost::Channel& channel) override;
        void Detach(PluginHost::Channel& channel) override;

        Core::ProxyType<Core::JSON::IElement> Inbound(const string& identifier) override;
        Core::ProxyType<Core::JSON::IElement> Inbound(const uint32_t ID, const Core::ProxyType<Core::JSON::IElement>& element) override;
#endif

    private:
        void Process(uint32_t channelId, uint32_t responseId, const string& token, Core::JSON::ArrayType<Core::JSONRPC::Message>& messages, bool parallel);
        void SendErrorResponse(uint32_t channelId, uint32_t responseId, uint32_t errorCode, const string& errorMessage);
        void SendTimeoutResponse(uint32_t channelId, uint32_t responseId, uint32_t batchId);
        bool TryClaimBatchSlot();
        uint32_t ValidateBatch(const Core::JSON::ArrayType<Core::JSONRPC::Message>& messages, string& errorMessage);
        uint32_t WaitForBatchCompletion(const uint32_t maxWaitMs = 5000);
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