/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
 
#include "Module.h"
#include <interfaces/IMemory.h>
#include <interfaces/IWebServer.h>
#ifdef ENABLE_SECURITY_AGENT
#include <securityagent/securityagent.h>
#endif

namespace Thunder {
namespace Plugin {

    static Core::ProxyPoolType<Web::TextBody> _textBodies(5);

    class WebServerImplementation : public Exchange::IWebServer, public PluginHost::IStateControl {
    private:
        enum enumState {
            UNINITIALIZED,
            RUNNING
        };
        class ChannelMap;
        class WebFlow {
        public:
            WebFlow(const WebFlow& a_Copy) = delete;
            WebFlow& operator=(const WebFlow& a_RHS) = delete;

            WebFlow(const Core::ProxyType<Web::Request>& request)
            {
                if (request.IsValid() == true) {
                    string text;

                    request->ToString(text);

                    _text = Core::ToString(string('\n' + text + '\n'));
                }
            }
            WebFlow(const Core::ProxyType<Web::Response>& response)
            {
                if (response.IsValid() == true) {
                    string text;

                    response->ToString(text);

                    _text = Core::ToString(string('\n' + text + '\n'));
                }
            }
            ~WebFlow() = default;

        public:
            inline const char* Data() const
            {
                return (_text.c_str());
            }
            inline uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };

        class Config : public Core::JSON::Container {
        public:
            class Proxy : public Core::JSON::Container {
            public:
                Proxy& operator=(const Proxy&) = delete;

                Proxy()
                    : Core::JSON::Container()
                    , Path()
                    , Subst()
                    , Server()
                {
                    Add(_T("path"), &Path);
                    Add(_T("subst"), &Subst);
                    Add(_T("server"), &Server);
                }
                Proxy(const Proxy& copy)
                    : Core::JSON::Container()
                    , Path(copy.Path)
                    , Subst(copy.Subst)
                    , Server(copy.Server)
                {
                    Add(_T("path"), &Path);
                    Add(_T("subst"), &Subst);
                    Add(_T("server"), &Server);
                }
                ~Proxy() override = default;

            public:
                Core::JSON::String Path;
                Core::JSON::String Subst;
                Core::JSON::String Server;
            };

        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , Port(8888)
                , Binding(_T("0.0.0.0"))
                , Interface()
                , Path()
                , IdleTime(180)
                , SecurityAgent(false)
            {
                Add(_T("port"), &Port);
                Add(_T("binding"), &Binding);
                Add(_T("interface"), &Interface);
                Add(_T("path"), &Path);
                Add(_T("idletime"), &IdleTime);
                Add(_T("proxies"), &Proxies);
                Add(_T("securityagent"), &SecurityAgent);
            }
            ~Config() override = default;

        public:
            Core::JSON::DecUInt16 Port;
            Core::JSON::String Binding;
            Core::JSON::String Interface;
            Core::JSON::String Path;
            Core::JSON::DecUInt16 IdleTime;
            Core::JSON::ArrayType<Proxy> Proxies;
            Core::JSON::Boolean SecurityAgent;
        };

        class RequestFactory {
         public:
            RequestFactory() = delete;
            RequestFactory(const RequestFactory&) = delete;
            RequestFactory operator=(const RequestFactory&) = delete;

            RequestFactory(const uint32_t) {
            };
            ~RequestFactory() = default;

        public:
            Core::ProxyType<Web::Request> Element()
            {
                return (PluginHost::IFactories::Instance().Request());
            }
        };

        class ResponseFactory {
        public:
            ResponseFactory() = delete;
            ResponseFactory(const ResponseFactory&) = delete;
            ResponseFactory operator=(const ResponseFactory&) = delete;

            ResponseFactory(const uint32_t) {
            }
            ~ResponseFactory() = default;

        public:
            Core::ProxyType<Web::Response> Element()
            {
                return (PluginHost::IFactories::Instance().Response());
            }
        };

