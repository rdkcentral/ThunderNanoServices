#pragma once

#include "Module.h"
#include <atomic>

namespace Thunder {
namespace Plugin {
    class JsonRpcMuxer : public PluginHost::IPluginExtended, public PluginHost::JSONRPC, public PluginHost::IWebSocket {
    private:
        class Config : public Core::JSON::Container {
        public:
            Config(Config&&) = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , TimeOut(Core::infinite)
                , StackSize(10)
            {
                Add(_T("timeout"), &TimeOut);
                Add(_T("stacksize"), &StackSize);
            }
            ~Config() override = default;

        public:
            Core::JSON::DecUInt32 TimeOut;
            Core::JSON::DecUInt16 StackSize;
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

        class Job : public Core::IDispatch {
        public:
            class State {
            public:
                State() = delete;
                State(State&&) = delete;
                State(const State&) = delete;
                State& operator=(const State&) = delete;

                State(uint32_t count, uint32_t channelId, uint32_t responseId, uint32_t batchId, uint32_t timeoutMs)
                    : _responseLock()
                    , _responseArray()
                    , _pendingCount(count)
                    , _channelId(channelId)
                    , _responseId(responseId)
                    , _batchId(batchId)
                    , _timeoutMs(timeoutMs)
                    , _startTime(Core::Time::Now().Ticks())
                {
                    // Pre-allocate responses
                    for (uint32_t i = 0; i < count; i++) {
                        _responseArray.Add();
                    }
                }

                ~State() = default;

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

            private:
                Core::CriticalSection _responseLock;
                Core::JSON::ArrayType<Response> _responseArray;
                std::atomic<uint32_t> _pendingCount;
                uint32_t _channelId;
                uint32_t _responseId;
                uint32_t _batchId;
                uint32_t _timeoutMs;
                uint64_t _startTime;
            };

            Job() = delete;
            Job(Job&&) = delete;
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;

            Job(JsonRpcMuxer& parent, const string& token, const Core::JSONRPC::Message& message, Core::ProxyType<State>& state, uint32_t index)
                : _parent(parent)
                , _token(token)
                , _message(message)
                , _state(state)
                , _index(index)
            {
            }

            void Dispatch() override
            {
                string output;

                TRACE(Trace::Information, (_T("Process %d:%d"), _state->ChannelId(), _message.Id.Value()));

                if (_state->IsTimedOut()) {
                    TRACE(Trace::Warning, (_T("Job skipped due to timeout, batchId=%u, jobId=%d"), _state->BatchId(), _message.Id.Value()));

                    // Mark this job as errored
                    Response& responseMsg = _state->Acquire(_index);
                    responseMsg.Id = _message.Id.Value();
                    responseMsg.Error.SetError(Core::ERROR_TIMEDOUT);
                    responseMsg.Error.Text = _T("Request timed out");

                    // Check if we're the last one
                    if (_state->ReleaseAndCheckComplete()) {
                        _parent.SendTimeoutResponse(_state->ChannelId(), _state->ResponseId(), _state->BatchId());
                    }
                    return; // Don't process this job
                }

                uint32_t invokeResult = _parent._dispatch->Invoke(
                    _state->ChannelId(),
                    _message.Id.Value(),
                    _token,
                    _message.Designator.Value(),
                    _message.Parameters.Value(),
                    output);

                // Update response (thread-safe)
                Response& responseMsg = _state->Acquire(_index);

                responseMsg.Id = _message.Id.Value();

                if (invokeResult == Core::ERROR_NONE) {
                    responseMsg.Result = output;
                } else {
                    responseMsg.Error.SetError(invokeResult);

                    if (!output.empty()) {
                        responseMsg.Error.Text = output;
                    }
                }

                if (_state->ReleaseAndCheckComplete()) {
                    if (_state->IsTimedOut()) {
                        _parent.SendTimeoutResponse(_state->ChannelId(), _state->ResponseId(), _state->BatchId());
                    } else {
                        // Normal response
                        string response = _state->ToString();

                        Core::ProxyType<Core::JSONRPC::Message> message(PluginHost::IFactories::Instance().JSONRPC());
                        if (message.IsValid()) {
                            message->Id = _state->ResponseId();
                            message->Result = response;
                            _parent._service->Submit(_state->ChannelId(), Core::ProxyType<Core::JSON::IElement>(message));
                        }

                        TRACE(Trace::Information, (_T("Batch completed, batchId=%u"), _state->BatchId()));
                    }
                }
            }

            ~Job() override = default;

        private:
            JsonRpcMuxer& _parent;
            string _token;
            Core::JSONRPC::Message _message;
            Core::ProxyType<State> _state;
            uint32_t _index;
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
            , _timeout(Core::infinite)
        {
        }

        ~JsonRpcMuxer() override = default;

        BEGIN_INTERFACE_MAP(JsonRpcMuxer)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IPluginExtended)
        INTERFACE_ENTRY(PluginHost::IWebSocket)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        bool Attach(PluginHost::Channel& channel) override;
        void Detach(PluginHost::Channel& channel) override;

        Core::hresult Invoke(
            const uint32_t channelId, const uint32_t id, const string& token,
            const string& method, const string& parameters, string& response) override;

        Core::ProxyType<Core::JSON::IElement> Inbound(const string& identifier) override;
        Core::ProxyType<Core::JSON::IElement> Inbound(const uint32_t ID, const Core::ProxyType<Core::JSON::IElement>& element) override;

    private:
        void Process(uint32_t channelId, uint32_t responseId, const string& token, Core::JSON::ArrayType<Core::JSONRPC::Message>& messages);
        void SendErrorResponse(uint32_t channelId, uint32_t responseId, uint32_t errorCode, const string& errorMessage);
        void SendTimeoutResponse(uint32_t channelId, uint32_t responseId, uint32_t batchId);

    private:
        friend class Job;

        PluginHost::IShell* _service;
        PluginHost::IDispatcher* _dispatch;
        uint32_t _activeWebSocket;
        std::atomic<uint32_t> _batchCounter;
        uint16_t _maxBatchSize;
        uint32_t _timeout;
    };
}
}