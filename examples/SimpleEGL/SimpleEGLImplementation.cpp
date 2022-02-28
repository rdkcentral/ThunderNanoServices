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
#include <interfaces/IComposition.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#ifdef __cplusplus
}
#endif

namespace WPEFramework {
namespace Plugin {

    class SimpleEGLImplementation : public Exchange::IBrowser , public PluginHost::IStateControl , public Core::Thread {
    public:

        class Config : public Core::JSON::Container {
        private :

            Config ( Config const & );
            Config & operator= ( Config const & );

        public :

            Config ();
            ~Config ();

            Core::JSON::DecUInt16 _sleep;
            Core::JSON::DecUInt16 _init;
            Core::JSON::Boolean _crash;
            Core::JSON::DecUInt32 _destruct;
            Core::JSON::Boolean _single;
        };

        class Job : public Core::IDispatch {
        public:

            enum runtype {
                SHOW ,
                HIDE ,
                RESUMED ,
                SUSPENDED 
            };

            Job ();

            Job ( SimpleEGLImplementation & , runtype const );
            Job ( Job const & );
            ~Job () override = default;

            Job & operator= ( Job const & );

            void Dispatch () override;

        private:

            SimpleEGLImplementation * _parent;
            runtype _type;
        };

        class Dispatcher : public Core::ThreadPool::IDispatcher {
        public :

            Dispatcher ( Dispatcher const & ) = delete;
            Dispatcher & operator= ( Dispatcher const & ) = delete;

            Dispatcher () = default;
            ~Dispatcher () override = default;

        private :

            void Initialize () override {}
            void Deinitialize () override {}

            void Dispatch( Core::IDispatch * ) override;
        };



        SimpleEGLImplementation ( SimpleEGLImplementation const & ) = delete;
        SimpleEGLImplementation & operator= ( SimpleEGLImplementation const & ) = delete;

        #ifdef __WINDOWS__
        #pragma warning ( disable : 4355 )
        #endif

        SimpleEGLImplementation ();

        #ifdef __WINDOWS__
        #pragma warning ( default : 4355 )
        #endif

        ~SimpleEGLImplementation () override;

        void SetURL ( string const & ) override;
        string GetURL () const override;

        uint32_t Configure ( PluginHost::IShell * ) override;

        uint32_t GetFPS () const override;

        void Register ( PluginHost::IStateControl::INotification * ) override;
        void Unregister ( PluginHost::IStateControl::INotification * ) override;

        void Register ( Exchange::IBrowser::INotification * ) override;
        virtual void Unregister ( Exchange::IBrowser::INotification * );

        uint32_t Request ( PluginHost::IStateControl::command const ) override;

        PluginHost::IStateControl::state State () const override;

        void StateChange ( PluginHost::IStateControl::state const );

        void Hide (bool const ) override;
        void Hidden ( bool const );

        BEGIN_INTERFACE_MAP ( SimpleEGLImplementation )
            INTERFACE_ENTRY ( Exchange::IBrowser )
            INTERFACE_ENTRY ( PluginHost::IStateControl )
            INTERFACE_AGGREGATE ( PluginHost::IPlugin::INotification , static_cast < IUnknown * > ( & _sink) )
        END_INTERFACE_MAP

    private:

        class PluginMonitor : public PluginHost::IPlugin::INotification {
        public :

            PluginMonitor ( PluginMonitor const & ) = delete;
            PluginMonitor & operator= ( PluginMonitor const & ) = delete;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            PluginMonitor ( SimpleEGLImplementation & );
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

            ~PluginMonitor () override = default;

            void Activated ( string const & , PluginHost::IShell * ) override;
            void Deactivated ( string const & , PluginHost::IShell * ) override;
            void Unavailable ( string const & , PluginHost::IShell * ) override;

            BEGIN_INTERFACE_MAP ( PluginMonitor )
                INTERFACE_ENTRY ( PluginHost::IPlugin::INotification )
            END_INTERFACE_MAP

        private :

            friend Core::ThreadPool::JobType < PluginMonitor & >;

            using Job = Core::ThreadPool::JobType < PluginMonitor >;

            void Dispatch ();

            SimpleEGLImplementation & _parent;
        };



        class Natives {
        public :

            using dpy_t = Compositor::IDisplay *;
            using surf_t = Compositor::IDisplay::ISurface *;
            using valid_t = bool;

            Natives () = delete;
            Natives ( Natives const & ) = delete;
            Natives ( Natives && ) = delete;

            Natives & operator= ( Natives const & ) = delete;
            Natives & operator= ( Natives && ) = delete;

            static Natives & Instance ( Exchange::IComposition::IDisplay * );

            dpy_t Display () const { return _dpy ; }
            surf_t Surface () const { return _surf ; }

            valid_t Valid () const;

            void DeInitialize ();

        private :

            using width_t = decltype ( std::declval < surf_t > () -> Width () );
            using height_t = decltype ( std::declval < surf_t > () -> Height () );

            static_assert ( std::is_integral < width_t > :: value != false );
            static constexpr width_t InvalidWidth () { return static_cast < width_t > ( 0 ) ; }

            static_assert ( std::is_integral < height_t > :: value != false );
            static constexpr height_t InvalidHeight () { return static_cast < height_t > ( 0 ) ; }

            static_assert ( std::is_pointer < dpy_t > :: value != false );
            static constexpr dpy_t InvalidDisplay () { return static_cast < dpy_t > ( nullptr ) ; };

            static_assert ( std::is_pointer < surf_t > :: value != false );
            static constexpr surf_t InvalidSurface () { return static_cast < surf_t > ( nullptr ) ; };

