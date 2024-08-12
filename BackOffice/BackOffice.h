// Copyright (c) 2022 Metrological. All rights reserved.

#pragma once

#include "Module.h"

namespace Thunder {
namespace Plugin {
    class BackOffice : public PluginHost::IPlugin {
    public:
        enum reportstate {
            ACTIVATED,
            DEACTIVATED,
            UNAVAILABLE,
            RESUMED,
            SUSPENDED
        };
 
    private:
        using QueryParameters = std::list<std::pair<string, string>>;

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , ServerAddress()
                , ServerPort()
                , UserAgent(_T("BackOffice Reporting Plugin"))
                , Customer()
                , Platform()
                , Country()
                , Type()
                , Session()
                , CallsignMapping()
            {
                Add(_T("server"), &ServerAddress);
                Add(_T("port"), &ServerPort);
                Add(_T("useragent"), &UserAgent);
                Add(_T("customer"), &Customer);
                Add(_T("platform"), &Platform);
                Add(_T("country"), &Country);
                Add(_T("type"), &Type);
                Add(_T("session"), &Session);
                Add(_T("callsign_mapping"), &CallsignMapping);
            }

            ~Config() override = default;

        public:
            Core::JSON::String ServerAddress;
            Core::JSON::DecUInt16 ServerPort;
            Core::JSON::String UserAgent;
            Core::JSON::String Customer;
            Core::JSON::String Platform;
            Core::JSON::String Country;
            Core::JSON::String Type;
            Core::JSON::DecUInt16 Session;
            Core::JSON::ArrayType<Core::JSON::String> CallsignMapping;
        };
        class WebClient : public Web::WebLinkType<Crypto::SecureSocketPort, Web::Response, Web::Request, WebClient&> {
        private:
            using BaseClass = Web::WebLinkType<Crypto::SecureSocketPort, Web::Response, Web::Request, WebClient&>;
            using Queue = std::list<std::pair<string, string>>;

            static constexpr uint32_t MaximumWaitTime = 5 * 1000; // 5 Seconds waiting time

        public:
            WebClient(const WebClient& copy) = delete;
            WebClient& operator=(const WebClient&) = delete;

            WebClient()
                : BaseClass(5, *this, Core::SocketPort::STREAM, Core::NodeId(), Core::NodeId(), 2048, 2048)
                , _lock()
                , _queue()
                , _hostAddress()
                , _userAgent()
                , _message()
                , _job(*this)
            {
            }
            ~WebClient() override {
                _job.Revoke();
                BaseClass::Close(1000);
            }

        public:
            uint32_t Configure(const Core::NodeId& remoteNode, const string& userAgent, const QueryParameters& queryParameters) {
                _hostAddress = remoteNode.HostAddress();
                _userAgent = userAgent;

                BaseClass::Link().LocalNode(remoteNode.AnyInterface());
                BaseClass::Link().RemoteNode(remoteNode.AnyInterface());

                for (const auto& entry : queryParameters) {
                    _queryParameters += Core::Format("%s=%s&", entry.first.c_str(), entry.second.c_str());
                }

                ASSERT(!_queryParameters.empty());

                return (_queryParameters.empty() ? Core::ERROR_UNKNOWN_TABLE : Core::ERROR_NONE);
            }
            void Send(const string& event, const string& id) {
                _lock.Lock();
                _queue.emplace_back(event, id);
                _lock.Unlock();

                // We have data to send, see if we are capable of sending
                if (BaseClass::IsClosed()) {
                    // Nope there is no connection, lets open it, the StateChange will trigger continuation
                    BaseClass::Open(0);
                }
            }
            // Notification of a Partial Request received, time to attach a body..
            void LinkBody(Core::ProxyType<Thunder::Web::Response>& element VARIABLE_IS_NOT_USED) override {
                // We are not expected to receive Bodies with the incoming message, so drop it...
            }
            void Received(Core::ProxyType<Thunder::Web::Response>& element) override {

                _job.Revoke();

                _lock.Lock();

                // Oke a request has been completed, it has to be the first one in the queue.
                if (element->ErrorCode != Web::STATUS_OK) {
                    SYSLOG(Logging::Error, (_T("Received error code: %d for stats reporting"), element->ErrorCode));
                }

                if (_queue.empty() == false) {
                    Submit();
                }
                else {
                    Close(0);
                }

                _lock.Unlock();
            }
            void Send(const Core::ProxyType<Thunder::Web::Request>& request VARIABLE_IS_NOT_USED) override
            {
                // Oke the request has been send, lets wait for the response..
            }
            void StateChange() override
            {
                _lock.Lock();

                if ((IsOpen() == false) || (_queue.empty() == true)) {
                    Close(0);
                }
                else {
                    Submit();
                }

                _lock.Unlock();
            }

