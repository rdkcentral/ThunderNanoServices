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
#include <core/core.h>
#include <com/com.h>

#include "ModeSet.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <sys/socket.h>
#include <fcntl.h>

#ifndef _GNU_SOURCE
#error mkotemp requires feature macro _GNU_SOURCE to be defined
#endif

#ifdef __cplusplus
}
#endif

#include <stdlib.h>
#include <cstring>
#include <mutex>
#include <type_traits>
#include <functional>
#include <chrono>
#include <set>
#include <queue>
#include <stack>

template < class T >
struct remove_reference {
    typedef T type;
};

template < class T >
struct remove_reference < T & > {
    typedef T type;
};

template < class T >
struct remove_pointer {
    typedef T type;
};

template < class T >
struct remove_pointer < T * > {
    typedef T type;
};

template < class T >
struct remove_pointer < T const * >  {
    typedef T type;
};

template < class T >
struct remove_const {
    typedef T type;
};

template < class T >
struct remove_const < T const > {
    typedef T type;
};

template < class T >
struct remove_const < T * > {
    typedef T * type;
};

template < class T >
struct remove_const < T const * > {
    typedef T * type;
};

template < class FROM , class TO , bool ENABLE >
struct narrowing {
    static_assert ( ( std::is_arithmetic < FROM > :: value && std::is_arithmetic < TO > :: value ) != false );

    // Not complete, assume zero is minimum for unsigned
    // Common type of signed and unsigned typically is unsigned
    using common_t = typename std::common_type < FROM, TO > :: type;
    static constexpr bool value =   ENABLE
                                    && ( (    std::is_signed < FROM > :: value
                                           && std::is_unsigned < TO > :: value
                                         )
                                         || (    std::is_same < FROM , TO > :: value != true
                                              && static_cast < common_t > (  std::numeric_limits < FROM > :: max () ) > static_cast < common_t > ( std::numeric_limits < TO > :: max () )
                                            )
                                       )
                                    ;
};

template < typename TYPE , intmax_t VAL >
struct in_signed_range {
    static_assert ( (    std::is_integral < TYPE > :: value
                      && std::is_signed < TYPE > :: value
                    )
                    != false
                  );
    using common_t = typename std::common_type < TYPE , decltype ( VAL ) > :: type;
    static constexpr bool value =    static_cast < common_t > ( VAL ) >= static_cast < common_t > ( std::numeric_limits < TYPE > :: min () )
                                  && static_cast < common_t > ( VAL ) <= static_cast < common_t > ( std::numeric_limits < TYPE > :: max () );
};

template < typename TYPE , uintmax_t VAL >
struct in_unsigned_range {
   static_assert ( (    std::is_integral < TYPE > :: value
                     && std::is_unsigned < TYPE > :: value
                   )
                   != false
                 );
    using common_t = typename std::common_type < TYPE , decltype ( VAL ) > :: type;
    static constexpr bool value =    static_cast < common_t > ( VAL ) >= static_cast < common_t > ( std::numeric_limits < TYPE > :: min () )
                                  && static_cast < common_t > ( VAL ) <= static_cast < common_t > ( std::numeric_limits < TYPE > :: max () );
};


// Suppress compiler warnings of unused (parameters)
// Omitting the name is sufficient but a search on this keyword provides easy access to the location
template < typename T >
void silence ( T && ) {}

// Show logging in blue to make it distinct from TRACE
#define TRACE_WITHOUT_THIS(level, ...) do {               \
    fprintf ( stderr, "\033[1;34m" );                     \
    fprintf ( stderr, "[%s:%d] : ", __FILE__, __LINE__ ); \
    fprintf ( stderr, ##__VA_ARGS__ );                    \
    fprintf ( stderr, "\n" );                             \
    fprintf ( stderr, "\033[0m" );                        \
    fflush ( stderr );                                    \
} while ( 0 )


MODULE_NAME_DECLARATION ( BUILD_REFERENCE )

namespace WPEFramework {
namespace Plugin {

    // Forward declaration
    class CompositorImplementation;



    // Different namespace and scope than CompositorImplementation::EGL
    namespace EGL {
#ifdef EGL_VERSION_1_5
        using img_t = EGLImage;
#else
        using img_t = EGLImageKHR;
#endif
        using width_t = EGLint;
        using height_t = EGLint;

#ifdef EGL_VERSION_1_5
        static_assert ( std::is_convertible < img_t , decltype ( EGL_NO_IMAGE ) > :: value != false );
        static constexpr img_t InvalidImage () { return static_cast < img_t > ( EGL_NO_IMAGE ) ; }
#else
        static_assert ( std::is_convertible < img_t , decltype ( EGL_NO_IMAGE_KHR ) > :: value != false );
        static constexpr img_t InvalidImage () { return static_cast < img_t > ( EGL_NO_IMAGE_KHR ); }
#endif
    }



    class ClientSurface : public Exchange::IComposition::IClient , Exchange::IComposition::IRender {
    private :

        // The buffer acts as a surface for the remote site
        struct {
            struct gbm_bo * _buf;
            int _fd;
            int _sync_fd;
            EGL::img_t _khr;

            bool Valid () const { return _buf != nullptr ; }
            bool DMAComplete () const { return Valid () && _fd != ModeSet::GBM::InvalidFd () && _sync_fd != ModeSet::GBM::InvalidFd (); };
            bool RenderComplete () const { return    Valid ()
                                                  && _fd != ModeSet::GBM::InvalidFd ()
                                                  && _sync_fd != ModeSet::GBM::InvalidFd ()
                                                  && _khr != WPEFramework::Plugin::EGL::InvalidImage ()
                                                  ;
                                         }
        } _nativeSurface;

    private :

        ModeSet & _modeSet;
        CompositorImplementation & _compositor;
        std::string const _name;

        uint32_t _opacity;
        uint32_t _layer;

        Exchange::IComposition::Rectangle _destination;

    public :

        using surf_t = decltype ( _nativeSurface );

        ClientSurface () = delete;
        ClientSurface ( ClientSurface const & ) = delete;
        ClientSurface & operator= ( ClientSurface const & ) = delete;

        ClientSurface ( ModeSet & , CompositorImplementation & , const string & , uint32_t const , uint32_t const );
        ~ClientSurface () override;

        // Underlying native surface handle
        Core::instance_id Native () const override;

        // Inter-process synchronization
        Core::instance_id SyncPrimitive () const;
        bool SyncPrimitiveStart () const;
        bool SyncPrimitiveEnd () const;

        // Just me
        string Name () const override { return _name; }

        // Opacity
        void Opacity ( uint32_t const value ) override { _opacity = value; }
        uint32_t Opacity () const override { return _opacity; }

        // Geometry
        static_assert ( std::is_enum < decltype ( Core::ERROR_NONE ) > :: value != false );
        static_assert ( narrowing < std::underlying_type < decltype ( Core::ERROR_NONE ) > :: type , uint32_t , true > :: value != true );
        uint32_t Geometry ( Exchange::IComposition::Rectangle const & rectangle ) override { _destination = rectangle; return static_cast < uint32_t > ( Core::ERROR_NONE ); }
        Exchange::IComposition::Rectangle Geometry () const override { return _destination; }

        // Z-ordering within multiple surfaces
        static_assert ( std::is_enum < decltype ( Core::ERROR_NONE ) > :: value != false );
        static_assert ( narrowing < std::underlying_type < decltype ( Core::ERROR_NONE ) > :: type , uint32_t , true > :: value != true );
        uint32_t ZOrder ( uint16_t const zorder ) override { _layer = zorder; return static_cast < uint32_t > ( Core::ERROR_NONE ); }
        uint32_t ZOrder () const override { return _layer ; }

        // Buffer to display flipping
        void ScanOut () override;

        surf_t const & Surface ( EGL::img_t const & khr = EGL::InvalidImage () );

        BEGIN_INTERFACE_MAP ( ClientSurface )
            INTERFACE_ENTRY ( Exchange::IComposition::IClient )
            INTERFACE_ENTRY ( Exchange::IComposition::IRender )
        END_INTERFACE_MAP
    };



    class CompositorImplementation : public Exchange::IComposition, public Exchange::IComposition::IDisplay {
    public :

        // The (preferred) type of lock
        using lock_t = std::mutex;

        using ClientContainer = Core::ProxyMapType < string , ClientSurface >;

        // Underlying display 'true' dimensions
        using platform_w_t =  decltype ( std::declval < ModeSet > () . Width () );
        using platform_h_t =  decltype ( std::declval < ModeSet > () . Height () );

        CompositorImplementation ( CompositorImplementation const & ) = delete;
        CompositorImplementation & operator= ( CompositorImplementation const & ) = delete;

    private :



        // The communication channel with the (remote) other site
        class ExternalAccess : public RPC::Communicator {
        public :

            ExternalAccess () = delete;
            ExternalAccess ( ExternalAccess const & ) = delete;
            ExternalAccess & operator= ( ExternalAccess const & ) = delete;

            ExternalAccess ( CompositorImplementation & , Core::NodeId const & , string const & , Core::ProxyType < RPC::InvokeServer > const & );
            ~ExternalAccess () override;

        private :

            void * Aquire ( string const & , uint32_t const , uint32_t const ) override;

            CompositorImplementation & _parent;
        };



        class DMATransfer final : public Core::Thread {
        private :

            CompositorImplementation & _compositor;

            // Waiting for connection requests
            int _listen;
            // Actual socket for communication
            int _transfer;

            struct sockaddr_un const _addr;

            bool _valid;

        public :

            using valid_t = decltype ( _valid );

            using timeout_t = remove_const < decltype ( Core::infinite ) > :: type;

            using socket_t = decltype ( _listen );
// TODO: see other parts of the code
            using fd_t = int;

            DMATransfer () = delete;
            DMATransfer ( DMATransfer const & ) = delete;
            DMATransfer & operator= ( DMATransfer const & ) = delete;

// TODO:
            // Sharing handles (file descriptors)
            static constexpr int8_t MAX_SHARING_FDS = 2;
            using fds_t = std::array < fd_t , MAX_SHARING_FDS >;

            explicit DMATransfer ( CompositorImplementation & );
            ~DMATransfer ();

            valid_t Valid () const { return _valid; }

            // Receive file descriptor(s) with additional message
            valid_t Receive ( std::string & , DMATransfer::fds_t & );

            // Send file descriptor(s) with additional message
            valid_t Send ( std::string const & , DMATransfer::fds_t const & );

            static_assert ( narrowing < decltype ( -1 ) , fd_t , true > :: value != true );
            static constexpr fd_t InvalidFD () { return static_cast < fd_t > ( -1 ) ; }

        private :

            timeout_t Worker () override;

            valid_t Init ();
            valid_t Deinit ();

            static_assert ( narrowing < decltype ( -1 ) , socket_t , true > :: value != true );
            static constexpr socket_t InvalidSocket () { return static_cast < socket_t > ( -1 ) ; }

            valid_t Connect ( timeout_t const );
            valid_t Disconnect ( timeout_t const );

            valid_t Send ( std::string const & , int const * , uint8_t const );
            valid_t Receive ( std::string & , int * , uint8_t const );
        };



        class Natives final {
        private :

            ModeSet & _set;

            using dpy_return_t = decltype ( std::declval < ModeSet > () . UnderlyingHandle () );
            using surf_return_t = decltype ( std::declval < ModeSet > () . CreateRenderTarget ( 0 , 0 ) );

            static_assert ( std::is_pointer < surf_return_t > :: value != false );
            surf_return_t _surf = nullptr;

            bool _valid = false;

        public :

            Natives () = delete;
            Natives ( Natives const & ) = delete;
            Natives & operator= ( Natives const & ) = delete;

            using dpy_t = dpy_return_t;
            using surf_t = decltype ( _surf );
            using valid_t = decltype ( _valid );

            explicit Natives ( ModeSet & );
            ~Natives ();

            dpy_t Display () const { return _set . UnderlyingHandle () ; }
            surf_t Surface () const { return _surf ; }

            valid_t Invalidate ();

            valid_t Valid () const { return _valid ; }

        private :

            valid_t Initialize ();
            void DeInitialize ();

            static_assert ( std::is_pointer < dpy_t > :: value != false );
            static constexpr dpy_t InvalidDisplay () { return nullptr ; }

            static_assert ( std::is_pointer < surf_t > :: value != false );
            static constexpr surf_t InvalidSurface () { return nullptr ; }
        };



        class GLES final {
        private :

            // x, y, z
            static constexpr uint8_t const VerticeDimensions = 3;

            struct offset {
                using coordinate_t = GLfloat;

                static_assert ( std::is_same < GLfloat , float > :: value != false );

                // Each coorrdinate in the range [-1.0f, 1.0f]
                static constexpr coordinate_t const _left = static_cast < coordinate_t > ( -1.0f );
                static constexpr coordinate_t const _right = static_cast < coordinate_t > ( 1.0f );
                static constexpr coordinate_t const _bottom = static_cast < coordinate_t > ( -1.0f );
                static constexpr coordinate_t const _top = static_cast < coordinate_t > ( 1.0f );
                static constexpr coordinate_t const _near = static_cast < coordinate_t > ( -1.0f );
                static constexpr coordinate_t const _far = static_cast < coordinate_t > ( 1.0f );

                coordinate_t _x;
                coordinate_t _y;
                coordinate_t _z;

                offset ();
                explicit offset ( coordinate_t const & , coordinate_t const & , coordinate_t const & );
            } _offset;



            struct scale {
                using fraction_t = GLclampf;

                static_assert ( std::is_same < float , fraction_t > :: value != false );

                static constexpr fraction_t const _identity = static_cast < fraction_t > ( 1.0f );
                static constexpr fraction_t const _min  = static_cast < fraction_t > ( 0.0f );
                static constexpr fraction_t const _max  = static_cast < fraction_t > ( 1.0f );

                fraction_t _horiz;
                fraction_t _vert;

                scale ();
                explicit scale ( fraction_t const & , fraction_t const & );
            } _scale;



            struct opacity {
                using alpha_t = GLfloat;

                static_assert ( std::is_same < float , alpha_t > :: value != false);

                static constexpr alpha_t const _min = static_cast < alpha_t > ( 0.0f );
                static constexpr alpha_t const _max = static_cast < alpha_t > ( 1.0f );

                alpha_t _alpha;

                opacity ();
                explicit opacity ( alpha_t const & );
                explicit opacity ( opacity const & );

                opacity & operator= ( alpha_t const & alpha ) {
                    _alpha = alpha;
                    return * this;
                }

                static opacity const & InitialOpacity () {
                    static opacity const op ( _max );
                    return op;
                }
            } _opacity;


            bool _valid;

            using prog_t = GLuint;

            // Possibly signed to unsigned
            static_assert ( std::is_integral < prog_t > :: value != false );
            static constexpr prog_t InvalidProg () { return static_cast < prog_t > ( 0 ) ; }

        public :

            using tgt_t = GLuint;
            using tex_t = GLuint;
            using offset_t = decltype ( _offset );
            using scale_t = decltype ( _scale );
            using opacity_t = decltype ( _opacity );

            using x_t = GLuint;
            using y_t = GLuint;
            using z_t = GLuint;
            using width_t = GLuint;
            using height_t = GLuint;

            using version_t = GLuint;

            using valid_t = decltype ( _valid );



        public :

            struct texture {
                tex_t _tex;
                tgt_t _target;

                opacity_t _opacity;

                x_t _x;
                y_t _y;
                z_t _z;

                width_t _width;
                height_t _height;

                texture ();
                explicit texture ( tgt_t const , opacity_t const & );
                explicit texture ( texture const & );
            };

        public :

            using texture_t = texture;

        private :

            std::map < EGL::img_t , texture_t > _scene;

            std::stack < texture_t > _preallocated;

            std::mutex _token;

        public :

            GLES ( GLES const & ) = delete;
            GLES & operator= ( GLES const & ) = delete;

            GLES ();
            ~GLES ();

            static offset_t const InitialOffset () { return offset () ; }
            valid_t UpdateOffset ( offset_t const & );

            static scale_t const InitialScale () { return scale () ; }
            valid_t UpdateScale ( scale_t const & );

            static opacity_t const & InitialOpacity () { return CompositorImplementation::GLES::opacity::InitialOpacity () ; }
            valid_t UpdateOpacity ( opacity_t const & );

            static_assert ( std::is_integral < tex_t > :: value != false );
            static constexpr tex_t InvalidTex () { return static_cast < tex_t > ( 0 ) ; }

            // Possibly signed to unsiged
            static_assert (    narrowing < decltype ( 2 ) , version_t , true > :: value != true
                                    // Trivial
                            || (    2 >= 0
                                 && in_unsigned_range < version_t , 2 > :: value != false
                               )
                          );
            static constexpr version_t MajorVersion () { return static_cast < version_t > ( 2 ) ; }
            static constexpr version_t MinorVersion () { return static_cast < version_t > ( 0 ) ; }

            valid_t Valid () const { return _valid ; }

            valid_t Render () { return Valid () ; }
            valid_t RenderColor ( bool const , bool const , bool const , bool const alpha = true );
            valid_t RenderEGLImage ( EGL::img_t const & , EGLint const , EGLint const , EGL::width_t const , EGL::height_t const , EGLint , EGLint const );
            valid_t RenderScene ( width_t const , height_t const , std::function < valid_t ( texture_t const & , texture_t const & ) > );

            valid_t SkipEGLImageFromScene ( EGL::img_t const & );

        private :

            valid_t Initialize ();
            valid_t Deinitialize ();

// TODO: precompile programs at initialization stage

            valid_t SetupProgram ( char const [] , char const [] , prog_t & );

            valid_t RenderTileOES ();
            valid_t RenderTile ();

            template < size_t N >
            valid_t RenderPolygon ( std::array < GLfloat const , N > const & );

            valid_t Supported ( std::string const & );

            valid_t SetupViewport ( EGLint const , EGLint const , EGL::width_t const , EGL::height_t const );
       };



        class EGL {
        private :
#define XSTRINGIFY(X) STRINGIFY(X)
#define STRINGIFY(X) #X

#ifdef EGL_VERSION_1_5
#define KHRFIX(name) name
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE
#define _EGL_NO_SYNC EGL_NO_SYNC
#define _EGL_FOREVER EGL_FOREVER
#define _EGL_NO_IMAGE EGL_NO_IMAGE
#define _EGL_NATIVE_PIXMAP EGL_NATIVE_PIXMAP_KHR
#define _EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED
#define _EGL_SYNC_STATUS EGL_SYNC_STATUS
#define _EGL_SIGNALED EGL_SIGNALED
#define _EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT
#else
#define _KHRFIX(left, right) left ## right
#define KHRFIX(name) _KHRFIX(name, KHR)
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE_KHR
#define _EGL_NO_SYNC EGL_NO_SYNC_KHR
#define _EGL_FOREVER EGL_FOREVER_KHR
#define _EGL_NO_IMAGE EGL_NO_IMAGE_KHR
#define _EGL_NATIVE_PIXMAP EGL_NATIVE_PIXMAP_KHR
#define _EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED_KHR
#define _EGL_SYNC_STATUS EGL_SYNC_STATUS_KHR
#define _EGL_SIGNALED EGL_SIGNALED_KHR
#define _EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT_KHR

