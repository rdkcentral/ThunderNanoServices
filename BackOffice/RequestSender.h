#pragma once

#include "Module.h"
#include <iostream>

namespace WPEFramework {
namespace Plugin {
    class RequestSender {
    private:
        class WebClient : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, WPEFramework::Core::ProxyPoolType<Web::Response>&> {
        private:
            typedef Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, WPEFramework::Core::ProxyPoolType<Web::Response>&> BaseClass;
            static WPEFramework::Core::ProxyPoolType<Web::Response> _responseFactory;

        public:
            WebClient() = delete;
            WebClient(const WebClient& copy) = delete;
            WebClient& operator=(const WebClient&) = delete;

            WebClient(RequestSender& parent, const WPEFramework::Core::NodeId& remoteNode)
                : BaseClass(5, _responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 2048)
                , _parent(parent)
                , _textBodyFactory(2)
            {
            }
            ~WebClient() override
            {
                Close(WPEFramework::Core::infinite);
            }

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
                _parent._inProgress.Unlock();
            }
            void StateChange() override
            {
                if (IsOpen()) {
                    Trace::Information(_T("State change to open, attempting to send a request"));
                    auto request = Core::ProxyType<WPEFramework::Web::Request>::Create();
                    request->Verb = Web::Request::HTTP_GET;
                    request->Query = Core::Format("%sevent=%s&id=%s", _parent._queryParameters.c_str(), _parent._event.c_str(), _parent._id.c_str());
                    request->Host = _parent._hostAddress;
                    request->Connection = Web::Request::CONNECTION_CLOSE;
                    Submit(request);
                } else {
                    Trace::Information(_T("State change to false"));
                    _parent._inProgress.Unlock();
                }
            }

        private:
            Core::ProxyPoolType<Web::TextBody> _textBodyFactory;
            RequestSender& _parent;
        };

    public:
        RequestSender(Core::NodeId node, const std::list<std::pair<string, string>>& queryParameters)
            : _webClient(*this, node)
            , _inProgress(true, true)
        {
            _hostAddress = node.HostAddress();
            for (const auto& entry : queryParameters) {
                _queryParameters += Core::Format("%s=%s&", entry.first.c_str(), entry.second.c_str());
            }

            ASSERT(!_queryParameters.empty());
        }
        ~RequestSender()
        {
            _webClient.Close(Core::infinite);
        }

        void Send(const string& event, const string& id)
        {
            _event = event;
            _id = id;
            if (_inProgress.Lock(0) == 0) {
                auto result = _webClient.Open(0);

                if (result == Core::ERROR_NONE) {
                    Trace::Information(_T("Connection opened"));

                } else {
                    Trace::Error(_T("Could not open the connection, error: %d"), result);
                    _inProgress.Unlock();
                }
            }
        }

    private:
        WebClient _webClient;
        Core::Event _inProgress;
        string _queryParameters;
        string _hostAddress;
        string _event;
        string _id;
    };
}
}