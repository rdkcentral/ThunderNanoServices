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

#include <interfaces/IComposition.h>
#include <com/com.h>

#include "ModeSet.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#ifdef __cplusplus
}
#endif

template <class T>
struct _remove_const {
    typedef T type;
};

template <class T>
struct _remove_const <T const> {
    typedef T type;
};

template <class T>
struct _remove_const <T *> {
    typedef T * type;
};

template <class T>
struct _remove_const <T const *>{
    typedef T * type;
};

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

// Suppress compiler warnings of unused (parameters)
// Omitting the name is sufficient but a search on this keyword provides easy access to the location
template <typename T>
void silence (T &&) {}

namespace WPEFramework {
namespace Plugin {

class CompositorImplementation;

    class ClientSurface : public Exchange::IComposition::IClient, Exchange::IComposition::IRender {
    public:
        ClientSurface() = delete;
        ClientSurface(const ClientSurface&) = delete;
        ClientSurface& operator= (const ClientSurface&) = delete;

        ClientSurface(ModeSet& modeSet, CompositorImplementation& compositor, const string& name, const uint32_t width, const uint32_t height);
        ~ClientSurface() override;

    public:
        RPC::instance_id Native () const override {
            // Sharing this handle does not imply its underlying contents can be accessed!
            return reinterpret_cast < RPC::instance_id > ( _nativeSurface );
        }
        string Name() const override
        {
                return _name;
        }
        void Opacity(const uint32_t value) override {
            TRACE(Trace::Error, (_T("Opacity is currently not supported for Surface %s"), _name.c_str()));
        }
        uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override {
            TRACE(Trace::Error, (_T("Geometry is currently not supported for Surface %s"), _name.c_str()));
            return Core::ERROR_UNAVAILABLE;
        }
        Exchange::IComposition::Rectangle Geometry() const override {
            return _destination;
        }
        uint32_t ZOrder(const uint16_t zorder) override {
            TRACE(Trace::Error, (_T("ZOrder is currently not supported for Surface %s"), _name.c_str()));
            return Core::ERROR_UNAVAILABLE;
        }
        uint32_t ZOrder() const override {
            return _layer;
        }

        void ScanOut () override;

        BEGIN_INTERFACE_MAP (ClientSurface)
            INTERFACE_ENTRY (Exchange::IComposition::IClient)
            INTERFACE_ENTRY (Exchange::IComposition::IRender)
        END_INTERFACE_MAP

    private:
        ModeSet& _modeSet;
        CompositorImplementation& _compositor; 
        const std::string _name;

        // The buffer acts as a surface for the remote site
        struct gbm_bo * _nativeSurface;

        uint32_t _opacity;
        uint32_t _layer;

        Exchange::IComposition::Rectangle _destination;
    };

    class CompositorImplementation : public Exchange::IComposition, public Exchange::IComposition::IDisplay {
    private:
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        class ExternalAccess : public RPC::Communicator {
        private:
            ExternalAccess() = delete;
            ExternalAccess(const ExternalAccess&) = delete;
            ExternalAccess& operator=(const ExternalAccess&) = delete;

        public:
            ExternalAccess(
                CompositorImplementation& parent, 
                const Core::NodeId& source, 
                const string& proxyStubPath, 
                const Core::ProxyType<RPC::InvokeServer>& handler)
                : RPC::Communicator(source,  proxyStubPath.empty() == false ? Core::Directory::Normalize(proxyStubPath) : proxyStubPath, Core::ProxyType<Core::IIPCServer>(handler))
                , _parent(parent)
            {
                uint32_t result = RPC::Communicator::Open(RPC::CommunicationTimeOut);

                handler->Announcements(Announcement());

                if (result != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Could not open RPI Compositor RPCLink server. Error: %s"), Core::NumberType<uint32_t>(result).Text()));
                } else {
                    // We need to pass the communication channel NodeId via an environment variable, for process,
                    // not being started by the rpcprocess...
                    Core::SystemInfo::SetEnvironment(_T("COMPOSITOR"), RPC::Communicator::Connector(), true);
                }
            }

            ~ExternalAccess() override = default;

        private:
            void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t version) override
            {
                // Use the className to check for multiple HDMI's. 
                return (_parent.QueryInterface(interfaceId));
            }

