#pragma once

#include "Module.h"
#include <cryptalgo/cryptalgo.h>
#include <iostream>
#include <list>

namespace WPEFramework {
namespace Plugin {
    class RequestSender {
    public:
        constexpr static uint32_t MAX_WAIT_TIME = 5 * 1000;

    private:
        class WebClient : public Web::WebLinkType<Crypto::SecureSocketPort, Web::Response, Web::Request, WPEFramework::Core::ProxyPoolType<Web::Response>&> {
        private:
            typedef Web::WebLinkType<Crypto::SecureSocketPort, Web::Response, Web::Request, WPEFramework::Core::ProxyPoolType<Web::Response>&> BaseClass;
            static WPEFramework::Core::ProxyPoolType<Web::Response> _responseFactory;

        public:
            WebClient() = delete;
            WebClient(const WebClient& copy) = delete;
            WebClient& operator=(const WebClient&) = delete;

            WebClient(RequestSender& parent, const WPEFramework::Core::NodeId& remoteNode)
                : BaseClass(5, _responseFactory, Core::SocketPort::STREAM, remoteNode.AnyInterface(), remoteNode, 2048, 2048)
                , _parent(parent)
                , _textBodyFactory(2)
            {
            }
            ~WebClient() override = default;

        public:
            // Notification of a Partial Request received, time to attach a body..
            void LinkBody(Core::ProxyType<WPEFramework::Web::Response>& element) override
            {
                // Time to attach a String Body
                element->Body<Web::TextBody>(_textBodyFactory.Element());
            }
            void Received(Core::ProxyType<WPEFramework::Web::Response>& element) override
            {
                if (element->ErrorCode != Web::STATUS_OK) {
                    TRACE(Trace::Error, (_T("Received error code: %d"), element->ErrorCode));
                }
            }
            void Send(const Core::ProxyType<WPEFramework::Web::Request>& response) override
            {
                string send;
                response->ToString(send);
                TRACE(Trace::Information, (_T("Send: %s"), send.c_str()));
                _parent._inProgress.SetEvent();
                _parent._canClose.SetEvent();
            }
            void StateChange() override
            {
                if (IsOpen()) {
                    _parent._inProgress.ResetEvent();
                    _parent._containerLock.Lock();

                    if (!_parent._queue.empty()) {
                        auto entry = _parent._queue.back();
                        _parent._queue.pop_back();

                        TRACE(Trace::Information, (_T("State change to open, attempting to send a request")));
                        auto request = Core::ProxyType<WPEFramework::Web::Request>::Create();
                        request->Verb = Web::Request::HTTP_GET;
                        request->Query = Core::Format("%sevent=%s&id=%s", _parent._queryParameters.c_str(), entry.first.c_str(), entry.second.c_str());
                        request->Host = _parent._hostAddress;
                        request->Accept = _T("*/*");
                        request->UserAgent = _parent._userAgent;
                        Submit(request);
                    }
                    _parent._containerLock.Unlock();

                } else {
                    TRACE(Trace::Information, (_T("State change to close!")));
                    _parent._inProgress.SetEvent();
                }
            }

        private:
            Core::ProxyPoolType<Web::TextBody> _textBodyFactory;
            RequestSender& _parent;
        };

        class Worker : public Core::IDispatch {
        public:
            Worker(RequestSender& parent)
                : _parent(parent)
            {
            }

            void Dispatch() override
            {
                _lock.Lock();

                if (_parent._inProgress.Lock(MAX_WAIT_TIME) == 0) {
                    auto result = _parent._webClient.Open(0);

                    if (result == Core::ERROR_NONE || result == Core::ERROR_INPROGRESS) {
                        TRACE(Trace::Information, (_T("Connection opened")));
                    } else {
                        TRACE(Trace::Error, (_T("Could not open the connection, error: %d"), result));
                        _parent._inProgress.ResetEvent();
                        _parent._canClose.SetEvent();
                    }

                    if (_parent._canClose.Lock(MAX_WAIT_TIME) == 0) {
                        _parent._webClient.Close(MAX_WAIT_TIME);
                        _parent._canClose.ResetEvent();
                    }
                }

                _lock.Unlock();
            }

        private:
            RequestSender& _parent;
            Core::CriticalSection _lock;
        };

    public:
        RequestSender(Core::NodeId node, const std::list<std::pair<string, string>>& queryParameters, const string& userAgent)
            : _webClient(*this, node)
            , _inProgress(true, true)
            , _canClose(false, true)
            , _worker(Core::ProxyType<Worker>::Create(*this))
            , _userAgent(userAgent)
        {
            _hostAddress = node.HostAddress();
            for (const auto& entry : queryParameters) {
                _queryParameters += Core::Format("%s=%s&", entry.first.c_str(), entry.second.c_str());
            }
            node.IsValid();
            ASSERT(!_queryParameters.empty());
        }
        ~RequestSender() = default;

        void Send(const string& event, const string& id)
        {
            _containerLock.Lock();
            _queue.emplace_back(event, id);
            _containerLock.Unlock();
            Core::IWorkerPool::Instance().Schedule(Core::Time::Now(), Core::ProxyType<Core::IDispatch>(_worker));
        }

    private:
        std::list<std::pair<string, string>> _queue;
        Core::CriticalSection _containerLock;

        WebClient _webClient;
        Core::Event _inProgress;
        Core::Event _canClose;
        string _queryParameters;
        string _hostAddress;
        Core::ProxyType<Worker> _worker;
        string _userAgent;
    };
}
}