            Core::ProxyType<Thunder::Web::Response> Element() {
                return (PluginHost::IFactories::Instance().Response());
            }

        private:
            friend class Core::ThreadPool::JobType<WebClient&>;
            void Dispatch() {
                // We got a time out.. Might be an error or a restart after an error. Retry/Connect
                if (BaseClass::IsOpen() == true) {
                    BaseClass::Close(1000);
                    if (_queue.empty() == false) {
                        Open(0);
                    }
                }
            }
            void Submit() {
                std::pair<string, string> entry = _queue.back();
                _queue.pop_back();

                _message.Verb       = Web::Request::HTTP_GET;
                _message.Query      = Core::Format("%sevent=%s&id=%s", _queryParameters.c_str(), entry.first.c_str(), entry.second.c_str());
                _message.Host       = _hostAddress;
                _message.Accept     = _T("*/*");
                _message.UserAgent  = _userAgent;
                _message.Connection = Web::Request::CONNECTION_KEEPALIVE;

                BaseClass::Submit(Core::ProxyType<Thunder::Web::Request>(_message));
                _job.Reschedule(Core::Time::Now().Add(MaximumWaitTime));
            }

        private:
            Core::CriticalSection _lock;
            Queue _queue;
            string _queryParameters;
            string _hostAddress;
            string _userAgent;
            Core::ProxyObject<Thunder::Web::Request> _message;
            Core::ThreadPool::JobType<WebClient&> _job;
        };
        class Observer : public PluginHost::IPlugin::INotification {
        private:
            class StateControlObserver : public PluginHost::IStateControl::INotification {
            public:
                StateControlObserver() = delete;
                StateControlObserver(const StateControlObserver&) = delete;
                StateControlObserver& operator=(const StateControlObserver&) = delete;

                explicit StateControlObserver(Observer& parent, const string& reportName)
                    : _parent(parent)
                    , _reportName(reportName)
                    , _stateControl(nullptr)
                {
                }
                ~StateControlObserver() override {
                    Unregister();
                }

            public:
                void Register(PluginHost::IShell* plugin) {

                    ASSERT(plugin != nullptr);
                    ASSERT(_stateControl == nullptr);

                    _stateControl = plugin->QueryInterface<PluginHost::IStateControl>();

                    if (_stateControl != nullptr) {
                        _stateControl->Register(this);
                    }
                }
                void Unregister() {

                    if (_stateControl != nullptr) {
                        _stateControl->Unregister(this);
                        _stateControl->Release();
                        _stateControl = nullptr;
                    }
                }
                void StateChange(const PluginHost::IStateControl::state state) override
                {
                    if (state == PluginHost::IStateControl::RESUMED) {
                        _parent.StateChange(_reportName, reportstate::RESUMED);
                    }
                    else if (state == PluginHost::IStateControl::SUSPENDED) {
                        _parent.StateChange(_reportName, reportstate::SUSPENDED);
                    }
                }
                const string& ReportName() const {
                    return (_reportName);
                }

                BEGIN_INTERFACE_MAP(StateControlObserver)
                    INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
                END_INTERFACE_MAP

            private:
                Observer& _parent;
                const string _reportName;
                PluginHost::IStateControl* _stateControl;
            };
            using CallsignMap = std::unordered_map<string, Core::SinkType<StateControlObserver> >;

        public:
            Observer(BackOffice& parent) 
                : _parent(parent)
                , _callsigns() {
            }
            ~Observer() override = default;