        private:
            CompositorImplementation& _parent;
        };

        class Natives {
            private :

                ModeSet /*const*/ & _set;

                using dpy_return_t = decltype ( std::declval < ModeSet > ().UnderlyingHandle () );
                using surf_return_t = decltype ( std::declval < ModeSet > ().CreateRenderTarget (0, 0) );

                static_assert (std::is_pointer < surf_return_t > ::value != false);
                surf_return_t _surf = nullptr;

                bool _valid = false;

            public :

                using dpy_t = dpy_return_t;
                using surf_t = decltype (_surf);
                using valid_t = decltype (_valid);

                Natives () = delete;
                Natives (ModeSet /*const*/ & set) : _set { set }, _valid { Initialize () } {}
                ~Natives () {
                    _valid = false;
                    DeInitialize ();
                }

                dpy_t Display () const { return _set.UnderlyingHandle (); }
                surf_t Surface () const { return _surf; }

                valid_t Valid () const { return _valid; }

            private :

                bool Initialize () {
                    bool _ret = false;

                    // The argument to Open is unused, any suffices
                    static_assert (std::is_pointer < dpy_t > ::value != false);
                    _ret =_set.Open ("") == Core::ERROR_NONE && Display () != nullptr;

                    using width_t = decltype ( std::declval < ModeSet > ().Width () );
                    using height_t = decltype ( std::declval < ModeSet > ().Height () );

                    width_t _width = _set.Width ();
                    height_t _height = _set.Height ();

                    if (_ret != false) {
                        _surf = _set.CreateRenderTarget (_width, _height);
                        _ret = _surf != nullptr;
                    }

                    if (_ret != true) {
                        static_assert (( std::is_integral < width_t >::value && std::is_integral < height_t >::value ) != false);
                        TRACE (Trace::Error, (_T ("Unable to create a compositor surface of dimensions: %d x %d [width, height]))."), _width, _height));
                    }

                    return _ret;
                }

                void DeInitialize () {
                    _valid = false;

                    if (_surf != nullptr) {
                        _set.DestroyRenderTarget (_surf);
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
                    /* void */ glClearColor (_rad, _rad, _rad, 0.0);

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

//                static_assert ( (std::is_convertible < decltype (_dpy->Native ()), EGLNativeDisplayType > :: value) != false);
//                static_assert ( (std::is_convertible < decltype (_surf->Native ()), EGLNativeWindowType > :: value) != false);

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
                        using native_dpy_no_const =  _remove_const < Natives::dpy_t > :: type;

                        // Again. EGL does use const sparsely
                        _dpy = eglGetDisplay (const_cast < native_dpy_no_const > (_natives.Display ()) );
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

                        _surf = eglCreateWindowSurface (_dpy, _conf, _natives.Surface (), &_attr [0]);
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

                        // Avoid any memory leak if the local thread is stopped (by another thread)
                        _ret = eglMakeCurrent (_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_FALSE && _ret;
                    }

                    return _ret;
                }
        };

    public:
        CompositorImplementation()
            : _adminLock()
            , _service(nullptr)
            , _engine()
            , _externalAccess(nullptr)
            , _observers()
            , _clients()
            , _platform()
            , _natives (_platform)
            , _egl (_natives)
        {
        }
        ~CompositorImplementation()
        {
            _clients.Clear();
            if (_externalAccess != nullptr) {
                delete _externalAccess;
                _engine.Release();
            }
        }

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        INTERFACE_ENTRY(Exchange::IComposition::IDisplay)
        END_INTERFACE_MAP

    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connector(_T("/tmp/compositor"))
                , Port(_T("HDMI0"))
            {
                Add(_T("connector"), &Connector);
                Add(_T("port"), &Port);
            }