        // IMPORTANT NOTE:
        // There is no need to lock/unlock usage on this server as all action->response senarious take place on
        // the communication thread from the SoketPortMonitor. There is only 1 such thread per process.
        // Given this, make sure that all actions done by the ProxyMap are deterministic and short <100ms as it
        // upholds all other network traffic.
        class ProxyMap {
        private:
            class OutgoingChannel : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, ResponseFactory> {
            private:
                struct OutstandingMessage {
                    Core::ProxyType<Web::Request> Request;
                    uint32_t Id;
                };

            public:
                OutgoingChannel() = delete;
                OutgoingChannel(const OutgoingChannel&) = delete;
                OutgoingChannel& operator=(const OutgoingChannel&) = delete;

                OutgoingChannel(const string& path, const string& replacement, ProxyMap& proxyMap, const Core::NodeId& remoteId)
                    : Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, ResponseFactory>(2, false, remoteId.AnyInterface(), remoteId, 1024, 1024)
                    , _path(path)
                    , _replacement(replacement)
                    , _outstandingMessages()
                    , _proxyMap(proxyMap)
                {
                }
                
                ~OutgoingChannel() override {
                    Close(Core::infinite);
                }

                void ProxyRequest(Core::ProxyType<Web::Request>& request, uint32_t id)
                {

                    OutstandingMessage message = { request, id };

                    _outstandingMessages.push_back(message);

                    if (_outstandingMessages.size() == 1) {
                        if (IsOpen() == false) {
                            Open(0);
                        } else {
                            Submit(request);
                        }
                    }
                }

            public:
                inline const string& Path() const
                {
                    return (_path);
                }
                void LinkBody(Core::ProxyType<Web::Response>& response) override
                {
                    response->Body(_textBodies.Element());
                } 
                void Send(const Core::ProxyType<Web::Request>& request VARIABLE_IS_NOT_USED) override
                {
                    std::list<OutstandingMessage>::iterator index(_outstandingMessages.begin());

                    while ((index != _outstandingMessages.end()) && (index->Request.IsValid() == false)) {
                        index++;
                    }

                    ASSERT(index != _outstandingMessages.end());
                    ASSERT(index->Request == request);

                    index->Request.Release();
                }
                // Whenever there is a state change on the link, it is reported here.
                void StateChange() override
                {
                    if (IsOpen() == true) {

                        if (!_outstandingMessages.empty()) {
                            Submit(_outstandingMessages.front().Request);
                        }
                    }
                }
                void Received(Core::ProxyType<Web::Response>& response) override;

            private:
                const string _path;
                const string _replacement;

                std::list<OutstandingMessage> _outstandingMessages;
                ProxyMap& _proxyMap;
            };

        public:
            ProxyMap() = delete;
            ProxyMap(const ProxyMap&) = delete;
            ProxyMap& operator=(const ProxyMap&) = delete;

            ProxyMap(ChannelMap& server)
                : _server(server)
                , _proxies()
                , _closures()
            {
            }
            ~ProxyMap()
            {
                // Clean up channels in map.
                while (_proxies.size() > 0) {
                    delete _proxies.front();
                    _proxies.pop_front();
                }
            }

        public:
            void Create(Core::JSON::ArrayType<Config::Proxy>::ConstIterator& index)
            {

                index.Reset();

                while (index.Next() == true) {

                    const string& path(index.Current().Path.Value());
                    const string& subst(index.Current().Subst.Value());
                    const Core::NodeId address(index.Current().Server.Value().c_str());

                    if (address.IsValid() == true) {

                        _proxies.push_back(new OutgoingChannel(path, subst, *this, address));
                    }
                }
            }

            void Destroy()
            {

                std::list<OutgoingChannel*>::iterator index(_proxies.begin());

                while (index != _proxies.end()) {

                    delete (*index);

                    index++;
                }
                _proxies.clear();
            }