            using EGLAttrib = EGLint;
#endif
            using EGLuint64KHR = khronos_uint64_t;

            EGLContext _ctx = EGL_NO_CONTEXT;

            using valid_t = bool;

        public :

            using ctx_t = decltype ( _ctx );



            class Sync final {
            public :

                using dpy_t = EGLDisplay;

            public :

                Sync () = delete;
                Sync ( Sync const & ) = delete;
                Sync ( Sync && ) = delete;

                Sync & operator= ( Sync const & ) = delete;
                Sync & operator= ( Sync && ) = delete;

                void * operator new ( size_t ) = delete;
                void * operator new [] ( size_t ) = delete;
                void operator delete ( void * ) = delete;
                void operator delete [] ( void * ) = delete;

                explicit Sync ( dpy_t & );
                ~Sync ();

            private :

                using sync_t = KHRFIX ( EGLSync );

                static constexpr dpy_t InvalidDpy () { return static_cast < dpy_t > ( EGL::InvalidDpy () ) ; }

                static_assert ( std::is_convertible < sync_t , decltype ( _EGL_NO_SYNC ) > :: value );
                static constexpr sync_t InvalidSync () { return static_cast < sync_t > ( _EGL_NO_SYNC ) ; }

                sync_t _sync;
                dpy_t & _dpy;
            };



            class RenderThread : public Core::Thread {
            protected :

                using timeout_t = remove_const < decltype ( WPEFramework::Core::infinite ) > :: type;
                using lock_t = std::mutex;

                // Shared texture access
                lock_t & _sharing;

                EGL & _egl;
                GLES & _gles;

                explicit RenderThread ( EGL & , GLES & );

            public :

                RenderThread ( RenderThread const & ) = delete;
                RenderThread & operator= ( RenderThread const & ) = delete;

                virtual ~RenderThread ();

                uint32_t Worker () override;

            private :

                lock_t & Instance ();
            };



            class SceneRenderer final : public RenderThread {
            private :

                using surf_t = ClientSurface;

                CompositorImplementation & _compositor;

                // Local
                std::mutex _access;

            public :

                SceneRenderer () = delete;
                SceneRenderer ( SceneRenderer const & ) = delete;
                SceneRenderer & operator= ( SceneRenderer const & ) = delete;

                explicit SceneRenderer ( EGL & , GLES & , CompositorImplementation & );
                ~SceneRenderer () override;

                timeout_t Worker () override;

            private :

                EGL::valid_t Render ();
            };



            class TextureRenderer final : public RenderThread {
            private :

                using surf_t = ClientSurface;

                // Local
                std::mutex _access;

                CompositorImplementation::ClientContainer & _clients;

                struct element {
                public :

                    std::string const & _name;

                    element () = delete;

                    explicit element (std::string const & name) : _name { name } { }
                    ~element () = default;
                };

                using element_t = struct element;

                using function_t = std::function < bool ( element_t const & , element_t const & ) >;

                // Unique elements, to prevent queue from growing beyond N
                std::set < element_t , function_t > set;
                // FIFO
                std::queue < element_t > queue;

            public :

                TextureRenderer () = delete;
                TextureRenderer ( TextureRenderer const & ) = delete;
                TextureRenderer & operator= ( TextureRenderer const & ) = delete;

                explicit TextureRenderer (EGL & , GLES & , CompositorImplementation::ClientContainer & );
                ~TextureRenderer ();

                void SetClientName ( std::string const & );

                timeout_t Worker () override;

            private :

                EGL::valid_t Render ();
        };



        public :

            using dpy_t = Sync::dpy_t;
            using surf_t = EGLSurface;

            using height_t = WPEFramework::Plugin::EGL::height_t;
            using width_t = WPEFramework::Plugin::EGL::width_t;

            using size_t = EGLint;

            using img_t = WPEFramework::Plugin::EGL::img_t;

            EGL () = delete;
            EGL ( EGL const & ) = delete;
            EGL & operator= ( EGL const & ) = delete;

            EGL ( Natives const & natives );
            ~EGL ();

            static constexpr img_t InvalidImage () { return WPEFramework::Plugin::EGL::InvalidImage () ; }

            static_assert ( std::is_convertible < dpy_t , decltype ( EGL_NO_DISPLAY ) > :: value );
            static constexpr dpy_t InvalidDpy () { return EGL_NO_DISPLAY ; }
            static_assert ( std::is_convertible < ctx_t , decltype ( EGL_NO_CONTEXT ) > :: value );
            static constexpr ctx_t InvalidCtx () { return EGL_NO_CONTEXT ; }
            static_assert ( std::is_convertible < surf_t , decltype ( EGL_NO_SURFACE ) > :: value );
            static constexpr surf_t InvalidSurf () { return EGL_NO_SURFACE ; }

            static_assert ( std::is_integral < EGL::size_t > :: value != false );
            static constexpr EGL::size_t RedBufferSize () { return static_cast < EGL::size_t > ( 8 ) ; }
            static constexpr EGL::size_t GreenBufferSize () { return static_cast < EGL::size_t > ( 8)  ; }
            static constexpr EGL::size_t BlueBufferSize () { return static_cast < EGL::size_t > ( 8 ) ; }
            static constexpr EGL::size_t AlphaBufferSize () { return static_cast < EGL::size_t > ( 8 ) ; }
            // For OpenGL ES 2.0 the only possible value is 16
            static constexpr EGL::size_t DepthBufferSize () { return static_cast < EGL::size_t > ( GLES::MajorVersion () == static_cast < GLES::version_t > ( 2 ) ? 16 : 0 ) ; }

            static_assert ( std::is_integral < width_t > :: value != false );
            static constexpr width_t InvalidWidth () { return static_cast < width_t > ( 0 ) ; }
            static_assert ( std::is_integral < height_t > :: value != false );
            static constexpr height_t InvalidHeight () { return static_cast < height_t > ( 0 ) ; }

            dpy_t Display () const { return _dpy ; }
            surf_t Surface () const { return _surf ; }
            ctx_t Context () const { return _ctx ; }

            height_t Height () const { return _height ; }
            width_t Width () const { return _width ; }

            static img_t CreateImage ( EGL const & , ClientSurface::surf_t const & );
            static img_t DestroyImage ( EGL const & , ClientSurface::surf_t const & );

            valid_t Invalidate ();

            valid_t Valid () const { return _valid ; }

        private :

            // Define the 'invalid' value
            static_assert ( std::is_pointer < EGLConfig > :: value != false );
            static constexpr void * const EGL_NO_CONFIG = nullptr;

            dpy_t _dpy = InvalidDpy ();
            EGLConfig _conf = EGL_NO_CONFIG;
            surf_t _surf = InvalidSurf ();

            width_t  _width = InvalidWidth ();
            height_t _height = InvalidHeight ();

            Natives const & _natives;

            valid_t _valid = false;


            valid_t Initialize ();
            void DeInitialize ();

            template < typename FUNC , typename... ARG >
            valid_t Render ( std::false_type const , FUNC && , bool const , ARG &&... );

            template < typename FUNC , typename ARG0 , typename... ARG >
            valid_t Render ( std::true_type const , FUNC && , bool const , ARG0 && , ARG &&... );

 //TODO: two different signatures for callables
            template < typename FUNC , typename... ARG >
            valid_t Render ( std::false_type const , FUNC && , FUNC && , ARG &&... );

            template < typename FUNC , typename ARG0 , typename... ARG >
            valid_t Render ( std::true_type const , FUNC && , FUNC && , ARG0 && , ARG &&... );

            template < typename FUNC , typename... ARG >
            valid_t RenderWithoutSwap ( std::false_type const , FUNC && , ARG &&... );

            template < typename FUNC , typename ARG0 , typename... ARG >
            valid_t RenderWithoutSwap ( std::true_type const , FUNC && , ARG0 && , ARG &&... );

        public :

            // Although compile / build time may succeed, runtime checks are also mandatory
            static valid_t Supported ( dpy_t const , std::string const & );

            valid_t Render ( GLES & );

            template < typename FUNC , typename... ARG >
            valid_t Render ( FUNC && , bool const , ARG &&... );

            template < typename FUNC , typename... ARG >
            valid_t Render ( FUNC && , FUNC  && , ARG &&... );

            template < typename FUNC , typename... ARG >
            valid_t RenderWithoutSwap ( FUNC && , ARG &&... );

#undef STRINGIFY
#ifdef _KHRFIX
#undef _KHRFIX
#endif
#undef KHRFIX
#undef _EGL_SYNC_FENCE
#undef _EGL_NO_SYNC
#undef _EGL_FOREVER
#undef _EGL_NO_IMAGE
#undef _EGL_NATIVE_PIXMAP
#undef _EGL_CONDITION_SATISFIED
#undef _EGL_SYNC_STATUS
#undef _EGL_SIGNALED
#undef _EGL_SYNC_FLUSH_COMMANDS_BIT
        };



    private :



        class Config : public Core::JSON::Container {
        public :

            Config ( Config const & ) = delete;
            Config & operator= ( Config const & ) = delete;

            Config ();
            ~Config ();

            Core::JSON::String _connector;
            Core::JSON::String _port;
        };



        void PlatformReady ();

        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell * _service;
        Core::ProxyType < RPC::InvokeServer > _engine;
        ExternalAccess * _externalAccess;
        std::list < Exchange::IComposition::INotification * > _observers;
        ClientContainer _clients;
        string _port;
        ModeSet _platform;

        DMATransfer * _dma;
        Natives _natives;
        EGL _egl;
        GLES _gles;

        lock_t _clientLock;

        EGL::TextureRenderer _textureRenderer;
        EGL::SceneRenderer _sceneRenderer;

    public :

        CompositorImplementation ();
        ~CompositorImplementation ();

        bool FDFor ( std::string const & , DMATransfer::fds_t & );
        bool FDFor ( std::string const & , DMATransfer::fds_t & , std::string & );

        bool CompositeFor ( std::string const & );

        bool FrameFlip ();

        //
        // Echange::IComposition
        // ==================================================================================================================

        static uint32_t WidthFromResolution ( ScreenResolution const );
        static uint32_t HeightFromResolution ( ScreenResolution const );
        static ScreenResolution ResolutionFromHeightWidth ( uint32_t const , uint32_t const );

        uint32_t Configure ( PluginHost::IShell * service) override;

        void Register ( Exchange::IComposition::INotification * ) override;
        void Unregister ( Exchange::IComposition::INotification * ) override;

        void Attached ( string const & , IClient * );
        void Detached ( string const & );

        //
        // Echange::IComposition::IDisplay
        // ==================================================================================================================
        Core::instance_id Native () const override;

        string Port () const override;

        IClient * CreateClient ( string const & , uint32_t const , uint32_t const ) override;
        void InvalidateClient ( string const & ) override;

        Exchange::IComposition::ScreenResolution Resolution () const override;
        uint32_t Resolution ( Exchange::IComposition::ScreenResolution const ) override;


        BEGIN_INTERFACE_MAP ( CompositorImplementation )
        INTERFACE_ENTRY ( Exchange::IComposition )
        INTERFACE_ENTRY ( Exchange::IComposition::IDisplay )
        END_INTERFACE_MAP
    };



    ClientSurface::ClientSurface ( ModeSet & modeSet , CompositorImplementation & compositor , string const & name , uint32_t const width , uint32_t const height )
        : _nativeSurface { ModeSet::GBM::InvalidBuf () , ModeSet::GBM::InvalidFd ()  , ModeSet::GBM::InvalidFd () , WPEFramework::Plugin::EGL::InvalidImage () }
        , _modeSet { modeSet }
        , _compositor { compositor }
        , _name { name }
        , _opacity { Exchange::IComposition::maxOpacity }
        , _layer { 0 }
        , _destination ( { 0 , 0 , width , height } ) {

        // The compositor should not be destroyed before any existing client surface
        _compositor . AddRef ();

        using surface_t = decltype ( _nativeSurface . _buf );
        using class_t = std::remove_reference < decltype ( _modeSet ) > :: type;
        using return_t = decltype ( std::declval < class_t > () . CreateBufferObject ( width , height ) );

        static_assert ( std::is_same < surface_t , return_t > :: value != false );

// TODO: The internal scan out flag might not be appropriate
        _nativeSurface . _buf = _modeSet . CreateBufferObject ( width, height );

        if (  _nativeSurface . Valid () != true ) {
            TRACE ( Trace::Error , ( _T ( "A ClientSurface cannot be created for %s" ) , name . c_str () ) );
        }
        else {
            _nativeSurface . _fd = gbm_bo_get_fd ( _nativeSurface ._buf );

// TODO:
            constexpr char const SYNC_FD_TEMPLATE [] = "/tmp/Compositor/sync_fdXXXXXX";

            _nativeSurface . _sync_fd = mkostemp ( const_cast < char * > ( SYNC_FD_TEMPLATE ) , O_CLOEXEC );

            if ( _nativeSurface . DMAComplete () != true ) {
                TRACE ( Trace::Error , ( _T ( "The created ClientSurface for %s is not suitable for DMA." ) , name . c_str () ) );
            }
        }
    }

    ClientSurface::~ClientSurface () {
        // Part of the client is cleaned up via the detached (hook)
        if ( _nativeSurface . _buf != ModeSet::GBM::InvalidBuf () ) {
            _modeSet . DestroyBufferObject ( _nativeSurface . _buf );
        }

        if ( _nativeSurface . _fd != ModeSet::GBM::InvalidFd () ) {
            int ret = close ( _nativeSurface . _fd );
            assert ( ret != -1 );
        }

        if ( _nativeSurface . _sync_fd != ModeSet::GBM::InvalidFd () ) {
            int ret = close ( _nativeSurface . _sync_fd );
            assert ( ret != -1 );
        }

        _nativeSurface = { ModeSet::GBM::InvalidBuf () , ModeSet::GBM::InvalidFd () , ModeSet::GBM::InvalidFd () , WPEFramework::Plugin::EGL::InvalidImage () };

        _compositor . Release ();
    }

    Core::instance_id ClientSurface::Native () const {
        // Sharing this handle does not imply its contents can be accessed!
// TODO: narrowing
        static_assert ( ( std::is_convertible < decltype ( _nativeSurface . _fd ) , Core::instance_id > :: value ) != false );
        static_assert ( sizeof ( decltype ( _nativeSurface . _fd ) ) <= sizeof ( Core::instance_id ) );

        return static_cast < Core::instance_id > ( _nativeSurface . _fd );
    }

    Core::instance_id ClientSurface::SyncPrimitive () const {
// TODO: narrowing
        static_assert ( ( std::is_convertible < decltype ( _nativeSurface . _sync_fd ) , Core::instance_id > :: value ) != false );
        static_assert ( sizeof ( decltype ( _nativeSurface . _sync_fd ) ) <= sizeof ( Core::instance_id ) );

        return static_cast < Core::instance_id > ( _nativeSurface . _sync_fd );
    }

    bool ClientSurface::SyncPrimitiveStart () const {
        auto init = [] () -> struct flock {
            struct flock fl;
            /* void * */ memset ( & fl , 0 , sizeof ( fl ) );

            fl . l_type = F_WRLCK;
            fl . l_whence = SEEK_SET;
            fl . l_start = 0;
            fl . l_len = 0;

            return fl;
        };

        static struct flock fl = init ();

        // Operates on i-node
        bool ret =    _nativeSurface . _sync_fd != ModeSet::GBM::InvalidFd ()
                   && fcntl ( _nativeSurface . _sync_fd , F_SETLKW ,  & fl ) != -1;

        assert ( ret != false );

        return ret;
    }

    bool ClientSurface::SyncPrimitiveEnd () const {
        auto init = [] () -> struct flock {
            struct flock fl;
            /* void * */ memset ( & fl , 0 , sizeof ( fl ) );

            fl . l_type = F_UNLCK;
            fl . l_whence = SEEK_SET;
            fl . l_start = 0;
            fl . l_len = 0;

            return fl;
        };

        static struct flock fl = init ();

        bool ret =    _nativeSurface . _sync_fd != ModeSet::GBM::InvalidFd ()
                   && fcntl ( _nativeSurface . _sync_fd , F_SETLK , & fl ) != -1;

        assert ( ret != false );

        return ret;
    }

    void ClientSurface::ScanOut () {
        /* bool */ _compositor . CompositeFor ( _name );
    }

    ClientSurface::surf_t const & ClientSurface::Surface ( EGL::img_t const & khr ) {
        if ( khr != EGL::InvalidImage () ) {
            _nativeSurface . _khr = khr;
        }

        return _nativeSurface;
    }



    CompositorImplementation::ExternalAccess::ExternalAccess ( CompositorImplementation & parent , Core::NodeId const & source , string const & proxyStubPath , Core::ProxyType < RPC::InvokeServer > const & handler )
        : RPC::Communicator ( source , proxyStubPath.empty () == false ? Core::Directory::Normalize ( proxyStubPath ) : proxyStubPath , Core::ProxyType < Core::IIPCServer > ( handler ) )
        , _parent ( parent ) {

        uint32_t result = RPC::Communicator::Open ( RPC::CommunicationTimeOut );

        handler -> Announcements ( Announcement() );

        static_assert ( std::is_enum < decltype ( Core::ERROR_NONE ) > :: value != false );
        static_assert (    narrowing < std::underlying_type < decltype ( Core::ERROR_NONE ) > :: type , decltype ( result ) , true > :: value != false
                        || (    Core::ERROR_NONE >= static_cast < std::underlying_type < decltype ( Core::ERROR_NONE ) > :: type > ( 0 )
                             && in_unsigned_range < decltype ( result ) , Core::ERROR_NONE > :: value != false
                           )
                      );

        if ( result != Core::ERROR_NONE ) {
            TRACE ( Trace::Error , ( _T ( "Could not open RPI Compositor RPCLink server. Error: %s" ) , Core::NumberType < uint32_t > ( result ) . Text () ) );
        } else {
            // We need to pass the communication channel NodeId via an environment variable, for process,
            // not being started by the rpcprocess...
            Core::SystemInfo::SetEnvironment ( _T ( "COMPOSITOR" ) , RPC::Communicator::Connector () , true );
        }
    }

    CompositorImplementation::ExternalAccess::~ExternalAccess () {
    }

    void * CompositorImplementation::ExternalAccess::Aquire ( string const & className , uint32_t const interfaceId , uint32_t const version ) {
        silence ( className );
        silence ( version );

        // Use the className to check for multiple HDMI's. 
        return ( _parent . QueryInterface ( interfaceId ) );
    }



    CompositorImplementation::DMATransfer::DMATransfer ( CompositorImplementation & compositor )
        : Core::Thread ( /* 0 , _T ( "" ) */ )
        , _compositor { compositor }
        , _listen { InvalidSocket () }
        , _transfer { InvalidSocket () }
        , _addr { AF_UNIX , "/tmp/Compositor/DMA" }
        , _valid { Init () } {
    }

    CompositorImplementation::DMATransfer::~DMATransfer () {
        Stop ();

        /* bool */ Wait ( WPEFramework::Core::Thread::STOPPED , WPEFramework::Core::infinite );

        /* valid_t */ Deinit ();
    }