            ~Config()
            {
            }

        public:
            Core::JSON::String Connector;
            Core::JSON::String Port;
        };

    public:

        bool CompositeFor (std::string const & name) {
            bool _ret = false;

            TRACE (Trace::Error, (_T ("%s requested an update. Update is currently not supported."), name.c_str ()));

            /* ProxyType <> */ auto _client = _clients.Find (name);

            if (name.compare (_client->Name ()) != 0) {
                TRACE (Trace::Error, (_T ("%s does not appear to be a valid client."), name.c_str ()));
            }
            else {
//                /* RPC::instance_id */ _client->Native ();

                static GLES _gles;

                _ret = _egl.Valid () != false &&  _egl.Render ( _gles );

                if (_ret != false) {
                    ModeSet::BufferInfo _bufferInfo = { _natives.Surface (), nullptr, 0 };
                    /* void */ _platform.Swap (_bufferInfo);
                }


            }

            return _ret;
        }

        //
        // Echange::IComposition
        // ==================================================================================================================

        static uint32_t WidthFromResolution(const ScreenResolution resolution) {
            // Asumme an invalid width equals 0
            uint32_t _width = 0;

            switch (resolution) {
                case ScreenResolution_480p      : // 720x480
                                                    _width = 720; break;
                case ScreenResolution_720p      : // 1280x720 progressive
                case ScreenResolution_720p50Hz  : // 1280x720 @ 50 Hz
                                                    _width = 720; break;
                case ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
                case ScreenResolution_1080i50Hz : // 1920x1080 interlaced  @ 50 Hz
                case ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
                case ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
                                                    _width = 1080; break;
                case ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
                case ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
                                                    _width = 2160; break;
                case ScreenResolution_480i      : // Unknown according to the standards (?)
                case ScreenResolution_Unknown   :
                default                         : _width = 0;
            }

            return _width;
        }

        static uint32_t HeightFromResolution(const ScreenResolution resolution) {
            // Asumme an invalid height equals 0
            uint32_t _height = 0;

            switch (resolution) {
                case ScreenResolution_480i      :
                case ScreenResolution_480p      : _height = 480; break;
                case ScreenResolution_720p      :
                case ScreenResolution_720p50Hz  : _height =720; break;
                case ScreenResolution_1080p24Hz :
                case ScreenResolution_1080i50Hz :
                case ScreenResolution_1080p50Hz :
                case ScreenResolution_1080p60Hz : _height = 1080; break;
                case ScreenResolution_2160p50Hz :
                case ScreenResolution_2160p60Hz : _height = 2160; break;
                case ScreenResolution_Unknown   :
                default                         : _height = 0;
            }

            return _height;
        }

        static ScreenResolution ResolutionFromHeightWidth(const uint32_t height, const uint32_t width) {
            // Given the options, the refresh rate is also important so the only sensible value is 'unknown'
            return Exchange::IComposition::ScreenResolution_Unknown;
        }

        uint32_t Configure(PluginHost::IShell* service) override
        {
            uint32_t result = Core::ERROR_NONE;
            _service = service;

            string configuration(service->ConfigLine());
            Config config;
            config.FromString(service->ConfigLine());

            _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());
            _externalAccess = new ExternalAccess(*this, Core::NodeId(config.Connector.Value().c_str()), service->ProxyStubPath(), _engine);

            if ((_externalAccess->IsListening() == true)) {
                _port = config.Port.Value();
                PlatformReady();
            } else {
                delete _externalAccess;
                _externalAccess = nullptr;
                _engine.Release();
                TRACE(Trace::Error, (_T("Could not report PlatformReady as there was a problem starting the Compositor RPC %s"), _T("server")));
                result = Core::ERROR_OPENING_FAILED;
            }
            return result;
        }
        void Register(Exchange::IComposition::INotification* notification) override
        {
            _adminLock.Lock();
            ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());
            notification->AddRef();
            _observers.push_back(notification);