            bool Relay(Core::ProxyType<Web::Request>& request, uint32_t channelId, const string& webToken)
            {

                bool found = false;
                const string& originalPath = request->Path;
                std::list<OutgoingChannel*>::iterator index(_proxies.begin());
                string proxyPath;

                while ((found == false) && (index != _proxies.end())) {

                    proxyPath = (*index)->Path();

                    uint32_t checkSize(static_cast<uint32_t>(proxyPath.length()));

                    // If path starts with mapped string, we should relay.
                    if (((originalPath.length() == checkSize) || ((originalPath.length() > checkSize) && (originalPath[checkSize] == '/'))) && (originalPath.compare(0, checkSize, proxyPath) == 0)) {
                        found = true;
                    } else {
                        index++;
                    }
                }

                // If we didn't find relay instructions for this path, return false.
                if (found == true) {

                    if (request->Connection.Value() == Web::Request::CONNECTION_CLOSE) {
                        // The client allows non-persistant connection, but the closure should not be applied to the relay connection.
                        request->Connection = Web::Request::CONNECTION_UNKNOWN;
                        _closures.push_back(channelId);
                    }

                    request->Path = (proxyPath + request->Path.substr(proxyPath.length()));
                    if (!webToken.empty()) {
                        request->WebToken = Web::Authorization(Web::Authorization::BEARER, webToken);
                    }

                    (*index)->ProxyRequest(request, channelId);
                }

                return (found);
            }

            void AddProxy(const string& path, const string& subst, const string& address)
            {
                const Core::NodeId node(address.c_str());

                if (node.IsValid() == true) {

                    _proxies.push_back(new OutgoingChannel(path, subst, *this, node));
                }
            }
            void RemoveProxy(const string& path)
            {
                std::list<OutgoingChannel*>::iterator index(_proxies.begin());

                while ((index != _proxies.end()) && ((*index)->Path() != path)) {

                    index++;
                }

                if (index != _proxies.end()) {

                    delete (*index);
                    _proxies.erase(index);
                }
            }
            void Submit(uint32_t channelId, Core::ProxyType<Web::Response>& response)
            {
                _server.Submit(channelId, response);
            }
            bool Completed(const uint32_t channelId)
            {
                // Relay completed; check if we can close the incoming connection
                bool close = false;
                auto it = std::find(_closures.begin(), _closures.end(), channelId);
                if (it != _closures.end()) {
                    _closures.erase(it);
                    close = true;
                }
                return (close);
            }

        private:
            ChannelMap& _server;
            std::list<OutgoingChannel*> _proxies;
            std::list<uint32_t> _closures;
        };

        class IncomingChannel : public Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, RequestFactory> {
        public:
            IncomingChannel() = delete;
            IncomingChannel(const IncomingChannel& copy) = delete;
            IncomingChannel& operator=(const IncomingChannel&) = delete;
            
            IncomingChannel(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<IncomingChannel>* parent)
                : Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, RequestFactory>(2, false, connector, remoteId, 1024, 1024)
                , _id(0)
                , _parent(static_cast<ChannelMap&>(*parent))
            {
            }
            
            ~IncomingChannel() override
            {
                Close(Core::infinite);
            }

        private:
            inline uint32_t Id() const
            {
                return (_id);
            }
            // Handle the HTTP Web requests.
            // [INBOUND]  Completed received requests are triggering the Received,
            // [OUTBOUND] Completed send responses are triggering the Send.
            void LinkBody(Core::ProxyType<Web::Request>& request) override
            {
                if (request->Verb == Web::Request::HTTP_POST) {
                    request->Body(_textBodies.Element());
                }
            }
            void Send(const Core::ProxyType<Web::Response>& response) override
            {
                TRACE(WebFlow, (response));
                if (_parent.RelayComplete(Id())) {
                    Close(0);
                }
            }
            void StateChange() override { }
            void Received(Core::ProxyType<Web::Request>& request) override;

        private:
            friend class Core::SocketServerType<IncomingChannel>;

            inline void Id(const uint32_t id)
            {
                _id = id;
            }

        private:
            uint32_t _id;
            ChannelMap& _parent;
        };