        public:
            uint32_t Initialize(PluginHost::IShell* service, const Config& config) {

                Core::JSON::ArrayType<Core::JSON::String>::ConstIterator index = config.CallsignMapping.Elements();

                // Check out for which Callsigns we need to report the lifetime stuff.
                while (index.Next() == true) {
                    // TODO: To make it more clear it is anothername for the callsign, suggest to use an equal sign
                    Core::TextSegmentIterator loop(Core::TextFragment(index.Current().Value()), false, ',');
                    if (loop.Next() == true) {
                        string callsign = loop.Current().Text();
                        string reportName = callsign;

                        if (loop.Next() == true) {
                            // Seems we have a "surrogate" name.
                            reportName = loop.Current().Text();
                        }
                        _callsigns.emplace(std::piecewise_construct,
                            std::forward_as_tuple(callsign),
                            std::forward_as_tuple(*this, reportName));
                    }
                }

                if (_callsigns.empty() == false) {
                    // Register for retrieval of information.
                    service->Register(this);
                }

                return (_callsigns.empty() == false ? Core::ERROR_NONE : Core::ERROR_UNKNOWN_TABLE);
            }
            void Deinitialize(PluginHost::IShell* service) {
                service->Unregister(this);

                _callsigns.clear();
            }

#if THUNDER_VERSION >= 3
            void Activated(const string& callsign, PluginHost::IShell* plugin) override
            {
                ASSERT(plugin != nullptr);

                CallsignMap::iterator index = _callsigns.find(callsign);

                // Is this a callsign which requires attention ?
                if (index != _callsigns.end()) {
                    StateChange(index->second.ReportName(), reportstate::ACTIVATED);

                    index->second.Register(plugin);
                }
            }
            void Deactivated(const string& callsign, PluginHost::IShell*) override
            {
                CallsignMap::iterator index = _callsigns.find(callsign);

                // Is this a callsign which requires attention ?
                if (index != _callsigns.end()) {
                    StateChange(index->second.ReportName(), reportstate::DEACTIVATED);

                    index->second.Unregister();
                }
            }
            void Unavailable(const string& callsign, PluginHost::IShell*) override
            {
                CallsignMap::iterator index = _callsigns.find(callsign);

                // Is this a callsign which requires attention ?
                if (index != _callsigns.end()) {
                    StateChange(index->second.ReportName(), reportstate::UNAVAILABLE);
                }
            }
#else
            void StateChange(PluginHost::IShell* plugin) override
            {
                ASSERT(plugin != nullptr);

                string callsign = plugin->Callsign();
                CallsignMap::iterator index = _callsigns.find(callsign);

                // Is this a callsign which requires attention ?
                if (index != _callsigns.end()) {
                    switch (plugin->State()) {
                    case PluginHost::IShell::ACTIVATED:
                        StateChange(index->second.ReportName(), reportstate::ACTIVATED);
                        index->second.Register(plugin);
                        break;
                    case PluginHost::IShell::DEACTIVATED:
                        StateChange(index->second.ReportName(), reportstate::DEACTIVATED);
                        index->second.Unregister();
                        break;
                    case PluginHost::IShell::UNAVAILABLE:
                        StateChange(index->second.ReportName(), reportstate::UNAVAILABLE);
                        break
                    default:
                        Trace::Information(_T("Change to unknown state, ignoring."));
                        break;
                    }
                }
            }
#endif

            BEGIN_INTERFACE_MAP(Observer)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            void StateChange(const string& reportName, const reportstate state) {
                _parent.Send(reportName, state);
            }

        private:
            BackOffice& _parent;
            CallsignMap _callsigns;
        };

    public:
        BackOffice(const BackOffice&) = delete;
        BackOffice& operator=(const BackOffice&) = delete;

        PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST);
        BackOffice()
            : _stateChangeObserver(*this)
            , _requestSender() {
        }
        POP_WARNING();
        ~BackOffice() override = default;

        BEGIN_INTERFACE_MAP(BackOffice)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        void Send(const string& callsign, reportstate state);
       uint32_t CreateQueryParameters(PluginHost::IShell* service, const Config& config, QueryParameters& params);

    private:
        Core::SinkType<Observer> _stateChangeObserver;
        WebClient _requestSender;
    };

} // namespace
} // namespace