            Natives ( Exchange::IComposition::IDisplay * );
            ~Natives ();

            valid_t Initialize ( Exchange::IComposition::IDisplay * );

            dpy_t _dpy = InvalidDisplay ();
            surf_t _surf = InvalidSurface ();

            valid_t _valid = false;
        };

        class GLES {
        private:
            class offset;

        public :

            using tgt_t = GLenum const;
            using tex_t = GLuint;

            using valid_t = bool;

            GLES ( GLES const & ) = delete;
            GLES & operator= ( GLES const & );

            static GLES & Instance ();

            valid_t Render ();
            valid_t RenderColor ();
            valid_t RenderTile ();

            valid_t Supported ( std::string const & );

            valid_t UpdateOffset ( struct offset const & );

            static_assert ( std::is_integral < tex_t > :: value != false );
            static constexpr tex_t InvalidTex () { return static_cast < tex_t > ( 0 ) ; }

            valid_t Valid () const { return _valid ; }

        private :

            // Each coorrdinate in the range [-1.0f, 1.0f]
            class offset {
            public:
                using coordinate_t = float;

                coordinate_t _x;
                coordinate_t _y;
                coordinate_t _z;

                offset ();
                offset ( coordinate_t , coordinate_t , coordinate_t );

                offset ( offset const & ) = default;
                offset & operator= ( offset const & ) = default;
            };

            using prog_t = GLuint;

            GLES ();
            ~GLES ();

            valid_t Initialize ();
            valid_t Deinitialize ();

            valid_t SetupProgram ( char const [] , char const [] , prog_t & );

            static_assert ( std::is_integral < prog_t > :: value != false );
            static constexpr prog_t InvalidProg () { return 0; }

            offset _offset;

            tgt_t _tgt;
            tex_t _tex;

            valid_t _valid;
        };

        class EGL  {
        public :

            using dpy_t = EGLDisplay;
            using surf_t = EGLSurface;
            using height_t = EGLint;
            using width_t = EGLint;
            using valid_t = bool;

            static EGL & Instance ( Natives & );

            dpy_t Display () const { return _dpy ; }
            surf_t Surface () const { return _surf ; }

            height_t Height () const { return _height ; }
            width_t Width () const { return _width ; }

            valid_t Render ( GLES & );
            valid_t Render ();

            static_assert ( std::is_convertible < decltype ( EGL_NO_DISPLAY ) , dpy_t > :: value != false );
            static constexpr dpy_t InvalidDisplay () { return static_cast < dpy_t > ( EGL_NO_DISPLAY ) ; }

            static_assert ( std::is_convertible < decltype ( EGL_NO_SURFACE ) , surf_t > :: value != false );
            static constexpr surf_t InvalidSurface () { return static_cast < surf_t > ( EGL_NO_SURFACE ) ; }

            valid_t Valid () const { return _valid ; }

        private :

            EGL () = delete;
            EGL ( Natives const & );
            ~EGL ();

            valid_t Initialize ();
            void DeInitialize ();

            // Define the 'invalid' value
            static_assert ( std::is_pointer < EGLConfig > :: value != false );
            static constexpr void * const EGL_NO_CONFIG = nullptr;

            dpy_t _dpy = EGL_NO_DISPLAY;

            EGLConfig _conf = EGL_NO_CONTEXT;
            EGLContext _cont = EGL_NO_CONFIG;

            surf_t _surf = EGL_NO_SURFACE;

            width_t _width = 0;
            height_t _height = 0;

            Natives const & _natives;

            valid_t _valid = false;
        };



        virtual uint32_t Worker () override;

        Core::CriticalSection _adminLock;
        Config _config;
        string _requestedURL;
        string _setURL;
        string _dataPath;
        mutable uint32_t _fps;
        bool _hidden;
        std::list < PluginHost::IStateControl::INotification * > _notificationClients;
        std::list < Exchange::IBrowser::INotification * > _browserClients;
        PluginHost::IStateControl::state _state;
        Dispatcher _dispatcher;
        Core::ThreadPool _executor;
        Core::Sink < PluginMonitor > _sink;
        PluginHost::IShell * _service;
        Exchange::IComposition::IDisplay * _display;
    };

    // As a result of ODR use
    void * const SimpleEGLImplementation::EGL::EGL_NO_CONFIG;