            _clients.Visit([=](const string& name, const Core::ProxyType<ClientSurface>& element){
                notification->Attached(name, &(*element));
            });

            _adminLock.Unlock();
        }
        void Unregister(Exchange::IComposition::INotification* notification) override
        {
            _adminLock.Lock();
            std::list<Exchange::IComposition::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));
            ASSERT(index != _observers.end());
            if (index != _observers.end()) {
                _observers.erase(index);
                notification->Release();
            }
            _adminLock.Unlock();
        }
        void Attached(const string& name, IClient* client) {
            _adminLock.Lock();
            for(auto& observer : _observers) {
                observer->Attached(name, client);
            }
            _adminLock.Unlock();
        }
        void Detached(const string& name) {
            _adminLock.Lock();
            for(auto& observer : _observers) {
                observer->Detached(name);
            }
            _adminLock.Unlock();
        }

        //
        // Echange::IComposition::IDisplay
        // ==================================================================================================================
        RPC::instance_id Native () const override {
            using class_t = std::remove_reference < decltype (_natives) > ::type;
            using return_t = decltype ( std::declval < class_t > ().Display () );

            // The EGL API uses const very sparsely, and here, a const is also not expected
            using return_no_const_t  = _remove_const < return_t > :: type;

            static_assert ( (std::is_convertible < return_no_const_t, EGLNativeDisplayType > :: value) != false);
            static_assert ( (std::is_convertible < decltype (EGL_DEFAULT_DISPLAY), EGLNativeDisplayType > :: value) != false);
            // Likely to almost always fail
//            static_assert ( (std::is_convertible < return_no_const_t, RPC::instance_id > :: value) != false);

            EGLNativeDisplayType result ( static_cast < EGLNativeDisplayType > ( EGL_DEFAULT_DISPLAY ) );

            if (_natives.Valid () != false) {
            // Just remove the unexpected const if it exist
                result = static_cast < EGLNativeDisplayType > (const_cast < return_no_const_t > ( _natives.Display () ));
            }
            else {
                TRACE (Trace::Error, (_T ("The native display (id) might be invalid / unsupported. Using the EGL default display instead!")));
            }

            return reinterpret_cast < RPC::instance_id > ( result );
        }

        string Port() const override {
            return (_port);
        }
        IClient* CreateClient(const string& name, const uint32_t width, const uint32_t height) override {
            IClient* client = nullptr;

                Core::ProxyType<ClientSurface> object = _clients. template Instance(
                             name,
                             _platform,
                             *this,
                             name,
                             width,
                             height);

                ASSERT(object.IsValid() == true);

                if( object.IsValid() == true ) {
                    client = &(*object);
                    Attached (name, client);
                }

            if(client == nullptr) {
                TRACE(Trace::Error, (_T("Could not create the Surface with name %s"), name.c_str()));
            }

            return (client);
        }

        Exchange::IComposition::ScreenResolution Resolution() const override
        {
            Exchange::IComposition::ScreenResolution _resolution = WPEFramework::Exchange::IComposition::ScreenResolution_Unknown;

            decltype ( std::declval <ModeSet> ().Width () ) _width = _platform.Width (); silence (_width);
            decltype ( std::declval <ModeSet> ().Height () ) _height = _platform.Height ();

// TODO: This might not be the whole story to determine progressive versus interlaced

            decltype ( std::declval <ModeSet> ().RefreshRate () ) _vrefresh = _platform.RefreshRate ();
            decltype ( std::declval <ModeSet> ().Interlaced () ) _interlaced = _platform.Interlaced ();

            if (_interlaced != true) {
                switch (_height) {
                    case 480    :   {
                                        _resolution = ScreenResolution_480p;
                                        break;
                                    }
                    case 720    :   {
                                        _resolution = _vrefresh != 50 ? ScreenResolution_720p : ScreenResolution_720p50Hz;
                                        break;
                                    }
                    case 1080   :   {
                                        switch (_vrefresh) {
                                            case 24 : _resolution = ScreenResolution_1080p24Hz; break;
                                            case 50 : _resolution = ScreenResolution_1080p50Hz; break;
                                            case 60 : _resolution = ScreenResolution_1080p60Hz; break;
                                            default : _resolution = ScreenResolution_Unknown;
                                        }
                                        break;
                                    }
                    case 2160   :   {
                                        switch (_vrefresh) {
                                            case 50 : _resolution = ScreenResolution_2160p50Hz; break;
                                            case 60 : _resolution = ScreenResolution_2160p60Hz; break;
                                            default : _resolution = ScreenResolution_Unknown;
                                        }
                                        break;
                                    }
                    default     :   {
                                        _resolution = ScreenResolution_Unknown;
                                    }
                }
            }
            else {
                switch (_height) {
                    case 480    :   {
                                        _resolution = ScreenResolution_480i;
                                        break;
                                    }
                    case 1080   :   {
                                        _resolution = _vrefresh != 50 ? ScreenResolution_Unknown : ScreenResolution_1080i50Hz;
                                        break;
                                    }
                    default     :   {
                                        _resolution = ScreenResolution_Unknown;
                                    }
                }
            }

            return _resolution;

        }
        uint32_t Resolution(const Exchange::IComposition::ScreenResolution format) override
        {
            TRACE(Trace::Error, (_T("Could not set screenresolution to %s. Not supported for Rapberry Pi compositor"), Core::EnumerateType<Exchange::IComposition::ScreenResolution>(format).Data()));
            return (Core::ERROR_UNAVAILABLE);
        }

    private:
        void PlatformReady()
        {
            PluginHost::ISubSystem* subSystems(_service->SubSystems());
            ASSERT(subSystems != nullptr);
            if (subSystems != nullptr) {
                subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
                subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                subSystems->Release();
            }
        }

    private:

        using ClientContainer = Core::ProxyMapType<string, ClientSurface>;

        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Core::ProxyType<RPC::InvokeServer> _engine;
        ExternalAccess* _externalAccess;
        std::list<Exchange::IComposition::INotification*> _observers;
        ClientContainer _clients;
        string _port;
        ModeSet _platform;

        Natives _natives;
        EGL _egl;
    };

    // ODR-use
    /* static */ constexpr void * const CompositorImplementation::EGL::EGL_NO_CONFIG;

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

    ClientSurface::ClientSurface(ModeSet& modeSet, CompositorImplementation& compositor, const string& name, const uint32_t width, const uint32_t height)
        : _modeSet(modeSet)
        , _compositor(compositor)
        , _name(name)
        , _nativeSurface { nullptr }
        , _opacity(Exchange::IComposition::maxOpacity)
        , _layer(0)
        , _destination( { 0, 0, width, height } ) {

        using surface_t = decltype (_nativeSurface);
        using class_t = std::remove_reference < decltype (_modeSet) > ::type;
        using return_t = decltype ( std::declval < class_t > ().CreateBufferObject (width, height) );

        static_assert (std::is_same < surface_t, return_t> :: value != false);

// TODO: The internal scan out flag might not be appropriate
        _nativeSurface = _modeSet.CreateBufferObject (width, height);

        if (_nativeSurface == nullptr) {
            TRACE (Trace::Error, (_T ("A sharing buffer/surface cannot be created for %s"), name.c_str ()));
        }

    }

    ClientSurface::~ClientSurface() {

        _compositor.Detached(_name);

        if (_nativeSurface != nullptr) {
            _modeSet.DestroyBufferObject (_nativeSurface);
        }

    }

    void ClientSurface::ScanOut () {
        _compositor.CompositeFor (_name);
    }

} // namespace Plugin
} // namespace WPEFramework
