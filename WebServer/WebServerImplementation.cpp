#include "Module.h"
#include <interfaces/IWebServer.h>
#include <interfaces/IMemory.h>

namespace WPEFramework {
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
        private:
            // -------------------------------------------------------------------
            // This object should not be copied or assigned. Prevent the copy
            // constructor and assignment constructor from being used. Compiler
            // generated assignment and copy methods will be blocked by the
            // following statments.
            // Define them but do not implement them, compile error/link error.
            // -------------------------------------------------------------------
            WebFlow(const WebFlow& a_Copy) = delete;
            WebFlow& operator=(const WebFlow& a_RHS) = delete;

        public:
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
            ~WebFlow()
            {
            }

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
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            class Proxy : public Core::JSON::Container {
            private:
                Proxy& operator=(const Proxy&) = delete;

            public:
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
                virtual ~Proxy()
                {
                }

            public:
                Core::JSON::String Path;
                Core::JSON::String Subst;
                Core::JSON::String Server;
            };

        public:
            Config()
                : Core::JSON::Container()
                , Port(8080)
                , Binding(_T("0.0.0.0"))
                , Interface()
                , Path(_T("www"))
                , IdleTime(180)
            {
                Add(_T("port"), &Port);
                Add(_T("binding"), &Binding);
                Add(_T("interface"), &Interface);
                Add(_T("path"), &Path);
                Add(_T("idletime"), &IdleTime);
                Add(_T("proxies"), &Proxies);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Port;
            Core::JSON::String Binding;
            Core::JSON::String Interface;
            Core::JSON::String Path;
            Core::JSON::DecUInt16 IdleTime;
            Core::JSON::ArrayType<Proxy> Proxies;
        };

        class RequestFactory {
        private:
            RequestFactory() = delete;
            RequestFactory(const RequestFactory&) = delete;
            RequestFactory operator=(const RequestFactory&) = delete;

        public:
            RequestFactory(const uint32_t)
            {
            }
            ~RequestFactory()
            {
            }

        public:
            Core::ProxyType<Web::Request> Element()
            {
                return (PluginHost::Factories::Instance().Request());
            }
        };

        class ResponseFactory {
        private:
            ResponseFactory() = delete;
            ResponseFactory(const ResponseFactory&) = delete;
            ResponseFactory operator=(const ResponseFactory&) = delete;

        public:
            ResponseFactory(const uint32_t)
            {
            }
            ~ResponseFactory()
            {
            }

        public:
            Core::ProxyType<Web::Response> Element()
            {
                return (PluginHost::Factories::Instance().Response());
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
                OutgoingChannel() = delete;
                OutgoingChannel(const OutgoingChannel&) = delete;
                OutgoingChannel& operator=(const OutgoingChannel&) = delete;

                struct OutstandingMessage {
                    Core::ProxyType<Web::Request> Request;
                    uint32_t Id;
                };

            public:
                OutgoingChannel(const string& path, const string& replacement, ProxyMap& proxyMap, const Core::NodeId& remoteId)
                    : Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, ResponseFactory>(2, false, remoteId.AnyInterface(), remoteId, 1024, 1024)
                    , _path(path)
                    , _replacement(replacement)
                    , _outstandingMessages()
                    , _proxyMap(proxyMap)
                {
                }

                void ProxyRequest(Core::ProxyType<Web::Request>& request, uint32_t id)
                {

                    OutstandingMessage message = { request, id };

                    _outstandingMessages.push_back(message);

                    if (_outstandingMessages.size() == 1) {
                        if (IsOpen() == false) {
                            Open(0);
                        }
                        else {
                            Submit(request);
                        }
                    }
                }

            public:
                inline const string& Path() const
                {
                    return (_path);
                }
                virtual void LinkBody(Core::ProxyType<Web::Response>& response)
                {
                    response->Body(_textBodies.Element());
                }
                virtual void Send(const Core::ProxyType<Web::Request>& request)
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
                virtual void StateChange()
                {
                    if (IsOpen() == true) {

                        if (!_outstandingMessages.empty()) {
                            Submit(_outstandingMessages.front().Request);
                        }
                    }
                }
                virtual void Received(Core::ProxyType<Web::Response>& response);

            private:
                const string _path;
                const string _replacement;

                std::list<OutstandingMessage> _outstandingMessages;
                ProxyMap& _proxyMap;
            };

        private:
            ProxyMap() = delete;
            ProxyMap(const ProxyMap&) = delete;
            ProxyMap& operator=(const ProxyMap&) = delete;

        public:
            ProxyMap(ChannelMap& server)
                : _server(server)
                , _proxies()
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

            bool Relay(Core::ProxyType<Web::Request>& request, uint32_t channelId)
            {

                bool found = false;
                const string& originalPath = request->Path;
                std::list<OutgoingChannel*>::iterator index(_proxies.begin());
                string proxyPath;

                while ((found == false) && (index != _proxies.end())) {

                    proxyPath = (*index)->Path();

                    uint32_t checkSize(proxyPath.length());

                    // If path starts with mapped string, we should relay.
                    if (((originalPath.length() == checkSize) || ((originalPath.length() > checkSize) && (originalPath[checkSize] == '/'))) && (originalPath.compare(0, checkSize, proxyPath) == 0)) {
                        found = true;
                    }
                    else {
                        index++;
                    }
                }

                // If we didn't find relay instructions for this path, return false.
                if (found == true) {

                    request->Path = (proxyPath + request->Path.substr(proxyPath.length()));

                    (*index)->ProxyRequest(request, channelId);
                }

                return (found);
            }

