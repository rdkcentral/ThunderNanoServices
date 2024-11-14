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
#include "OutOfProcessPlugin.h"
#include <interfaces/ITimeSync.h>
#include <interfaces/IComposition.h>
#include <thread>

#ifdef __CORE_EXCEPTION_CATCHING__
#include <stdexcept>
#endif

namespace Thunder {
namespace Plugin {

    static Core::NodeId CompositorConnector(const string& configuredValue)
    {
        Core::NodeId destination;
        if (configuredValue.empty() == false) {
            destination = Core::NodeId(configuredValue.c_str());
        }
        if (destination.IsValid() == false) {
            string value;
            if ((Core::SystemInfo::GetEnvironment(_T("COMPOSITOR"), value) == false) || (value.empty() == true)) {
                value = _T("/tmp/compositor");
            }
            destination = Core::NodeId(value.c_str());
        }
        return (destination);
    }

    class TooMuchInfo {
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------

    public:
        TooMuchInfo(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit TooMuchInfo(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~TooMuchInfo() = default;

        TooMuchInfo(const TooMuchInfo& a_Copy) = delete;
        TooMuchInfo& operator=(const TooMuchInfo& a_RHS) = delete;

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
        string _text;
    };

    class OutOfProcessImplementation 
        : public Exchange::IBrowser
        , public Exchange::IBrowserResources
        , public PluginHost::IStateControl
        , public Core::Thread {
    private:
        enum StateType {
            SHOW,
            HIDE,
            RESUMED,
            SUSPENDED
        };

        class PluginMonitor : public PluginHost::IPlugin::INotification {
        public:
            PluginMonitor(const PluginMonitor&) = delete;
            PluginMonitor& operator=(const PluginMonitor&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            PluginMonitor(OutOfProcessImplementation& parent)
                : _parent(parent)
            {
            }
POP_WARNING()
            ~PluginMonitor() override
            {
            }

        public:
            void Activated(const string&, PluginHost::IShell* service) override
            {
                Exchange::ITimeSync* time = service->QueryInterface<Exchange::ITimeSync>();
                if (time != nullptr) {
                    TRACE(Trace::Information, (_T("Time interface supported")));
                    time->Release();
                }
            }
            void Deactivated(const string&, PluginHost::IShell*) override
            {
            }
            void Unavailable(const string&, PluginHost::IShell*) override
            {
            }
            BEGIN_INTERFACE_MAP(PluginMonitor)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            VARIABLE_IS_NOT_USED OutOfProcessImplementation& _parent;
        };

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Sleep(4)
                , Init(100)
                , Crash(false)
                , Destruct(1000)
                , Single(false)
                , ExternalAccess(_T("/tmp/oopexample"))
                , Compositor(_T(""))
            {
                Add(_T("sleep"), &Sleep);
                Add(_T("config"), &Init);
                Add(_T("crash"), &Crash);
                Add(_T("destruct"), &Destruct);
                Add(_T("single"), &Single);
                Add(_T("accessor"), &ExternalAccess);
                Add(_T("compositor"), &Compositor);
            }
            ~Config() override = default;

        public:
            Core::JSON::DecUInt16 Sleep;
            Core::JSON::DecUInt16 Init;
            Core::JSON::Boolean Crash;
            Core::JSON::DecUInt32 Destruct;
            Core::JSON::Boolean Single;
            Core::JSON::String ExternalAccess;
            Core::JSON::String Compositor;
        };

    public:
        OutOfProcessImplementation(const OutOfProcessImplementation&) = delete;
        OutOfProcessImplementation& operator=(const OutOfProcessImplementation&) = delete;

        // note: ExternalAccess not actualy meant to be used (connections to be made to), InvokeServer needed to trigger testing use cases where it is present
        class ExternalAccess : public RPC::Communicator {
        private:
            ExternalAccess() = delete;
            ExternalAccess(const ExternalAccess&) = delete;
            ExternalAccess& operator=(const ExternalAccess&) = delete;

        public:
            ExternalAccess(
                const Core::NodeId& source,
                Exchange::IBrowser* parentInterface,
                const string& proxyStubPath,
                const Core::ProxyType<RPC::InvokeServer> & engine)
                : RPC::Communicator(source, proxyStubPath, Core::ProxyType<Core::IIPCServer>(engine), _T("@OutOfProcessPlugin"))
                , _parentInterface(parentInterface)
            {
                Open(Core::infinite);
            }
            ~ExternalAccess()
            {
                Close(Core::infinite);
            }

        private:
            virtual void* Acquire(const string& className VARIABLE_IS_NOT_USED, const uint32_t interfaceId, const uint32_t versionId)
            {
                void* result = nullptr;

                // Currently we only support version 1 of the IRPCLink :-)
                if (((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) && ((interfaceId == Exchange::IBrowser::ID) || (interfaceId == Core::IUnknown::ID))) {
                    // Reference count our parent
                    _parentInterface->AddRef();
                    TRACE(Trace::Information, ("Browser interface acquired => %p", this));
                    // Allright, respond with the interface.
                    result = _parentInterface;
                }
                return (result);
            }

        private:
            Exchange::IBrowser* _parentInterface;
        };

        class CompositorRemoteAccess : public Exchange::IComposition::IClient {
        public:
            CompositorRemoteAccess(CompositorRemoteAccess&&) = delete;
            CompositorRemoteAccess(const CompositorRemoteAccess&) = delete;
            CompositorRemoteAccess& operator= (CompositorRemoteAccess&&) = delete;
            CompositorRemoteAccess& operator= (const CompositorRemoteAccess&) = delete;
            CompositorRemoteAccess(const string& name)
                : _name(name) {
            }
            ~CompositorRemoteAccess() override = default;

        public:
            string Name() const override
            {
                return _name;
            }
            void Opacity(const uint32_t) override {}
            uint32_t Geometry(const Exchange::IComposition::Rectangle&) override
            {
                return 0;
            }
            Exchange::IComposition::Rectangle Geometry() const override
            {
                Exchange::IComposition::Rectangle rectangle = {0, 0, 1080, 920};
                return rectangle;
            }
            uint32_t ZOrder(const uint16_t) override
            {
                return 0;
            }
            uint32_t ZOrder() const override
            {
                return 0;
            }
            BEGIN_INTERFACE_MAP(CompositorRemoteAccess)
                INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP

        private:
            const string _name;
        };

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        OutOfProcessImplementation()
            : Core::Thread(0, _T("OutOfProcessImplementation"))
            , _requestedURL()
            , _setURL()
            , _fps(0)
            , _hidden(false)
            , _sink(*this)
            , _service(nullptr)
            , _engine()
            , _externalAccess(nullptr)
            , _compositerServerRPCConnection()
            , _type(SHOW)
            , _job(*this)
        {
            TRACE(Trace::Information, (_T("Constructed the OutOfProcessImplementation")));
        }
POP_WARNING()
        ~OutOfProcessImplementation() override
        {
            _job.Revoke();
            TRACE(Trace::Information, (_T("Destructing the OutOfProcessImplementation")));
            Block();

            if (Wait(Core::Thread::STOPPED | Core::Thread::BLOCKED, _config.Destruct.Value()) == false) {
                TRACE(Trace::Information, (_T("Bailed out before the thread signalled completion. %d ms"), _config.Destruct.Value()));
            }

            TRACE(Trace::Information, (_T("Destructed the OutOfProcessImplementation")));

            if (_externalAccess != nullptr) {

                TRACE(Trace::Information, (_T("Destructing _externalAccess")));
                delete _externalAccess;
                _engine.Release();
                TRACE(Trace::Information, (_T("Destructed _externalAccess")));
            }

            DestroyCompositerServerRPCConnection();

            if (_service) {
                TRACE(Trace::Information, (_T("Releasing service")));
                _service->Release();
                _service = nullptr;
                TRACE(Trace::Information, (_T("Released service")));
            }
            TRACE(Trace::Information, (_T("Destructed the OutOfProcessImplementation")));
        }

    public:
        void SetURL(const string& URL) override
        {
            #ifdef __CORE_EXCEPTION_CATCHING__
            static const string exceptionPrefix (_T("ThrowException"));

            if (URL.compare(0, exceptionPrefix.length(), exceptionPrefix) == 0) {
                string message = _T("BlankException");
                if (URL[exceptionPrefix.length()] == ':') {
                    message = URL.substr(exceptionPrefix.length() + 1, string::npos);
                }
                throw(std::domain_error(message));
            }
            else {
            #endif
                _requestedURL = URL;
                TRACE(Trace::Information, (_T("New URL [%d]: [%s]"), URL.length(), URL.c_str()));
            #ifdef __CORE_EXCEPTION_CATCHING__
            }
            #endif
        }
        uint32_t Configure(PluginHost::IShell* service) override
        {
            uint32_t result = Core::ERROR_NONE;

            TRACE(Trace::Information, (_T("Configuring: [%s]"), service->Callsign().c_str()));

            _service = service;

            if (_service) {
                TRACE(Trace::Information, (_T("OutOfProcessImplementation::Configure: AddRef service")));
                _service->AddRef();
            }

            _config.FromString(service->ConfigLine());

            if (_config.ExternalAccess.IsSet() == true) {
                _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());
                _externalAccess = new ExternalAccess(Core::NodeId(_config.ExternalAccess.Value().c_str()), this, service->ProxyStubPath(), _engine);

                result = Core::ERROR_OPENING_FAILED;
                if (_externalAccess != nullptr) {
                    if (_externalAccess->IsListening() == false) {
                        TRACE(Trace::Information, (_T("Deleting the External Server, it is not listening!")));
                        delete _externalAccess;
                        _externalAccess = nullptr;
                        _engine.Release();
                    }
                }
            }

            if (result == Core::ERROR_NONE) {

                _dataPath = service->DataPath();

                if (_config.Init.Value() > 0) {
                    TRACE(Trace::Information, (_T("Configuration requested to take [%d] mS"), _config.Init.Value()));
                    SleepMs(_config.Init.Value());
                }
                Run();
            }

            return (result);
        }
        void CreateCompositerServerRPCConnection(const string& connection) {
            if (Core::WorkerPool::IsAvailable() == true) {
                // If we are in the same process space as where a WorkerPool is registered (Main Process or
                // hosting ptocess) use, it!
                Core::ProxyType<RPC::InvokeServer> engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::WorkerPool::Instance());
                ASSERT(engine.IsValid() == true);

                _compositerServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(CompositorConnector(connection), Core::ProxyType<Core::IIPCServer>(engine));
                ASSERT(_compositerServerRPCConnection.IsValid() == true);

            } else {
                // Seems we are not in a process space initiated from the Main framework process or its hosting process.
                // Nothing more to do than to create a workerpool for RPC our selves !
                Core::ProxyType<RPC::InvokeServerType<2,0,8>> engine = Core::ProxyType<RPC::InvokeServerType<2,0,8>>::Create();
                ASSERT(engine.IsValid() == true);

                _compositerServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(CompositorConnector(connection), Core::ProxyType<Core::IIPCServer>(engine));
                ASSERT(_compositerServerRPCConnection.IsValid() == true);

            }

            uint32_t result = _compositerServerRPCConnection->Open(RPC::CommunicationTimeOut);
            if (result != Core::ERROR_NONE) {
                _compositerServerRPCConnection.Release();
            } else {
                _compositorRemoteAccess = Core::ServiceType<CompositorRemoteAccess>::Create<CompositorRemoteAccess>(_service->Callsign());
                result = _compositerServerRPCConnection->Offer(_compositorRemoteAccess);
                if (result != Core::ERROR_NONE) {
                    printf(_T("Could not offer IClient interface with callsign %s to Compositor. Error: %s\n"), _compositorRemoteAccess->Name().c_str(), Core::NumberType<uint32_t>(result).Text().c_str());
                }
	    }
	}
        void DestroyCompositerServerRPCConnection() {
            if (_compositerServerRPCConnection.IsValid() == true) {
                uint32_t result = _compositerServerRPCConnection->Revoke(_compositorRemoteAccess);

                if (result != Core::ERROR_NONE) {
                    printf(_T("Could not revoke IClient interface with callsign %s to Compositor. Error: %s\n"), _compositorRemoteAccess->Name().c_str(), Core::NumberType<uint32_t>(result).Text().c_str());
                }
                _compositorRemoteAccess->Release();
                _compositerServerRPCConnection.Release();
            }
        }
        string GetURL() const override
        {
            TRACE(Trace::Information, (_T("Requested URL: [%s]"), _requestedURL.c_str()));
            return (_requestedURL);
        }
        uint32_t GetFPS() const override
        {
            TRACE(Trace::Information, (_T("Requested FPS: %d"), _fps));
            return (++_fps);
        }
        void Register(PluginHost::IStateControl::INotification* sink) override
        {
            _adminLock.Lock();

            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_notificationClients.begin(), _notificationClients.end(), sink));

