/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include "SimpleEGL.h"
#include <interfaces/ITimeSync.h>

#include <compositor/Client.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#ifdef __cplusplus
}
#endif

// Suppress compiler warnings of unused (parameters)
// Omitting the name is sufficient but a search on this keyword provides easy access to the location
template <typename T>
void silence (T &&) {}

namespace WPEFramework {
namespace Plugin {

    class SimpleEGLImplementation 
        : public Exchange::IBrowser
        , public PluginHost::IStateControl
        , public Core::Thread {
    private:
        class PluginMonitor : public PluginHost::IPlugin::INotification {
        private:
            using Job = Core::ThreadPool::JobType<PluginMonitor>;

        public:
            PluginMonitor(const PluginMonitor&) = delete;
            PluginMonitor& operator=(const PluginMonitor&) = delete;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            PluginMonitor(SimpleEGLImplementation& parent)
                : _parent(parent)
            {
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            ~PluginMonitor() override = default;

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
            BEGIN_INTERFACE_MAP(PluginMonitor)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            friend Core::ThreadPool::JobType<PluginMonitor&>;

            // Dispatch can be run in an unlocked state as the destruction of the observer list
            // is always done if the thread that calls the Dispatch is blocked (paused)
            void Dispatch()
            {
            }

        private:
            SimpleEGLImplementation& _parent;
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
            {
                Add(_T("sleep"), &Sleep);
                Add(_T("config"), &Init);
                Add(_T("crash"), &Crash);
                Add(_T("destruct"), &Destruct);
                Add(_T("single"), &Single);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Sleep;
            Core::JSON::DecUInt16 Init;
            Core::JSON::Boolean Crash;
            Core::JSON::DecUInt32 Destruct;
            Core::JSON::Boolean Single;
        };

        class Job : public Core::IDispatch {
        public:
            enum runtype {
                SHOW,
                HIDE,
                RESUMED,
                SUSPENDED
            };
            Job()
                : _parent(nullptr)
                , _type(SHOW)
            {
            }
            Job(SimpleEGLImplementation& parent, const runtype type)
                : _parent(&parent)
                , _type(type)
            {
            }
            Job(const Job& copy)
                : _parent(copy._parent)
                , _type(copy._type)
            {
            }
            ~Job() override = default;

            Job& operator=(const Job& RHS)
            {
                _parent = RHS._parent;
                _type = RHS._type;
                return (*this);
            }

        public:
            void Dispatch() override
            {
                switch (_type) {
                case SHOW:
                    ::SleepMs(300);
                    _parent->Hidden(false);
                    break;
                case HIDE:
                    ::SleepMs(100);
                    _parent->Hidden(true);
                    break;
                case RESUMED:
                    _parent->StateChange(PluginHost::IStateControl::RESUMED);
                    break;
                case SUSPENDED:
                    _parent->StateChange(PluginHost::IStateControl::SUSPENDED);
                    break;
                }
            }

        private:
            SimpleEGLImplementation* _parent;
            runtype _type;
        };

    public:
        SimpleEGLImplementation(const SimpleEGLImplementation&) = delete;
        SimpleEGLImplementation& operator=(const SimpleEGLImplementation&) = delete;

        class Dispatcher : public Core::ThreadPool::IDispatcher {
        public:
            Dispatcher(const Dispatcher&) = delete;
            Dispatcher& operator=(const Dispatcher&) = delete;

            Dispatcher() = default;
            ~Dispatcher() override = default;

        private:
            void Initialize() override {
            }
            void Deinitialize() override {
            }
            void Dispatch(Core::IDispatch* job) override {
                job->Dispatch();
            }
        };

        #ifdef __WINDOWS__
        #pragma warning(disable : 4355)
        #endif
        SimpleEGLImplementation()
            : Core::Thread(0, _T("SimpleEGLImplementation"))
            , _requestedURL()
            , _setURL()
            , _fps(0)
            , _hidden(false)
            , _dispatcher()
            , _executor(1, 0, 4, &_dispatcher)
            , _sink(*this)
            , _service(nullptr)
        {
            TRACE(Trace::Information, (_T("Constructed the SimpleEGLImplementation")));
        }
        #ifdef __WINDOWS__
        #pragma warning(default : 4355)
        #endif
        ~SimpleEGLImplementation() override
        {
            TRACE(Trace::Information, (_T("Destructing the SimpleEGLImplementation")));
            Block();

            if (Wait(Core::Thread::STOPPED | Core::Thread::BLOCKED, _config.Destruct.Value()) == false) {
                TRACE(Trace::Information, (_T("Bailed out before the thread signalled completion. %d ms"), _config.Destruct.Value()));
            }

            TRACE(Trace::Information, (_T("Destructed the SimpleEGLImplementation")));
        }

    public:
        void SetURL(const string& URL) override
        {
            _requestedURL = URL;
            TRACE(Trace::Information, (_T("New URL [%d]: [%s]"), URL.length(), URL.c_str()));
        }
        uint32_t Configure(PluginHost::IShell* service) override
        {
            uint32_t result = Core::ERROR_NONE;

            TRACE(Trace::Information, (_T("Configuring: [%s]"), service->Callsign().c_str()));

            _service = service;

            if (_service) {
                TRACE(Trace::Information, (_T("SimpleEGLImplementation::Configure: AddRef service")));
                _service->AddRef();
            }

            if (result == Core::ERROR_NONE) {

                _dataPath = service->DataPath();
                _config.FromString(service->ConfigLine());

                if (_config.Init.Value() > 0) {
                    TRACE(Trace::Information, (_T("Configuration requested to take [%d] mS"), _config.Init.Value()));
                    SleepMs(_config.Init.Value());
                }
                Run();
            }

            return (result);
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

            // Make sure a sink is not registered multiple times.
            ASSERT(std::find(_notificationClients.begin(), _notificationClients.end(), sink) == _notificationClients.end());

            _notificationClients.push_back(sink);
            sink->AddRef();

            TRACE(Trace::Information, (_T("IStateControl::INotification Registered: %p"), sink));
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
            _adminLock.Lock();

            // Make sure a sink is not registered multiple times.
            ASSERT(std::find(_browserClients.begin(), _browserClients.end(), sink) == _browserClients.end());

            _browserClients.push_back(sink);
            sink->AddRef();

            TRACE(Trace::Information, (_T("IBrowser::INotification Registered: %p"), sink));
            _adminLock.Unlock();
        }

        virtual void Unregister(Exchange::IBrowser::INotification* sink)
        {
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
                _executor.Submit(Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(*this, Job::SUSPENDED)), 20000);
                result = Core::ERROR_NONE;
                break;
            case PluginHost::IStateControl::RESUME:
                _executor.Submit(Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(*this, Job::RESUMED)), 20000);
                result = Core::ERROR_NONE;
                break;
            }

            return (result);
        }