            inline void AddProxy(const string& path, const string& subst, const string& address)
            {
                const Core::NodeId node(address.c_str());

                if (node.IsValid() == true) {

                    _proxies.push_back(new OutgoingChannel(path, subst, *this, node));
                }
            }
            inline void RemoveProxy(const string& path)
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
            inline void Submit(uint32_t channelId, Core::ProxyType<Web::Response>& response)
            {
                _server.Submit(channelId, response);
            }

        private:
            ChannelMap& _server;
            std::list<OutgoingChannel*> _proxies;
        };

        class IncomingChannel : public Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, RequestFactory> {
        private:
            IncomingChannel() = delete;
            IncomingChannel(const IncomingChannel& copy) = delete;
            IncomingChannel& operator=(const IncomingChannel&) = delete;

        public:
            IncomingChannel(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<IncomingChannel>* parent)
                : Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, RequestFactory>(2, false, connector, remoteId, 1024, 1024)
                , _id(0)
                , _parent(static_cast<ChannelMap&>(*parent))
            {
            }
            virtual ~IncomingChannel()
            {
            }

        private:
            inline uint32_t Id() const
            {
                return (_id);
            }
            // Handle the HTTP Web requests.
            // [INBOUND]  Completed received requests are triggering the Received,
            // [OUTBOUND] Completed send responses are triggering the Send.
            virtual void LinkBody(Core::ProxyType<Web::Request>& request)
            {
                if (request->Verb == Web::Request::HTTP_POST) {
                    request->Body(_textBodies.Element());
                }
            }
            virtual void Send(const Core::ProxyType<Web::Response>& response)
            {
                TRACE(WebFlow, (response));
            }
            virtual void StateChange()
            {
            }
            virtual void Received(Core::ProxyType<Web::Request>& request);

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
            ChannelMap(const ChannelMap&) = delete;
            ChannelMap& operator=(const ChannelMap&) = delete;

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
                ~TimeHandler()
                {
                }

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
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
            ChannelMap()
                : Core::SocketServerType<IncomingChannel>()
                , _accessor()
                , _prefixPath()
                , _connectionCheckTimer(0)
                , _cleanupTimer(Core::Thread::DefaultStackSize(), _T("ConnectionChecker"))
                , _proxyMap(*this)
            {
            }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif
            ~ChannelMap()
            {
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

        public:
            inline uint32_t Configure(const string& prefixPath, const Config& configuration)
            {
                Core::NodeId accessor;
                uint32_t result(Core::ERROR_INCOMPLETE_CONFIG);
                Core::NodeId listenNode(configuration.Binding.Value().c_str());
                Core::JSON::ArrayType<Config::Proxy>::ConstIterator index(configuration.Proxies.Elements());

                if (configuration.Path.Value()[0] == '/') {
                    _prefixPath = Core::Directory::Normalize(configuration.Path);
                }
                else {
                    _prefixPath = prefixPath + Core::Directory::Normalize(configuration.Path);
                }

                _proxyMap.Create(index);

                if (configuration.Interface.Value().empty() == false) {
                    Core::NodeId selectedNode = Plugin::Config::IPV4UnicastNode(configuration.Interface.Value());

                    if (selectedNode.IsValid() == true) {
                        accessor = selectedNode;
                        listenNode = accessor;
                    }
                }
                else if (listenNode.IsAnyInterface() == true) {
                    Core::NodeId selectedNode = Plugin::Config::IPV4UnicastNode(configuration.Interface.Value());

                    if (selectedNode.IsValid() == true) {
                        accessor = selectedNode;
                    }
                }
                else {
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
            inline uint32_t Open(const uint32_t waitTime)
            {
                return (Core::SocketServerType<IncomingChannel>::Open(waitTime));
            }
            inline uint32_t Close(const uint32_t waitTime)
            {
                return (Core::SocketServerType<IncomingChannel>::Close(waitTime));
            }
            inline const string& PrefixPath() const
            {
                return (_prefixPath);
            }
            inline bool Relay(Core::ProxyType<Web::Request>& request, const uint32_t id)
            {
                return (_proxyMap.Relay(request, id));
            }
            inline string Accessor() const
            {
                return (_accessor);
            }
            void Close(IncomingChannel& data)
            {
            }
            inline void AddProxy(const string& path, const string& subst, const string& address)
            {
                _proxyMap.AddProxy(path, subst, address);
            }
            inline void RemoveProxy(const string& path)
            {
                _proxyMap.RemoveProxy(path);
            }

        private:
            uint64_t Timed(const uint64_t scheduledTime)
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
                    }
                    else {
                        index.Client()->ResetActivity();
                    }
                }

                return (NextTick.Ticks());
            }

