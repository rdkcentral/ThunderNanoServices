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

        class Natives {
            private :

                Compositor::IDisplay * _dpy = nullptr;
                Compositor::IDisplay::ISurface * _surf = nullptr;

                bool _valid = false;

            public :

                using dpy_t = decltype (_dpy);
                using surf_t = decltype (_surf);
                using valid_t = decltype (_valid);

                Natives () : _valid { Initialize () } {}
                ~Natives () {
                    _valid = false;
                    DeInitialize ();
                }

                dpy_t Display () const { return _dpy; }
                surf_t Surface () const { return _surf; }

                static_assert ( (std::is_convertible < decltype (_dpy->Native ()), EGLNativeDisplayType > :: value) != false);
                static_assert ( (std::is_convertible < decltype (_surf->Native ()), EGLNativeWindowType > :: value) != false);

                valid_t Valid () const { return _valid; }

            private :

                bool Initialize () {
                    constexpr char const * Name  = "SimpleEGL::Worker";

                    bool _ret = false;

                    if (_dpy != nullptr) {
                        _ret = false;

                        if (_dpy->Release () == Core::ERROR_NONE) {
                            _ret = true;
                        }
                    }
                    else {
                        _ret = true;
                    }

                    if (_ret != false) {
                        // Trigger the creation of the display.
                        _dpy = Compositor::IDisplay::Instance (std::string (Name).append (":Display"));
                        _ret = _dpy != nullptr;
                    }

                    using height_t = decltype (_surf->Height ());
                    using width_t = decltype (_surf->Width ());

                    // Just here to support surface creation, the exact values are probably changed by the underlying platform
                    constexpr height_t _height = 0;
                    constexpr width_t _width = 0;

                    if (_ret != false) {
                        /* void */ _dpy->AddRef ();

                        // Trigger the creation of the surface
                        _surf = _dpy->Create (std::string (Name).append (":Surface"), _width, _height);
                        _ret = _surf != nullptr;
                    }

                    if (_ret != false) {
                        /* void */ _surf->AddRef ();

                        width_t _realWidth = _surf->Width ();
                        height_t _realHeight = _surf->Height ();

                        if (_realWidth == 0 || _realWidth != _width) {
                            TRACE (Trace::Information, (_T ("Real surface width is (%d) invalid or differs from requested width (%d))."), _realWidth, _width));
                        }

                        if (_realHeight == 0 || _realHeight != _height) {
                            TRACE (Trace::Information, (_T ("Real surface height is (%d) invalid or differs from requested height (%d)."), _realHeight, _height));
                        }

                        _ret = _realHeight > 0 && _realWidth > 0;
                    }

                    return _ret;
                }

                void DeInitialize () {
                    _valid = false;

                    if (_surf != nullptr) {
                        _surf->Release ();
                    }

                    if (_dpy != nullptr) {
                        _dpy->Release ();
                    }
                }
        };

        class GLES {
            private :

                uint16_t _degree = 0;

            public :

                GLES () = default;
                ~GLES () = default;

                bool Render () {
                    constexpr decltype (_degree) const ROTATION = 360;

                    constexpr float const OMEGA = 3.14159265 / 180;

                    bool _ret = false;

                    // Here, for C(++) these type should be identical
                    // Type information: https://www.khronos.org/opengl/wiki/OpenGL_Type
                    static_assert (std::is_same <float, GLfloat>::value);

                    GLfloat _rad = static_cast <GLfloat> (cos (_degree * OMEGA));

                    // The function clamps the input to [0, 1]
                    /* void */ glClearColor (_rad, _rad, _rad, 1.0);

                    _ret = glGetError () == GL_NO_ERROR;

                    if (_ret != false) {
                        /* void */ glClear (GL_COLOR_BUFFER_BIT);
                        _ret = glGetError () == GL_NO_ERROR;
                    }

                    if (_ret != false) {
                        /* void */ glFlush ();
                        _ret = glGetError () == GL_NO_ERROR;
                    }

                    _degree = (_degree + 1) % ROTATION;

                    return _ret;
                }
        };

        class EGL  {
            private :

                // Define the 'invalid' value
                static_assert (std::is_pointer <EGLConfig>::value != false);
                static constexpr void * const EGL_NO_CONFIG = nullptr;

                EGLDisplay _dpy = EGL_NO_DISPLAY;
                EGLConfig _conf = EGL_NO_CONTEXT;
                EGLContext _cont = EGL_NO_CONFIG;
                EGLSurface _surf = EGL_NO_SURFACE;

                EGLint _width = 0;
                EGLint _height = 0;

                Natives const & _natives;

                bool _valid = false;

            public :

                using dpy_t = decltype (_dpy);
                using surf_t = decltype (_surf);
                using height_t = decltype (_height);
                using width_t = decltype (_width);
                using valid_t = decltype (_valid);

                EGL () = delete;
                EGL (Natives const & natives) : _natives { natives }, _valid { Initialize () } {}
                ~EGL () {
                    _valid = false;
                    DeInitialize ();
                }

                dpy_t Display () const { return _dpy; }
                surf_t Surface () const { return _surf; }

                height_t Height () const { return _height; }
                width_t Width () const { return _width; }

                valid_t Valid () const { return _valid; }

            private :

                bool Initialize () {
                    bool _ret = _natives.Valid ();

                    if (_ret != false) {

                        if (_dpy != EGL_NO_DISPLAY) {
                            _ret = false;

                            if (eglTerminate (_dpy) != EGL_FALSE) {
                                _ret = true;
                            }
                        }
                    }

                    if (_ret != false) {
                        _dpy = eglGetDisplay (_natives.Display ()->Native ());
                        _ret = _dpy != EGL_NO_DISPLAY;
                    }

                    if (_ret != false) {
                        EGLint _major = 0, _minor = 0;
                        _ret = eglInitialize (_dpy, &_major, &_minor) != EGL_FALSE;

                        // Just take the easy approach (for now)
                        static_assert ((std::is_same <decltype (_major), int> :: value) != false);
                        static_assert ((std::is_same <decltype (_minor), int> :: value) != false);
                        TRACE (Trace::Information, (_T ("EGL version : %d.%d"), static_cast <int> (_major), static_cast <int> (_minor)));
                    }

                    if (_ret != false) {
                        constexpr EGLint const _attr [] = {
                            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                            EGL_RED_SIZE    , 8,
                            EGL_GREEN_SIZE  , 8,
                            EGL_BLUE_SIZE   , 8,
                            EGL_ALPHA_SIZE  , 8,
                            EGL_BUFFER_SIZE , 32,
                            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                            EGL_NONE
                        };

                        EGLint _count = 0;

                        if (eglGetConfigs (_dpy, nullptr, 0, &_count) != EGL_TRUE) {
                            _count = 1;
                        }

                        std::vector <EGLConfig> _confs (_count, EGL_NO_CONFIG);

                        /* EGLBoolean */ eglChooseConfig (_dpy, &_attr [0], _confs.data (), _confs.size (), &_count);

                        _conf = _confs [0];

                        _ret = _conf != EGL_NO_CONFIG;
                    }

                    if (_ret != false) {
                        constexpr EGLint const _attr [] = {
                            EGL_CONTEXT_CLIENT_VERSION, 2,
                            EGL_NONE
                        };

                        _cont = eglCreateContext (_dpy, _conf, EGL_NO_CONTEXT, _attr);
                        _ret = _cont != EGL_NO_CONTEXT;
                    }

                    if (_ret != false) {
                        constexpr EGLint const _attr [] = {
                            EGL_NONE
                        };

                        _surf = eglCreateWindowSurface (_dpy, _conf, _natives.Surface ()->Native (), &_attr [0]);
                        _ret = _surf != EGL_NO_SURFACE;
                    }

                    if (_ret != true) {
                        DeInitialize ();
                    }

                    return _ret;
                }

                void DeInitialize () {
                    _valid = false;
                    /* EGLBoolean */ eglTerminate (_dpy);
                }

            public :

                bool Render (GLES & gles) {
                    bool _ret = Valid () != false && eglMakeCurrent(_dpy, _surf, _surf, _cont) != EGL_FALSE;

                    if (_ret != false) {
                        _ret = gles.Render () != false && eglSwapBuffers (_dpy, _surf) != EGL_FALSE;

                        // Avoid any memory leak if the worker is stopped (by another thread)
                        _ret = eglMakeCurrent (_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_FALSE && _ret;
                    }

                    return _ret;
                }
        };

        virtual uint32_t Worker()
        {
            TRACE(Trace::Information, (_T("Main task of execution reached. Starting with a Sleep of [%d] S"), _config.Sleep.Value()));

            // First Sleep the expected time..
            SleepMs(_config.Sleep.Value() * 1000);

            Natives _natives;

            EGL _egl (_natives);

            if (_egl.Valid () != false) {
                GLES _gles;

                TRACE (Trace::Information, (_T ("Hardware accelerated rendering properly set up!")));

                while (IsRunning() == true && _egl.Render (_gles) != false) {
                    SleepMs(200);
                }
            }
            else {
                TRACE (Trace::Information, (_T ("Hardware accelerated rendering NOT properly set up!")));
            }

            TRACE (Trace::Information, (_T("Leaving the main task of execution, we are done.")));

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

    // As a result of ODR use
    void * const SimpleEGLImplementation::EGL::EGL_NO_CONFIG;

    SERVICE_REGISTRATION(SimpleEGLImplementation, 1, 0);
} // namespace Plugin

} // namespace WPEFramework::SimpleEGL