    CompositorImplementation::DMATransfer::valid_t CompositorImplementation::DMATransfer::Receive ( std::string & msg , DMATransfer::fds_t & fds ) {
        valid_t ret =    Valid ()
                      && Connect ( Core::infinite );

        if ( ret != true ) {
            TRACE ( Trace::Information , ( _T ( "Unable to receive (DMA) data." ) ) );
        }
        else {
            ret = Receive ( msg , fds . data () , fds . size () );
            ret = Disconnect ( Core::infinite ) && ret;
        }

        return ret;
    }

    CompositorImplementation::DMATransfer::valid_t CompositorImplementation::DMATransfer::Send ( std::string const & msg, DMATransfer::fds_t const & fds ) {
        valid_t ret =    Valid ()
                      && Connect ( Core::infinite );

        if ( ret != true ) {
            TRACE ( Trace::Information , ( _T ( "Unable to send (DMA) data." ) ) );
        }
        else {
            ret =    Send ( msg , fds . data () , fds . size () )
                  && Disconnect ( Core::infinite );

            ret =    Disconnect ( Core::infinite )
                  && ret;
        }

        return ret;
    }

    CompositorImplementation::DMATransfer::timeout_t CompositorImplementation::DMATransfer::Worker () {
        // The actual time (schedule) resolution
// TODO:: arbitrary value
        // Approximately 1 Hz
        static_assert ( narrowing < decltype ( 1000 ) , CompositorImplementation::DMATransfer::timeout_t , true > :: value != true
                                // Trivial
                        || (    1000 >= 0
                             && in_unsigned_range < timeout_t , 1000 > :: value != false
                           )
                      );
        constexpr CompositorImplementation::DMATransfer::timeout_t const ret = 1000;

        // Default behavior
        Block ();

        fd_set fds;

        FD_ZERO ( & fds );
        FD_SET ( _listen , & fds );

        // Just use 'polling'
        constexpr struct timespec const timeout = { . tv_sec = 0, . tv_nsec = 0 };

        int err = pselect ( _listen + 1 , & fds , nullptr , nullptr , & timeout , nullptr );

        if ( err < 0) {
            // Error
            TRACE ( Trace::Error , ( _T ( "Unable to accept DMA connections" ) ) );

            // 'Kill' (future) thread's operation
            Stop ();
        }
        else {
            if ( err == 0 ) {
                // Timeout, no (new) connection available
            }
            else {
                if ( FD_ISSET ( _listen , & fds ) != 0 ) {

// TODO: set flags on _transfer ?
                    _transfer = accept ( _listen , nullptr , nullptr );

                    // Do some processing on the clients

                    std::string msg;

                    // Shared buffer and synchronization primitive
                    DMATransfer::fds_t handles = { InvalidFD () , InvalidFD () };

                    std::string props;

                    if ( _transfer != InvalidSocket () ) {
                        if ( (    Receive ( msg , handles )
                               && _compositor . FDFor ( msg , handles , props )
                               && Send ( msg + props, handles )
                             ) != false
                           ) {

                            // Just wait for the remote peer to close the connection
                            ssize_t size = read ( _transfer , nullptr , 0 );

                            decltype ( errno ) err = errno;

                            switch ( size ) {
                                case -1 :   // Error
                                            static_assert ( narrowing < std::remove_reference < decltype ( err ) > :: type , int , true > :: value != true );
                                            TRACE ( Trace::Error , ( _T ( "Error after DMA transfer : %d." ) , err ) );
                                            break;
                                case 0  :   // remote has closed the connection
                                            TRACE ( Trace::Information , ( _T  ( "Remote has closed the DMA connection." ) ) );
                                            break;
                                default :   // Unexpected data available
                                            TRACE ( Trace::Error , ( _T ( "Unexpected data read after DMA transfer." ) ) );
                            }

                            /* int */ close ( _transfer );

                            _transfer = InvalidSocket ();
                        }
                        else {
                            TRACE ( Trace::Error , ( _T ( "Failed to exchange DMA information for %s." ) ,  msg . length () > 0 ? msg . c_str () : "'<no name provided>'" ) );
                        }
                    }
                    else {
                        // Not necessarily an error
                        TRACE ( Trace::Information , ( _T ( "Premature incoming connection loss." ) ) );
                    }
                }
            }
        }

        return ret;
    }

    CompositorImplementation::DMATransfer::valid_t CompositorImplementation::DMATransfer::Init () {
        valid_t ret = false;

        // Just a precaution
        /* int */ unlink ( _addr.sun_path );

        _listen = socket ( _addr . sun_family /* domain */ , SOCK_STREAM /* type */ , 0 /* protocol */ );

// TODO: Derive it from maximum number of supported clients
        // Number of pending requests for accept to handle
        constexpr int queue_size = 1;

        ret =    _listen != InvalidSocket ()
              && bind ( _listen , reinterpret_cast < struct sockaddr const * > ( & _addr ) , sizeof ( _addr ) ) == 0
              && listen ( _listen , queue_size ) == 0
              && fcntl ( _listen , F_SETFL , O_NONBLOCK ) == 0
              ;

         return ret;
    }

    CompositorImplementation::DMATransfer::valid_t CompositorImplementation::DMATransfer::Deinit () {
        valid_t ret = false;

        ret =    _listen != InvalidSocket ()
              && close ( _listen ) == 0
              && _transfer != InvalidSocket ()
              && close ( _transfer ) == 0
              ;

        // (always) delete the (bind) socket in the file system if no reference exist (anymore)
        ret =    unlink ( _addr . sun_path ) == 0
              && ret;

        return ret;
    }

    CompositorImplementation::DMATransfer::valid_t CompositorImplementation::DMATransfer::Connect ( CompositorImplementation::DMATransfer::timeout_t const timeout ) {
        silence ( timeout );

        using err_t = remove_reference < decltype ( errno ) > :: type;

        err_t err = errno;

        valid_t ret =    _transfer != InvalidSocket ()
                      && err == static_cast < err_t > ( 0 );

        return ret;
    }

    CompositorImplementation::DMATransfer::valid_t CompositorImplementation::DMATransfer::Disconnect ( CompositorImplementation::DMATransfer::timeout_t const timeout ) {
        silence ( timeout );

        using err_t = remove_reference < decltype ( errno ) > :: type;

        err_t err = errno;

        valid_t ret =    _transfer != InvalidSocket ()
                      && err == static_cast < err_t > ( 0 );

        return ret;
    }

    CompositorImplementation::DMATransfer::valid_t CompositorImplementation::DMATransfer::Send ( std::string const & msg , int const * fd , uint8_t const count ) {
        using fd_t = remove_const < std::remove_pointer < decltype ( fd ) > :: type > :: type;

        valid_t ret = false;

        // Logical const
        static_assert ( ( std::is_same < char * , remove_const < decltype ( & msg [ 0 ] ) > :: type > :: value ) != false );
        char * buf  = const_cast < char * > ( & msg [ 0 ] );

        size_t const bufsize = msg . size ();

        constexpr decltype ( bufsize ) minimum_buf_size = 0;

        if ( bufsize > minimum_buf_size ) {

            // Scatter array for vector I/O
            struct iovec iov;

            // Starting address
            iov . iov_base = reinterpret_cast < void * > ( buf );
            // Number of bytes to transfer
            iov . iov_len = bufsize;

            // Actual message
            struct msghdr msgh {};

            // Optional address
            msgh . msg_name = nullptr;
            // Size of address
            msgh . msg_namelen = 0;
            // Scatter array
            msgh . msg_iov = & iov;
            // Elements in msg_iov
            msgh . msg_iovlen = 1;

            // Ancillary data
            // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
            char control [ CMSG_SPACE ( sizeof ( fd_t ) * count ) ];

            // Only valid file descriptor (s) can be sent via extra payload
            ret = true;
            for ( remove_const < decltype ( count ) > :: type i = 0 ; i < count && fd != nullptr ; i++ ) {
                ret = fd [ i ] != InvalidFD () && ret;
            }

            // At least  the first fd should be valid
            if ( ret != false ) {
                // Contruct ancillary data to be added to the transfer via the control message

                // Ancillary data, pointer
                msgh . msg_control = control;

                // Ancillary data buffer length
                msgh . msg_controllen = sizeof ( control );

                // Ancillary data should be access via cmsg macros
                // https://linux.die.net/man/2/recvmsg
                // https://linux.die.net/man/3/cmsg
                // https://linux.die.net/man/2/setsockopt
                // https://www.man7.org/linux/man-pages/man7/unix.7.html

                // Control message

                // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                struct cmsghdr * cmsgh = CMSG_FIRSTHDR ( & msgh );

                if ( cmsgh != nullptr ) {
                    // Originating protocol
                    // To manipulate options at the sockets API level
                    cmsgh -> cmsg_level = SOL_SOCKET;

                    // Protocol specific type
                    // Option at the API level, send or receive a set of open file descriptors from another process
                    cmsgh -> cmsg_type = SCM_RIGHTS;

                    // The value to store in the cmsg_len member of the cmsghdr structure, taking into account any necessary alignment, eg byte count of control message including header
                    cmsgh -> cmsg_len = CMSG_LEN ( sizeof ( fd_t ) * count );

                    // Initialize the payload
                    /* void */ memcpy ( CMSG_DATA ( cmsgh ) , fd , sizeof ( fd_t ) * count );

                    ret = true;
                }
                else {
                    // Error
                }
            }
            else {
                // No extra payload, ie  file descriptor(s), to include
                msgh . msg_control = nullptr;
                msgh . msg_controllen = 0;

                ret = true;
            }

            if ( ret != false ) {
                // Configuration succeeded

                // https://linux.die.net/man/2/sendmsg
                // https://linux.die.net/man/2/write
                // Zero flags is equivalent to write

                constexpr ssize_t const data_invalid_size = 0;

                remove_const < decltype ( data_invalid_size ) > :: type  size = data_invalid_size;

                socklen_t len = sizeof ( size );

                // Only send data if the buffer is large enough to contain all data
                if ( getsockopt ( _transfer , SOL_SOCKET , SO_SNDBUF , & size , & len ) == 0 ) {
                    // Most options use type int, ssize_t was just a placeholder
                    static_assert ( narrowing < decltype ( size ) , long , true > :: value != true );
                    TRACE ( Trace::Information , ( _T ( "The sending buffer capacity equals %ld bytes." ) , size ) );

// TODO: do not send if the sending buffer is too small
                    size = sendmsg ( _transfer , & msgh , 0);
                }
                else {
                    // Unable to determine buffer capacity
                }

                ret = size != data_invalid_size;

                if ( ret != false ) {
                    // Ancillary data is not included
                    static_assert ( narrowing < decltype ( size ) , long , true > :: value != true );
                    static_assert ( narrowing < decltype ( bufsize ) , unsigned long , true > :: value != true );
                    TRACE ( Trace::Information , ( _T ( "Send %ld bytes out of %lu." ) , size , bufsize ) );
                }
                else {
                    TRACE ( Trace::Error , ( _T ( "Failed to send data." ) ) );
                }
            }

        }
        else {
            TRACE ( Trace::Error , ( _T ( "A data message to be send cannot be empty!" ) ) );
        }

        return ret;
    }

    CompositorImplementation::DMATransfer::valid_t CompositorImplementation::DMATransfer::Receive ( std::string & msg , int * fd , uint8_t const count ) {
        bool ret = false;

        msg . clear ();

        ssize_t size = -1;
        socklen_t len = sizeof ( size );

        if ( getsockopt ( _transfer , SOL_SOCKET , SO_RCVBUF , & size , & len ) == 0 ) {
            TRACE (Trace::Information, ( _T ( "The receiving buffer maximum capacity equals %d bytes." ) , size ) );

            // Most options use type int, ssize_t was just a placeholder
            static_assert ( sizeof ( int ) <= sizeof ( ssize_t ) );
             msg . reserve ( static_cast < int > ( size ) );
        }
        else {
            TRACE ( Trace::Information , ( _T ( "Unable to determine buffer maximum cpacity. Using %d bytes instead." ) , msg . capacity () ) );
        }

        size_t const bufsize = msg . capacity ();

        if (    bufsize > 0
             && count > 0
             && fd != nullptr
           ) {
            using fd_t = std::remove_pointer < decltype ( fd ) > :: type;

            for ( remove_const < decltype ( count ) > :: type i = 0 ; i < count ; i++ ) {
                fd [ i ] = InvalidFD ();
            }

            static_assert ( ( std::is_same < char * , remove_const  < decltype ( & msg [ 0 ] ) > :: type > :: value ) != false );
            char buf [ bufsize ];

            // Scatter array for vector I/O
            struct iovec iov;

            // Starting address
            iov . iov_base = reinterpret_cast < void * > ( & buf [ 0 ] );
            // Number of bytes to transfer
            iov . iov_len = bufsize;

            // Actual message
            struct msghdr msgh {};

            // Optional address
            msgh . msg_name = nullptr;
            // Size of address
            msgh . msg_namelen = 0;
            // Elements in msg_iov
            msgh . msg_iovlen = 1;
            // Scatter array
            msgh . msg_iov = &iov;

            // Ancillary data
            // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
            char control [ CMSG_SPACE ( sizeof ( fd_t ) * count  ) ];

            // Ancillary data, pointer
            msgh . msg_control = control;

            // Ancillery data buffer length
            msgh . msg_controllen = sizeof ( control );

// TODO: do not receive if the receiving buffer is too small
            // No flags set
            size = recvmsg ( _transfer, & msgh , 0 );

            ret = size > 0;

            switch ( size ) {
                case -1 :   // Error
                            {
                                TRACE ( Trace::Error , ( _T ( "Error receiving remote (DMA) data." ) ) );
                                break;
                            }
                case 0  :   // Peer has done an orderly shutdown, before any data was received
                            {
                                TRACE ( Trace::Error , ( _T ( "Error receiving remote (DMA) data. Compositorclient may have become unavailable. " ) ) );
                                break;
                            }
                default :   // Data
                            {
                                // Extract the file descriptor information
                                static_assert ( narrowing < decltype ( size ) , long , true > :: value != true );
                                TRACE ( Trace::Information , ( _T ( "Received %ld bytes." ) , size ) );

                                // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                                // Assume a single cmsgh was sent
                                struct cmsghdr * cmsgh = CMSG_FIRSTHDR ( & msgh );

                                // Check for the expected properties the client should have set
                                if (     cmsgh != nullptr
                                      && cmsgh -> cmsg_level == SOL_SOCKET
                                      && cmsgh -> cmsg_type == SCM_RIGHTS
                                    ) {
                                    // The macro returns a pointer to the data portion of a cmsghdr.
                                    /* void */ memcpy ( fd , CMSG_DATA ( cmsgh ) , sizeof ( fd_t ) * count );
                                }
                                else {
                                    TRACE ( Trace::Information , ( _T ( "No (valid) ancillary data received." ) ) );
                                }

                                msg . assign ( buf, size );
                            }
            }
        }
        else {
            // Error
            TRACE ( Trace::Error , ( _T ( "A receiving data buffer (message) cannot be empty!" ) ) );
        }

        return ret;
    }



    CompositorImplementation::Natives::Natives ( ModeSet & set)
        : _set { set }
        , _valid { Initialize () } {
    }

    CompositorImplementation::Natives::~Natives () {
        DeInitialize ();
        _valid = false;
    }

    CompositorImplementation::Natives::valid_t CompositorImplementation::Natives::Invalidate () {
        valid_t ret = true;

        DeInitialize ();

        return ret;
    }

    CompositorImplementation::Natives::valid_t CompositorImplementation::Natives::Initialize () {
        valid_t ret = false;

        // The argument to Open is unused, an empty string suffices
        static_assert ( std::is_pointer < dpy_t > :: value != false );
// TODO: return type Open
        ret =    _set . Open ("") == Core::ERROR_NONE
              && Display () != InvalidDisplay ();

        using width_t = decltype ( std::declval < ModeSet > () . Width () );
        using height_t = decltype ( std::declval < ModeSet > () . Height () );

        width_t width = _set . Width ();
        height_t height = _set . Height ();

        if ( ret != false ) {
            _surf = _set . CreateRenderTarget ( width , height );
            ret = _surf != InvalidSurface ();
        }

        if ( ret != true ) {
            static_assert (    narrowing < width_t , unsigned int , true > :: value != true
                            && narrowing < height_t , unsigned int , true > :: value != true
                          );
            TRACE ( Trace::Error , ( _T ( "Unable to create a compositor surface of dimensions: %u x %u [width, height]))." ) , width , height ) );
        }

        return ret;
    }

    void CompositorImplementation::Natives::DeInitialize () {
        _valid = false;

        if ( _surf != InvalidSurface () ) {
            _set . DestroyRenderTarget ( _surf );
        }

        _set . Close ();
    }


#define GL_ERROR_WITH_RETURN() ( glGetError () == GL_NO_ERROR )
#ifdef NDEBUG
#define GL_ERROR() do {} while (0)
#else
#define GL_ERROR() assert (GL_ERROR_WITH_RETURN ())
#endif

    CompositorImplementation::GLES::offset::offset ()
        : offset ( ( _right - _left ) / static_cast < coordinate_t > ( 2.0f ) + _left , ( _top - _bottom ) / static_cast < coordinate_t > ( 2.0f ) + _bottom , ( _far - _near ) / static_cast < coordinate_t > ( 2.0f ) + _near ) {
        // Repeated test
        static_assert ( narrowing < float , coordinate_t , true > :: value != true );
    }

    CompositorImplementation::GLES::offset::offset ( coordinate_t const & x , coordinate_t const & y , coordinate_t const & z )
        : _x { x }
        , _y { y }
        , _z{ z } {
    }



    // ODR use

    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_left;
    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_right;
    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_bottom;
    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_top;
    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_near;
    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_far;



    /* static */ constexpr CompositorImplementation::GLES::scale::fraction_t const CompositorImplementation::GLES::scale::_identity;
    /* static */ constexpr CompositorImplementation::GLES::scale::fraction_t const CompositorImplementation::GLES::scale::_min;
    /* static */ constexpr CompositorImplementation::GLES::scale::fraction_t const CompositorImplementation::GLES::scale::_max;

    CompositorImplementation::GLES::scale::scale ()
        : scale ( CompositorImplementation::GLES::scale::_identity , CompositorImplementation::GLES::scale::_identity ) {
    }

    CompositorImplementation::GLES::scale::scale ( CompositorImplementation::GLES::scale::fraction_t const & horiz , CompositorImplementation::GLES::scale::fraction_t const & vert )
        : _horiz { horiz }
        , _vert { vert } {
    }



    /* static */ constexpr CompositorImplementation::GLES::opacity::alpha_t const CompositorImplementation::GLES::opacity::_min;
    /* static */ constexpr CompositorImplementation::GLES::opacity::alpha_t const CompositorImplementation::GLES::opacity::_max;

    CompositorImplementation::GLES::opacity::opacity ()
        : opacity { _max } {
    }

    CompositorImplementation::GLES::opacity::opacity ( CompositorImplementation::GLES::opacity::alpha_t const & alpha )
        : _alpha { alpha } {
    }

    CompositorImplementation::GLES::opacity::opacity ( CompositorImplementation::GLES::opacity const & other )
        : _alpha { other . _alpha } {
    }