        class ChannelMap : public Core::SocketServerType<IncomingChannel> {
        private:
            typedef Core::SocketServerType<IncomingChannel> BaseClass;

            class TimeHandler {
            public:
                TimeHandler()
                    : _parent(nullptr)
                {
                }
                TimeHandler(ChannelMap& parent)
                    : _parent(&parent)
                {
                }
                TimeHandler(const TimeHandler& copy)
                    : _parent(copy._parent)
                {
                }
                ~TimeHandler() = default;

                TimeHandler& operator=(const TimeHandler& RHS)
                {
                    _parent = RHS._parent;
                    return (*this);
                }

            public:
                uint64_t Timed(const uint64_t scheduledTime)
                {
                    ASSERT(_parent != nullptr);

                    return (_parent->Timed(scheduledTime));
                }

            private:
                ChannelMap* _parent;
            };

        public:
            ChannelMap(const ChannelMap&) = delete;
            ChannelMap& operator=(const ChannelMap&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            ChannelMap()
                : Core::SocketServerType<IncomingChannel>()
                , _accessor()
                , _prefixPath()
                , _webToken()
                , _connectionCheckTimer(0)
                , _cleanupTimer(Core::Thread::DefaultStackSize(), _T("ConnectionChecker"))
                , _proxyMap(*this)
            {
            }
POP_WARNING()
            ~ChannelMap()
            {
                BaseClass::Iterator index(BaseClass::Clients());
                ASSERT(index.Count() == 0);
            }

        public:
            void CloseAndCleanupConnections() {
                // Start by closing the server thread..
                Core::SocketServerType<IncomingChannel>::Close(1000);

                // Kill all open connections, we are shutting down !!!
                BaseClass::Iterator index(BaseClass::Clients());

                while (index.Next() == true) {
                    // Oops nothing hapened for a long time, kill the connection
                    // give it 100ms to actually close, if not do it forcefully !!
                    index.Client()->Close(100);
                }

                // Cleanup the closed sockets we created..
                Cleanup();
            }
            uint32_t Configure(const string& prefixPath, const Config& configuration)
            {
                Core::NodeId accessor;
                uint32_t result(Core::ERROR_INCOMPLETE_CONFIG);
                Core::NodeId listenNode(configuration.Binding.Value().c_str());
                Core::JSON::ArrayType<Config::Proxy>::ConstIterator index(configuration.Proxies.Elements());

                SetWebToken(configuration);

                if (configuration.Path.IsSet() == false) {
                    _prefixPath.clear();
                }
                else if (configuration.Path.Value()[0] == '/') {
                    _prefixPath = Core::Directory::Normalize(configuration.Path.Value());
                }
                else {
                    _prefixPath = prefixPath + Core::Directory::Normalize(configuration.Path.Value());
                }

                _proxyMap.Create(index);

                if (configuration.Interface.Value().empty() == false) {
                    Core::NodeId selectedNode = Plugin::Config::IPV4UnicastNode(configuration.Interface.Value());

                    if (selectedNode.IsValid() == true) {
                        accessor = selectedNode;
                        listenNode = accessor;
                    }
                } else if (listenNode.IsAnyInterface() == true) {
                    Core::NodeId selectedNode = Plugin::Config::IPV4UnicastNode(configuration.Interface.Value());

                    if (selectedNode.IsValid() == true) {
                        accessor = selectedNode;
                    }
                } else {
                    accessor = listenNode;
                }

                listenNode.PortNumber(configuration.Port.Value());

                if (listenNode.IsValid() == true) {

                    _accessor = _T("http://") + accessor.HostAddress();

                    if (configuration.Port.Value() != 80) {

                        _accessor += ':' + Core::NumberType<uint32_t>(configuration.Port.Value()).Text();
                    }

                    Core::SocketServerType<IncomingChannel>::LocalNode(listenNode);

                    _connectionCheckTimer = configuration.IdleTime.Value() * 1000;

                    if (_connectionCheckTimer != 0) {
                        Core::Time NextTick = Core::Time::Now();

                        NextTick.Add(_connectionCheckTimer);

                        _cleanupTimer.Schedule(NextTick.Ticks(), TimeHandler(*this));
                    }
                    result = Core::ERROR_NONE;
                }

                return (result);
            }
            uint32_t Open(const uint32_t waitTime)
            {
                return (Core::SocketServerType<IncomingChannel>::Open(waitTime));
            }
            uint32_t Close(const uint32_t waitTime)
            {
                return (Core::SocketServerType<IncomingChannel>::Close(waitTime));
            }
            const string& PrefixPath() const
            {
                return (_prefixPath);
            }
            bool Relay(Core::ProxyType<Web::Request>& request, const uint32_t id)
            {
                return (_proxyMap.Relay(request, id, _webToken));
            }
            bool RelayComplete(const uint32_t id)
            {
                return (_proxyMap.Completed(id));
            }
            string Accessor() const
            {
                return (_accessor);
            }
            inline void AddProxy(const string& path, const string& subst, const string& address)
            {
                _proxyMap.AddProxy(path, subst, address);
            }
            inline void RemoveProxy(const string& path)
            {
                _proxyMap.RemoveProxy(path);
            }
            inline bool IsFileServerEnabled()
            {
                return (_prefixPath.empty() == false);
            }
        private:
            void SetWebToken(const Config& configuration)
            {
                if (configuration.SecurityAgent.IsSet() && configuration.SecurityAgent.Value() == true) {
#ifdef ENABLE_SECURITY_AGENT
                    // Need to grab the localhost security token so that we can proxy request to
                    // other plugins and get them authorized.
                    int length;
                    unsigned char buffer[2 * 1024];
                    string payload = "{\"url\": \"http://localhost\"}";

                    ::snprintf(reinterpret_cast<char*>(buffer), sizeof(buffer), "%s", payload.c_str());
                    length = GetToken(static_cast<unsigned short>(sizeof(buffer)),
                                      static_cast<unsigned short>(::strlen(reinterpret_cast<const char*>(buffer))),
                                      buffer);
                    if (length > 0) {
                        _webToken.assign(string(reinterpret_cast<const char*>(buffer), length));
                    }

                    TRACE(Trace::Information, (_T("SetWebToken: Got token with length %d"), _webToken.length()));
#else
                    TRACE(Trace::Error, (_T("SetWebToken: Configured for SecurityAgent but no support enabled")));
#endif /* ENABLE_SECURITY_AGENT */
                }
            }

            uint64_t Timed(const uint64_t)
            {
                Core::Time NextTick(Core::Time::Now());

                NextTick.Add(_connectionCheckTimer);

                // First clear all shit from last time..
                Cleanup();

                // Now suspend those that have no activity.
                BaseClass::Iterator index(BaseClass::Clients());

                while (index.Next() == true) {
                    if (index.Client()->HasActivity() == false) {
                        // Oops nothing hapened for a long time, kill the connection
                        // Give it all the time (0) if it i not yet suspended to close. If it is
                        // suspended, force the close down if not closed in 100ms.
                        index.Client()->Close(0);
                    } else {
                        index.Client()->ResetActivity();
                    }
                }

                return (NextTick.Ticks());
            }

        private:
            string _accessor;
            string _prefixPath;
            string _webToken;
            uint32_t _connectionCheckTimer;
            Core::TimerType<TimeHandler> _cleanupTimer;
            ProxyMap _proxyMap;
        };

