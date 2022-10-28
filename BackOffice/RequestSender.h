#pragma once

#include "Module.h"
#include <cryptalgo/cryptalgo.h>
#include <iostream>
#include <list>

namespace WPEFramework {
namespace Plugin {
    class RequestSender {
    public:
        constexpr static uint32_t MAX_WAIT_TIME = 1000;

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
                    Trace::Error(_T("Received error code: %d"), element->ErrorCode);
                }
            }
            void Send(const Core::ProxyType<WPEFramework::Web::Request>& response) override
            {
                string send;
                response->ToString(send);
                Trace::Information(_T("Send: %s"), send.c_str());
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

                        Trace::Information(_T("State change to open, attempting to send a request"));
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
                    Trace::Information(_T("State change to false"));
                    _parent._inProgress.SetEvent();
                }
            }

        private:
            RequestSender& _parent;
            Core::ProxyPoolType<Web::TextBody> _textBodyFactory;
        };

    public:
        RequestSender(Core::NodeId node, const std::list<std::pair<string, string>>& queryParameters, const string& userAgent)
            : _containerLock()
            , _webClient(*this, node)
            , _inProgress(true, true)
            , _canClose(false, true)
            , _userAgent(userAgent)
            , _job(*this)
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
            _job.Submit();
        }

    private:
        friend Core::ThreadPool::JobType<RequestSender&>;
        void Dispatch()
        {
            Trace::Information(_T("BackOffice: RequestHandler Job is Dispatched"));
            if (_inProgress.Lock(MAX_WAIT_TIME) == 0) {
                auto result = _webClient.Open(0);

                if (result == Core::ERROR_NONE || result == Core::ERROR_INPROGRESS) {
                    Trace::Information(_T("Connection opened"));
                } else {
                    Trace::Error(_T("Could not open the connection, error: %d"), result);
                    _inProgress.ResetEvent();
                    _canClose.SetEvent();
                }

                uint32_t ret = _canClose.Lock(MAX_WAIT_TIME);
                if (ret == 0) {
                    _webClient.Close(MAX_WAIT_TIME);
                    _canClose.ResetEvent();
                }
            }
        }

    private:
        std::list<std::pair<string, string>> _queue;
        Core::CriticalSection _containerLock;

        WebClient _webClient;
        Core::Event _inProgress;
        Core::Event _canClose;
        string _queryParameters;
        string _hostAddress;
        string _userAgent;

        Core::WorkerPool::JobType<RequestSender&> _job;
    };
}
}