    CompositorImplementation::GLES::texture::texture ()
        : texture { static_cast < tgt_t > ( GL_INVALID_ENUM )
        , CompositorImplementation::GLES::opacity::InitialOpacity () } {
        // Possibly signed tp unsigned
        static_assert (    narrowing < decltype ( GL_INVALID_ENUM ) , tgt_t , true > :: value != true
                        || (    GL_INVALID_ENUM >= 0
                             && in_unsigned_range < tgt_t , GL_INVALID_ENUM > :: value != false
                           )
                      );
    }

    CompositorImplementation::GLES::texture::texture ( CompositorImplementation::GLES::tgt_t const target , CompositorImplementation::GLES::opacity_t const & opacity )
        : _tex { 0 }
        , _target { target }
        , _opacity { opacity }
        , _x { 0 }
        , _y { 0 }
        , _z { 0 }
        , _width { 0 }
        , _height { 0 } {
        static_assert ( std::is_integral < tex_t > :: value != false );
        static_assert ( std::is_integral < x_t > :: value != false );
        static_assert ( std::is_integral < y_t > :: value != false );
        static_assert ( std::is_integral < z_t > :: value != false );
        static_assert ( std::is_integral < width_t > :: value != false );
        static_assert ( std::is_integral < height_t > :: value != false );
    }

    CompositorImplementation::GLES::texture::texture ( CompositorImplementation::GLES::texture const & other )
        : _tex { other . _tex }
        , _target { other . _target }
        , _opacity { other . _opacity }
        , _x { other . _x }
        , _y { other . _y }
        , _z { other . _z }
        , _width { other . _width }
        , _height  { other . _height } {
    }



    CompositorImplementation::GLES::GLES ()
        : _offset { InitialOffset () }
        , _scale { InitialScale () }
        , _opacity { InitialOpacity () }
        , _valid { Initialize () } {
    }