    public:
        WebServerImplementation(const WebServerImplementation&) = delete;
        WebServerImplementation& operator=(const WebServerImplementation&) = delete;

        WebServerImplementation()
            : _channelServer()
            , _adminLock()
            , _observers()
            , _state(PluginHost::IStateControl::UNINITIALIZED)
            , _requestedCommand(PluginHost::IStateControl::SUSPEND)
            , _job(*this)
        {
        }

        ~WebServerImplementation() override = default;
        
        friend Core::ThreadPool::JobType<WebServerImplementation&>;

        uint32_t Configure(PluginHost::IShell* service) override
        {
            ASSERT(service != nullptr);
            Config config;
            config.FromString(service->ConfigLine());
            uint32_t result = _channelServer.Configure(service->DataPath(), config);
            return (result);
        }
        PluginHost::IStateControl::state State() const override
        {
            PluginHost::IStateControl::state state;
            _adminLock.Lock();
            state = _state;
            _adminLock.Unlock();
            return state;
        }


        void Dispatch() {
            bool stateChanged = false;
            uint32_t result = Core::ERROR_NONE;
            _adminLock.Lock();
            if(_requestedCommand == PluginHost::IStateControl::RESUME ) {
                if ((_state == PluginHost::IStateControl::UNINITIALIZED || _state == PluginHost::IStateControl::SUSPENDED)) {
                    result = _channelServer.Open(2000);
                    if ( result == Core::ERROR_NONE) {
                        stateChanged = true;
                        _state = PluginHost::IStateControl::RESUMED;
                    }
                }
            } else {
                if(_state == PluginHost::IStateControl::RESUMED || _state == PluginHost::IStateControl::UNINITIALIZED ) {
                    _channelServer.CloseAndCleanupConnections();
                    stateChanged = true;
                    _state = PluginHost::IStateControl::SUSPENDED;
                }
            }
            if(stateChanged) {
                for(auto& observer: _observers) {
                    observer->StateChange(_state);
                }
            }
            _adminLock.Unlock();
        }