        PluginHost::IStateControl::state State() const override
        {
            return (PluginHost::IStateControl::RESUMED);
        }

        void Hide(const bool hidden) override
        {

            if (hidden == true) {
                TRACE(Trace::Information, (_T("Requestsed a Hide.")));
                _executor.Submit(Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(*this, Job::HIDE)), 20000);
            } else {
                TRACE(Trace::Information, (_T("Requestsed a Show.")));
                _executor.Submit(Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(*this, Job::SHOW)), 20000);
            }
        }
        void StateChange(const PluginHost::IStateControl::state state)
        {
            _adminLock.Lock();

            std::list<PluginHost::IStateControl::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end()) {
                TRACE(Trace::Information, (_T("State change from SimpleEGLTest 0x%X"), state));
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
                TRACE(Trace::Information, (_T("State change from SimpleEGLTest 0x%X"), __LINE__));
                (*index)->Hidden(hidden);
                index++;
            }
            TRACE(Trace::Information, (_T("Changing state to [%s]"), hidden ? _T("Hidden") : _T("Shown")));

            _hidden = hidden;

            _adminLock.Unlock();
        }

        BEGIN_INTERFACE_MAP(SimpleEGLImplementation)
            INTERFACE_ENTRY(Exchange::IBrowser)
            INTERFACE_ENTRY(PluginHost::IStateControl)
            INTERFACE_AGGREGATE(PluginHost::IPlugin::INotification,static_cast<IUnknown*>(&_sink))
        END_INTERFACE_MAP

    private:

        virtual uint32_t Worker()
        {
            TRACE(Trace::Information, (_T("Main task of execution reached. Starting with a Sleep of [%d] S"), _config.Sleep.Value()));
            // First Sleep the expected time..
            SleepMs(_config.Sleep.Value() * 1000);

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
        Dispatcher _dispatcher;
        Core::ThreadPool _executor;
        Core::Sink<PluginMonitor> _sink;
        PluginHost::IShell* _service;
    };

    SERVICE_REGISTRATION(SimpleEGLImplementation, 1, 0);

} // namespace Plugin

} // namespace WPEFramework::SimpleEGL
