#pragma once

#include "Module.h"

namespace Thunder {
	namespace Plugin {
		class SourceTransfer {
		private:
            template<typename SOCKETPORT>
            class WebClientType : public Web::WebLinkType<SOCKETPORT, Web::Response, Web::Request, Core::ProxyPoolType<Web::Response>&> {
            private:
                using BaseClass = Web::WebLinkType<SOCKETPORT, Web::Response, Web::Request, Core::ProxyPoolType<Web::Response>&>;

            public:
                WebClientType() = delete;
                WebClientType(WebClientType<SOCKETPORT>&& copy) = delete;
                WebClientType(const WebClientType<SOCKETPORT>& copy) = delete;
                WebClientType<SOCKETPORT>& operator=(const WebClientType<SOCKETPORT>&) = delete;

                WebClientType(SourceTransfer& parent, const Core::URL& url)
                    : BaseClass(5, _responseFactory, Core::SocketPort::STREAM, Core::NodeId(), Core::NodeId(), 2048, 2048)
                    , _parent(parent)
                    , _request()
                    , _content() {
                    if (url.Host().IsSet() == true) {
                        Core::NodeId nodeId(url.Host().Value().c_str(), url.Port().Value());
                        BaseClass::Link().RemoteNode(nodeId);
                        BaseClass::Link().LocalNode(nodeId.AnyInterface());

                        _request.Verb = Web::Request::HTTP_GET;
                        _request.Path = url.Path().Value();

                        uint32_t result = BaseClass::Open(100);

                        if ((result != Core::ERROR_INPROGRESS) && (result != Core::ERROR_NONE)) {
                            _request.Verb = Web::Request::HTTP_NONE;
                            BaseClass::Close(0);
                        }
                    }
                }
                ~WebClientType() override {
                    BaseClass::Close(Core::infinite);
                }

            public:
                // Notification of a Partial Request received, time to attach a body..
                void LinkBody(Core::ProxyType<Web::Response>& element) override {
                    // Time to attach a String Body
                    element->Body<Web::TextBody>(Core::ProxyType<Web::TextBody>(_content));
                }
                void Received(Core::ProxyType<Web::Response>& element) override {
                    _request.Verb = Web::Request::HTTP_NONE;
                    _parent.StateChange(element->ErrorCode == Web::WebStatus::STATUS_OK ? Core::ERROR_NONE : Core::ERROR_BAD_REQUEST);
                }
                void Send(const Core::ProxyType<Web::Request>& response) override {
                }
                void StateChange() override {
                    if ((BaseClass::IsOpen() == false) && (_request.Verb == Web::Request::HTTP_GET)) {
                        _request.Verb = Web::Request::HTTP_NONE;
                        _parent.StateChange(Core::ERROR_ASYNC_FAILED);
                    }
                }
                void Load(string content) {
                    content = std::move(_content);
                }

            private:
                SourceTransfer& _parent;
                Core::ProxyObject<Web::Request> _request;
                Core::ProxyObject<Web::TextBody> _content;
            };

            using WebClient       = WebClientType<Core::SocketPort>;
            #if defined(SECURESOCKETS_ENABLED)
            using SecureWebClient = WebClientType<Crypto::SecureSocketPort>;
            #endif

        public:
			struct ICallback {
				virtual ~ICallback() = default;
				virtual void Transfered(const string& url, const uint32_t result) = 0;
			};

		public:
			SourceTransfer() = delete;
			SourceTransfer(SourceTransfer&&) = delete;
			SourceTransfer(const SourceTransfer&) = delete;
			SourceTransfer& operator= (const SourceTransfer&) = delete;

			SourceTransfer(ICallback* callback, const string& storageSpace)
				: _adminLock()
                , _url()
				, _callback(callback)
				, _source()
                , _downloader() {
			}
			~SourceTransfer() {
				Abort();
			}

		public:
            inline void Reset() {
                _url.Clear();
            }
            inline bool IsValid() const {
                return (_url.IsValid());
            }
            inline bool IsLoaded() const {
                return ((_url.IsValid()) && (_downloader.IsValid() == false));
            }
            inline const string& Source() const {
                return(_source);
            }
            uint32_t Download(const string& url);
			uint32_t Abort();

        private:
            void StateChange(const uint32_t error) {
            }

        private:
            Core::CriticalSection _adminLock;
			Core::URL _url;
			ICallback* _callback;
            string _source;
            Core::ProxyType<Core::IReferenceCounted> _downloader;

            // All requests needed by any instance of this socket class, is coming from this static factory.
            // This means that all Requests, received are shared among all WebServer sockets, hopefully limiting
            // the number of requests that need to be created.
            static Core::ProxyPoolType<Web::Response> _responseFactory;
		};
	}
}
