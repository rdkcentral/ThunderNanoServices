#pragma once

#include "Module.h"
#include <deque>

namespace Thunder {
namespace Plugin {
    class JsonRpcMuxer : public PluginHost::IPluginExtended, public PluginHost::JSONRPC, public PluginHost::IWebSocket {
    private:
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

        class Request : public Core::JSON::ArrayType<Core::JSONRPC::Message> {
            static uint32_t _sequenceId;

        public:
            Request() = delete;
            Request(const uint32_t channelId, const uint32_t responseId, const string& token)
                : Core::JSON::ArrayType<Core::JSONRPC::Message>()
                , _tag(Core::InterlockedIncrement(_sequenceId))
                , _responseId(responseId)
                , _channelId(channelId)
                , _token(token)
            {
            }

        public:
            uint32_t BatchId() const { return _tag; }
            uint32_t ResponseId() const { return _responseId; }
            uint32_t ChannelId() const { return _channelId; }
            const string& Token() const { return _token; }

        private:
            uint32_t _tag;
            uint32_t _responseId;
            uint32_t _channelId;
            string _token;
        };

        class Processor : public Core::Thread {
        public:
            Processor() = delete;
            Processor(Processor&&) = delete;
            Processor(const Processor&) = delete;
            Processor& operator=(Processor&&) = delete;
            Processor& operator=(const Processor&) = delete;

            Processor(JsonRpcMuxer& parent)
                : Core::Thread()
                , _parent(parent)
                , _adminLock()
                , _queue()
            {
            }

            virtual ~Processor()
            {
                Core::Thread::Stop();
                Core::Thread::Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED, Core::infinite);
            }

            uint32_t Add(Request&& request)
            {
                uint32_t tag = request.BatchId();
                _adminLock.Lock();
                _queue.push_back(std::move(request));
                _adminLock.Unlock();
                Run();
                return tag;
            }

            void Remove(uint32_t tag)
            {
                _adminLock.Lock();
                _queue.erase(
                    std::remove_if(_queue.begin(), _queue.end(),
                        [tag](const Request& request) {
                            return request.BatchId() == tag;
                        }),
                    _queue.end());
                _adminLock.Unlock();
            }

        private:
            uint32_t Worker() override
            {
                Core::Thread::Block();
                _adminLock.Lock();

                while (!_queue.empty()) {
                    Request request(std::move(_queue.front()));
                    _queue.pop_front();
                    _adminLock.Unlock();
                    _parent.Process(request);
                    _adminLock.Lock();
                }

                _adminLock.Unlock();
                return Core::infinite;
            }

        private:
            JsonRpcMuxer& _parent;
            Core::CriticalSection _adminLock;
            std::deque<Request> _queue;
        };

    public:
        JsonRpcMuxer(JsonRpcMuxer&&) = delete;
        JsonRpcMuxer(const JsonRpcMuxer&) = delete;
        JsonRpcMuxer& operator=(const JsonRpcMuxer&) = delete;

        JsonRpcMuxer()
            : _service(nullptr)
            , _dispatch(nullptr)
            , _processor(*this)
            , _activeWebSocket(0)
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
        uint32_t Process(const Request& request);
        uint32_t SendResponse(const Request& request, const string& response);

    private:
        PluginHost::IShell* _service;
        PluginHost::IDispatcher* _dispatch;
        Processor _processor;
        uint32_t _activeWebSocket;
    };

    uint32_t JsonRpcMuxer::Request::_sequenceId = 1;
}
}