    CompositorImplementation::GLES::~GLES () {
        _valid = false;

         /* valid_t */ Deinitialize ();
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::UpdateOffset ( CompositorImplementation::GLES::offset_t const & off ) {
        valid_t ret = false;

        // Ramge check without taking into account rounding errors
        if (     off . _x >= offset_t::_left
              && off . _x <= offset_t::_right
              && off . _y >= offset_t::_bottom
              && off . _y <= offset_t::_top
              && off . _z >= offset_t::_near
              && off . _z <= offset_t::_far
            ) {
            _offset = off;
            ret = true;
        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::UpdateScale ( CompositorImplementation::GLES::scale_t const & scale ) {
        valid_t ret = false;

        // Ramge check without taking into account rounding errors
        if (    scale . _horiz >= scale_t::_min
             && scale . _horiz <= scale_t::_max
             && scale . _vert >= scale_t::_min
             && scale . _vert <= scale_t::_max
        ) {
            _scale = scale;
            ret = true;
        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::UpdateOpacity ( CompositorImplementation::GLES::opacity_t const & opacity ) {
        valid_t ret = false;

        if (     opacity . _alpha >= opacity_t::_min
             && _opacity . _alpha <= opacity_t::_max
        ) {
            _opacity = opacity;
            ret = true;
        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderColor ( bool const red , bool const green , bool const blue , bool const alpha ) {
        static uint16_t degree = 0;

        constexpr decltype ( degree ) const ROTATION = 360;

        constexpr float const OMEGA = 3.14159265 / 180;

        valid_t ret = Valid ();

        if ( ret != false ) {
            // Here, for C(++) these type should be identical
            // Type information: https://www.khronos.org/opengl/wiki/OpenGL_Type
            static_assert ( std::is_same < float , GLfloat > :: value );

            GLfloat rad = static_cast < GLfloat > ( cos ( static_cast < float > ( degree ) * OMEGA ) );

            constexpr GLfloat default_color = 0.0f;

            // The function clamps the input to [0.0f, 1.0f]
            /* void */ glClearColor ( red != false ? rad : default_color , green != false ? rad : default_color , blue != false ? rad : default_color , alpha != false ? default_color : 1.0f );
            GL_ERROR ();

            /* void */ glClear ( GL_COLOR_BUFFER_BIT );
            GL_ERROR ();

            degree = ( degree + 1 ) % ROTATION;
        }

        ret =    ret
              && GL_ERROR_WITH_RETURN ();

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderEGLImage ( EGL::img_t const & img , EGLint const x , EGLint const y , EGL::width_t const width , EGL::height_t const height , EGLint const zorder , EGLint const opacity ) {
        EGLDisplay dpy = EGL::InvalidDpy ();
        EGLDisplay ctx = EGL::InvalidCtx ();

        auto DestroyTexture = [ this ] ( texture_t & tex ) -> valid_t {
            valid_t ret = tex . _tex != InvalidTex ();

            // Delete the previously created texture
            glDeleteTextures ( 1 , & ( tex . _tex ) );

            tex . _tex = InvalidTex ();

            ret =    ret
                  && GL_ERROR_WITH_RETURN ();

            return ret;
        };

        auto SetupTexture = [ this , & dpy , & ctx ] ( texture_t & tex , EGL::img_t const & img , EGL::width_t const width , EGL::height_t const height , bool const quick ) -> valid_t {
            valid_t ret = GL_ERROR_WITH_RETURN ();

            if ( quick != true ) {
                glGenTextures ( 1 , & ( tex . _tex ) );
                GL_ERROR ();
            }

            glBindTexture ( tex . _target , ( tex . _tex ) );
            GL_ERROR ();

            glTexParameteri ( tex . _target , GL_TEXTURE_WRAP_S , GL_CLAMP_TO_EDGE );
            GL_ERROR ();

            glTexParameteri ( tex . _target , GL_TEXTURE_WRAP_T , GL_CLAMP_TO_EDGE );
            GL_ERROR ();

            glTexParameteri ( tex . _target , GL_TEXTURE_MIN_FILTER , GL_LINEAR );
            GL_ERROR ();

            glTexParameteri ( tex . _target , GL_TEXTURE_MAG_FILTER , GL_LINEAR );
            GL_ERROR ();

            tex . _width = width;
            tex . _height = height;

            static_assert ( std::is_convertible < decltype ( tex . _target ) , decltype ( GL_TEXTURE_EXTERNAL_OES ) > :: value != false );
            static_assert ( std::is_convertible < decltype ( tex . _target ) , decltype ( GL_TEXTURE_2D ) > :: value != false );

            switch ( tex . _target ) {
                case GL_TEXTURE_EXTERNAL_OES :
                    {
                        // A valid GL context should exist for GLES::Supported ()

                        ret =    dpy != EGL::InvalidDpy ()
                              && ctx != EGL::InvalidCtx ();

                        static valid_t supported = (    GLES::Supported ( "GL_OES_EGL_image" )
                                                     && (    EGL::Supported ( dpy , "EGL_KHR_image" )
                                                          || EGL::Supported ( dpy, "EGL_KHR_image_base" )
                                                        )
                                                   ) != false;

                        if ( ( ret && supported ) != false ) {
                            // Take storage for the texture from the EGLImage; Pixel data becomes undefined
                            static void ( * pEGLImageTargetTexture2DOES ) ( GLenum , GLeglImageOES ) = reinterpret_cast < void ( * ) ( GLenum , GLeglImageOES ) > ( eglGetProcAddress ( "glEGLImageTargetTexture2DOES" ) );

                            // Assume nullptr represents invalid / not found
                            ret = pEGLImageTargetTexture2DOES != nullptr;

                            if ( ret != false ) {
                                // Logical const
                                using no_const_img_t = remove_const < decltype ( img ) > :: type;

                                pEGLImageTargetTexture2DOES ( tex . _target , reinterpret_cast < GLeglImageOES > ( const_cast < no_const_img_t > ( img ) ) );
                                GL_ERROR ();
                            }
                        }
                    }; break;

                case GL_TEXTURE_2D :
                    {
                        if ( quick != true ) {
                            glTexImage2D ( tex . _target , 0 , GL_RGBA , tex . _width , tex . _height , 0 , GL_RGBA , GL_UNSIGNED_BYTE , nullptr );
                            GL_ERROR ();
                        }
                        else {
                            // Clear the content
                            glTexSubImage2D ( tex . _target , 0 , 0 , 0 , tex . _width , tex . _height , GL_RGBA , GL_UNSIGNED_BYTE , nullptr );
                        }
                    }; break;

                default :
                    {
                        ret = false;
                    }
            }


            glBindTexture ( tex . _target , InvalidTex () );


            ret =    ret
                  && GL_ERROR_WITH_RETURN ();

            return ret;
        };


        silence ( DestroyTexture );

        valid_t ret =    GL_ERROR_WITH_RETURN ()
                      && img != EGL::InvalidImage ()
                      && width > 0
                      && height > 0;

        // A valid GL context should exist for GLES::Supported ()
        /* EGL::ctx_t */ ctx = eglGetCurrentContext ();
        /* EGL::dpy_t */ dpy = ctx != EGL::InvalidCtx () ? eglGetCurrentDisplay () : EGL::InvalidDpy ();


        // OES dimensions should match the underlying buffer dimensions
        // Estimate using the EGL surface

        EGLSurface surf = EGL::InvalidSurf ();

        if ( ret != false ) {
            surf = eglGetCurrentSurface ( EGL_DRAW );

            ret  =     eglGetError () == EGL_SUCCESS
                    && surf != EGL::InvalidSurf ();
        }

        EGLint e_width = 0, e_height = 0;

        if ( ret != false ) {
            ret =      eglQuerySurface ( dpy , surf , EGL_WIDTH, & e_width ) != EGL_FALSE
                    && eglQuerySurface ( dpy , surf , EGL_HEIGHT , & e_height) != EGL_FALSE
                    && eglGetError () == EGL_SUCCESS;
        }

        ret =    ret
              && eglGetError () == EGL_SUCCESS
              && ctx != EGL::InvalidCtx ();

        // Set up the required textures

        // The  'shared' texture
        static texture_t tex_oes ( GL_TEXTURE_EXTERNAL_OES , CompositorImplementation::GLES::opacity::InitialOpacity () );

        static bool reuse = false;

        // The 'scene' texture
        texture_t tex_fbo ( GL_TEXTURE_2D , CompositorImplementation::GLES::opacity::InitialOpacity () );


        if ( ret != false ) {
            // Just an arbitrarily selected texture unit
            glActiveTexture ( GL_TEXTURE0 );
            GL_ERROR ();

            // Using the EGL surface dimensions

#ifdef _RESIZE_TEXTURE
            // Did dimensions change ?

            using common_w_t = std::common_type < decltype ( tex_oes . _width ) , decltype ( e_width ) > :: type;
            using common_h_t = std::common_type < decltype ( tex_oes . _height ) , decltype ( e_height ) > :: type;

            if (    static_cast < common_w_t > ( tex_oes . _width ) != static_cast < common_w_t > ( e_width )
                 || static_cast < common_h_t > ( tex_oes . _height ) != static_cast < common_h_t > ( e_height )
               ) {

                reuse = false;

                if (    tex_oes . _width > 0
                     && tex_oes . _height > 0
                   ) {
                    reuse = DestroyTexture ( tex_oes ) != true;
                }
            }
#endif

            ret = SetupTexture ( tex_oes , img , e_width , e_height , reuse ) != false;
            reuse = ret;

            {
                std::lock_guard < decltype ( _token ) > const lock ( _token );

                auto it = _scene . find ( img );

                ret = it != _scene . end ();

                if ( ret != false ) {
                    // Found, just update values
                    tex_fbo = it -> second;

                    // Did dimensions change ?

                    using common_w_t = std::common_type < decltype ( width ) , decltype ( it -> second . _width ) > :: type;
                    using common_h_t = std::common_type < decltype ( height ) , decltype ( it -> second . _height ) > :: type;

                    if (    static_cast < common_w_t > ( width ) != static_cast < common_w_t > ( it -> second . _width)
                         || static_cast < common_h_t > ( height ) != static_cast < common_h_t > (it -> second . _height )
                       ) {

                        TRACE_WITHOUT_THIS ( Trace::Error , ( _T ( "Unsupported texture dimensions change detected!" ) ) );

//                        assert ( false );

                    }

                }
                else {
                    if (_preallocated . size () > 0 ) {
                        tex_fbo = _preallocated . top ();

                        _preallocated . pop ();

                        // Did dimensions change ?

                        using common_w_t = std::common_type < decltype ( width ) , decltype ( tex_fbo . _width ) > :: type;
                        using common_h_t = std::common_type < decltype ( height ) , decltype ( tex_fbo . _height ) > :: type;

                        if (    static_cast < common_w_t > ( width ) != static_cast < common_w_t > ( tex_fbo . _width)
                             || static_cast < common_h_t > ( height ) != static_cast < common_h_t > (tex_fbo . _height )
                           ) {

                            TRACE_WITHOUT_THIS ( Trace::Error , ( _T ( "Unsupported texture dimensions change detected!" ) ) );

                            assert ( false );

                        }

                        ret = SetupTexture ( tex_fbo , img , width , height , true ) != false;
                    }
                    else {
                        ret = SetupTexture ( tex_fbo , img , width , height , false ) != false;
                    }

                    if ( ret != false ) {
                        auto it = _scene . insert ( std::pair < EGL::img_t , texture_t > ( img , tex_fbo ) );
                        ret = it . second != false;
                    }
                }

                // Update
                if ( ret != false ) {
                    using opacity_a_t = GLES::opacity_t::alpha_t;

                    using common_opacity_t = std::common_type < decltype ( opacity ) , opacity_a_t , decltype ( WPEFramework::Exchange::IComposition::minOpacity ) , decltype ( WPEFramework::Exchange::IComposition::maxOpacity ) > :: type;

                    // Narrowing is not allowed
//                    static_assert ( narrowing < opacity_a_t , common_opacity_t , true > :: value != true );

                    tex_fbo . _opacity = ( static_cast < opacity_a_t > ( static_cast < common_opacity_t > ( opacity ) / ( static_cast < common_opacity_t > ( WPEFramework::Exchange::IComposition::maxOpacity ) - static_cast < common_opacity_t > ( WPEFramework::Exchange::IComposition::minOpacity ) ) ) );
                    tex_fbo . _x = x;
                    tex_fbo . _y = y;
                    tex_fbo . _z = zorder;

                    _scene [ img ] = tex_fbo;
                }
            }

            GLuint fbo;

            glGenFramebuffers ( 1 , & fbo );
            GL_ERROR ();

            glBindFramebuffer ( GL_FRAMEBUFFER , fbo );
            GL_ERROR ();

            glBindTexture ( tex_oes . _target , tex_oes . _tex );
            GL_ERROR ();

            glBindTexture ( tex_fbo . _target , tex_fbo . _tex );
            GL_ERROR ();


            glFramebufferTexture2D ( GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D , tex_fbo . _tex , 0 );
            GL_ERROR ();

            GLenum status = glCheckFramebufferStatus ( GL_FRAMEBUFFER );
            ret =    ret
                  && GL_ERROR_WITH_RETURN ()
                  && status == GL_FRAMEBUFFER_COMPLETE;;


            glDisable ( GL_DEPTH_TEST );
            GL_ERROR ();

            glDisable ( GL_BLEND );
            GL_ERROR ();


            ret = (    ret
                    && UpdateScale ( GLES::InitialScale () )
                    && UpdateOffset ( GLES::InitialOffset () )
                    && UpdateOpacity ( tex_fbo . _opacity )
                    && SetupViewport ( 0 , 0 , width , height )
                    && RenderTileOES ()
                  ) != false;


            glFramebufferTexture2D ( GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D , InvalidTex () , 0 );
            GL_ERROR ();

            glBindTexture ( tex_oes . _target , InvalidTex () );
            GL_ERROR ();

            glBindTexture ( tex_fbo . _target , InvalidTex () );
            GL_ERROR ();

            glDeleteFramebuffers ( 1 , & fbo );
            GL_ERROR ();
        }

        ret =    ret
              && GL_ERROR_WITH_RETURN ();


        // Do not destroy tex-fbo and _tex_oes !

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderScene ( CompositorImplementation::GLES::width_t const width , CompositorImplementation::GLES::height_t const height , std::function < CompositorImplementation::GLES::valid_t ( CompositorImplementation::GLES::texture_t const & left , CompositorImplementation::GLES::texture_t const & right ) > sortfunc ) {
        valid_t ret = GL_ERROR_WITH_RETURN ();

        if ( ret != false ) {

// TODO: very inefficient way to get z-order sorted textures
            std::list < texture_t > sorted;

            {
                std::lock_guard < decltype ( _token ) > const lock ( _token );

                for ( auto begin = _scene . begin () , it = begin , end = _scene . end () ; it != end ; it ++ ) {
                    sorted . push_back ( it -> second );
                }
            }

            sorted . sort ( sortfunc );


            glBindFramebuffer ( GL_FRAMEBUFFER , 0 );
            GL_ERROR ();

            GLenum status = glCheckFramebufferStatus ( GL_FRAMEBUFFER );

            ret =    ret
                  && GL_ERROR_WITH_RETURN ()
                  && status == GL_FRAMEBUFFER_COMPLETE;

            // Blend pixels with pixels already present in the frame buffer
            glEnable ( GL_BLEND );
            GL_ERROR ();

            glBlendEquationSeparate ( GL_FUNC_ADD , GL_FUNC_ADD );
            GL_ERROR ();

            glBlendFuncSeparate ( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA , GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );
            GL_ERROR ();


            GLint bits = 0;

            glGetIntegerv ( GL_DEPTH_BITS , & bits );

            ret =    ret
                  && GL_ERROR_WITH_RETURN ()
                  && bits > static_cast < GLint > ( 0 );

            glEnable ( GL_DEPTH_TEST );
            GL_ERROR ();

            glDepthMask ( GL_TRUE );
            GL_ERROR ();

            glDepthFunc ( GL_LESS );
            GL_ERROR ();

            // Fully utilize the depth buffer range
            glDepthRangef ( GLES::offset_t::_near , GLES::offset_t::_far );
            GL_ERROR ();

// TODO: magic number
            glClearDepthf ( 1.0f );
            GL_ERROR ();

            glClear ( GL_DEPTH_BUFFER_BIT );
            GL_ERROR ();


            // Start with an empty (solid) background
            ret = (    ret
                    && RenderColor ( false , false , false , false )
                  ) != false;

            // For all textures in map
            if ( ret != false ) {
                offset_t off ( _offset );
                scale_t scl ( _scale );
                opacity_t op ( _opacity );


                for ( auto begin = sorted . begin () , it = begin , end = sorted . end () ; it != end ; it ++ ) {
                    texture_t & tex = * it;

                    assert ( tex . _target == static_cast < tgt_t > ( GL_TEXTURE_2D ) );

                    glBindTexture ( tex . _target , tex . _tex );
                    GL_ERROR ();

                    using offset_x_t = decltype ( GLES::offset_t::_x );
                    using offset_y_t = decltype ( GLES::offset_t::_y );
                    using offset_z_t = decltype ( GLES::offset_t::_z );

                    using scale_h_t = decltype ( GLES::scale_t::_horiz );
                    using scale_v_t = decltype ( GLES::scale_t::_vert );

                    using common_scale_h_t = std::common_type < scale_h_t , GLES::x_t , decltype ( width ) > :: type;
                    using common_scale_v_t = std::common_type < scale_v_t , GLES::y_t , decltype ( height ) > :: type;


                    // Narrowing is not allowed
                    static_assert ( narrowing < scale_h_t , common_scale_h_t , true > :: value != true );
                    static_assert ( narrowing < scale_v_t , common_scale_v_t , true > :: value != true );

                    GLES::scale_t g_scale ( static_cast < GLES::scale::fraction_t > ( static_cast < common_scale_h_t > ( tex . _width ) / static_cast < common_scale_h_t > ( width ) ) , static_cast < GLES::scale::fraction_t > ( static_cast < common_scale_v_t > ( tex . _height ) / static_cast < common_scale_v_t > ( height ) ) );


                    using offset_x_t = decltype ( GLES::offset_t::_x );
                    using offset_y_t = decltype ( GLES::offset_t::_y );
                    using offset_z_t = decltype ( GLES::offset_t::_z );

                    using zorder_t = GLES::z_t;

                    using common_offset_x_t = std::common_type < scale_h_t , GLES::x_t , offset_x_t > :: type;
                    using common_offset_y_t = std::common_type < scale_v_t , GLES::y_t , offset_y_t > :: type;
                    using common_offset_z_t = std::common_type < zorder_t , decltype ( WPEFramework::Exchange::IComposition::minZOrder ) , decltype ( WPEFramework::Exchange::IComposition::maxZOrder ) , offset_z_t > :: type;


                    // Narrowing is not allowed
                    static_assert ( narrowing < offset_x_t , common_offset_x_t , true > :: value != true );
                    static_assert ( narrowing < offset_y_t , common_offset_y_t , true > :: value != true );
                    static_assert ( narrowing < offset_z_t , common_offset_z_t , true > :: value != true );


                    GLES::offset_t g_offset ( static_cast < offset_x_t > ( static_cast < common_offset_x_t > ( g_scale . _horiz ) * static_cast < common_offset_x_t > ( tex . _x ) / static_cast < common_offset_x_t > ( tex . _width ) ) , static_cast < offset_y_t > ( static_cast < common_offset_y_t > ( g_scale . _vert ) * static_cast < common_offset_y_t > ( tex . _y ) / static_cast < common_offset_y_t > ( tex . _height ) ) , static_cast < offset_z_t > ( static_cast < common_offset_z_t > ( tex . _z ) / ( static_cast < common_offset_z_t > ( WPEFramework::Exchange::IComposition::maxZOrder ) - static_cast < common_offset_z_t > ( WPEFramework::Exchange::IComposition::minZOrder ) ) ) );

                    // Width and height are screen dimensions, eg the geomtery values are in this space
                    ret = (    ret
                            && UpdateScale ( g_scale )
                            && UpdateOffset ( g_offset )
                            && UpdateOpacity (tex . _opacity )
                            && SetupViewport ( 0 , 0 , width , height )
                            && RenderTile ()
                          ) != false;

                    if ( ret != true ) {
                        break;
                    }
                }

                ret = (    ret
                        && UpdateScale ( scl )
                        && UpdateOffset ( off )
                        && UpdateOpacity ( op )
                      ) != false;
            }


            glBindTexture ( GL_TEXTURE_2D , InvalidTex () );
            GL_ERROR ();

            // Unconditionally
            glDisable ( GL_DEPTH_TEST );
            GL_ERROR ();

            glDisable ( GL_BLEND );
            GL_ERROR ();


            ret =    ret
                  && GL_ERROR_WITH_RETURN ();

        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::SkipEGLImageFromScene ( EGL::img_t const & img ) {
        valid_t ret = false;

        std::lock_guard < decltype ( _token ) > const lock ( _token );

        auto it = _scene . find ( img );

        ret = it != _scene . end ();

        if ( ret != false ) {

// TODO: Apparently it is hard to release (GPU) memory allocated so reuse it
            // Move texture to the stack for reuse
            _preallocated . push ( it -> second );

            using scene_t = decltype ( _scene );

            scene_t::size_type size = _scene . size ();

            _scene . erase ( it );

            ret = ( size - _scene . size () ) == static_cast < scene_t::size_type > ( 1 );
        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::Initialize () {
        valid_t ret = GL_ERROR_WITH_RETURN ();
        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::Deinitialize () {
        valid_t ret = GL_ERROR_WITH_RETURN ();

        glBindTexture ( GL_TEXTURE_2D , InvalidTex () );
        GL_ERROR ();

        glBindTexture ( GL_TEXTURE_EXTERNAL_OES , InvalidTex () );
        GL_ERROR ();

        std::lock_guard < decltype ( _token ) > const lock ( _token );

        for ( auto begin = _scene . begin () , it = begin , end = _scene . end () ; it != end ; it++ ) {
            tex_t & tex = it -> second . _tex;
            glDeleteTextures ( 1 , & tex );
            GL_ERROR ();
        }

        ret =    ret
              && GL_ERROR_WITH_RETURN ();

        return ret;
    }

// TODO: precompile programs at initialization stage

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::SetupProgram ( char const vtx_src [] , char const frag_src [] , prog_t & prog ) {
        auto LoadShader = [] ( GLuint const type , GLchar const code [] ) -> GLuint {
            GLuint shader = glCreateShader ( type );
            GL_ERROR ();

            static_assert ( std::is_integral < decltype ( shader ) > :: value != false );

            valid_t ret = (    GL_ERROR_WITH_RETURN ()
                            && shader != 0
                          ) != false;

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
                      ) != false;

#ifndef NDEBUG
                if ( ret != true ) {
                    GLint size = 0;

                    glGetShaderiv ( shader , GL_INFO_LOG_LENGTH , & size );
                    GL_ERROR ();

                    if ( GL_ERROR_WITH_RETURN () != false ) {
                        GLchar info [ size ];
                        GLsizei length = 0;

                        glGetShaderInfoLog ( shader , static_cast < GLsizei > ( size ) , & length , & info [ 0 ] );
                        GL_ERROR ();

                        info [ size ] = '\0';

                        TRACE_WITHOUT_THIS ( Trace::Error , _T ( "Error: shader log: %s" ) , static_cast < char * > ( & info [ 0 ] ) );
                    }
                }
#endif

                ret =    ret
                      && GL_ERROR_WITH_RETURN ();
            }

            return shader;
        };

        auto ShadersToProgram = [] ( GLuint const vertex , GLuint const fragment , prog_t & prog ) -> valid_t {
            prog = glCreateProgram ();
            GL_ERROR ();

            valid_t ret = (    GL_ERROR_WITH_RETURN ()
                            && prog != InvalidProg ()
                          ) != false;

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
                      ) != false;

#ifndef NDEBUG
                if ( ret != true ) {
                    GLint size = 0;

                    glGetProgramiv ( prog , GL_INFO_LOG_LENGTH , & size );
                    GL_ERROR ();

                    if ( GL_ERROR_WITH_RETURN () != false ) {
                        GLchar info [ size ];
                        GLsizei length = 0;

                        glGetProgramInfoLog ( prog , static_cast < GLsizei > ( size ) , & length , & info [ 0 ] );
                        GL_ERROR ();

                        info [ size ] = '\0';

                        TRACE_WITHOUT_THIS ( Trace::Error , _T ( "Error: program log: %s" ) , static_cast < char * > ( & info [ 0 ] ) );
                    }
                }
#endif

                ret =    ret
                      && GL_ERROR_WITH_RETURN ();
            }

            return ret;
        };

        auto DeleteCurrentProgram = [] () -> valid_t {
            GLuint prog = InvalidProg ();

            glGetIntegerv ( GL_CURRENT_PROGRAM , reinterpret_cast < GLint * > ( & prog ) );
            GL_ERROR ();

            valid_t ret = (    GL_ERROR_WITH_RETURN ()
                            && prog != InvalidProg ()
                          ) != false;

            if ( ret != false ) {
                GLint count = 0;

                glGetProgramiv ( prog , GL_ATTACHED_SHADERS , & count );
                GL_ERROR ();

                ret = (    GL_ERROR_WITH_RETURN ()
                        && count > 0
                      ) != false;

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

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderTileOES () {
        valid_t ret = GL_ERROR_WITH_RETURN ();

        constexpr char const vtx_src [] =
            "#version 100                                  \n"
            "attribute vec3 position;                      \n"
            "varying vec2 coordinates;                     \n"
            "void main () {                                \n"
                "gl_Position = vec4 ( position . xyz , 1); \n"
                "coordinates = position . xy;              \n"
            "}                                             \n"
        ;

        constexpr char const frag_src [] =
            "#version 100                                                                     \n"
            "#extension GL_OES_EGL_image_external : require                                   \n"
            "precision mediump float;                                                         \n"
            "uniform samplerExternalOES sampler;                                              \n"
            "uniform float opacity;                                                           \n"
            "varying vec2 coordinates;                                                        \n"
            "void main () {                                                                   \n"
                "gl_FragColor = vec4 ( texture2D ( sampler , coordinates ) . rgb , opacity ); \n"
            "}                                                                                \n"
        ;

        static_assert ( std::is_same < GLfloat , GLES::offset::coordinate_t > :: value != false );
        std::array < GLfloat const , 4 * VerticeDimensions > const vert = {
            0.0f , 0.0f , 0.0f /* v0 */,
            1.0f , 0.0f , 0.0f /* v1 */,
            0.0f , 1.0f , 0.0f /* v2 */,
            1.0f , 1.0f , 0.0f /* v3 */
        };

        if ( ret != false ) {
            glDisable ( GL_BLEND );
            GL_ERROR ();

            static prog_t prog = InvalidProg ();

            static valid_t supported = Supported ( "GL_OES_EGL_image_external" ) != false;

            ret = (    ret
                    && RenderColor ( false , false , false )
                    && supported
                    && SetupProgram ( vtx_src , frag_src , prog )
                    && RenderPolygon ( vert )
                  ) != false
            ;
        }

        return ret;
     }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderTile () {
        valid_t ret = GL_ERROR_WITH_RETURN ();

        constexpr char const vtx_src [] =
            "#version 100                                   \n"
            "attribute vec3 position;                       \n"
            "varying vec2 coordinates;                      \n"
            "void main () {                                 \n"
                "gl_Position = vec4 ( position . xyz , 1 ); \n"
                "coordinates = position . xy;               \n"
            "}                                              \n"
        ;

        constexpr char const frag_src [] =
            "#version 100                                                            \n"
            "precision mediump float;                                                \n"
            "uniform sampler2D sampler;                                              \n"
            // Required by RenderPolygon
            "uniform float opacity;                                                  \n"
            "varying vec2 coordinates;                                               \n"
            "void main () {                                                          \n"
                "gl_FragColor = vec4 ( texture2D ( sampler , coordinates ) . rgba ); \n"
            "}                                                                       \n"
        ;

        static_assert ( std::is_same < GLfloat , GLES::offset::coordinate_t > :: value != false );
        std::array < GLfloat const , 4 * VerticeDimensions > const vert = {
            0.0f , 0.0f , _offset . _z /* v0 */,
            1.0f , 0.0f , _offset . _z /* v1 */,
            0.0f , 1.0f , _offset . _z /* v2 */,
            1.0f , 1.0f , _offset . _z /* v3 */
        };


        if ( ret != false ) {
// TODO: version match
#ifdef _0
// TODO: type check GL_SHADING_LANGUAGE_VERSION
            using string_t = std::string::value_type;
            string_t const * ext = reinterpret_cast < string_t const * > ( glGetString ( GL_SHADING_LANGUAGE_VERSION ) );

            valid_t ret = GL_ERROR_WITH_RETURN ();
#endif

            static prog_t prog = InvalidProg ();

            ret = (    ret
                    && SetupProgram ( vtx_src , frag_src , prog )
                    && RenderPolygon ( vert )
                  ) != false;
        }

        return ret;
    }

    template < size_t N >
    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderPolygon ( std::array < GLfloat const , N > const & vert ) {
        GLuint prog = InvalidProg ();

        glGetIntegerv ( GL_CURRENT_PROGRAM , reinterpret_cast < GLint * > ( & prog ) );
        GL_ERROR ();

        valid_t ret = (    GL_ERROR_WITH_RETURN ()
                        && prog != InvalidProg ()
                      ) != false;

        if ( ret != false ) {
            GLint loc_vert = 0 , loc_op = 0;

            loc_op = glGetUniformLocation ( prog , "opacity" );
            GL_ERROR ();

            glUniform1f ( loc_op, _opacity . _alpha );
            GL_ERROR ();

            loc_vert = glGetAttribLocation ( prog , "position" );
            GL_ERROR ();

            glVertexAttribPointer ( loc_vert , VerticeDimensions , GL_FLOAT , GL_FALSE , 0 , vert . data () );
            GL_ERROR ();

            glEnableVertexAttribArray ( loc_vert );
            GL_ERROR ();

            glDrawArrays ( GL_TRIANGLE_STRIP , 0 , vert . size () / VerticeDimensions );
            GL_ERROR ();

            glDisableVertexAttribArray ( loc_vert );
            GL_ERROR ();

            ret =    ret
                  && GL_ERROR_WITH_RETURN ();
        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::Supported ( std::string const & name ) {
        valid_t ret = false;

        using string_t = std::string::value_type;
        using ustring_t = std::make_unsigned < string_t > :: type;

        // Identical underlying types except for signedness
        static_assert ( std::is_same < ustring_t , GLubyte > :: value != false );

        string_t const * ext = reinterpret_cast < string_t const * > ( glGetString ( GL_EXTENSIONS ) );
        GL_ERROR ();

        ret = (    GL_ERROR_WITH_RETURN () != false
                && ext != nullptr
                && name . size () > 0
                && ( std::string ( ext ) . find ( name ) != std::string::npos )
              ) != false;

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::SetupViewport ( EGLint const x , EGLint const y , EGL::width_t const width , EGL::height_t const height ) {
        silence ( x );
        silence ( y );

        valid_t ret = GL_ERROR_WITH_RETURN ();

        GLint dims [ 2 ] = { 0 , 0 };

        glGetIntegerv ( GL_MAX_VIEWPORT_DIMS , & dims [ 0 ] );
        GL_ERROR ();

#define _QUIRKS
#ifdef _QUIRKS
        // glViewport (x, y, width, height)
        //
        // Applied width = width / 2
        // Applied height = height / 2
        // Applied origin's x = width / 2 + x
        // Applied origin's y = height / 2 + y
        //
        // Compensate to origin bottom left and true size by
        // glViewport (-width, -height, width * 2, height * 2)
        //
        // _offset is in the range -1..1 wrt to origin, so the effective value maps to -width to width, -height to height


        constexpr uint8_t const mult = 2;

        using common_t = std::common_type < decltype ( width ) , decltype ( height ) , decltype ( mult ) , decltype ( _scale . _horiz ) , decltype ( _scale . _vert ) , decltype ( _offset . _x ) , decltype ( _offset . _y ) , remove_pointer < std::decay < decltype ( dims ) > :: type > :: type > :: type;

        common_t quirk_width = static_cast < common_t > ( width ) * static_cast < common_t > ( mult ) * static_cast < common_t > ( _scale . _horiz );
        common_t quirk_height = static_cast < common_t > ( height ) * static_cast < common_t > ( mult ) * static_cast < common_t > ( _scale . _vert );

        common_t quirk_x = ( static_cast < common_t > ( - width ) * static_cast < common_t > ( _scale . _horiz ) ) + ( static_cast < common_t > ( _offset . _x ) * static_cast < common_t > ( width ) );
        common_t quirk_y = ( static_cast < common_t > ( - height ) * static_cast < common_t > ( _scale . _vert ) ) + ( static_cast < common_t > ( _offset . _y ) * static_cast < common_t > ( height ) );

        if (    quirk_x < ( - quirk_width / static_cast < common_t > ( mult ) )
             || quirk_y < ( - quirk_height / static_cast < common_t > ( mult ) )
             || quirk_x  > static_cast < common_t > ( 0 )
             || quirk_y  > static_cast < common_t > ( 0 )
             || quirk_width > ( static_cast < common_t > ( width ) * static_cast < common_t > ( mult ) )
             || quirk_height > ( static_cast < common_t > ( height ) * static_cast < common_t > ( mult ) )
             || static_cast < common_t > ( width ) > static_cast < common_t > ( dims [ 0 ] )
             || static_cast < common_t > ( height ) > static_cast < common_t > ( dims [ 1 ] )
        ) {
            // Clipping, or undefined / unknown behavior
            std::cout << "Warning: possible clipping or unknown behavior detected. [" << quirk_x << ", " << quirk_y << ", " << quirk_width << ", " << quirk_height << ", " << width << ", " << height << ", " << dims [ 0 ] << ", " << dims [ 1 ] << "]" << std::endl;
        }


        // Static asserts fail due to common type
//        static_assert ( narrowing < common_t , GLint , true > :: value != true );
//        static_assert ( narrowing < common_t , GLsizei , true > :: value != true );
//      If common type typically is unsigned then checking mimm values is only allowed on unsigned
//        assert ( static_cast < common_t > ( quirk_x ) >= static_cast < common_t > ( std::numeric_limits < GLint > :: min () ) );
        assert ( static_cast < common_t > ( quirk_x ) <= static_cast < common_t > ( std::numeric_limits < GLint > :: max () ) );
//        assert ( static_cast < common_t > ( quirk_y ) >= static_cast < common_t > ( std::numeric_limits < GLint > :: min () ) );
        assert ( static_cast < common_t > ( quirk_y ) <= static_cast < common_t > ( std::numeric_limits < GLint > :: max () ) );
//        assert ( static_cast < common_t > ( quirk_width ) >= static_cast < common_t > ( std::numeric_limits < GLint > :: min () ) );
        assert ( static_cast < common_t > ( quirk_width ) <= static_cast < common_t > ( std::numeric_limits < GLint > :: max () ) );
//        assert ( static_cast < common_t > ( quirk_height ) >= static_cast < common_t > ( std::numeric_limits < GLint > :: min () ) );
        assert ( static_cast < common_t > ( quirk_height ) <= static_cast < common_t > ( std::numeric_limits < GLint > :: max () ) );

        glViewport ( static_cast < GLint > ( quirk_x ) , static_cast < GLint > ( quirk_y ) , static_cast < GLsizei > ( quirk_width ) , static_cast < GLsizei > ( quirk_height ) );
#else
        static_assert ( narrowing < decltype ( width ) , GLsizei , true > :: value != true );
        static_assert ( narrowing < decltype ( height ) , GLsizei , true > :: value != true );

        glViewport ( 0 , 0 , static_cast < GLsizei > ( width ) , static_cast < GLsizei > ( height ) );
#endif
        GL_ERROR ();

        ret = (    ret
                && GL_ERROR_WITH_RETURN ()
              ) != false;

        return ret;
    }

#undef GL_ERROR_WITH_RETURN
#undef GL_ERROR



// These ate just a copy of the previously defined macros
#define XSTRINGIFY(X) STRINGIFY(X)
#define STRINGIFY(X) #X

#ifdef EGL_VERSION_1_5
#define KHRFIX(name) name
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE
#define _EGL_NO_SYNC EGL_NO_SYNC
#define _EGL_FOREVER EGL_FOREVER
#define _EGL_NO_IMAGE EGL_NO_IMAGE
#define _EGL_NATIVE_PIXMAP EGL_NATIVE_PIXMAP_KHR
#define _EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED
#define _EGL_SYNC_STATUS EGL_SYNC_STATUS
#define _EGL_SIGNALED EGL_SIGNALED
#define _EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT
#else
#define _KHRFIX(left, right) left ## right
#define KHRFIX(name) _KHRFIX(name, KHR)
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE_KHR
#define _EGL_NO_SYNC EGL_NO_SYNC_KHR
#define _EGL_FOREVER EGL_FOREVER_KHR
#define _EGL_NO_IMAGE EGL_NO_IMAGE_KHR
#define _EGL_NATIVE_PIXMAP EGL_NATIVE_PIXMAP_KHR
#define _EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED_KHR
#define _EGL_SYNC_STATUS EGL_SYNC_STATUS_KHR
#define _EGL_SIGNALED EGL_SIGNALED_KHR
#define _EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT_KHR
#endif

    // ODR use
    /* static */ constexpr void * const CompositorImplementation::EGL::EGL_NO_CONFIG;

    CompositorImplementation::EGL::Sync::Sync ( dpy_t & dpy ) : _dpy { dpy } {
        static bool supported = EGL::Supported ( dpy , "EGL_KHR_fence_sync" ) != false;

        assert ( supported != false );
        assert ( _dpy != InvalidDpy () );

        _sync = ( supported && _dpy != InvalidDpy () ) != false ? KHRFIX ( eglCreateSync ) ( _dpy , _EGL_SYNC_FENCE , nullptr ) : InvalidSync ();
    }

    CompositorImplementation::EGL::Sync::~Sync () {
        if ( _sync == InvalidSync () ) {
            // Error creating sync object or unable to create one
            glFinish ();
        }
        else {
            // Mandatory
            glFlush ();

            // .. but still execute, when needed, an additional flush to be on the safe sidei, and avoid a dreaded  deadlock
            EGLint val = static_cast < EGLint > ( KHRFIX ( eglClientWaitSync ) ( _dpy , _sync , _EGL_SYNC_FLUSH_COMMANDS_BIT , _EGL_FOREVER ) );

            static_assert ( std::is_convertible < decltype ( EGL_FALSE ) , decltype ( val ) > :: value != false );
            static_assert ( narrowing < decltype ( EGL_FALSE ), EGLint , true > :: value != true );
            static_assert ( narrowing < decltype ( _EGL_CONDITION_SATISFIED ), EGLint , true > :: value != true );

            if (    val == static_cast < EGLint > ( EGL_FALSE )
                 || val != static_cast < EGLint > ( _EGL_CONDITION_SATISFIED )
               ) {

                EGLAttrib status;

                bool ret = KHRFIX ( eglGetSyncAttrib ) ( _dpy , _sync , _EGL_SYNC_STATUS , & status ) != EGL_FALSE;

                static_assert ( std::is_convertible < decltype ( _EGL_SIGNALED ) , decltype ( status) > :: value != false );

                ret = ret
                      && status == _EGL_SIGNALED;

                    // Assert on error
                if ( ret != true ) {
                    TRACE ( Trace::Error , ( _T ( "EGL: synchronization primitive" ) ) );
                }
            }

            /* EGLBoolean */ val = static_cast < EGLint > ( KHRFIX ( eglDestroySync ) ( _dpy , _sync ) );

            if ( val != EGL_TRUE ) {
                // Error
            }

            // Consume the (possible) error(s)
            /* EGLint */ glGetError ();
            /* EGLint */ eglGetError ();
        }
    }



    CompositorImplementation::EGL::RenderThread::RenderThread ( EGL & egl , GLES & gles )
        : Core::Thread ()
        , _sharing { Instance () }
        , _egl { egl }
        , _gles { gles } {
    }

    CompositorImplementation::EGL::RenderThread::~RenderThread () {
        Stop ();

        /* bool */ Wait ( WPEFramework::Core::Thread::STOPPED , WPEFramework::Core::infinite );
    }

    uint32_t CompositorImplementation::EGL::RenderThread::Worker () {
        static_assert ( narrowing < decltype ( Core::infinite ) , uint32_t , true > :: value != true );
        return static_cast < uint32_t > ( Core::infinite );
    }

    CompositorImplementation::EGL::RenderThread::lock_t & CompositorImplementation::EGL::RenderThread::Instance () {
        static lock_t lock;
        return lock;
    }



    CompositorImplementation::EGL::SceneRenderer::SceneRenderer ( EGL & egl , GLES & gles , CompositorImplementation & compositor )
        : RenderThread ( egl , gles )
        , _compositor { compositor } {
    }

    CompositorImplementation::EGL::SceneRenderer::~SceneRenderer () {
        Stop ();

        /* bool */ Wait ( WPEFramework::Core::Thread::STOPPED , WPEFramework::Core::infinite );
    }

    CompositorImplementation::EGL::SceneRenderer::timeout_t CompositorImplementation::EGL::SceneRenderer::Worker ()  {
        // 'Lightning speed' frame flipping
        constexpr timeout_t const ret = 0;
// TODO: frame flipping should limit the rate

        EGL::valid_t status = (    Render ()
                                && _compositor . FrameFlip ()
                              ) != false;

        if ( status != false ) {
            Block ();
        }
        else {
            Stop ();
        }

        return ret;
    }

    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::SceneRenderer::Render () {
        // No resize supported
        static Exchange::IComposition::ScreenResolution resolution = _compositor . Resolution ();

        std::lock_guard < decltype ( _sharing ) > const sharing ( _sharing );

        EGL::valid_t ret = _egl . Render ( std::bind ( & GLES::RenderScene , & _gles , static_cast < GLES::width_t > ( WidthFromResolution ( resolution ) ) , static_cast < GLES::height_t > ( HeightFromResolution ( resolution ) ) , [] ( GLES::texture_t const & left , GLES::texture_t const & right ) -> GLES::valid_t { GLES::valid_t ret = left . _z > right . _z ; return ret ; } ) , true ) != false;

        return ret;
    }

    CompositorImplementation::EGL::TextureRenderer::TextureRenderer ( EGL & egl , GLES & gles , CompositorImplementation::ClientContainer & clients ) :
        RenderThread ( egl , gles)
        , _clients { clients }
        , set   {   [ ] ( element_t const & lhs , element_t const & rhs ) -> bool {
                        bool ret = ! ( ! ( lhs . _name < rhs . _name ) && ! ( lhs . _name > rhs . _name ) );
                        return ret;
                    }
                }
    {}

    CompositorImplementation::EGL::TextureRenderer::~TextureRenderer () {
        Stop ();

        /* bool */ Wait ( WPEFramework::Core::Thread::STOPPED , WPEFramework::Core::infinite );
    }

    void CompositorImplementation::EGL::TextureRenderer::SetClientName ( std::string const & name ) {
        std::lock_guard < decltype ( _access ) > const lock ( _access );

        auto result = set . insert ( element_t ( name ) );

        if ( result . second != true ) {
            // Probably the element exist
        }
        else {
             // Add element to the queue
            queue . push ( element_t ( name ) );
        }
    }

    CompositorImplementation::EGL::TextureRenderer::timeout_t CompositorImplementation::EGL::TextureRenderer::Worker () {
        timeout_t ret = WPEFramework::Core::infinite;

        EGL::valid_t status = Render () != false;

        Block ();

        if ( status != false ) {
// TODO: do not exceed a single frame time for multiple

            std::lock_guard < decltype ( _access ) > const lock ( _access );
            if ( queue . size () > 0 ) {
                static_assert ( std::is_integral < decltype ( ret ) > :: value != false );
                ret = 0;
            }

        }
        else {
// TODO: Stop () implies no state change possblie anymore.
//            Stop ();
        }

        return ret;
    }

    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::TextureRenderer::Render () {
        EGL::valid_t ret = false;

        Core::ProxyType < ClientSurface > client;

        {
            std::lock_guard < decltype ( _access ) > const lock ( _access );

            if ( queue . size () > 0 ) {
                client = _clients . Find ( queue . front () . _name);
            }
        }


        if ( client . IsValid () != true) {
            TRACE ( Trace::Error , ( _T ( "%s does not appear to be a valid client." ) , client -> Name () ) );
        }
        else {
            ClientSurface::surf_t const & surf = client -> Surface ();

            ret = (    surf . RenderComplete ()
                    && _egl . Valid ()
                    && _gles . Valid ()
                    && client -> SyncPrimitiveStart ()
                  ) != false;

            if ( ret != false ) {
                TRACE ( Trace::Information , ( _T ( "Client has an associated EGL image." ) ) );

                using geom_t = decltype ( std::declval < WPEFramework::Exchange::IComposition::IClient > () . Geometry () );
                geom_t geom = client -> Geometry ();

                //  Geom values should not exceed buf dimensions
                auto width = gbm_bo_get_width ( surf . _buf );
                auto height = gbm_bo_get_height ( surf . _buf );


                using buf_w_t = decltype ( width );
                using buf_h_t = decltype ( height );
                using geom_w_t = decltype ( geom . width );
                using geom_h_t = decltype ( geom . height );

// TODO:
                constexpr bool const enable = true;

                if (    narrowing < buf_w_t , geom_w_t , enable > :: value != false
                     || narrowing < buf_h_t , geom_h_t , enable > :: value != false
                   ) {
                    // Narrowing, but not necessarily a problem
// TODO minimum value
                    using common_w_t = std::common_type < buf_w_t , geom_w_t > :: type;
                    assert ( static_cast < common_w_t > ( geom . width ) <= static_cast < common_w_t > ( width ) );

                    using common_h_t = std::common_type < buf_h_t , geom_h_t > :: type;
                    assert ( static_cast < common_h_t > ( geom . height ) <= static_cast < common_h_t > ( height ) );
                }
                else {
                  // No narrowing
//                assert (false);
                }


                // Opacity value should be within expected ranges
                auto opa = client -> Opacity ();

                using opacity_t = decltype ( opa );

                if ( narrowing < opacity_t , EGLint , enable > :: value != false ) {
                    // Narrowing detected

                    using common_opacity_t = std::common_type < opacity_t , EGLint , decltype ( WPEFramework::Exchange::IComposition::minOpacity ) , decltype ( WPEFramework::Exchange::IComposition::maxOpacity ) > :: type;

                    assert (    static_cast < common_opacity_t > ( opa ) >= static_cast < common_opacity_t > ( std::numeric_limits < EGLint > :: min () )
                             || static_cast < common_opacity_t > ( opa ) <= static_cast < common_opacity_t > ( std::numeric_limits < EGLint > :: max () )
                           );
                }


                auto zorder = client -> ZOrder ();

                using zorder_t = decltype ( zorder );

                if ( narrowing < zorder_t , EGLint , enable > :: value != false ) {
                    // Narrowing detected

                    using common_zorder_t = std::common_type <zorder_t, EGLint> :: type;

                    assert (    static_cast < common_zorder_t > ( zorder ) >= static_cast < common_zorder_t > ( std::numeric_limits < EGLint > :: min () )
                             || static_cast < common_zorder_t > ( zorder ) <= static_cast < common_zorder_t > ( std::numeric_limits < EGLint > :: max () )
                           );
                }


                std::lock_guard < decltype ( _sharing ) > const sharing ( _sharing );

                ret = ( _egl . RenderWithoutSwap ( std::bind ( & GLES::RenderEGLImage , & _gles , std::cref ( surf . _khr) , geom . x , geom . y , geom . width , geom . height , static_cast < EGLint > ( zorder ) , static_cast < EGLint > ( opa )) ) ) != false;

                ret =    client -> SyncPrimitiveEnd ()
                      && ret;

            }

            client . Release ();
        }

        {
            std::lock_guard < decltype ( _access ) > const lock ( _access );

            /**/ set . erase ( queue . front () );
            /* void */ queue . pop ();
        }

        return ret;
    }



    CompositorImplementation::EGL::EGL ( Natives const & natives )
        : _natives { natives }
        , _valid { Initialize () }
    {}

    CompositorImplementation::EGL::~EGL () {
        DeInitialize ();
        _valid = false;
    }

    EGL::img_t CompositorImplementation::EGL::CreateImage ( EGL const & egl , ClientSurface::surf_t const & surf ) {
        img_t ret = InvalidImage ();

        static valid_t supported = (    Supported ( egl . Display () , "EGL_KHR_image" )
                                     && Supported ( egl . Display () , "EGL_KHR_image_base" )
                                     && Supported ( egl . Display () , "EGL_EXT_image_dma_buf_import" )
                                     && Supported ( egl . Display () , "EGL_EXT_image_dma_buf_import_modifiers" )
                                   ) != false;

        if ( (    egl . Valid ()
               && supported
            ) != false ) {

            static_assert ( (    std::is_same < dpy_t , EGLDisplay > :: value
                              && std::is_same < ctx_t , EGLContext > :: value
                              && std::is_same < img_t , KHRFIX ( EGLImage ) > :: value
                            ) != false
                          );

            constexpr char methodName [] = XSTRINGIFY ( KHRFIX ( eglCreateImage ) );

            static KHRFIX ( EGLImage ) ( * peglCreateImage ) ( EGLDisplay , EGLContext , EGLenum , EGLClientBuffer , EGLAttrib const * ) = reinterpret_cast < KHRFIX ( EGLImage ) ( * ) ( EGLDisplay , EGLContext , EGLenum , EGLClientBuffer , EGLAttrib const * ) > ( eglGetProcAddress ( methodName ) );

            if ( peglCreateImage != nullptr ) {

                auto width = gbm_bo_get_width ( surf . _buf );
                auto height = gbm_bo_get_height ( surf . _buf );
                auto stride = gbm_bo_get_stride ( surf . _buf );
                auto format = gbm_bo_get_format ( surf . _buf );
                auto modifier = gbm_bo_get_modifier ( surf . _buf );

                using width_t =  decltype ( width );
                using height_t = decltype ( height );
                using stride_t = decltype ( stride );
                using format_t = decltype ( format );
                using fd_t = decltype ( surf . _fd );
                using modifier_t = decltype ( modifier );

                // Does it already exist ?
                assert ( surf . _fd != ModeSet::GBM::InvalidFd () );

                // Test our initial assumption
                assert ( format == ModeSet::SupportedBufferType () );
                assert ( modifier == ModeSet::FormatModifier () );

                // Narrowing detection

                // Enable narrowing detecttion
// TODO:
                 constexpr bool const enable = false;

                // (Almost) all will fail!
                if (    narrowing < width_t , EGLAttrib , enable > :: value != false
                     || narrowing < height_t , EGLAttrib , enable > :: value != false
                     || narrowing < stride_t , EGLAttrib , enable > :: value != false
                     || narrowing < format_t , EGLAttrib , enable > :: value != false
                     || narrowing < fd_t , EGLAttrib , enable > :: value != false
                     || narrowing < modifier_t , EGLuint64KHR , enable > :: value != false
                   ) {

                    TRACE_WITHOUT_THIS ( Trace::Information , ( _T ( "Possible narrowing detected!" ) ) );
                }

                // EGL may report differently than DRM
                ModeSet::GBM::dev_t dev = gbm_bo_get_device ( surf . _buf );
                ModeSet::GBM::fd_t fd = gbm_device_get_fd ( dev );

                // EGL may report differently than DRM
                auto list_d_for = ModeSet::AvailableFormats ( static_cast < ModeSet::DRM::fd_t > ( fd ) );
                auto it_d_for = std::find ( list_d_for . begin () , list_d_for . end () , format );

                bool valid = it_d_for != list_d_for . end ();

                // Query EGL
                static EGLBoolean ( * peglQueryDmaBufFormatsEXT ) ( EGLDisplay , EGLint , EGLint * , EGLint * ) = reinterpret_cast < EGLBoolean ( * ) ( EGLDisplay , EGLint , EGLint * , EGLint * ) > ( eglGetProcAddress ( "eglQueryDmaBufFormatsEXT" ) );
                static EGLBoolean ( * peglQueryDmaBufModifiersEXT ) ( EGLDisplay , EGLint , EGLint , EGLuint64KHR * , EGLBoolean * , EGLint * ) = reinterpret_cast < EGLBoolean ( * ) ( EGLDisplay , EGLint , EGLint , EGLuint64KHR * , EGLBoolean * , EGLint * ) > ( eglGetProcAddress ( "eglQueryDmaBufModifiersEXT" ) );

                valid =    valid
                        && peglQueryDmaBufFormatsEXT != nullptr
                        && peglQueryDmaBufModifiersEXT != nullptr
                        ;

                EGLint count = 0;

                valid =    valid
                        && peglQueryDmaBufFormatsEXT ( egl . Display () , 0 , nullptr , & count ) != EGL_FALSE
                        ;

                valid = valid
                        && peglQueryDmaBufFormatsEXT ( egl . Display () , 0 , nullptr , & count ) != EGL_FALSE
                        ;

                EGLint formats [ count ];

                valid = valid
                        && peglQueryDmaBufFormatsEXT ( egl . Display () , count , & formats [ 0 ] , & count ) != EGL_FALSE
                        ;

                // format should be listed as supported
                if ( valid != false ) {
                    std::list < EGLint > list_e_for ( & formats [ 0 ] , & formats [ count ] );

                    auto it_e_for = std::find ( list_e_for . begin () , list_e_for . end () , format);

                    valid = it_e_for != list_e_for.end ();
                }

                valid =    valid
                        && peglQueryDmaBufModifiersEXT ( egl . Display () , format , 0 , nullptr , nullptr , & count ) != EGL_FALSE
                        ;

                EGLuint64KHR modifiers [ count ];
                EGLBoolean external [ count ];

                // External is required to exclusive use withGL_TEXTURE_EXTERNAL_OES
                valid =    valid
                        && peglQueryDmaBufModifiersEXT ( egl . Display () , format , count , & modifiers [ 0 ] , & external [ 0 ] , & count ) != FALSE
                        ;

                // _modifier should be listed as supported, and _external should be true
                if ( valid != false ) {
                    std::list < EGLuint64KHR > list_e_mod ( & modifiers [ 0 ] , & modifiers [ count ] );

                    auto it_e_mod = std::find ( list_e_mod . begin () , list_e_mod . end () , static_cast < EGLuint64KHR > ( modifier ) );

                    valid = it_e_mod != list_e_mod . end ();
                }

                static_assert ( std::is_integral < EGLAttrib > :: value != false );
                static_assert ( std::is_signed < EGLAttrib > :: value != false );

                static_assert ( in_signed_range < EGLAttrib , EGL_WIDTH > :: value != false );
                static_assert ( in_signed_range < EGLAttrib , EGL_HEIGHT > :: value != false );
                static_assert ( in_signed_range < EGLAttrib , EGL_LINUX_DRM_FOURCC_EXT > :: value != false );
                static_assert ( in_signed_range < EGLAttrib , EGL_DMA_BUF_PLANE0_FD_EXT > :: value != false );
                static_assert ( in_signed_range < EGLAttrib , EGL_DMA_BUF_PLANE0_OFFSET_EXT > :: value != false );
                static_assert ( in_signed_range < EGLAttrib , EGL_DMA_BUF_PLANE0_PITCH_EXT > :: value != false );
                static_assert ( in_signed_range < EGLAttrib , EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT > :: value != false );
                static_assert ( in_signed_range < EGLAttrib , EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT > :: value != false );
                static_assert ( in_signed_range < EGLAttrib , EGL_IMAGE_PRESERVED_KHR > :: value != false );
                static_assert ( in_signed_range < EGLAttrib , EGL_NONE > :: value != false );

                static_assert ( sizeof ( EGLuint64KHR ) == static_cast < size_t > ( 8 ) );

                if ( valid != false ) {
                    EGLAttrib const _attrs [] = {
                        EGL_WIDTH , static_cast < EGLAttrib > ( width ) ,
                        EGL_HEIGHT , static_cast < EGLAttrib > ( height ) ,
                        EGL_LINUX_DRM_FOURCC_EXT , static_cast < EGLAttrib > ( format ) ,
                        EGL_DMA_BUF_PLANE0_FD_EXT , static_cast < EGLAttrib > ( surf . _fd ) ,
// TODO: magic constant
                        EGL_DMA_BUF_PLANE0_OFFSET_EXT , static_cast < EGLAttrib > ( 0 ) ,
                        EGL_DMA_BUF_PLANE0_PITCH_EXT , static_cast < EGLAttrib > ( stride ) ,
                        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT , static_cast < EGLAttrib > ( static_cast < EGLuint64KHR > ( modifier ) & 0xFFFFFFFF ) ,
                        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT , static_cast < EGLAttrib > ( static_cast < EGLuint64KHR > ( modifier ) >> 32 ) ,
                        EGL_IMAGE_PRESERVED_KHR, static_cast < EGLAttrib > ( EGL_TRUE ) ,
                        EGL_NONE
                    };

                    static_assert ( std::is_convertible < decltype ( surf . _buf ) , EGLClientBuffer > :: value != false );
                    ret = peglCreateImage ( egl . Display () , EGL_NO_CONTEXT , EGL_LINUX_DMA_BUF_EXT , nullptr , _attrs );

                }
            }
            else {
                // Error
                TRACE_WITHOUT_THIS ( Trace::Error , _T ( "%s is unavailable or invalid parameters." ) , methodName );
            }
        }
        else {
            TRACE_WITHOUT_THIS ( Trace::Error , _T ( "EGL is not properly initialized." ) );
        }

        return ret;
    }

    EGL::img_t CompositorImplementation::EGL::DestroyImage (EGL const & egl , ClientSurface::surf_t const & surf ) {
        img_t ret = surf . _khr;

        static valid_t supported = (    Supported ( egl . Display () , "EGL_KHR_image" )
                                     && Supported ( egl . Display () , "EGL_KHR_image_base" )
                                   ) != false;

        if ( (    egl . Valid ()
               && supported
             ) != false
           ) {

            static_assert ( (    std::is_same < dpy_t , EGLDisplay > :: value
                              && std::is_same < ctx_t , EGLContext > :: value
                              && std::is_same < img_t , KHRFIX ( EGLImage ) > :: value
                            ) != false
                          );

            constexpr char const methodName [] = XSTRINGIFY ( KHRFIX ( eglDestroyImage ) );

            static EGLBoolean ( * peglDestroyImage ) ( EGLDisplay , KHRFIX ( EGLImage ) ) = reinterpret_cast < EGLBoolean ( * ) ( EGLDisplay , KHRFIX ( EGLImage ) ) > ( eglGetProcAddress ( KHRFIX ( "eglDestroyImage" ) ) );

            if (    peglDestroyImage != nullptr
                 && surf . RenderComplete () != false
               ) {

                ret = peglDestroyImage ( egl . Display () , surf . _khr ) != EGL_FALSE ? EGL::InvalidImage () : ret;
            }
            else {
                // Error
                TRACE_WITHOUT_THIS ( Trace::Error , _T ( "%s is unavailable or invalid parameters are provided." ) , methodName );
            }
        }
        else {
            TRACE_WITHOUT_THIS ( Trace::Error , _T ( "EGL is not properly initialized." ) );
        }

        return ret;
    }

    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Invalidate () {
        valid_t ret = true;

        DeInitialize ();

        return ret;
    }

    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Initialize () {
        valid_t ret = _natives . Valid ();

        if ( ret != false ) {
            if ( _dpy != InvalidDpy () ) {
                ret = false;

                if ( eglTerminate ( _dpy ) != EGL_FALSE ) {
                    ret = true;
                }
            }
        }

        if ( ret != false ) {
             using native_dpy_no_const = remove_const < Natives::dpy_t > :: type;

            // Again. EGL does use const sparsely
            _dpy = eglGetDisplay ( const_cast < native_dpy_no_const > ( _natives . Display () ) );
            ret = _dpy != InvalidDpy ();
        }

        if ( ret != false ) {
            EGLint major = 0, minor = 0;
            ret = eglInitialize ( _dpy , & major , & minor ) != EGL_FALSE;

            // Just take the easy approach (for now)
            static_assert ( ( narrowing < decltype ( major ) , int , true > :: value ) != true );
            static_assert ( ( narrowing < decltype ( minor ) , int , true > :: value ) != true );
            TRACE ( Trace::Information , ( _T ( "EGL version : %d.%d" ) , static_cast < int > ( major ) , static_cast < int > ( minor ) ) );
        }

        if ( ret != false ) {
            static_assert ( GLES::MajorVersion () == static_cast < GLES::version_t > ( 2 ) );

            static_assert ( in_signed_range < EGLint , EGL_WINDOW_BIT > :: value != false );
            static_assert ( in_signed_range < EGLint , EGL_OPENGL_ES2_BIT > :: value != false );

            static_assert ( in_signed_range < EGLint , EGL_SURFACE_TYPE > :: value != false );
            static_assert ( in_signed_range < EGLint , EGL_RED_SIZE > :: value != false );
            static_assert ( in_signed_range < EGLint , EGL_GREEN_SIZE > :: value != false );
            static_assert ( in_signed_range < EGLint , EGL_BLUE_SIZE > :: value != false );
            static_assert ( in_signed_range < EGLint , EGL_ALPHA_SIZE > :: value != false );
            static_assert ( in_signed_range < EGLint , EGL_DEPTH_SIZE > :: value != false );

            static_assert ( in_signed_range < EGLint , DepthBufferSize () > :: value != false );
            static_assert ( in_signed_range < EGLint , RedBufferSize () + GreenBufferSize () + BlueBufferSize () + DepthBufferSize () > :: value != false );

            constexpr EGLint const attr [] = {
                EGL_SURFACE_TYPE, static_cast < EGLint > ( EGL_WINDOW_BIT ) ,
                EGL_RED_SIZE    , RedBufferSize () ,
                EGL_GREEN_SIZE  , GreenBufferSize () ,
                EGL_BLUE_SIZE   , BlueBufferSize () ,
                EGL_ALPHA_SIZE  , AlphaBufferSize () ,
                EGL_BUFFER_SIZE , RedBufferSize () + GreenBufferSize () + BlueBufferSize () + AlphaBufferSize () ,
                EGL_RENDERABLE_TYPE , static_cast < EGLint > ( EGL_OPENGL_ES2_BIT ) ,
                EGL_DEPTH_SIZE  , DepthBufferSize () ,
                EGL_NONE
            };

            EGLint count = 0;

            if ( eglGetConfigs ( _dpy , nullptr , 0 , & count ) != EGL_TRUE) {
                count = 1;
            }


            std::vector < EGLConfig > confs ( count , EGL_NO_CONFIG );

            /* EGLBoolean */ eglChooseConfig ( _dpy , & attr [ 0 ] , confs . data () , confs . size () , & count );

            _conf = confs [ 0 ];

            ret = _conf != EGL_NO_CONFIG;
        }


        if ( ret != false ) {
            EGLenum api = eglQueryAPI ();

            ret = api == EGL_OPENGL_ES_API;

            if ( ret != true ) {
                /* EGLBoolean */ eglBindAPI ( EGL_OPENGL_ES_API );
                ret = eglGetError () == EGL_SUCCESS;
            }
        }

        static_assert ( in_signed_range < EGLint , GLES::MajorVersion () > :: value != false );

        if ( ret != false ) {
            // Possibly unsigned to signed
            static_assert ( narrowing < GLES::version_t , intmax_t , true > :: value != true );
            static_assert ( in_signed_range < EGLint , GLES::MajorVersion () > :: value != false );

            constexpr EGLint const attr [] = {
                EGL_CONTEXT_CLIENT_VERSION , static_cast < EGLint > ( static_cast < GLES::version_t > ( GLES::MajorVersion () ) ) ,
                EGL_NONE
            };

            _ctx = eglCreateContext ( _dpy , _conf , EGL_NO_CONTEXT , attr );
            ret = _ctx != EGL_NO_CONTEXT;
        }

        if ( ret != false ) {
             constexpr EGLint const attr [] = {
                EGL_NONE
            };

            _surf = eglCreateWindowSurface ( _dpy , _conf , _natives . Surface () , & attr [ 0 ] );
            ret = _surf != EGL::InvalidSurf ();
        }

        if ( ret != true ) {
            DeInitialize ();
        }

        return ret;
    }

    void CompositorImplementation::EGL::DeInitialize () {
        valid_t ret =    _valid != false
                      && eglMakeCurrent ( _dpy , EGL::InvalidSurf () , EGL::InvalidSurf () , EGL::InvalidCtx () ) != EGL_FALSE
                      && eglGetError () == EGL_SUCCESS
                      && eglDestroySurface ( _dpy , _surf ) != FALSE
                      && eglGetError () == EGL_SUCCESS
                      && eglDestroyContext ( _dpy , _ctx ) != EGL_FALSE
                      && eglGetError () == EGL_SUCCESS
                      && eglTerminate ( _dpy ) != EGL_FALSE
                      && eglGetError () == EGL_SUCCESS;

        _valid = false;

        silence ( ret );
    }

    // Although compile / build time may succeed, runtime checks are also mandatory
    bool CompositorImplementation::EGL::Supported ( dpy_t const dpy , std::string const & name ) {
        bool ret = false;

        static_assert ( ( std::is_same < dpy_t , EGLDisplay > :: value ) != false);

#ifdef EGL_VERSION_1_5
        // KHR extentions that have become part of the standard

        // Sync capability
        ret =    name . find ( "EGL_KHR_fence_sync" ) != std::string::npos
              /* CreateImage / DestroyImage */
              || name . find ( "EGL_KHR_image" ) != std::string::npos
              || name . find ( "EGL_KHR_image_base" ) != std::string::npos;
#endif

        if ( ret != true ) {
            static_assert ( std::is_same < std::string::value_type , char > :: value != false );
            char const * ext = eglQueryString ( dpy, EGL_EXTENSIONS );

            ret =    ext != nullptr
                  && name . size () > 0
                  && (    std::string ( ext ) . find (name)
                       != std::string::npos
                     )
                  ;
        }

        return ret;
    }

    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render ( CompositorImplementation::GLES & gles ) {
        silence ( gles );

        // Ensure the client API is set per thread basis
        valid_t ret =    Valid () != false
                      && eglMakeCurrent ( _dpy , _surf , _surf , _ctx ) != EGL_FALSE
                      && eglBindAPI ( EGL_OPENGL_ES_API ) != EGL_FALSE
                      ;

        if ( ret != false ) {
            ret = eglSwapBuffers ( _dpy , _surf ) != EGL_FALSE;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ) ; }

             // Avoid any memory leak if the local thread is stopped (by another thread)
            ret =    eglMakeCurrent ( _dpy , EGL::InvalidSurf () , EGL::InvalidSurf () , EGL::InvalidCtx () ) != EGL_FALSE
                  && ret
                  ;
        }

        if ( ret != true ) {
            TRACE ( Trace::Error , ( _T ( "Failed to complete rendering content." ) ) );
        }

        return ret;
    }

    template < typename FUNC , typename... ARG >
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render ( std::false_type const , FUNC && func , bool const post , ARG &&... arg ) {
        // Ensure the client API is set per thread basis
        valid_t ret =    Valid () != false
                      && eglMakeCurrent ( _dpy , _surf , _surf , _ctx ) != EGL_FALSE
                      && eglBindAPI ( EGL_OPENGL_ES_API ) != EGL_FALSE
                      ;

        if ( ret != false ) {
            if ( post != false ) {
                ret = func ( std::forward < ARG > ( arg )... ) != false;

                { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ) ; }

                ret =    ret
                      && eglSwapBuffers ( _dpy , _surf ) != EGL_FALSE
                      ;
            }
            else {
                ret =    eglSwapBuffers ( _dpy , _surf ) != EGL_FALSE
                      && func ( std::forward < ARG > ( arg )... ) != false
                      ;
            }

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ) ; }

            // Expensive, but it avoids any memory leak if the local thread is stopped (by another thread)
            ret =    eglMakeCurrent ( _dpy , EGL::InvalidSurf () , EGL::InvalidSurf () , EGL::InvalidCtx () ) != EGL_FALSE
                  && ret
                  ;
        }

        if ( ret != true ) {
            TRACE ( Trace::Error , ( _T ( "Failed to complete rendering content." ) ) );
        }

        return ret;
    }

    template < typename FUNC , typename ARG0 , typename... ARG >
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render ( std::true_type const , FUNC && func , bool const post , ARG0 && arg0 , ARG &&... arg ) {
        // Ensure the client API is set per thread basis
        valid_t ret = Valid () != false
                      && eglMakeCurrent ( _dpy , _surf , _surf , _ctx ) != EGL_FALSE
                      && eglBindAPI ( EGL_OPENGL_ES_API ) != EGL_FALSE
                      ;

        if ( ret != false ) {
            if ( post != false ) {
                ret = ( std::forward < ARG0 > ( arg0 ) .* func ) ( std::forward < ARG > ( arg )... ) != false;

                { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ); }

                ret =    ret
                      && eglSwapBuffers ( _dpy , _surf ) != EGL_FALSE
                      ;
            }
            else {
                ret =    eglSwapBuffers ( _dpy , _surf ) != EGL_FALSE
                      && ( std::forward < ARG0 > ( arg0 ) .* func ) ( std::forward < ARG > ( arg )... ) != false
                      ;
            }

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ) ; }

            // Expensive, but it avoids any memory leak if the local thread is stopped (by another thread)
            ret =    eglMakeCurrent ( _dpy , EGL::InvalidSurf () , EGL::InvalidSurf () , EGL::InvalidCtx () ) != EGL_FALSE
                  && ret
                  ;
        }

        if ( ret != true ) {
            TRACE ( Trace::Error , ( _T ( "Failed to complete rendering content." ) ) );
        }

        return ret;
    }

    template < typename FUNC, typename... ARG >
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render ( FUNC && func , bool const post , ARG &&... arg ) {
        valid_t ret = Render ( typename std::is_member_pointer < FUNC > :: type () , func , post , std::forward < ARG > ( arg )... );
        return ret;
    }

    template < typename FUNC , typename... ARG >
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render ( std::false_type const , FUNC && prefunc , FUNC && postfunc , ARG &&... arg ) {
        // Ensure the client API is set per thread basis
        valid_t ret = Valid () != false
                       && eglMakeCurrent ( _dpy , _surf , _surf , _ctx ) != EGL_FALSE
                       && eglBindAPI ( EGL_OPENGL_ES_API ) != EGL_FALSE
                       ;

        if ( ret != false ) {
            ret = prefunc ( std::forward < ARG > ( arg )... ) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ) ; }

            ret =    ret
                  && eglSwapBuffers ( _dpy, _surf ) != EGL_FALSE
                  && postfunc ( std::forward < ARG > ( arg )... ) != false
                  ;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ) ; }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret =    eglMakeCurrent ( _dpy , EGL::InvalidSurf () , EGL::InvalidSurf () , EGL::InvalidCtx () ) != EGL_FALSE
                  && ret
                  ;
        }

        if ( ret != true ) {
            TRACE ( Trace::Error , ( _T ( "Failed to complete rendering content." ) ) );
        }

        return ret;
    }

    template < typename FUNC , typename ARG0 , typename... ARG >
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render ( std::true_type const , FUNC && prefunc , FUNC && postfunc , ARG0 && arg0 , ARG &&... arg ) {
        // Ensure the client API is set per thread basis
        valid_t ret =    Valid () != false
                      && eglMakeCurrent ( _dpy , _surf , _surf , _ctx ) != EGL_FALSE
                      && eglBindAPI ( EGL_OPENGL_ES_API ) != EGL_FALSE
                      ;

        if ( ret != false ) {
            ret =  ( std::forward < ARG0 > ( arg0 ) .* prefunc ) ( std::forward < ARG > ( arg )... ) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ) ; }

            ret =    ret
                  && eglSwapBuffers ( _dpy, _surf ) != EGL_FALSE
                  && ( std::forward < ARG0 > ( arg0 ) .* postfunc ) ( std::forward < ARG > ( arg )... ) != false
                  ;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ) ; }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret =    eglMakeCurrent ( _dpy , EGL::InvalidSurf () , EGL::InvalidSurf () , EGL::InvalidCtx () ) != EGL_FALSE
                  && ret
                  ;
        }

        if ( ret != true ) {
            TRACE ( Trace::Error , ( _T ( "Failed to complete rendering content." ) ) );
        }

        return ret;
    }

