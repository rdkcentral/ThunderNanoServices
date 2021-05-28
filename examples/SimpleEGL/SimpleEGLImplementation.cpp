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

#include <vector>
#include <cmath>
#include <type_traits>
#include <limits>
#include <cstdlib>


namespace WPEFramework {
namespace Plugin {

    class SimpleEGLImplementation 
        : public Exchange::IBrowser
        , public PluginHost::IStateControl
        , public Core::Thread {

    private:

        class RenderThread : public Core::Thread {

            public: 

                static constexpr const char* Name () {
                    return "SimpleEGL:RenderThread";
                }

                RenderThread () : RenderThread (Core::Thread::DefaultStackSize (), RenderThread::Name ()) {
                }

                RenderThread (const uint32_t _stacksize, const TCHAR* _threadName ) : Core::Thread (_stacksize, _threadName), _n_dpy (nullptr), _n_surf (nullptr), _e_dpy (EGL_NO_DISPLAY), _e_surf (EGL_NO_SURFACE), _e_cont (EGL_NO_CONTEXT) {
                }

                ~RenderThread () override {
                    constexpr const char SKIP_EGL_CLEANUP[] = "SKIP_EGL_CLEANUP";
                    constexpr const char SKIP_NATIVE_CLEANUP[] = "SKIP_NATIVE_CLEANUP";

                    const char* env_egl = std::getenv (SKIP_EGL_CLEANUP);
                    if (env_egl != nullptr) {
                        std::cout << "Skipping releasing EGL resources" << std::endl;
                    }
                    else {
                        if (_e_dpy != EGL_NO_DISPLAY) {
                            if (_e_cont != EGL_NO_CONTEXT) {
                                /* EGLBoolean */ eglDestroyContext(_e_dpy, _e_cont);
                                _e_cont = EGL_NO_CONTEXT;
                            }

                            if (_e_surf != EGL_NO_SURFACE) {
                                /* EGLBoolean */ eglDestroySurface (_e_dpy, _e_surf);
                                _e_surf = EGL_NO_SURFACE;
                            }

                            /* EGLBoolean */ eglTerminate (_e_dpy);
                            _e_dpy = EGL_NO_DISPLAY;
                        }
                    }

                    const char* env_native = std::getenv (SKIP_NATIVE_CLEANUP);
                    if (env_native != nullptr) {
                        std::cout << "Skipping releasing native resources" << std::endl;
                    }
                    else {
                        if (_n_surf != nullptr) {
                            /* uint32_t */ _n_surf->Release ();
                            _n_surf = nullptr;
                        }

                        if (_n_dpy != nullptr) {
                            /* uint32_t */ _n_dpy->Release ();
                            _n_dpy = nullptr;
                        }
                    }
                }

            private :
                Compositor::IDisplay* _n_dpy;
                Compositor::IDisplay::ISurface* _n_surf;

                EGLDisplay _e_dpy;
                EGLSurface _e_surf;
                EGLContext _e_cont;

                uint32_t _width;
                uint32_t _height;

                bool Initialize () override {

                    static_assert (std::is_pointer <EGLConfig>::value);

                    constexpr void* EGL_NO_CONFIG = nullptr;

                    auto Config = [] (EGLDisplay& _e_dpy) -> EGLConfig {
                        constexpr EGLint const _e_attr[] = {
                            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                            EGL_RED_SIZE    , 8,
                            EGL_GREEN_SIZE  , 8,
                            EGL_BLUE_SIZE   , 8,
                            //EGL_ALPHA_SIZE  , 8,
                            //EGL_BUFFER_SIZE , 32,
                            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                            EGL_NONE
                        };

                        EGLint _e_count = 0;

                        if (eglGetConfigs (_e_dpy, nullptr, 0, &_e_count) != EGL_TRUE) {
                            _e_count = 1;
                        }

                        std::vector <EGLConfig> _e_conf (_e_count, EGL_NO_CONFIG);

                        /* EGLBoolean */ eglChooseConfig (_e_dpy, &_e_attr[0], _e_conf.data (), _e_conf.size (), &_e_count);

                        return _e_conf [0];
                    };

                    bool _ret = false;

                    if (_n_dpy != nullptr) {
                        if (_n_dpy->Release () != Core::ERROR_NONE) {
                        }
                        else {
                            _ret = true;
                        }
                    }
                    else {
                        _ret = true;
                    }

                    if (_ret != false) {
                        _n_dpy = Compositor::IDisplay::Instance (Name ());

                        _ret = _n_dpy != nullptr;
                    }

                    if (_ret != false) {
                        _n_surf = _n_dpy->Create (Name (), _width, _height);

                        _ret = _n_surf != nullptr;
                    }

                    if (_ret != false) {
                        _width = _n_surf->Width ();
                        _height = _n_surf->Height ();
                    }

                    if (_ret != false) {
                        _e_dpy = eglGetDisplay (_n_dpy->Native ());

                        if (_e_dpy != EGL_NO_DISPLAY) {
                           _ret = eglInitialize (_e_dpy, nullptr, nullptr) != EGL_FALSE;
                        }
                    }

                    EGLConfig _e_conf = EGL_NO_CONFIG;

                    if (_ret != false) {
                        _e_surf = EGL_NO_SURFACE;

                        _e_conf = Config (_e_dpy);

                        if (_e_conf != EGL_NO_CONFIG) {
                            constexpr EGLint _e_attr[] = {
                                EGL_NONE
                            };

                            _e_surf = eglCreateWindowSurface (_e_dpy, _e_conf, _n_surf->Native (), &_e_attr[0]);
                        }

                        _ret = _e_surf != EGL_NO_SURFACE;
                    }

                    if (_ret != false) {
                        constexpr EGLint const _e_attr[] = {
                            EGL_CONTEXT_CLIENT_VERSION, 2,
                            EGL_NONE
                        };

                        _e_cont = eglCreateContext (_e_dpy, _e_conf, EGL_NO_CONTEXT, _e_attr);

                        _ret = _e_cont != EGL_NO_CONTEXT;
                    }

                    return _ret;
                }

                uint32_t Worker () override {

                    auto Color = [] (float degree) -> bool {
                        bool _ret = false;

                        constexpr float OMEGA = 3.14159265 / 180;

                        // Here, for C(++) these type should be identical
                        // Type information: https://www.khronos.org/opengl/wiki/OpenGL_Type
                        static_assert (std::is_same <float, GLfloat>::value);

                        GLfloat _rad = static_cast <GLfloat> (cos (degree * OMEGA));

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

                        return _ret;
                    };

                    uint32_t _ret = 0;

                    if (eglMakeCurrent(_e_dpy, _e_surf, _e_surf, _e_cont) != EGL_FALSE) {
                        constexpr uint16_t ROTATION = 360;

                        static uint16_t _degree = 0;

                        static_assert (std::numeric_limits <uint16_t>::max () <= std::numeric_limits <float>::max ());

                        /* bool */ Color (static_cast <float> (_degree));

                        /* EGLBoolean */ eglSwapBuffers (_e_dpy, _e_surf);

                        /* EGLBoolean */ eglMakeCurrent (_e_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

                        _degree = (_degree + 1) % ROTATION;
                    }

                    return _ret;
                }
        };

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
            void StateChange(PluginHost::IShell* service) override
            {
                if (service->State() == PluginHost::Service::ACTIVATED) {
                    string name(service->Callsign());

                    Exchange::ITimeSync* time = service->QueryInterface<Exchange::ITimeSync>();
                    if (time != nullptr) {
                        printf("Time interface supported\n");
                        time->Release();
                    }
                }
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
                : Sleep(90)
                , Crash(false)
                , Destruct(1000)
                , Single(false)
            {
                Add(_T("sleep"), &Sleep);
                Add(_T("crash"), &Crash);
                Add(_T("destruct"), &Destruct);
                Add(_T("single"), &Single);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Sleep;
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
            ~Job()
            {
            }

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
                    _parent->Hidden(false);
                    _parent->_hidden = false;
                    break;
                case HIDE:
                    _parent->Hidden(true);
                    _parent->_hidden = true;
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

        #ifdef __WINDOWS__
        #pragma warning(disable : 4355)
        #endif
        SimpleEGLImplementation()
            : Core::Thread(0, _T("SimpleEGLImplementation"))
            , _requestedURL()
            , _setURL()
            , _fps(0)
            , _hidden(false)
            , _executor(1, 0, 4)
            , _sink(*this)
            , _renderThread (Core::Thread::DefaultStackSize (), RenderThread::Name ())
        {
            fprintf(stderr, "---------------- Constructed the SimpleEGLImplementation ----------------------\n"); fflush(stderr);
        }

        #ifdef __WINDOWS__
        #pragma warning(default : 4355)
        #endif
        ~SimpleEGLImplementation() override
        {
            fprintf(stderr, "---------------- Destructing the SimpleEGLImplementation ----------------------\n"); fflush(stderr);
            Block();

            _renderThread.Stop ();

            if (_renderThread.Wait (Core::Thread::STOPPED | Core::Thread::BLOCKED, Core::infinite) != true) {
                TRACE_L1 ("Unable to stop the render thread properly."); 
            }

            if (Wait(Core::Thread::STOPPED | Core::Thread::BLOCKED, _config.Destruct.Value()) == false)
                TRACE_L1("Bailed out before the thread signalled completion. %d ms", _config.Destruct.Value());

            fprintf(stderr, "---------------- Destructed the SimpleEGLImplementation -----------------------\n"); fflush(stderr);
        }

        static constexpr const char* Name () {
            return "SimpleEGLImplementation";
        }

    public:
        virtual void SetURL(const string& URL)
        {
            _requestedURL = URL;

            TRACE(Trace::Information, (_T("New URL: %s"), URL.c_str()));

            TRACE_L1("Received a new URL: %s", URL.c_str());
            TRACE_L1("URL length: %u", static_cast<uint32_t>(URL.length()));

            Run();
        }

        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            fprintf(stderr, "---------------- Configuring the SimpleEGLImplementation -----------------------\n"); fflush(stderr);
            _dataPath = service->DataPath();
            _config.FromString(service->ConfigLine());
            _endTime = Core::Time::Now();

            if (_config.Sleep.Value() > 0) {
                TRACE_L1("Going to sleep for %d seconds.", _config.Sleep.Value());
                _endTime.Add(1000 * _config.Sleep.Value());
            }

            Run();
            fprintf(stderr, "---------------- Configured the SimpleEGLImplementation ------------------------\n"); fflush(stderr);
            return (Core::ERROR_NONE);
        }
        virtual string GetURL() const
        {
            string message;

            for (unsigned int teller = 0; teller < 120; teller++) {
                message += static_cast<char>('0' + (teller % 10));
            }
            return (message);
        }
        virtual bool IsVisible() const
        {
            return (!_hidden);
        }
        virtual uint32_t GetFPS() const
        {
            TRACE(Trace::Fatal, (_T("Fatal ingested: %d!!!"), _fps));
            return (++_fps);
        }
        virtual void Register(PluginHost::IStateControl::INotification* sink)
        {
            _adminLock.Lock();

            // Make sure a sink is not registered multiple times.
            ASSERT(std::find(_notificationClients.begin(), _notificationClients.end(), sink) == _notificationClients.end());

            _notificationClients.push_back(sink);
            sink->AddRef();

            TRACE_L1("IStateControl::INotification Registered in webkitimpl: %p", sink);
            _adminLock.Unlock();
        }

        virtual void Unregister(PluginHost::IStateControl::INotification* sink)
        {
            _adminLock.Lock();

            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_notificationClients.begin(), _notificationClients.end(), sink));

            // Make sure you do not unregister something you did not register !!!
            ASSERT(index != _notificationClients.end());

            if (index != _notificationClients.end()) {
                TRACE_L1("IStateControl::INotification Removing registered listener from browser %d", __LINE__);
                (*index)->Release();
                _notificationClients.erase(index);
            }

            _adminLock.Unlock();
        }
        virtual void Register(Exchange::IBrowser::INotification* sink)
        {
            _adminLock.Lock();

            // Make sure a sink is not registered multiple times.
            ASSERT(std::find(_browserClients.begin(), _browserClients.end(), sink) == _browserClients.end());

            _browserClients.push_back(sink);
            sink->AddRef();

            TRACE_L1("IBrowser::INotification Registered in webkitimpl: %p", sink);
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
                TRACE_L1("IBrowser::INotification Removing registered listener from browser %d", __LINE__);
                (*index)->Release();
                _browserClients.erase(index);
            }

            _adminLock.Unlock();
        }