    SERVICE_REGISTRATION ( SimpleEGLImplementation , 1 , 0 );
} // namespace Plugin

} // namespace WPEFramework::SimpleEGL



    WPEFramework::Plugin::SimpleEGLImplementation::Config::Config ()
        : _sleep ( 4 )
        , _init ( 100 )
        , _crash ( false )
        , _destruct ( 1000 )
        , _single ( false ) {

        Add ( _T ( "sleep" ) , & _sleep );
        Add ( _T ( "config" ) , & _init );
        Add ( _T ( "crash" ) , & _crash );
        Add ( _T ( "destruct" ) , & _destruct );
        Add ( _T ( "single" ) , & _single );
    }

    WPEFramework::Plugin::SimpleEGLImplementation::Config::~Config () {
    }

    WPEFramework::Plugin::SimpleEGLImplementation::Job::Job ()
        : _parent ( nullptr )
        , _type ( SHOW ) {
    }

    WPEFramework::Plugin::SimpleEGLImplementation::Job::Job ( SimpleEGLImplementation & parent , runtype const type )
        : _parent ( & parent )
        , _type ( type ) {
    }

    WPEFramework::Plugin::SimpleEGLImplementation::Job::Job ( Job const & copy )
        : _parent ( copy . _parent )
        , _type ( copy._type ) {
    }

    WPEFramework::Plugin::SimpleEGLImplementation::Job & WPEFramework::Plugin::SimpleEGLImplementation::Job::operator= ( Job const & RHS ) {
        _parent = RHS . _parent;
        _type = RHS . _type;
        return ( * this );
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::Job::Dispatch () {
        switch ( _type ) {
            case SHOW       :
                                ::SleepMs ( 300 );
                                _parent -> Hidden ( false );
                                break;
            case HIDE       :
                                ::SleepMs ( 100 );
                                _parent -> Hidden ( true );
                                break;
            case RESUMED    :
                                _parent -> StateChange ( PluginHost::IStateControl::RESUMED );
                                break;
            case SUSPENDED  :
                                _parent -> StateChange ( PluginHost::IStateControl::SUSPENDED );
                                break;
        }
    }



    void WPEFramework::Plugin::SimpleEGLImplementation::Dispatcher::Dispatch ( Core::IDispatch * job ) {
        job -> Dispatch ();
    }



    WPEFramework::Plugin::SimpleEGLImplementation::SimpleEGLImplementation ()
        : Core::Thread ( 0 , _T ( "SimpleEGLImplementation" ) )
        , _requestedURL ()
        , _setURL ()
        , _fps ( 0 )
        , _hidden ( false )
        , _dispatcher ()
        , _executor ( 1 , 0 , 4 , & _dispatcher , nullptr )
        , _sink ( * this )
        , _service ( nullptr ) {

        TRACE ( Trace::Information , ( _T ( "Constructed the SimpleEGLImplementation" ) ) );
    }

     WPEFramework::Plugin::SimpleEGLImplementation::~SimpleEGLImplementation () {
        TRACE ( Trace::Information , ( _T ( "Destructing the SimpleEGLImplementation" ) ) );

        Stop (); //Block();

        if ( Wait ( Core::Thread::STOPPED | Core::Thread::BLOCKED , _config . _destruct . Value () ) == false ) {
            TRACE ( Trace::Information , ( _T ( "Bailed out before the thread signalled completion. %d ms" ) , _config . _destruct . Value () ) );
        }

        if ( _display != nullptr ) {
            _display -> Release ();

            _display = nullptr;
        }

        TRACE ( Trace::Information , ( _T ( "Destructed the SimpleEGLImplementation" ) ) );
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::SetURL ( string const & URL ) {
        _requestedURL = URL;
        TRACE ( Trace::Information , ( _T ( "New URL [%d]: [%s]" ) , URL . length () , URL . c_str () ) );
    }

    string WPEFramework::Plugin::SimpleEGLImplementation::GetURL () const {
        TRACE ( Trace::Information , ( _T ( "Requested URL: [%s]" ) , _requestedURL . c_str () ) );
        return (_requestedURL);
    }

    uint32_t WPEFramework::Plugin::SimpleEGLImplementation::Configure ( PluginHost::IShell * service ) {
        uint32_t result = Core::ERROR_NONE;

        TRACE ( Trace::Information , ( _T ( "Configuring: [%s]" ) , service -> Callsign () . c_str () ) );

        _service = service;

        if ( _service != nullptr ) {
            TRACE ( Trace::Information , ( _T ( "SimpleEGLImplementation::Configure: AddRef service" ) ) );
            _service -> AddRef ();
        }

        if ( result == Core::ERROR_NONE ) {

            _dataPath = service -> DataPath ();
            _config . FromString ( service -> ConfigLine () );

            if ( _config . _init .Value () > 0 ) {
                TRACE ( Trace::Information , ( _T ( "Configuration requested to take [%d] mS" ) , _config . _init . Value () ) );
                SleepMs ( _config . _init . Value () );
            }

            _display = nullptr;

            if ( service -> COMLink () != nullptr ) {
                auto composition = service -> QueryInterfaceByCallsign < Exchange::IComposition > ( "Compositor" );

                if ( composition != nullptr ) {
                    _display = composition -> QueryInterface < Exchange::IComposition::IDisplay > ();

                    TRACE( Trace::Information , ( _T ( "Using non-COM-RPC path %p" ) , _display ) );

                    composition -> Release ();
                }
            }

            Run ();

        }

        return ( result );
    }

    uint32_t WPEFramework::Plugin::SimpleEGLImplementation::GetFPS () const {
        TRACE ( Trace::Information , ( _T ( "Requested FPS: %d" ) , _fps ) );
        return ( ++_fps );
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::Register ( PluginHost::IStateControl::INotification * sink ) {
        _adminLock . Lock ();

        // Make sure a sink is not registered multiple times.
        ASSERT ( std::find ( _notificationClients . begin () , _notificationClients . end () , sink ) == _notificationClients . end () );

        _notificationClients . push_back ( sink );
        sink -> AddRef ();

        TRACE ( Trace::Information , ( _T ( "IStateControl::INotification Registered: %p" ) , sink ) );

        _adminLock . Unlock ();
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::Unregister ( PluginHost::IStateControl::INotification * sink ) {
        _adminLock . Lock ();

        std::list < PluginHost::IStateControl::INotification * > :: iterator index ( std::find ( _notificationClients . begin () , _notificationClients . end () , sink ) );

        // Make sure you do not unregister something you did not register !!!
        ASSERT ( index != _notificationClients . end () );

        if ( index != _notificationClients . end () ) {
            TRACE ( Trace::Information , ( _T ( "IStateControl::INotification Unregistered: %p" ) , sink ) );

            ( * index ) -> Release ();

            _notificationClients . erase ( index );
        }

        _adminLock . Unlock ();
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::Register ( Exchange::IBrowser::INotification * sink ) {
         _adminLock . Lock ();

        // Make sure a sink is not registered multiple times.
        ASSERT ( std::find ( _browserClients . begin () , _browserClients . end () , sink ) == _browserClients . end () );

        _browserClients . push_back ( sink );
        sink -> AddRef ();

        TRACE ( Trace::Information , ( _T ( "IBrowser::INotification Registered: %p" ) , sink ) );

        _adminLock . Unlock ();
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::Unregister ( Exchange::IBrowser::INotification * sink ) {
        _adminLock . Lock ();

        std::list < Exchange::IBrowser::INotification * > :: iterator index ( std::find ( _browserClients . begin () , _browserClients .end () , sink ) );

        // Make sure you do not unregister something you did not register !!!
        // Since we have the Single shot parameter, it could be that it is deregistered, it is not an error !!!!
        // ASSERT(index != _browserClients.end());

        if ( index != _browserClients . end () ) {
            TRACE ( Trace::Information , ( _T ( "IBrowser::INotification Unregistered: %p" ) , sink ) );
            ( * index ) -> Release ();
            _browserClients . erase ( index );
        }

        _adminLock . Unlock ();
    }

     uint32_t WPEFramework::Plugin::SimpleEGLImplementation::Request ( PluginHost::IStateControl::command const command ) {
        TRACE ( Trace::Information , ( _T ( "Requested a state change. Moving to %s" ) , command == PluginHost::IStateControl::command::RESUME ? _T ( "RESUMING" ) : _T ( "SUSPENDING" ) ) );

        uint32_t result ( Core::ERROR_ILLEGAL_STATE );

        switch ( command ) {
            case PluginHost::IStateControl::SUSPEND :
                _executor . Submit ( Core::ProxyType < Core::IDispatch > ( Core::ProxyType < Job > :: Create ( * this , Job::SUSPENDED ) ) ,  20000 );
                result = Core::ERROR_NONE;
                break;
            case PluginHost::IStateControl::RESUME :
                _executor . Submit ( Core::ProxyType < Core::IDispatch > ( Core::ProxyType < Job > :: Create ( * this , Job::RESUMED ) ) , 20000 );
                result = Core::ERROR_NONE;
                break;
        }

        return ( result );
    }

    WPEFramework::PluginHost::IStateControl::state WPEFramework::Plugin::SimpleEGLImplementation::State () const {
         return ( PluginHost::IStateControl::RESUMED );
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::StateChange ( PluginHost::IStateControl::state const state ) {
        _adminLock . Lock();

        std::list < PluginHost::IStateControl::INotification * > :: iterator index ( _notificationClients . begin () );

         while ( index != _notificationClients . end () ) {
            TRACE ( Trace::Information , ( _T ( "State change from SimpleEGLTest 0x%X" ) , state ) );

            ( * index ) -> StateChange ( state );

            index++;
        }

        TRACE ( Trace::Information , ( _T ( "Changing state to [%s]" ) , state == PluginHost::IStateControl::state::RESUMED ? _T ( "Resumed" ) : _T ( "Suspended" ) ) );

        _state = state;

        _adminLock . Unlock ();
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::Hide ( bool const hidden ) {
        if ( hidden == true ) {
            TRACE ( Trace::Information , ( _T ( "Requestsed a Hide." ) ) );

            _executor . Submit ( Core::ProxyType < Core::IDispatch > ( Core::ProxyType < Job > :: Create ( * this , Job::HIDE ) ) , 20000 );
        }
        else {
            TRACE( Trace::Information , ( _T ( "Requestsed a Show." ) ) );

            _executor . Submit ( Core::ProxyType < Core::IDispatch > ( Core::ProxyType < Job > :: Create ( * this , Job::SHOW ) ) , 20000 );
        }
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::Hidden ( bool const hidden ) {
        _adminLock . Lock ();

        std::list < Exchange::IBrowser::INotification * > :: iterator index ( _browserClients . begin () );

        while ( index != _browserClients . end () ) {
            TRACE ( Trace::Information , ( _T ( "State change from SimpleEGLTest 0x%X" ) , __LINE__ ) );

            ( * index ) -> Hidden ( hidden );

            index++;
        }

        TRACE ( Trace::Information , ( _T ( "Changing state to [%s]" ) , hidden ? _T ( "Hidden" ) : _T ( "Shown" ) ) );

        _hidden = hidden;

        _adminLock . Unlock ();
    }



    WPEFramework::Plugin::SimpleEGLImplementation::PluginMonitor::PluginMonitor ( SimpleEGLImplementation & parent )
        : _parent(parent)
    {}

    void WPEFramework::Plugin::SimpleEGLImplementation::PluginMonitor::Activated ( string const & , PluginHost::IShell * service ) {
        Exchange::ITimeSync * time = service -> QueryInterface < Exchange::ITimeSync > ();

        if ( time != nullptr ) {
            TRACE ( Trace::Information , ( _T ( "Time interface supported" ) ) );
            time -> Release ();
        }
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::PluginMonitor::Deactivated ( string const & callsign , PluginHost::IShell * plugin ) {
        _parent . Stop ();

        silence ( callsign );
        silence ( plugin );
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::PluginMonitor::Unavailable ( string const & callsign , PluginHost::IShell * plugin ) {
        silence ( callsign );
        silence ( plugin );
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::PluginMonitor::Dispatch () {
    }



    /* static */ WPEFramework::Plugin::SimpleEGLImplementation::Natives & WPEFramework::Plugin::SimpleEGLImplementation::Natives::Instance ( Exchange::IComposition::IDisplay * display ) {
        static Natives natives ( display );
        return natives;
    }

    WPEFramework::Plugin::SimpleEGLImplementation::Natives::valid_t WPEFramework::Plugin::SimpleEGLImplementation::Natives::Valid () const {
        return _valid;
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::Natives::DeInitialize () {
        _valid = false;

        if ( _surf != InvalidSurface () ) {
            _surf -> Release ();
            _surf = InvalidSurface ();
        }

        if ( _dpy != InvalidDisplay () ) {
            _dpy -> Release ();
            _dpy = InvalidDisplay ();
        }
    }

    WPEFramework::Plugin::SimpleEGLImplementation::Natives::Natives ( Exchange::IComposition::IDisplay * display )
        : _dpy { InvalidDisplay () }
        , _surf { InvalidSurface () }
        , _valid { Initialize ( display ) } {
    }

    WPEFramework::Plugin::SimpleEGLImplementation::Natives::~Natives () {
        _valid = false;
        DeInitialize ();

        _surf = InvalidSurface ();
        _dpy = InvalidDisplay ();
    }

    WPEFramework::Plugin::SimpleEGLImplementation::Natives::valid_t WPEFramework::Plugin::SimpleEGLImplementation::Natives::Initialize ( Exchange::IComposition::IDisplay * display ) {
        constexpr char const * Name  = "SimpleEGL::Worker";

        valid_t ret = ! (    _dpy != InvalidDisplay ()
                          && _dpy -> Release () != Core::ERROR_NONE
                        )
                      ;

        if ( ret != false ) {
            // Trigger the creation of the display, implicit AddRef () on _dpy
            _dpy = Compositor::IDisplay::Instance ( std::string ( Name ) . append ( ":Display" ) , display );

            ret = _dpy != InvalidDisplay ();
        }

        // Just here to support surface creation, the exact values are probably changed by the underlying platform
        constexpr height_t const height = InvalidHeight ();
        constexpr width_t const width = InvalidWidth ();

        if ( ret != false )  {
            // Trigger the creation of the surfac, implicit AddRef () on _surf!
            _surf = _dpy -> Create ( std::string ( Name ) . append ( ":Surface" ) , width , height );
            ret = _surf != InvalidSurface ();
        }

        if ( ret != false ) {
            width_t realWidth = _surf -> Width ();
            height_t realHeight = _surf -> Height ();

            if (    realWidth == InvalidWidth ()
                 || realWidth != width
               ) {

                TRACE ( Trace::Information , ( _T ( "Real surface width is (%d) invalid or differs from requested width (%d))." ) , realWidth, width ) );
            }

            if (    realHeight == InvalidHeight ()
                 || realHeight != height
               ) {

                TRACE ( Trace::Information , ( _T ( "Real surface height is (%d) invalid or differs from requested height (%d)." ) , realHeight , height ) );
            }

            ret =    realHeight != InvalidHeight ()
                  && realWidth != InvalidWidth ();
        }

        return ret;
    }



#define GL_ERROR_WITH_RETURN() ( glGetError () == GL_NO_ERROR )
#ifdef NDEBUG
#define GL_ERROR() do {} while (0)
#else
#define GL_ERROR() assert (GL_ERROR_WITH_RETURN ())
#endif

    /* static */ WPEFramework::Plugin::SimpleEGLImplementation::GLES & WPEFramework::Plugin::SimpleEGLImplementation::GLES::Instance () {
        static GLES gles;
        return gles;
    }

    WPEFramework::Plugin::SimpleEGLImplementation::GLES::valid_t WPEFramework::Plugin::SimpleEGLImplementation::GLES::Render () {
        valid_t ret = Valid ();
        return ret;
    }



    WPEFramework::Plugin::SimpleEGLImplementation::GLES::offset::offset ()
        : offset { 0.0f , 0.0f , 0.0f }
    {}

    WPEFramework::Plugin::SimpleEGLImplementation::GLES::offset::offset ( coordinate_t x , coordinate_t y , coordinate_t z )
        : _x { x }
        , _y { y }
        , _z { z }
    {}



    WPEFramework::Plugin::SimpleEGLImplementation::GLES::GLES ()
        : _offset {}
        , _tgt {}
        , _tex { InvalidTex () }
        , _valid { Initialize () } {
    }

    WPEFramework::Plugin::SimpleEGLImplementation::GLES::~GLES () {
        _valid = false;
        /* valid_t */ Deinitialize ();
    }

    WPEFramework::Plugin::SimpleEGLImplementation::GLES::valid_t WPEFramework::Plugin::SimpleEGLImplementation::GLES::RenderColor () {
        uint16_t degree = 0;

        constexpr decltype ( degree ) const ROTATION = 360;

        constexpr float const SPEED = 10;

        constexpr float const OMEGA = 3.14159265f / 180.0f;

        valid_t ret = Valid ();

        if ( ret != false ) {
            // Here, for C(++) these type should be identical
            // Type information: https://www.khronos.org/opengl/wiki/OpenGL_Type
            static_assert ( std::is_same < float , GLfloat > :: value );

            GLfloat rad = static_cast < GLfloat > ( cos ( degree * OMEGA * SPEED ) );

            // The function clamps the input to [0.0f, 1.0f]
            /* void */ glClearColor ( 0.0f , rad , 0.0f , 0.0f );
            GL_ERROR ();

            /* void */ glClear ( GL_COLOR_BUFFER_BIT );
            GL_ERROR ();

            /* void */ glFlush ();
            GL_ERROR ();

            ret = GL_ERROR_WITH_RETURN ();

            degree = ( degree + 1 ) % ROTATION;
        }

        return ret;
    }

    WPEFramework::Plugin::SimpleEGLImplementation::GLES::valid_t WPEFramework::Plugin::SimpleEGLImplementation::GLES::RenderTile () {
        valid_t ret = GL_ERROR_WITH_RETURN ();

        if ( ret != false ) {
            constexpr char const vtx_src [] =
                "#version 100                              \n"
                "attribute vec3 position;                  \n"
                "void main () {                            \n"
                    "gl_Position = vec4 (position.xyz, 1); \n"
                "}                                         \n"
            ;

            constexpr char const frag_src [] =
                "#version 100                                      \n"
                "precision mediump float;                          \n"
                "uniform float red;                                \n"
                "uniform float green;                              \n"
                "uniform float blue;                               \n"
                "void main () {                                    \n"
                    "gl_FragColor = vec4 (red, green, blue, 1.0f); \n"
                "}                                                 \n"
            ;

            static prog_t prog = InvalidProg ();

            ret = SetupProgram ( vtx_src , frag_src , prog );
        }


        // x, y, z
        constexpr uint8_t const VerticeDimensions = 3;


        // Blend pixels with pixels already present in the frame buffer

        if ( ret != false ) {
            glEnable ( GL_BLEND );
            GL_ERROR ();

            glBlendFunc ( GL_ONE , GL_ONE_MINUS_SRC_ALPHA );
            GL_ERROR ();

            // Color on error
            glClearColor ( 1.0f , 0.0f , 0.0f , 0.5f );
            ret = GL_ERROR_WITH_RETURN ();
        }

        EGL::dpy_t dpy = EGL::InvalidDisplay ();

        if ( ret != false ) {
            dpy = eglGetCurrentDisplay ();
            ret = eglGetError () ==    EGL_SUCCESS
                                    && dpy != EGL::InvalidDisplay ();
        }

        EGL::surf_t surf = EGL::InvalidSurface ();

        if ( ret != false ) {
            surf = eglGetCurrentSurface ( EGL_DRAW );

            ret = eglGetError () ==    EGL_SUCCESS
                                    && surf != EGL::InvalidSurface ();
        }

        EGLint width = 0, height = 0;

        if ( ret != false ) {
            ret =    eglQuerySurface ( dpy , surf , EGL_WIDTH , & width ) != EGL_FALSE
                  && eglQuerySurface ( dpy , surf , EGL_HEIGHT , & height ) != EGL_FALSE
                  && eglGetError () == EGL_SUCCESS;
        }

        if ( ret != false ) {
            glViewport ( static_cast < GLint > ( 0 ) , static_cast < GLint > ( 0 ) , static_cast < GLsizei > ( width ) , static_cast < GLsizei > ( height ) );
            GL_ERROR ();

            static_assert ( std::is_same < GLfloat , GLES::offset::coordinate_t > :: value != false );
            std::array < GLfloat , 4 * VerticeDimensions > const vert = { -0.5f , -0.5f , 0.0f /* v0 */ , 0.5f , -0.5f , 0.0f /* v1 */ , -0.5f , 0.5f , 0.0f /* v2 */ , 0.5f , 0.5f , 0.0f /* v3 */ };

            prog_t prog = 0;

            glGetIntegerv ( GL_CURRENT_PROGRAM , reinterpret_cast < GLint * > ( & prog ) );
            GL_ERROR ();

            GLint loc = glGetAttribLocation ( prog , "position" );
            GL_ERROR ();

            static GLfloat red = 0.0f;
            static GLfloat green = 0.0f;
            static GLfloat blue = 1.0f;

            GLfloat color = red;
            red = green;
            green = blue;
            blue = color;

            GLint loc_red = glGetUniformLocation ( prog , "red" );
            GL_ERROR ();

            glUniform1f ( loc_red , red );
            GL_ERROR ();

            GLint loc_green = glGetUniformLocation ( prog , "green" );
            GL_ERROR ();

            glUniform1f ( loc_green , green );
            GL_ERROR ();

            GLint loc_blue = glGetUniformLocation ( prog , "blue" );
            GL_ERROR ();

            glUniform1f ( loc_blue , blue );
            GL_ERROR ();

            glVertexAttribPointer ( loc , VerticeDimensions,  GL_FLOAT , GL_FALSE , 0 , vert . data () );
            GL_ERROR ();

            glEnableVertexAttribArray ( loc );
            GL_ERROR ();

            glDrawArrays ( GL_TRIANGLE_STRIP , 0 , vert . size () / VerticeDimensions );
            GL_ERROR ();

            glFinish ();
            ret = GL_ERROR_WITH_RETURN ();
        }

        return ret;
    }

    WPEFramework::Plugin::SimpleEGLImplementation::GLES::valid_t WPEFramework::Plugin::SimpleEGLImplementation::GLES::Supported ( std::string const & name ) {
        valid_t ret = false;

        using string_t = std::string::value_type;
        using ustring_t = std::make_unsigned < string_t > :: type;

        // Identical underlying types except for signedness
        static_assert ( std::is_same < ustring_t , GLubyte > :: value != false );

        string_t const * ext = reinterpret_cast < string_t const * > ( glGetString ( GL_EXTENSIONS ) );

        ret =    ext != nullptr
              && name . size () > 0
              && ( std::string ( ext ) . find ( name ) != std::string::npos
                 )
              ;

        return ret;
    }


    WPEFramework::Plugin::SimpleEGLImplementation::GLES::valid_t WPEFramework::Plugin::SimpleEGLImplementation::GLES::UpdateOffset ( struct offset const & off ) {
        valid_t ret = false;

        // Range check without taking into account rounding errors
        if (    ( ( off . _x - -0.5f ) * ( off . _x - 0.5f ) <= 0.0f )
             && ( ( off . _y - -0.5f ) * ( off . _y - 0.5f) <= 0.0f )
             && ( ( off . _z - -0.5f ) * ( off . _z - 0.5f) <= 0.0f )
           ) {

            _offset = off;

            ret = true;
        }

        return ret;
    }

    WPEFramework::Plugin::SimpleEGLImplementation::GLES::valid_t WPEFramework::Plugin::SimpleEGLImplementation::GLES:: Initialize () {
        valid_t ret = true;
        return ret;
    }

    WPEFramework::Plugin::SimpleEGLImplementation::GLES::valid_t WPEFramework::Plugin::SimpleEGLImplementation::GLES::Deinitialize () {
        valid_t ret = false;

        if ( _tex != InvalidTex () ) {
            glDeleteTextures ( 1 , & _tex );
            GL_ERROR ();
        }

        ret = GL_ERROR_WITH_RETURN ();

        return ret;
    }

    WPEFramework::Plugin::SimpleEGLImplementation::GLES::valid_t WPEFramework::Plugin::SimpleEGLImplementation::GLES::SetupProgram ( char const vtx_src [] , char const frag_src [] , prog_t & prog ) {
        auto LoadShader = [] ( GLuint const type , GLchar const code [] ) -> GLuint {
            GLuint shader = glCreateShader ( type );
            GL_ERROR ();

            valid_t ret = (    GL_ERROR_WITH_RETURN ()
                            && shader != 0
                          ) != false
                          ;

            if ( ret != false ) {
                glShaderSource ( shader , 1 , & code , nullptr );
                GL_ERROR ();

                glCompileShader ( shader );
                GL_ERROR ();

                GLint status = GL_FALSE;

                glGetShaderiv ( shader , GL_COMPILE_STATUS , & status );
                GL_ERROR ();

                ret = (    GL_ERROR_WITH_RETURN ()
                        && status != GL_FALSE
                      ) != false
                      ;
            }

            return shader;
        };

        auto ShadersToProgram = [] ( GLuint const vertex , GLuint const fragment , prog_t & prog ) -> valid_t {
            prog = glCreateProgram ();
            GL_ERROR ();

            valid_t ret = (    GL_ERROR_WITH_RETURN ()
                            && prog != 0
                          ) != false
                          ;

            if ( ret != false ) {
                glAttachShader ( prog , vertex );
                GL_ERROR ();

                glAttachShader ( prog , fragment );
                GL_ERROR ();

                glBindAttribLocation ( prog , 0 , "position" );
                GL_ERROR ();

                glLinkProgram ( prog );
                GL_ERROR ();

                GLint status = GL_FALSE;

                glGetProgramiv ( prog , GL_LINK_STATUS , & status );
                GL_ERROR ();

                ret = (    GL_ERROR_WITH_RETURN ()
                        && status != GL_FALSE
                      ) != false
                      ;
            }

            return ret;
        };

        auto DeleteCurrentProgram = [] () -> valid_t {
            GLuint prog = 0;

            glGetIntegerv ( GL_CURRENT_PROGRAM , reinterpret_cast < GLint * > ( & prog ) );
            GL_ERROR ();

            valid_t ret = (    GL_ERROR_WITH_RETURN ()
                            && prog != 0
                          ) != false
                          ;

            if ( ret != false ) {
                GLint count = 0;

                glGetProgramiv ( prog , GL_ATTACHED_SHADERS , & count );
                GL_ERROR ();

                ret = (    GL_ERROR_WITH_RETURN ()
                        && count > 0
                      ) != false
                      ;

                if ( ret != false ) {
                    GLuint shaders [ count ];

                    glGetAttachedShaders ( prog , count , static_cast < GLsizei * > ( & count ) , & shaders [ 0 ] );

                    if ( GL_ERROR_WITH_RETURN () != false ) {
                        for ( count-- ; count >= 0 ; count-- ) {
                            glDetachShader ( prog , shaders [ count ] );
                            GL_ERROR ();

                            glDeleteShader ( shaders [ count ] );
                            GL_ERROR ();
                        }

                        glDeleteProgram ( prog );
                        GL_ERROR ();
                    }
                }

                ret =    ret
                      && GL_ERROR_WITH_RETURN ();
            }

            return ret;
        };

        bool ret = GL_ERROR_WITH_RETURN ();

        if (    ret != false
             && prog == InvalidProg ()
           ) {

            GLuint vtxShader = LoadShader ( GL_VERTEX_SHADER , vtx_src );
            GLuint fragShader = LoadShader ( GL_FRAGMENT_SHADER , frag_src );

            ret = ShadersToProgram ( vtxShader , fragShader , prog );
        }

        if ( ret != false ) {
            glUseProgram ( prog );
            GL_ERROR ();

            ret = GL_ERROR_WITH_RETURN ();

            if ( ret != true ) {
                 /* valid_t */ DeleteCurrentProgram ();
                GL_ERROR ();

                prog = InvalidProg ();

                // Color on error
                glClearColor ( 1.0f , 0.0f , 0.0f , 0.5f );
                GL_ERROR ();
            }
        }

        ret =    ret
              && GL_ERROR_WITH_RETURN ();

        return ret;
    }

#undef GL_ERROR_WITH_RETURN
#undef GL_ERROR



    /* static */ WPEFramework::Plugin::SimpleEGLImplementation::EGL & WPEFramework::Plugin::SimpleEGLImplementation::EGL::Instance ( Natives & natives ) {
        static EGL egl ( natives );
        return egl;
    }

    WPEFramework::Plugin::SimpleEGLImplementation::EGL::valid_t WPEFramework::Plugin::SimpleEGLImplementation::EGL::Render (GLES & gles) {
        valid_t ret =    Valid () != false
                      && eglMakeCurrent( _dpy , _surf , _surf , _cont ) != EGL_FALSE
                      ;

        if ( ret != false ) {
            ret =    gles.RenderTile () != false
                  && eglSwapBuffers ( _dpy , _surf ) != EGL_FALSE
                  ;
        }

        return ret;
    }

    WPEFramework::Plugin::SimpleEGLImplementation::EGL::valid_t WPEFramework::Plugin::SimpleEGLImplementation::EGL::Render () {
        // Avoid any memory leak if the worker is stopped (by another thread)
        valid_t ret =    Valid () != false
                      && eglMakeCurrent ( _dpy , InvalidSurface () , InvalidSurface () , EGL_NO_CONTEXT ) != EGL_FALSE
                      ;

        return ret;
    }

    WPEFramework::Plugin::SimpleEGLImplementation::EGL::EGL ( Natives const & natives )
        : _natives { natives }
        , _valid { Initialize () }
    {}

    WPEFramework::Plugin::SimpleEGLImplementation::EGL::~EGL () {
        _valid = false;

        DeInitialize ();
    }

    WPEFramework::Plugin::SimpleEGLImplementation::EGL::valid_t WPEFramework::Plugin::SimpleEGLImplementation::EGL::Initialize () {
        valid_t ret = _natives.Valid ();

        if ( ret != false ) {

            if ( _dpy != InvalidDisplay ())  {
                ret = false;

                if ( eglTerminate ( _dpy ) != EGL_FALSE ) {
                    ret = true;
                }
            }
        }

        if ( ret != false ) {
            _dpy = eglGetDisplay ( _natives . Display () -> Native () );
            ret = _dpy != InvalidDisplay ();
        }

        if ( ret != false ) {
            EGLint major = 0, minor = 0;
            ret = eglInitialize ( _dpy , & major , & minor ) != EGL_FALSE;

            // Just take the easy approach (for now)
            static_assert ( ( std::is_same < decltype ( major ) , int > :: value ) != false );
            static_assert ( ( std::is_same < decltype ( minor ) , int > :: value ) != false );
            TRACE ( Trace::Information , ( _T ( "EGL version : %d.%d" ) , static_cast < int > ( major ) , static_cast < int > ( minor ) ) );
        }

        if ( ret != false ) {
            constexpr EGLint const attr [] = {
                EGL_SURFACE_TYPE, static_cast < EGLint > ( EGL_WINDOW_BIT ) ,
                EGL_RED_SIZE    , static_cast < EGLint > ( 8 ) ,
                EGL_GREEN_SIZE  , static_cast < EGLint > ( 8 ) ,
                EGL_BLUE_SIZE   , static_cast < EGLint > ( 8 ) ,
                EGL_ALPHA_SIZE  , static_cast < EGLint > ( 8 ) ,
                EGL_BUFFER_SIZE , static_cast < EGLint > ( 32 ) ,
                EGL_RENDERABLE_TYPE , static_cast < EGLint > ( EGL_OPENGL_ES2_BIT ),
                EGL_NONE
            };

            EGLint count = 0;

            if ( eglGetConfigs ( _dpy , nullptr , 0 , & count ) != EGL_TRUE ) {
                count = 1;
            }

            std::vector < EGLConfig > confs ( count , EGL_NO_CONFIG );

            /* EGLBoolean */ eglChooseConfig ( _dpy , & attr [ 0 ] , confs . data () , confs . size () , & count );

            _conf = confs [ 0 ];

            ret = _conf != EGL_NO_CONFIG;
        }

        if ( ret != false ) {
            constexpr EGLint const attr [] = {
                EGL_CONTEXT_CLIENT_VERSION , static_cast < EGLint > ( 2 ) ,
                EGL_NONE
            };

            _cont = eglCreateContext ( _dpy , _conf , EGL_NO_CONTEXT , attr );

            ret = _cont != EGL_NO_CONTEXT;
        }

        if ( ret != false ) {
            constexpr EGLint const attr [] = {
                EGL_NONE
            };

            _surf = eglCreateWindowSurface ( _dpy , _conf , _natives . Surface () -> Native () , & attr [ 0 ] );

            ret = _surf != InvalidSurface ();
        }

        if ( ret != true ) {
            DeInitialize ();
        }

        return ret;
    }

    void WPEFramework::Plugin::SimpleEGLImplementation::EGL::DeInitialize () {
        _valid = false;
        /* EGLBoolean */ eglTerminate ( _dpy );
    }



    uint32_t WPEFramework::Plugin::SimpleEGLImplementation::Worker () {
        Natives & natives = Natives::Instance ( _display );

        EGL & egl = EGL::Instance ( natives );

        using timeout_t = decltype ( WPEFramework::Core::infinite );

        // Maximum rate
// TODO: match display refresh rate
        constexpr timeout_t const ret = 0;

        EGL::valid_t status =    natives . Valid () != false
                              && egl . Valid () != false
                              ;

        if ( status != false ) {
            GLES & gles = GLES::Instance ();

            status =    egl . Render ( gles ) != false
                     && natives . Display () -> Process ( 0 ) == 0
                     && egl .Render () != false
                     ;
        }

        if ( status != false ) {
            Block ();
        }
        else {
            Stop ();

            natives . DeInitialize ();
        }

        return ret;
    }