    template < typename FUNC, typename... ARG >
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render ( FUNC && prefunc , FUNC && postfunc , ARG &&... arg ) {
        valid_t ret = Render ( typename std::is_member_pointer < FUNC > :: type () , prefunc , postfunc, std::forward < ARG > ( arg )... );
        return ret;
    }

    template < typename FUNC , typename... ARG >
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::RenderWithoutSwap ( std::false_type const , FUNC && func , ARG &&... arg ) {
        // Ensure the client API is set per thread basis
        CompositorImplementation::EGL::valid_t ret =    Valid () != false
                                                     && eglMakeCurrent ( _dpy , _surf , _surf , _ctx ) != EGL_FALSE
                                                     && eglBindAPI ( EGL_OPENGL_ES_API ) != EGL_FALSE
                                                     ;

        if ( ret != false ) {
            ret = func ( std::forward < ARG > ( arg )... ) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ) ; }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret =    eglMakeCurrent ( _dpy , EGL::InvalidSurf (), EGL::InvalidSurf () , EGL::InvalidCtx () ) != EGL_FALSE
                  && ret
                  ;
        }

        if ( ret != true ) {
            TRACE ( Trace::Error , ( _T ( "Failed to complete rendering content." ) ) );
        }

        return ret;
    }

    template < typename FUNC , typename ARG0 , typename... ARG >
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::RenderWithoutSwap ( std::true_type const , FUNC && func, ARG0 && arg0 , ARG &&... arg ) {
        // Ensure the client API is set per thread basis
        CompositorImplementation::EGL::valid_t ret =    Valid () != false
                                                     && eglMakeCurrent ( _dpy , _surf , _surf , _ctx ) != EGL_FALSE
                                                     && eglBindAPI ( EGL_OPENGL_ES_API ) != EGL_FALSE
                                                     ;

        if ( ret != false ) {
            ret = ( std::forward < ARG0 > ( arg0 ) .* func ) ( std::forward < ARG > ( arg )... ) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            { WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync ( _dpy ) ; }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret =    eglMakeCurrent ( _dpy , EGL::InvalidSurf () , EGL::InvalidSurf () , EGL::InvalidCtx () ) != EGL_FALSE
                  && ret
                  ;
        }

        if ( ret != true ) {
            TRACE ( Trace::Error , ( _T ( "Failed to complete rendering content." ) ) );
        }

        return ret;
    }

    template < typename FUNC , typename... ARG >
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::RenderWithoutSwap ( FUNC && func , ARG &&... arg ) {
        valid_t ret = RenderWithoutSwap ( typename std::is_member_pointer < FUNC > :: type () , func, std::forward < ARG > ( arg )... );
        return ret;
    }