        virtual uint32_t Request(const PluginHost::IStateControl::command command)
        {

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

        virtual PluginHost::IStateControl::state State() const
        {
            return (PluginHost::IStateControl::RESUMED);
        }

        virtual void Hide(const bool hidden)
        {

            if (hidden == true) {

                printf("Hide called. About to sleep for 2S.\n");
                _executor.Submit(Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(*this, Job::HIDE)), 20000);
                SleepMs(2000);
                printf("Hide completed.\n");
            } else {
                printf("Show called. About to sleep for 4S.\n");
                _executor.Submit(Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(*this, Job::SHOW)), 20000);
                SleepMs(4000);
                printf("Show completed.\n");
            }
        }

        virtual void Precondition(const bool)
        {
            printf("We are good to go !!!.\n");
        }
        virtual void Closure()
        {
            printf("Closure !!!!\n");
        }
        void StateChange(const PluginHost::IStateControl::state state)
        {
            _adminLock.Lock();

            std::list<PluginHost::IStateControl::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end()) {
                TRACE_L1("State change from SimpleEGLImplementation 0x%X", state);
                (*index)->StateChange(state);
                index++;
            }

            _adminLock.Unlock();
        }
        void Hidden(const bool hidden)
        {
            _adminLock.Lock();

            std::list<Exchange::IBrowser::INotification*>::iterator index(_browserClients.begin());

            while (index != _browserClients.end()) {
                TRACE_L1("State change from SimpleEGLImplementation 0x%X", __LINE__);
                (*index)->Hidden(hidden);
                index++;
            }

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
            fprintf(stderr, "---------------- Running the SimpleEGLImplementation ------------------------\n"); fflush(stderr);
            // First Sleep the expected time..
            SleepMs(_config.Sleep.Value() * 1000);

            fprintf(stderr, "---------------- Notifying the SimpleEGLImplementation ------------------------\n"); fflush(stderr);
            _adminLock.Lock();

            std::list<Exchange::IBrowser::INotification*>::iterator index(_browserClients.begin());

            _setURL = _requestedURL;

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
                fprintf(stderr, "---------------- Crashing the SimpleEGLImplementation ------------------------\n"); fflush(stderr);
                TRACE_L1("Going to CRASH as requested %d.", 0);
                abort();
            }

            // Just do nothing :-)
            Block();

            fprintf(stderr, "---------------- Completed the SimpleEGLImplementation ------------------------\n"); fflush(stderr);

            _renderThread.Run ();

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
        Core::Time _endTime;
        std::list<PluginHost::IStateControl::INotification*> _notificationClients;
        std::list<Exchange::IBrowser::INotification*> _browserClients;
        Core::ThreadPool _executor;
        Core::Sink<PluginMonitor> _sink;
        RenderThread _renderThread;
    };

    SERVICE_REGISTRATION(SimpleEGLImplementation, 1, 0);

} // namespace Plugin

} // namespace WPEFramework::SimpleEGL