        private:
            string _accessor;
            string _prefixPath;
            uint32_t _connectionCheckTimer;
            Core::TimerType<TimeHandler> _cleanupTimer;
            ProxyMap _proxyMap;
        };

    private:
        WebServerImplementation(const WebServerImplementation&) = delete;
        WebServerImplementation& operator=(const WebServerImplementation&) = delete;

    public:
        WebServerImplementation()
            : _channelServer()
            , _observers()
        {
        }

        virtual ~WebServerImplementation()
        {
        }

        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            ASSERT(service != nullptr);

            Config config;
            config.FromString(service->ConfigLine());

            uint32_t result(_channelServer.Configure(service->DataPath(), config));

            if (result == Core::ERROR_NONE) {

                result = _channelServer.Open(2000);

                PluginHost::ISubSystem* subSystem = service->SubSystems();

                if (subSystem != nullptr) {
                    ASSERT (subSystem->IsActive(PluginHost::ISubSystem::WEBSOURCE) == false);
                    subSystem->Set(PluginHost::ISubSystem::WEBSOURCE, nullptr);
                    subSystem->Release();
                }
            }

            return (result);
        }
        virtual PluginHost::IStateControl::state State() const
        {
            return (PluginHost::IStateControl::RESUMED);
        }
        virtual uint32_t Request(const PluginHost::IStateControl::command state)
        {
            // No state can be set, we can only move from ININITIALIZED to RUN...
            return (Core::ERROR_NONE);
        }

        virtual void Register(PluginHost::IStateControl::INotification* notification)
        {

            // Only subscribe an interface once.
            ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());

            // We will keep a reference to this observer, reference it..
            notification->AddRef();
            _observers.push_back(notification);
        }
        virtual void Unregister(PluginHost::IStateControl::INotification* notification)
        {
            // Only subscribe an interface once.
            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

            // Unregister only once :-)
            ASSERT(index != _observers.end());

            if (index != _observers.end()) {

                // We will keep a reference to this observer, reference it..
                (*index)->Release();
                _observers.erase(index);
            }
        }
        virtual void AddProxy(const string& path, const string& subst, const string& address)
        {
            _channelServer.AddProxy(path, subst, address);
        }
        virtual void RemoveProxy(const string& path)
        {
            _channelServer.RemoveProxy(path);
        }
        virtual string Accessor() const
        {
            return (_channelServer.Accessor());
        }

        BEGIN_INTERFACE_MAP(WebServerImplementation)
        INTERFACE_ENTRY(Exchange::IWebServer)
        INTERFACE_ENTRY(PluginHost::IStateControl)
        END_INTERFACE_MAP

    private:
        ChannelMap _channelServer;
        std::list<PluginHost::IStateControl::INotification*> _observers;
    };

    SERVICE_REGISTRATION(WebServerImplementation, 1, 0);

    /* virtual */ void WebServerImplementation::IncomingChannel::Received(Core::ProxyType<Web::Request>& request)
    {

        TRACE(WebFlow, (Core::proxy_cast<Web::Request>(request)));

        // Check if the channel server will relay this message.
        if (_parent.Relay(request, Id()) == false) {

            Core::ProxyType<Web::Response> response(PluginHost::Factories::Instance().Response());
            Core::ProxyType<Web::FileBody> fileBody(PluginHost::Factories::Instance().FileBody());

            // If so, don't deal with it ourselves.
            Web::MIMETypes result;
            string fileToService = _parent.PrefixPath();

            if (Web::MIMETypeForFile(request->Path, fileToService, result) == false) {

                string fullPath = fileToService + _T("index.html");

                // No filename gives, be default, we go for the index.html page..
                *fileBody = fileToService + _T("index.html");
                response->ContentType = Web::MIME_HTML;
                response->Body<Web::FileBody>(fileBody);
            }
            else {
                *fileBody = fileToService;
                response->ContentType = result;
                response->Body<Web::FileBody>(fileBody);
            }
            Submit(response);
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
        MemoryObserverImpl(const uint32_t id)
            : _main(id == 0 ? Core::ProcessInfo().Id() : id)
            , _observable(false)
        {
        }
        ~MemoryObserverImpl()
        {
        }

    public:
        virtual void Observe(const bool enable)
        {
            _observable = enable;
        }

        virtual uint64_t Resident() const
        {
            return (_main.Resident());
        }
        virtual uint64_t Allocated() const
        {
            return (_main.Allocated());
        }
        virtual uint64_t Shared() const
        {
            return (_main.Shared());
        }
        virtual uint8_t Processes() const
        {
            return (1);
        }
        virtual const bool IsOperational() const
        {
            return (_observable == false) || (_main.IsActive());
        }

        BEGIN_INTERFACE_MAP(MemoryObserverImpl)
        INTERFACE_ENTRY(Exchange::IMemory)
        END_INTERFACE_MAP

    private:
        Core::ProcessInfo _main;
        bool _observable;
    };

    Exchange::IMemory* MemoryObserver(const uint32_t PID)
    {
        return (Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(PID));
    }
}
} // namespace WebServer