#undef STRINGIFY
#ifdef _KHRFIX
#undef _KHRFIX
#endif
#undef KHRFIX
#undef _EGL_SYNC_FENCE
#undef _EGL_NO_SYNC
#undef _EGL_FOREVER
#undef _EGL_NO_IMAGE
#undef _EGL_NATIVE_PIXMAP
#undef _EGL_CONDITION_SATISFIED
#undef _EGL_SYNC_STATUS
#undef _EGL_SIGNALED
#undef _EGL_SYNC_FLUSH_COMMANDS_BIT



    CompositorImplementation::Config::Config ()
        : Core::JSON::Container ()
        , _connector ( _T ( "/tmp/compositor" ) )
        , _port ( _T ( "HDMI0" ) ) {

        Add ( _T ( "connector" ) , & _connector );
        Add ( _T ( "port" ) , & _port );
    }

    CompositorImplementation::Config::~Config () {
    }

    void CompositorImplementation::PlatformReady () {
        PluginHost::ISubSystem* subSystems ( _service -> SubSystems () );

        ASSERT ( subSystems != nullptr );

        if ( subSystems != nullptr ) {
            subSystems -> Set ( PluginHost::ISubSystem::PLATFORM , nullptr );
            subSystems -> Set ( PluginHost::ISubSystem::GRAPHICS , nullptr );

            subSystems -> Release ();
        }
    }

    CompositorImplementation::CompositorImplementation ()
        : _adminLock ()
        , _service ( nullptr )
        , _engine ()
        , _externalAccess ( nullptr )
        , _observers ()
        , _clients ()
        , _platform ()
        , _dma { nullptr }
        , _natives ( _platform )
        , _egl ( _natives )
        , _clientLock ()
        , _textureRenderer ( _egl , _gles , _clients )
        , _sceneRenderer ( _egl , _gles , * this ) {
    }

    CompositorImplementation::~CompositorImplementation () {
        _textureRenderer . Stop ();

        /* bool */ _textureRenderer . Wait ( WPEFramework::Core::Thread::STOPPED , WPEFramework::Core::infinite );

        _sceneRenderer . Stop ();

        /* bool */ _sceneRenderer . Wait ( WPEFramework::Core::Thread::STOPPED , WPEFramework::Core::infinite );

        /* valid_t ret = */ _egl . Invalidate ();

        /* valid_t ret = */ _natives . Invalidate ();

        if ( _dma != nullptr ) {
            delete _dma;
        }

        _clients . Clear ();

        _dma = nullptr;

        if ( _externalAccess != nullptr ) {
            delete _externalAccess;
            _engine . Release();
        }

        _service -> Release ();

        for ( auto & observer : _observers) {
           observer -> Release ();
        }
    }

    bool CompositorImplementation::FDFor ( std::string const & name , CompositorImplementation::DMATransfer::fds_t & fds ) {
        std::string prop;

        bool ret = FDFor ( name , fds , prop );

        return ret;
    }

    bool CompositorImplementation::FDFor ( std::string const & name , CompositorImplementation::DMATransfer::fds_t & fds , std::string & properties ) {
        std::lock_guard < decltype ( _clientLock ) > const lock ( _clientLock );

        bool ret = false;

        TRACE ( Trace::Information , ( _T ( "%s requested a DMA transfer." ) , name . c_str () ) );

        /* ProxyType <> */ auto client = _clients . Find ( name );

        if (    client . IsValid () != true
             || name . compare ( client -> Name () ) != 0
           ) {

               TRACE ( Trace::Error , ( _T ( "%s does not appear to be a valid client." ) , name . length () > 0 ? name . c_str () : "'<no name provided>'") );
        }
        else {
            if ( _dma != nullptr ) {
                using fd_t = DMATransfer::fds_t::value_type;
                using class_t = decltype ( client );
                using return_t = decltype ( std::declval < class_t > () . operator-> () -> Native () );

                static_assert ( ( std::is_convertible < return_t , fd_t > :: value ) != false );

                fds [ 0 ] = static_cast < fd_t > ( client -> Native () );
                fds [ 1 ] = static_cast < fd_t > ( client -> SyncPrimitive () );

                ret =    fds  [ 0 ] != DMATransfer::InvalidFD ()
                      && fds [ 1 ] != DMATransfer::InvalidFD ();
            }
        }

        ClientSurface::surf_t surf;

        if ( ret != false ) {
            surf = client -> Surface ( EGL::CreateImage ( _egl , client -> Surface () ) );

            ret = ( surf . Valid () != false );
        }

        if ( ret != false ) {
            properties . clear ();

            auto width = gbm_bo_get_width ( surf . _buf );
            auto height = gbm_bo_get_height ( surf . _buf );
            auto stride = gbm_bo_get_stride ( surf . _buf );
            auto format = gbm_bo_get_format ( surf . _buf );
            auto modifier = gbm_bo_get_modifier ( surf . _buf );

            constexpr char spacer [] = ":" ;

            properties =   std::string ( spacer )
                         + std::to_string ( width ) . append ( spacer )
                         + std::to_string ( height ) . append ( spacer )
                         + std::to_string ( stride ) . append ( spacer )
                         + std::to_string ( format ) . append ( spacer )
                         + std::to_string ( modifier )
                         ;
        }

        client . Release ();

        return ret;
    }

    bool CompositorImplementation::CompositeFor ( std::string const & name ) {
        bool ret = true;

        // One client at a time
        std::lock_guard < decltype ( _clientLock ) > const lock ( _clientLock );

        /* void */ _textureRenderer . SetClientName ( name );

        // Skip the request to create a frame texture if the rate is too high to be processed
        if ( _textureRenderer . IsRunning () != true ) {
            /* void */ _textureRenderer . Run ();
        }

        return ret;
    }

     bool CompositorImplementation::FrameFlip () {

        using milli_t = int64_t;

        auto RefreshRateFromResolution = [] ( Exchange::IComposition::ScreenResolution const resolution ) -> milli_t {
            // Assume 'unknown' rate equals 60 Hz
            milli_t rate = 60;

            switch ( resolution ) {
                case Exchange::IComposition::ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
                                                                            rate = 24; break;
                case Exchange::IComposition::ScreenResolution_720p50Hz  : // 1280x720 progressive @ 50 Hz
                case Exchange::IComposition::ScreenResolution_1080i50Hz : // 1920x1080 interlaced @ 50 Hz
                case Exchange::IComposition::ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
                case Exchange::IComposition::ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
                                                                            rate = 50; break;
                case Exchange::IComposition::ScreenResolution_2160p30Hz : // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
                                                                            rate = 30; break;
                case Exchange::IComposition::ScreenResolution_480i      : // 720x480
                case Exchange::IComposition::ScreenResolution_480p      : // 720x480 progressive
                case Exchange::IComposition::ScreenResolution_720p      : // 1280x720 progressive
                case Exchange::IComposition::ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
                case Exchange::IComposition::ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
                case Exchange::IComposition::ScreenResolution_Unknown   :   rate = 60;
            }

            return rate;
        };

        constexpr milli_t milli = 1000;

        // No resize supported
        static decltype ( milli ) rate = RefreshRateFromResolution ( Resolution () );

        static std::chrono::milliseconds delay ( milli / rate );

        static std::chrono::milliseconds duration ( delay );

        static auto start = std::chrono::steady_clock::now ();


        ModeSet::BufferInfo bufferInfo ( _natives . Surface () , ModeSet::GBM::InvalidBuf () , ModeSet::DRM::InvalidFb () );
        /* void */ _platform . Swap ( bufferInfo );


        auto finish = std::chrono::steady_clock::now ();

        duration = std::chrono::duration_cast < std::chrono::milliseconds > ( finish - start );

        // Do not overstress the system
// TODO: allow delta
        if ( duration . count () < delay . count ()) {
            SleepMs ( delay . count () - duration . count () );
        }

        start = finish;


        bool ret = true;

        return ret;
    }

    uint32_t CompositorImplementation::WidthFromResolution ( ScreenResolution const resolution ) {
        // Asumme an invalid width equals 0
        uint32_t width = 0;

        switch ( resolution ) {
            case ScreenResolution_480p      : // 720x480
                                                width = 720; break;
            case ScreenResolution_720p      : // 1280x720 progressive
            case ScreenResolution_720p50Hz  : // 1280x720 @ 50 Hz
                                                width = 1280; break;
            case ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
            case ScreenResolution_1080i50Hz : // 1920x1080 interlaced  @ 50 Hz
            case ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
            case ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
                                                width = 1920; break;
            case ScreenResolution_2160p30Hz : // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
            case ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
            case ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
                                                width = 3840; break;
            case ScreenResolution_480i      : // Unknown according to the standards (?)
            case ScreenResolution_Unknown   :
            default                         :   width = 0;
        }

        return width;
    }

    uint32_t CompositorImplementation::HeightFromResolution ( ScreenResolution const resolution ) {
        // Asumme an invalid height equals 0
        uint32_t height = 0;

         switch ( resolution ) {
            case ScreenResolution_480i      :
            case ScreenResolution_480p      : height = 480; break;
            case ScreenResolution_720p      :
            case ScreenResolution_720p50Hz  : height = 720; break;
            case ScreenResolution_1080p24Hz :
            case ScreenResolution_1080i50Hz :
            case ScreenResolution_1080p50Hz :
            case ScreenResolution_1080p60Hz : height = 1080; break;
            case ScreenResolution_2160p30Hz : // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
            case ScreenResolution_2160p50Hz :
            case ScreenResolution_2160p60Hz : height = 2160; break;
            case ScreenResolution_Unknown   :
            default                         : height = 0;
        }

        return height;
    }

    Exchange::IComposition::ScreenResolution CompositorImplementation::ResolutionFromHeightWidth ( uint32_t const height , uint32_t const width ) {
        silence ( height );
        silence ( width );

        // Given the options, the refresh rate is also important so the only sensible value is 'unknown'
        return Exchange::IComposition::ScreenResolution_Unknown;
    }

    uint32_t CompositorImplementation::Configure ( PluginHost::IShell * service ) {
        static_assert ( std::is_enum < decltype ( Core::ERROR_NONE ) > :: value != false );
        static_assert ( narrowing < std::underlying_type < decltype ( Core::ERROR_NONE ) > :: type , uint32_t , true > :: value != true );

        uint32_t result = Core::ERROR_NONE;

        _service = service;

        _service -> AddRef ();

        string configuration ( service -> ConfigLine () );

        Config config;
        config . FromString ( service -> ConfigLine () );

        _engine = Core::ProxyType < RPC::InvokeServer > :: Create ( & Core::IWorkerPool::Instance () );

        _externalAccess = new ExternalAccess ( * this , Core::NodeId ( config . _connector . Value () . c_str () ) , service -> ProxyStubPath () , _engine );

        if ( _externalAccess -> IsListening () == true ) {
            _port = config . _port . Value ();
            PlatformReady ();
        } else {
            delete _externalAccess;
            _externalAccess = nullptr;

            _engine . Release ();

            TRACE ( Trace::Error , ( _T ( "Could not report PlatformReady as there was a problem starting the Compositor RPC %s" ) , _T ( "server" ) ) );

            static_assert ( std::is_enum < decltype ( Core::ERROR_OPENING_FAILED ) > :: value != false );
            static_assert ( narrowing < std::underlying_type < decltype ( Core::ERROR_OPENING_FAILED ) > :: type , uint32_t , true > :: value != true );

            result = Core::ERROR_OPENING_FAILED;
        }

        return result;
    }

    void CompositorImplementation::Register ( Exchange::IComposition::INotification * notification ) {
        _adminLock . Lock ();

        ASSERT ( std::find ( _observers . begin () , _observers . end () , notification ) == _observers . end () );

        notification -> AddRef ();

        _observers . push_back ( notification );

        _clients . Visit (
            [ = ] ( string const & name , Core::ProxyType < ClientSurface > const & element ) { notification -> Attached ( name , & ( * element ) ) ; }
        );

        _adminLock . Unlock ();
    }

    void CompositorImplementation::Unregister ( Exchange::IComposition::INotification * notification ) {
        _adminLock . Lock ();

        std::list < Exchange::IComposition::INotification * > :: iterator index ( std::find ( _observers . begin () , _observers . end () , notification ) );

        ASSERT ( index != _observers . end () );

        if ( index != _observers . end () ) {

            _clients . Visit (
                [ = ] ( string const & name , Core::ProxyType < ClientSurface > const & element ) { silence ( element ); notification -> Detached ( name ) ;  }
            );

            _observers . erase ( index );

            notification -> Release ();
        }

        _adminLock . Unlock ();
    }

    void CompositorImplementation::Attached ( string const & name , IClient * client ) {
        _adminLock . Lock ();

        for ( auto & observer : _observers) {
            observer -> Attached ( name , client );
        }

        _adminLock . Unlock ();
    }

    void CompositorImplementation::Detached ( string const & name ) {
        _adminLock . Lock ();

        for ( auto & observer : _observers ) {
            observer -> Detached ( name );
        }

        _adminLock . Unlock ();
    }

    Core::instance_id CompositorImplementation::Native () const {
        using class_t = std::remove_reference < decltype ( _natives ) > :: type;
        using return_t = decltype ( std::declval < class_t > () . Display () );

        // The EGL API uses const very sparsely, and here, a const is also not expected
        using return_no_const_t = remove_const < return_t > :: type;

        static_assert ( ( std::is_convertible < return_no_const_t , EGLNativeDisplayType > :: value ) != false ) ;
        static_assert ( ( std::is_convertible < decltype ( EGL_DEFAULT_DISPLAY ) , EGLNativeDisplayType > :: value ) != false );

        EGLNativeDisplayType result ( static_cast < EGLNativeDisplayType > ( EGL_DEFAULT_DISPLAY ) );

        if ( _natives . Valid () != false ) {
            // Just remove the unexpected const if it exist
            result = static_cast < EGLNativeDisplayType > ( const_cast < return_no_const_t > ( _natives.Display () ) );
        }
        else {
            TRACE ( Trace::Error , ( _T ( "The native display (id) might be invalid / unsupported. Using the EGL default display instead!" ) ) );
        }

        return reinterpret_cast < Core::instance_id > ( result );
    }

    string CompositorImplementation::Port () const {
        return ( _port );
    }

    WPEFramework::Exchange::IComposition::IClient * CompositorImplementation::CreateClient ( string const & name , uint32_t const width , uint32_t const height ) {
        IClient * client = nullptr;

        Core::ProxyType < ClientSurface > object = _clients . Instance < ClientSurface > ( name , _platform , * this , name , width , height );

        ASSERT ( object . IsValid () == true );

        if ( object . IsValid () == true ) {
            client = & ( * object );

            Attached ( name , client );

            client -> AddRef ();
        }

        if ( client == nullptr ) {
            TRACE ( Trace::Error , ( _T ( "Unable to create the ClientSurface with name %s" ) , name . c_str () ) );
        }
        else {
            if ( _dma == nullptr ) {
                _dma = new DMATransfer ( * this );
            }

            if (    _dma == nullptr
                 || _dma -> Valid () != true
               ) {

                TRACE ( Trace::Error , ( _T ( "DMA transfers are not supported." ) ) );

                delete _dma;
                _dma = nullptr;
            }
            else {
                if ( _dma -> IsRunning () != true ) {
                    /* void */ _dma -> Run ();
                }

                if ( _sceneRenderer . IsRunning () != true ) {
                    /* void */ _sceneRenderer . Run ();
                }
            }
        }

        return client;
    }

    void CompositorImplementation::InvalidateClient ( string const & name ) {

        Core::ProxyType < ClientSurface > object = _clients . Find ( name );

        if ( object . IsValid () != false ) {

            Detached ( object -> Name () );

            ClientSurface::surf_t const & surf = object ->  Surface ();

            _gles . SkipEGLImageFromScene ( surf . _khr );

// TODO: Const object contains invalid member
           /*  surf . _khr = */ EGL::DestroyImage ( _egl , surf );
        }
    }

    Exchange::IComposition::ScreenResolution CompositorImplementation::Resolution () const {
        Exchange::IComposition::ScreenResolution resolution = WPEFramework::Exchange::IComposition::ScreenResolution_Unknown;

        decltype ( std::declval < ModeSet > () . Width () ) width = _platform . Width ();
        silence ( width );

        decltype ( std::declval < ModeSet > () . Height () ) height = _platform . Height ();

// TODO: This might not be the whole story to determine progressive versus interlaced

        decltype ( std::declval < ModeSet > () . RefreshRate () ) vrefresh = _platform . RefreshRate ();
        decltype ( std::declval < ModeSet > () . Interlaced () ) interlaced = _platform . Interlaced ();

        static_assert ( std::is_integral < decltype ( height ) > :: value != false );

        if ( interlaced != true ) {
            switch ( height ) {
                case 480    :   {
                                    resolution = ScreenResolution_480p;
                                    break;
                                }
                case 720    :   {
                                    resolution = vrefresh != 50 ? ScreenResolution_720p : ScreenResolution_720p50Hz;
                                    break;
                                }
                case 1080   :   {
                                    switch ( vrefresh ) {
                                        case 24 : resolution = ScreenResolution_1080p24Hz; break;
                                        case 50 : resolution = ScreenResolution_1080p50Hz; break;
                                        case 60 : resolution = ScreenResolution_1080p60Hz; break;
                                        default : resolution = ScreenResolution_Unknown;
                                    }
                                    break;
                                }
                case 2160   :   {
                                    switch ( vrefresh ) {
                                        case 30 : resolution = ScreenResolution_2160p30Hz; break; // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
                                        case 50 : resolution = ScreenResolution_2160p50Hz; break;
                                        case 60 : resolution = ScreenResolution_2160p60Hz; break;
                                        default : resolution = ScreenResolution_Unknown;
                                    }
                                    break;
                                }
                default     :   {
                                    resolution = ScreenResolution_Unknown;
                                }
            }
        }
        else {
            switch ( height ) {
                case 480    :   {
                                    resolution = ScreenResolution_480i;
                                    break;
                                }
                case 1080   :   {
                                    resolution = vrefresh != 50 ? ScreenResolution_Unknown : ScreenResolution_1080i50Hz;
                                    break;
                                }
                default     :   {
                                    resolution = ScreenResolution_Unknown;
                                }
            }
         }

        return resolution;
     }

    uint32_t CompositorImplementation::Resolution ( Exchange::IComposition::ScreenResolution const format ) {
        TRACE ( Trace::Error , ( _T ( "Could not set screenresolution to %s. Not supported for Raspberry Pi compositor" ) , Core::EnumerateType < Exchange::IComposition::ScreenResolution > ( format ) . Data () ) );
        return Core::ERROR_UNAVAILABLE;
    }

    SERVICE_REGISTRATION ( CompositorImplementation , 1 , 0 );

} // namespace Plugin
} // namespace WPEFramework