        uint32_t Request(const PluginHost::IStateControl::command command) override
        {
            _adminLock.Lock();
            _requestedCommand = command;
            _job.Submit();
            _adminLock.Unlock();
            return (Core::ERROR_NONE);
        }

        void Register(PluginHost::IStateControl::INotification* notification) override
        {
            ASSERT(notification != nullptr);

            _adminLock.Lock();
            // Only subscribe an interface once.
            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));
            ASSERT(index == _observers.end());

            if (index == _observers.end()) {
                // We will keep a reference to this observer, reference it..
                notification->AddRef();
                _observers.push_back(notification);
            }
            _adminLock.Unlock();
        }
        void Unregister(PluginHost::IStateControl::INotification* notification) override
        {
            ASSERT(notification != nullptr);

            _adminLock.Lock();
            // Only subscribe an interface once.
            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

            // Unregister only once :-)
            ASSERT(index != _observers.end());

            if (index != _observers.end()) {

                // We will keep a reference to this observer, reference it..
                (*index)->Release();
                _observers.erase(index);
            }
            _adminLock.Unlock();
        }
        void AddProxy(const string& path, const string& subst, const string& address) override
        {
            _channelServer.AddProxy(path, subst, address);
        }
        void RemoveProxy(const string& path) override
        {
            _channelServer.RemoveProxy(path);
        }
        string Accessor() const override
        {
            return (_channelServer.Accessor());
        }

        BEGIN_INTERFACE_MAP(WebServerImplementation)
            INTERFACE_ENTRY(Exchange::IWebServer)
            INTERFACE_ENTRY(PluginHost::IStateControl)
        END_INTERFACE_MAP

    private:
        ChannelMap _channelServer;
        mutable Core::CriticalSection _adminLock;
        std::list<PluginHost::IStateControl::INotification*> _observers;
        PluginHost::IStateControl::state _state;
        PluginHost::IStateControl::command _requestedCommand;
        Core::WorkerPool::JobType<WebServerImplementation&> _job;

    };

    SERVICE_REGISTRATION(WebServerImplementation, 1, 0)