            // Make sure a sink is not registered multiple times.
            ASSERT(index == _notificationClients.end());

            if (index == _notificationClients.end()) {
                _notificationClients.push_back(sink);
                sink->AddRef();

                TRACE(Trace::Information, (_T("IStateControl::INotification Registered: %p"), sink));
            }
            _adminLock.Unlock();
        }

        void Unregister(PluginHost::IStateControl::INotification* sink) override
        {
            _adminLock.Lock();

            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_notificationClients.begin(), _notificationClients.end(), sink));

            // Make sure you do not unregister something you did not register !!!
            ASSERT(index != _notificationClients.end());

            if (index != _notificationClients.end()) {
                TRACE(Trace::Information, (_T("IStateControl::INotification Unregistered: %p"), sink));
                (*index)->Release();
                _notificationClients.erase(index);
            }

            _adminLock.Unlock();
        }
        void Register(Exchange::IBrowser::INotification* sink) override
        {
            ASSERT(sink != nullptr);

            _adminLock.Lock();
            
            std::list<Exchange::IBrowser::INotification*>::iterator index(std::find(_browserClients.begin(), _browserClients.end(), sink));

            // Make sure a sink is not registered multiple times.
            ASSERT(index == _browserClients.end());

            if (index == _browserClients.end()) {
                _browserClients.push_back(sink);
                sink->AddRef();

                TRACE(Trace::Information, (_T("IBrowser::INotification Registered: %p"), sink));
            }
            _adminLock.Unlock();
        }

        virtual void Unregister(Exchange::IBrowser::INotification* sink) override
        {
            ASSERT(sink != nullptr);

            _adminLock.Lock();

            std::list<Exchange::IBrowser::INotification*>::iterator index(std::find(_browserClients.begin(), _browserClients.end(), sink));

            // Make sure you do not unregister something you did not register !!!
            // Since we have the Single shot parameter, it could be that it is deregistered, it is not an error !!!!
            // ASSERT(index != _browserClients.end());

            if (index != _browserClients.end()) {
                TRACE(Trace::Information, (_T("IBrowser::INotification Unregistered: %p"), sink));
                (*index)->Release();
                _browserClients.erase(index);
            }

            _adminLock.Unlock();
        }

        uint32_t Request(const PluginHost::IStateControl::command command) override
        {
            TRACE(Trace::Information, (_T("Requested a state change. Moving to %s"), command == PluginHost::IStateControl::command::RESUME ? _T("RESUMING") : _T("SUSPENDING")));
            uint32_t result(Core::ERROR_ILLEGAL_STATE);

            switch (command) {
            case PluginHost::IStateControl::SUSPEND:
                UpdateState(SUSPENDED);
                result = Core::ERROR_NONE;
                break;
            case PluginHost::IStateControl::RESUME:
                UpdateState(RESUMED);
                result = Core::ERROR_NONE;
                break;
            }

            return (result);
        }

        PluginHost::IStateControl::state State() const override
        {
            return (_state);
        }

        void Hide(const bool hidden) override
        {

            if (hidden == true) {
                TRACE(Trace::Information, (_T("Requestsed a Hide.")));
                UpdateState(HIDE);
            } else {
                UpdateState(SHOW);
                TRACE(Trace::Information, (_T("Requestsed a Show.")));
            }
        }
        void StateChange(const PluginHost::IStateControl::state state)
        {
            _adminLock.Lock();

            std::list<PluginHost::IStateControl::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end()) {
                TRACE(Trace::Information, (_T("State change from OutofProcessTest 0x%X"), state));
                (*index)->StateChange(state);
                index++;
            }

            TRACE(Trace::Information, (_T("Changing state to [%s]"), state == PluginHost::IStateControl::state::RESUMED ? _T("Resumed") : _T("Suspended")));
            _state = state;

            _adminLock.Unlock();
        }
        void Hidden(const bool hidden)
        {
            _adminLock.Lock();

            std::list<Exchange::IBrowser::INotification*>::iterator index(_browserClients.begin());

            while (index != _browserClients.end()) {
                TRACE(Trace::Information, (_T("State change from OutofProcessTest 0x%X"), __LINE__));
                (*index)->Hidden(hidden);
                index++;
            }
            TRACE(Trace::Information, (_T("Changing state to [%s]"), hidden ? _T("Hidden") : _T("Shown")));

            _hidden = hidden;

            _adminLock.Unlock();
        }
        void UpdateState(const StateType type)
        {
            _adminLock.Lock();
            _type = type;
            _adminLock.Unlock();
            _job.Submit();
        }

        // IBrowserResources (added so we can test lengthty calls back to Thunder )
        uint32_t Headers(IStringIterator*& header ) const override {
            std::list<string> headers;
            for(uint16_t i = 0; i<1000; ++i) {
                string s("Header_");
                s += std::to_string(i);
                headers.emplace_back(std::move(s));
            }

            header = Core::ServiceType<RPC::StringIterator>::Create<RPC::IStringIterator>(headers);

            return Core::ERROR_NONE;
        }
        uint32_t Headers(IStringIterator* const header) override {
            TRACE(Trace::Information, (_T("Headers update start")));
            ASSERT(header != nullptr);
            header->Reset(0);
            string value;
            while (header->Next(value) == true) {
                TRACE(TooMuchInfo, (_T("Header [%s] processed"), value.c_str()));
            }
            TRACE(Trace::Information, (_T("Headers update finished")));
            return Core::ERROR_NONE;
        }

        uint32_t UserScripts(IStringIterator*& uris ) const override {
            std::list<string> scripts;
            for(uint16_t i = 0; i<1000; ++i) {
                string s("UserScripts_");
                s += std::to_string(i);
                scripts.emplace_back(std::move(s));
            }

            uris = Core::ServiceType<RPC::StringIterator>::Create<RPC::IStringIterator>(scripts);

            return Core::ERROR_NONE;
        }

        uint32_t UserScripts(IStringIterator* const uris) override {
            TRACE(Trace::Information, (_T("UserScripts update start")));
            ASSERT(uris != nullptr);
            uris->Reset(0);
            string value;
            uint32_t i = 0;
            uint32_t sleep = 10;
            while (uris->Next(value) == true) {
                if(i == 0) {
                    sleep = stoi(value);
                }
                TRACE(TooMuchInfo, (_T("UserScript [%s] processed"), value.c_str()));
                if( ++i == 5000 ) {
                    if(sleep == 0 ) {
                        int* crash = nullptr;
                        *crash = 10;
                    } else if(sleep < 1000) {
                        std::thread t([=](){
                            std::this_thread::sleep_for(std::chrono::milliseconds(sleep));            
                            int* crash = nullptr;
                            *crash = 10;
                        });
                        t.detach();
                    } 
                }
            }
            TRACE(Trace::Information, (_T("UserScripts update finished")));
            return Core::ERROR_NONE;
        }

        uint32_t UserStyleSheets(IStringIterator*& uris ) const override {
            std::list<string> sheets;
            for(uint16_t i = 0; i<1000; ++i) {
                string s("UserStyleSheets_");
                s += std::to_string(i);
                sheets.emplace_back(std::move(s));
            }

            uris = Core::ServiceType<RPC::StringIterator>::Create<RPC::IStringIterator>(sheets);

            return Core::ERROR_NONE;
        }
        uint32_t UserStyleSheets(IStringIterator* const uris) override {
            TRACE(Trace::Information, (_T("UserStyleSheets update start")));
            ASSERT(uris != nullptr);
            uris->Reset(0);
            string value;
            while (uris->Next(value) == true) {
                TRACE(TooMuchInfo, (_T("UserStyleSheets [%s] processed"), value.c_str()));
            }
            TRACE(Trace::Information, (_T("UserStyleSheets update finished")));
            return Core::ERROR_NONE;
           
        }

        BEGIN_INTERFACE_MAP(OutOfProcessImplementation)
            INTERFACE_ENTRY(Exchange::IBrowser)
            INTERFACE_ENTRY(Exchange::IBrowserResources)
            INTERFACE_ENTRY(PluginHost::IStateControl)
            INTERFACE_AGGREGATE(PluginHost::IPlugin::INotification,static_cast<IUnknown*>(&_sink))
        END_INTERFACE_MAP

    private:
        friend Core::ThreadPool::JobType<OutOfProcessImplementation&>;

        // Dispatch can be run in an unlocked state as the destruction of the observer list
        // is always done if the thread that calls the Dispatch is blocked (paused)
        void Dispatch()
        {
            Trace::Information(_T("PluginMonitor: Job is Dispatched"));
            _adminLock.Lock();
            switch (_type) {
            case SHOW:
                ::SleepMs(300);
                Hidden(false);
                break;
            case HIDE:
                ::SleepMs(100);
                Hidden(true);
                break;
            case RESUMED:
                StateChange(PluginHost::IStateControl::RESUMED);
                break;
            case SUSPENDED:
                StateChange(PluginHost::IStateControl::SUSPENDED);
                break;
            }
            _adminLock.Unlock();
        }

    private:
        virtual uint32_t Worker() override
        {
            if (_config.Sleep.Value() > 0) {
                TRACE(Trace::Information, (_T("Main task of execution reached. Starting with a Sleep of [%d] S"), _config.Sleep.Value()));
                // First Sleep the expected time..
                SleepMs(_config.Sleep.Value() * 1000);
            }

            _adminLock.Lock();

            std::list<Exchange::IBrowser::INotification*>::iterator index(_browserClients.begin());

            _state = PluginHost::IStateControl::SUSPENDED;

            _setURL = _requestedURL;

            TRACE(Trace::Information, (_T("Send out a notification change of the URL: [%s]"), _setURL.c_str()));

            while (index != _browserClients.end()) {
                (*index)->URLChanged(_setURL);
                // See if we need to "Fire and forget, single shot..
                if (_config.Single.Value() == true) {
                    (*index)->Release();
                }
                index++;
            }

            if (_config.Single.Value() == true) {
                _browserClients.clear();
            }

            _adminLock.Unlock();

            if (_config.Crash.Value() == true) {
                TRACE(Trace::Information, (_T("Crash request honoured. We are going to CRASH!!!")));
                TRACE(Trace::Information, (_T("Going to CRASH as requested %d."), 0));
                abort();
            }

            if (_config.Compositor.IsSet() == true) {
                CreateCompositerServerRPCConnection(_config.Compositor.Value());
            }

            // Just do nothing :-)
            Block();

            while (IsRunning() == true) {
                SleepMs(200);
            }

            TRACE(Trace::Information, (_T("Leaving the main task of execution, we are done.")));

            return (Core::infinite);
        }

    private:
        Core::CriticalSection _adminLock;
        Config _config;
        string _requestedURL;
        string _setURL;
        string _dataPath;
        mutable uint32_t _fps;
        bool _hidden;
        std::list<PluginHost::IStateControl::INotification*> _notificationClients;
        std::list<Exchange::IBrowser::INotification*> _browserClients;
        PluginHost::IStateControl::state _state;
        Core::SinkType<PluginMonitor> _sink;
        PluginHost::IShell* _service;

        Core::ProxyType<RPC::InvokeServer> _engine;
        ExternalAccess* _externalAccess;

        CompositorRemoteAccess* _compositorRemoteAccess;
        Core::ProxyType<RPC::CommunicatorClient> _compositerServerRPCConnection;

        StateType _type;
        Core::WorkerPool::JobType<OutOfProcessImplementation&> _job;
    };

    SERVICE_REGISTRATION(OutOfProcessImplementation, 1, 0)

} // namespace Plugin

namespace OutOfProcessPlugin {

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
        virtual uint64_t Resident() const override
        {
            return _main.Resident();
        }
        virtual uint64_t Allocated() const override
        {
            return _main.Allocated();
        }
        virtual uint64_t Shared() const override
        {
            return _main.Shared();
        }
        virtual uint8_t Processes() const override
        {
            return (IsOperational() ? 1 : 0);
        }
        virtual bool IsOperational() const override
        {
            return (_main.IsActive());
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
} // namespace Thunder::OutOfProcessPlugin