    /* virtual */ void WebServerImplementation::IncomingChannel::Received(Core::ProxyType<Web::Request>& request)
    {
        TRACE(WebFlow, (request));

        const string realPath = Core::File::Normalize(request->Path, true /* safe path */);
        if (realPath.empty() == false) {
            // Use Normalized Path
            request->Path = realPath;
        }

        // Check if the channel server will relay this message.
        if (realPath.empty() == true) {
            Core::ProxyType<Web::Response> response(PluginHost::IFactories::Instance().Response());
            response->ErrorCode = Web::STATUS_BAD_REQUEST;
            response->Message = "Invalid Request";
            Submit(response);
        }
        else if (_parent.Relay(request, Id()) == false) {

            Core::ProxyType<Web::Response> response(PluginHost::IFactories::Instance().Response());
            Core::ProxyType<Web::FileBody> fileBody(PluginHost::IFactories::Instance().FileBody());

            if (_parent.IsFileServerEnabled() == false) {
                Core::ProxyType<Web::Response> response(PluginHost::IFactories::Instance().Response());
                response->ErrorCode = Web::STATUS_BAD_REQUEST;
                response->Message = "Invalid Request";
                Submit(response);
            }
            else {

                // If so, don't deal with it ourselves.
                Web::MIMETypes result;
                Web::EncodingTypes encoding;
                string fileToService = _parent.PrefixPath();

                if (Web::MIMETypeAndEncodingForFile(request->Path, fileToService, result, encoding) == false) {
                    string fullPath = fileToService + _T("index.html");

                    // No filename gives, be default, we go for the index.html page..
                    *fileBody = fileToService + _T("index.html");
                    response->ContentType = Web::MIME_HTML;
                    response->Body<Web::FileBody>(fileBody);
                }
                else {
                    *fileBody = fileToService;
                    if (fileBody->Exists() == true) {
                        response->ContentType = result;
                        if (encoding != Web::ENCODING_UNKNOWN) {
                            response->ContentEncoding = encoding;
                        }
                        response->Body<Web::FileBody>(fileBody);
                    } else {
                        response->ErrorCode = Web::STATUS_NOT_FOUND;
                        response->Message = "Not Found";
                    }
                }
                Submit(response);
            }
        }
    }

    /* virtual */ void WebServerImplementation::ProxyMap::OutgoingChannel::Received(Core::ProxyType<Web::Response>& response)
    {
        // Is response to our front of the list
        ASSERT(_outstandingMessages.empty() == false);
        ASSERT(_outstandingMessages.front().Request.IsValid() == false);

        if (_outstandingMessages.empty() == false) {
            _proxyMap.Submit(_outstandingMessages.front().Id, response);
            _outstandingMessages.pop_front();

            // See if ther is a next one to send.
            if (_outstandingMessages.size() > 0) {

                ASSERT(_outstandingMessages.front().Request.IsValid() == true);

                Submit(_outstandingMessages.front().Request);
            }
        }
    }

} /* namespace Plugin */

namespace WebServer {

    class MemoryObserverImpl : public Exchange::IMemory {
    private:
        MemoryObserverImpl();
        MemoryObserverImpl(const MemoryObserverImpl&);
        MemoryObserverImpl& operator=(const MemoryObserverImpl&);

    public:
        MemoryObserverImpl(const RPC::IRemoteConnection* connection)
            : _main(connection == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId())
        {
        }
        ~MemoryObserverImpl()
        {
        }

    public:
        uint64_t Resident() const override
        {
            return _main.Resident();
        }
        uint64_t Allocated() const override
        {
            return _main.Allocated();
        }
        uint64_t Shared() const override
        {
            return _main.Shared();
        }
        uint8_t Processes() const override
        {
            return (IsOperational() ? 1 : 0);
        }
        bool IsOperational() const override
        {
            return _main.IsActive();
        }

        BEGIN_INTERFACE_MAP(MemoryObserverImpl)
        INTERFACE_ENTRY(Exchange::IMemory)
        END_INTERFACE_MAP

    private:
        Core::ProcessInfo _main;
    };

    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection)
    {
        ASSERT(connection != nullptr);
        Exchange::IMemory* result = Core::ServiceType<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);
        return (result);
    }
}
} // namespace WebServer
