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

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <sys/socket.h>

#ifdef __cplusplus
}
#endif

#include <mutex>
#include <type_traits>
#include <functional>

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
struct _remove_const <T const *> {
    typedef T * type;
};

template <class FROM, class TO, bool ENABLE>
struct _narrowing {
    static_assert (( std::is_arithmetic < FROM > :: value && std::is_arithmetic < TO > :: value ) != false);

    // Not complete, assume zero is minimum for unsigned
    // Common type of signed and unsigned typically is unsigned
    using common_t = typename std::common_type < FROM, TO > :: type;
    static constexpr bool value =   ENABLE
                                    && (
                                        ( std::is_signed < FROM > :: value && std::is_unsigned < TO > :: value )
                                        || static_cast < common_t > ( std::numeric_limits < FROM >::max () ) >= static_cast < common_t > ( std::numeric_limits < TO >::max () )
                                    )
                                    ;
};

// Suppress compiler warnings of unused (parameters)
// Omitting the name is sufficient but a search on this keyword provides easy access to the location
template <typename T>
void silence (T &&) {}

// Show logging in blue to make it distinct from TRACE
#define TRACE_WITHOUT_THIS(level, ...) do {             \
    fprintf (stderr, "\033[1;34m");                     \
    fprintf (stderr, "[%s:%d] : ", __FILE__, __LINE__); \
    fprintf (stderr, ##__VA_ARGS__);                    \
    fprintf (stderr, "\n");                             \
    fprintf (stderr, "\033[0m");                        \
    fflush (stderr);                                    \
} while (0)


MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {

// TODO: EGL and GLES (type and) extentions support

class CompositorImplementation;

    // Some compilers might struggle with identical names for class and namespace, but for now it simplifies a lot
    namespace EGL {
        using img_t = EGLImageKHR;
        using width_t = EGLint;
        using height_t = EGLint;

        static constexpr img_t InvalidImage () { return EGL_NO_IMAGE_KHR; }
    }

    class ClientSurface : public Exchange::IComposition::IClient, Exchange::IComposition::IRender {
    private:

        // The buffer acts as a surface for the remote site
        struct {
            struct gbm_bo * _buf;
            int _fd;
            EGL::img_t _khr;

            bool Valid () const { return _buf != nullptr; }
            bool DMAComplete () const { return Valid () && _fd > -1; };
            bool RenderComplete () const { return Valid () && _fd > -1 && _khr != WPEFramework::Plugin::EGL::InvalidImage (); }
        } _nativeSurface;

    public:

        using surf_t = decltype (_nativeSurface);

    public:
        ClientSurface() = delete;
        ClientSurface(const ClientSurface&) = delete;
        ClientSurface& operator= (const ClientSurface&) = delete;

        ClientSurface(ModeSet& modeSet, CompositorImplementation& compositor, const string& name, const uint32_t width, const uint32_t height);
        ~ClientSurface() override;

    public:
        RPC::instance_id Native () const override {
            // Sharing this handle does not imply its contents can be accessed!

            static_assert ((std::is_convertible < decltype (_nativeSurface._fd), RPC::instance_id > :: value) != false);
            static_assert (sizeof (decltype (_nativeSurface._fd) ) <= sizeof (RPC::instance_id));

            return static_cast < RPC::instance_id > ( _nativeSurface._fd );
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

        surf_t const & Surface ( EGL::img_t const & khr = EGL::InvalidImage () ) {
            if (khr != EGL::InvalidImage ()) {
                _nativeSurface._khr = khr;
            }

            return _nativeSurface;
        }

        BEGIN_INTERFACE_MAP (ClientSurface)
            INTERFACE_ENTRY (Exchange::IComposition::IClient)
            INTERFACE_ENTRY (Exchange::IComposition::IRender)
        END_INTERFACE_MAP

    private:
        ModeSet& _modeSet;
        CompositorImplementation& _compositor; 
        const std::string _name;

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

        class DMATransfer : public Core::Thread {

            private :

                CompositorImplementation & _compositor;

                // Waiting for connection requests
                int _listen;
                // Actual socket for communication
                int _transfer;

                struct sockaddr_un const _addr;

                bool _valid;

                using valid_t = decltype (_valid);

            public :

                DMATransfer () = delete;
                DMATransfer (CompositorImplementation & compositor) : Core::Thread (/*0, _T ("")*/), _compositor (compositor), _listen { -1 }, _transfer { -1 }, _addr { AF_UNIX, "/tmp/Compositor/DMA" }, _valid { _Initialize () } {}
                ~DMATransfer () {
                    Stop ();
                    /* valid_t */ _Deinitialize ();
                }

                DMATransfer (DMATransfer const &) = delete;
                DMATransfer & operator = (DMATransfer const &) = delete;

            private :

                using timeout_t = _remove_const < decltype (Core::infinite) > :: type;

            public :

                timeout_t Worker () override {
                    // Never call 'us' again, delay the next call an infinite amount of time if the state is not 'stopped'
                    timeout_t _ret = Core::infinite;

                    if (IsRunning () != false) {

                        // Blocking
                        _transfer = accept (_listen, nullptr, nullptr);

                        // Do some processing on the clients

                        std::string _msg;
                        int _fd = -1;

                        if (Receive (_msg, _fd) && _compositor.FDFor (_msg, _fd) && Send (_msg, _fd) != false) {
                            // Just wait for the remote peer to close the connection
                            ssize_t _size = read (_transfer, nullptr, 0);

                            decltype (errno) _err = errno;

                            switch (_size) {
                                case -1 :   // Error
                                            TRACE (Trace::Error, (_T ("Error after DMA transfer : %d."), _err));
                                            break;
                                case 0  :   // remote has closed the connection
                                            TRACE (Trace::Information, (_T ("Remote has closed the DMA connection.")));
                                            break;
                                default :   // Unexpected data available
                                            TRACE (Trace::Error, (_T ("Unexpected data read after DMA transfer.")));
                            }

                            /* int */ close (_transfer);

                            _transfer = -1;
                        }
                        else {
                            TRACE (Trace::Error, (_T ("Failed to exchange DMA information for %s."),  _msg.length () > 0 ? _msg.c_str () : "'<no name provided>'"));
                        }
                    }

                    return _ret;
                }

                valid_t Valid () const { return _valid; }

                // Receive file descriptor with additional message
                valid_t Receive (std::string & msg, int & fd) {
                    valid_t _ret = Valid () && Connect (Core::infinite);

                    if (_ret != true) {
                        TRACE (Trace::Information, (_T ("Unable to receive (DMA) data.")));
                    }
                    else {
                        _ret = _Receive (msg, fd);
                        _ret = Disconnect (Core::infinite) && _ret;
                    }

                    return _ret;
                }

                // Send file descriptor with additional message
                valid_t Send (std::string const & msg, int fd) {
                    valid_t _ret = Valid () && Connect (Core::infinite);

                    if (_ret != true) {
                        TRACE (Trace::Information, (_T ("Unable to send (DMA) data.")));
                    }
                    else {
                        _ret = _Send (msg, fd) && Disconnect (Core::infinite);
                        _ret = Disconnect (Core::infinite) && _ret;
                    }

                    return _ret;
                }

            private :

                valid_t _Initialize () {
                    valid_t _ret = false;

                    // Just a precaution
                    /* int */ unlink (_addr.sun_path);

                    _listen = socket (_addr.sun_family /* domain */, SOCK_STREAM /* type */, 0 /* protocol */);
                    _ret = _listen != -1;

                    if (_ret != false) {
                        _ret = bind (_listen, reinterpret_cast < struct sockaddr const * > (&_addr), sizeof (_addr)) == 0;
                    }

                    if (_ret != false) {
// TODO: Derive it from maximum number of supported clients
                        // Number of pending requests for accept to handle
                        constexpr int queue_size = 1;
                        _ret = listen (_listen, queue_size) == 0;
                    }

                    return _ret;
                }

                valid_t _Deinitialize () {
                    valid_t _ret = false;

                    _ret = close (_listen) == 0 && close (_transfer) == 0;

                    // Delete the (bind) socket in the file system if no reference exist (anymore)
                    _ret = unlink (_addr.sun_path) == 0 && _ret;

                    return _ret;
                }

                valid_t Connect (timeout_t timeout) {
                    silence (timeout);

                    valid_t _ret = false;

                    decltype (errno) _err = errno;

                    _ret = _transfer > -1 && _err == 0;

                    return _ret;
                }

                valid_t Disconnect (timeout_t timeout) {
                    silence (timeout);

                    valid_t _ret = false;

                    decltype (errno) _err = errno;

                    _ret = _transfer > -1 && _err == 0;

                    return _ret;
                }

                valid_t _Send (std::string const & msg, int fd) {
                    valid_t _ret = false;

                    // Logical const
                    static_assert ((std::is_same <char *, _remove_const  < decltype ( & msg [0] ) > :: type >:: value) != false);
                    char * _buf  = const_cast <char *> ( & msg [0] );

                    size_t const _bufsize = msg.size ();

                    if (_bufsize > 0) {

                        // Scatter array for vector I/O
                        struct iovec _iov;

                        // Starting address
                        _iov.iov_base = reinterpret_cast <void *> (_buf);
                        // Number of bytes to transfer
                        _iov.iov_len = _bufsize;

                        // Actual message
                        struct msghdr _msgh {};

                        // Optional address
                        _msgh.msg_name = nullptr;
                        // Size of address
                        _msgh.msg_namelen = 0;
                        // Scatter array
                        _msgh.msg_iov = &_iov;
                        // Elements in msg_iov
                        _msgh.msg_iovlen = 1;

                        // Ancillary data
                        // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
                        char _control [CMSG_SPACE (sizeof ( decltype ( fd) ))];

                        if (fd > -1) {
                            // Contruct ancillary data to be added to the transfer via the control message

                            // Ancillary data, pointer
                            _msgh.msg_control = _control;

                            // Ancillary data buffer length
                            _msgh.msg_controllen = sizeof ( _control );

                            // Ancillary data should be access via cmsg macros
                            // https://linux.die.net/man/2/recvmsg
                            // https://linux.die.net/man/3/cmsg
                            // https://linux.die.net/man/2/setsockopt
                            // https://www.man7.org/linux/man-pages/man7/unix.7.html

                            // Control message

                            // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                            struct cmsghdr * _cmsgh = CMSG_FIRSTHDR ( &_msgh );

                            if (_cmsgh != nullptr) {
                                // Originating protocol
                                // To manipulate options at the sockets API level
                                _cmsgh->cmsg_level = SOL_SOCKET;

                                // Protocol specific type
                                // Option at the API level, send or receive a set of open file descriptors from another process
                                _cmsgh->cmsg_type = SCM_RIGHTS;

                                // The value to store in the cmsg_len member of the cmsghdr structure, taking into account any necessary alignmen, eg byte count of control message including header
                                _cmsgh->cmsg_len = CMSG_LEN (sizeof ( decltype ( fd ) ));

                                // Initialize the payload
                                // Pointer to the data portion of a cmsghdr, ie unsigned char []
                                * reinterpret_cast < decltype (fd) * > ( CMSG_DATA ( _cmsgh ) ) = fd;

                                _ret = true;
                            }
                            else {
                                // Error
                            }
                        }
                        else {
                            // No extra payload, ie  file descriptor(s), to include
                            _msgh.msg_control = nullptr;
                            _msgh.msg_controllen = 0;

                            _ret = true;
                        }

                        if (_ret != false) {
                            // Configuration succeeded

                            // https://linux.die.net/man/2/sendmsg
                            // https://linux.die.net/man/2/write
                            // Zero flags is equivalent to write

                            ssize_t _size = -1;
                            socklen_t _len = sizeof (_size);

                            // Only send data if the buffer is large enough to contain all data
                            if (getsockopt (_transfer, SOL_SOCKET, SO_SNDBUF, &_size, &_len) == 0) {
                                // Most options use type int, ssize_t was just a placeholder
                                static_assert (sizeof (int) <= sizeof (ssize_t));
                                TRACE (Trace::Information, (_T ("The sending buffer capacity equals %d bytes."), _size));

// TODO: do not send if the sending buffer is too small
                                _size = sendmsg (_transfer, &_msgh, 0);
                            }
                            else {
                                // Unable to determine buffer capacity
                            }

                            _ret = _size != -1;

                            if (_ret != false) {
                                // Ancillary data is not included
                                TRACE (Trace::Information, (_T ("Send %d bytes out of %d."), _size, _bufsize));
                            }
                            else {
                                TRACE (Trace::Error, (_T ("Failed to send data.")));
                            }
                        }

                    }
                    else {
                        TRACE (Trace::Error, (_T ("A data message to be send cannot be empty!")));
                    }

                    return _ret;
                }

                valid_t _Receive (std::string & msg, int & fd) {
                    bool _ret = false;

                    msg.clear ();
                    fd = -1;

                    ssize_t _size = -1;
                    socklen_t _len = sizeof (_size);

                    if (getsockopt (_transfer, SOL_SOCKET, SO_RCVBUF, &_size, &_len) == 0) {
                        TRACE (Trace::Information, (_T ("The receiving buffer maximum capacity equals %d bytes."), _size));

                        // Most options use type int, ssize_t was just a placeholder
                        static_assert (sizeof (int) <= sizeof (ssize_t));
                        msg.reserve (static_cast < int > (_size));
                    }
                    else {
                        TRACE (Trace::Information, (_T ("Unable to determine buffer maximum cpacity. Using %d bytes instead."), msg.capacity ()));
                    }

                    size_t const _bufsize = msg.capacity ();

                    if (_bufsize > 0) {
                        using fd_t = std::remove_reference < decltype (fd) > :: type;

                        static_assert ((std::is_same <char *, _remove_const  < decltype ( & msg [0] ) > :: type >:: value) != false);
                        char _buf [_bufsize];

                        // Scatter array for vector I/O
                        struct iovec _iov;

                        // Starting address
                        _iov.iov_base = reinterpret_cast <void *> (&_buf [0]);
                        // Number of bytes to transfer
                        _iov.iov_len = _bufsize;

                        // Actual message
                        struct msghdr _msgh {};

                        // Optional address
                        _msgh.msg_name = nullptr;
                        // Size of address
                        _msgh.msg_namelen = 0;
                        // Elements in msg_iov
                        _msgh.msg_iovlen = 1;
                        // Scatter array
                        _msgh.msg_iov = &_iov;

                        // Ancillary data
                        // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
                        char _control [CMSG_SPACE (sizeof ( fd_t ))];

                        // Ancillary data, pointer
                        _msgh.msg_control = _control;

                        // Ancillery data buffer length
                        _msgh.msg_controllen = sizeof (_control);

// TODO: do not receive if the receiving buffer is too small
                        // No flags set
                        _size = recvmsg (_transfer, &_msgh, 0);

                        _ret = _size > 0;

                        switch (_size) {
                            case -1 :   // Error
                                        {
                                           TRACE (Trace::Error, (_T ("Error receiving remote (DMA) data.")));
                                           break;
                                        }
                            case 0  :   // Peer has done an orderly shutdown, before any data was received
                                        {
                                            TRACE (Trace::Error, (_T ("Error receiving remote (DMA) data. Compositorclient may have become unavailable.")));
                                            break;
                                        }
                            default :   // Data
                                        {
                                            // Extract the file descriptor information
                                            TRACE (Trace::Information, (_T ("Received %d bytes."), _size));

                                            // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                                            // Assume a single cmsgh was sent
                                            struct cmsghdr * _cmsgh = CMSG_FIRSTHDR ( &_msgh );

                                            // Check for the expected properties the client should have set
                                            if (_cmsgh != nullptr
                                                && _cmsgh->cmsg_level == SOL_SOCKET
                                                && _cmsgh->cmsg_type == SCM_RIGHTS) {

                                                // The macro returns a pointer to the data portion of a cmsghdr.
                                                fd = * reinterpret_cast < fd_t * > ( CMSG_DATA ( _cmsgh ) );
                                            }
                                            else {
                                                TRACE (Trace::Information, (_T ("No (valid) ancillary data received.")));
                                            }

                                            msg.assign (_buf, _size);
                                        }
                        }
                    }
                    else {
                        // Error
                        TRACE (Trace::Error, (_T ("A receiving data buffer (message) cannot be empty!")));
                    }

                    return _ret;
                }
        };

        class Natives {
            private :

                ModeSet & _set;

                using dpy_return_t = decltype ( std::declval < ModeSet > ().UnderlyingHandle () );
                using surf_return_t = decltype ( std::declval < ModeSet > ().CreateRenderTarget (0, 0) );

                static_assert (std::is_pointer < surf_return_t > :: value != false);
                surf_return_t _surf = nullptr;

                bool _valid = false;

            public :

                using dpy_t = dpy_return_t;
                using surf_t = decltype (_surf);
                using valid_t = decltype (_valid);

                Natives () = delete;
                Natives (ModeSet & set) : _set { set }, _valid { Initialize () } {}
                ~Natives () {
                    _valid = false;
                    DeInitialize ();
                }

                dpy_t Display () const { return _set.UnderlyingHandle (); }
                surf_t Surface () const { return _surf; }

                valid_t Valid () const { return _valid; }

            private :

                valid_t Initialize () {
                    valid_t _ret = false;

                    // The argument to Open is unused, an empty string suffices
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

                // x, y, z
                static constexpr uint8_t VerticeDimensions = 3;

                uint16_t _degree = 0;

                GLenum const _tgt;
                GLuint _tex;

                // Each coorrdinate in the range [-1.0f, 1.0f]
                struct offset {
                    using coordinate_t =  float;

                    coordinate_t _x;
                    coordinate_t _y;
                    coordinate_t _z;

                    offset () : offset (0.0f, 0.0f, 0.0f) {}
                    offset (coordinate_t x, coordinate_t y, coordinate_t z) : _x {x}, _y {y}, _z {z} {}
                } _offset;

                bool _valid;

            public :

                using tgt_t = decltype (_tgt);
                using tex_t = decltype (_tex);
                using offset_t = decltype (_offset);

                using valid_t = decltype (_valid);

                GLES () : _tgt { GL_TEXTURE_EXTERNAL_OES }, _tex { InvalidTex () }, _offset { InitialOffset () }, _valid { Initialize () } {}
                ~GLES () {
                    _valid = false;
                    /* valid_t */ Deinitialize ();
                }

                static constexpr tex_t InvalidTex () { return 0; }

                valid_t Valid () const { return _valid; }

                valid_t Render () {
                    bool _ret = Valid ();
                    return _ret;
                }

                valid_t RenderColor () {
                    constexpr decltype (_degree) const ROTATION = 360;

                    constexpr float const OMEGA = 3.14159265 / 180;

                    valid_t  _ret = Valid ();

                    if (_ret != false) {
                        // Here, for C(++) these type should be identical
                        // Type information: https://www.khronos.org/opengl/wiki/OpenGL_Type
                        static_assert (std::is_same <float, GLfloat>::value);

                        GLfloat _rad = static_cast <GLfloat> (cos (_degree * OMEGA));

                        // The function clamps the input to [0.0f, 1.0f]
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
                    }

                    return _ret;
                }

                // Values used at render stages of different objects
                static offset InitialOffset () { return offset (0.0f, 0.0f, 0.0f); }

                valid_t UpdateOffset (struct offset const & off) {
                    valid_t _ret = false;

                    // Range check without taking into account rounding errors
                    if ( ((off._x - -0.5f) * (off._x - 0.5f) <= 0.0f)
                        && ((off._y - -0.5f) * (off._y - 0.5f) <= 0.0f)
                        && ((off._z - -0.5f) * (off._z - 0.5f) <= 0.0f) ){

                        _offset = off;

                        _ret = true;
                    }

                    return _ret;
                }

                valid_t RenderEGLImage (EGL::img_t const & img, EGL::width_t width, EGL::height_t height) {
                    valid_t _ret = glGetError () == GL_NO_ERROR && img != EGL::InvalidImage () && width > 0 && height > 0;

                    if (_ret != false) {
                        // Just an arbitrarily selected texture unit
                        glActiveTexture (GL_TEXTURE0);
                        _ret = glGetError () == GL_NO_ERROR;
                    }

                    if (_ret != false) {
                        // Delete the previously created texture
                        if (_tex != InvalidTex ()) {
                            glDeleteTextures (1, &_tex);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        _tex = InvalidTex ();

                        if (_ret != false) {
                            glGenTextures (1, &_tex);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glBindTexture (_tgt, _tex);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glTexParameteri (_tgt, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glTexParameteri (_tgt, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glTexParameteri (_tgt, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glTexParameteri (_tgt, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        // Requires EGL 1.2 and either the EGL_OES_image or EGL_OES_image_base
                        // Use eglGetProcAddress, or dlsym for the function pointer of this GL extenstion
                        // https://www.khronos.org/registry/OpenGL/extensions/OES/OES_EGL_image_external.txt
                        static void (* _EGLImageTargetTexture2DOES) (GLenum, GLeglImageOES) = reinterpret_cast < void (*) (GLenum, GLeglImageOES) > (eglGetProcAddress ("glEGLImageTargetTexture2DOES"));

                        if (_ret != false && _EGLImageTargetTexture2DOES != nullptr) {
                            // Logical const
                            using no_const_img_t = _remove_const < decltype (img) > :: type;
                            _EGLImageTargetTexture2DOES (_tgt, reinterpret_cast <GLeglImageOES> (const_cast < no_const_img_t  >(img)));
                            _ret = glGetError () == GL_NO_ERROR;
                        }
                        else {
                            _ret = false;
                        }

                        if (_ret != false) {
                            // Image to full size

                            using width_t = decltype (width);
                            using height_t = decltype (height);

                            // Narrowing detection

                            // Enable narrowing detection
// TODO:
                            constexpr bool _enable = false;

                            if ( ( _narrowing < width_t, GLsizei, _enable > ::value != true
                                && _narrowing < height_t, GLsizei, _enable > ::value ) != true) {
                                TRACE (Trace::Information, (_T ("Possible narrowing detected!")));
                            }

                            glViewport (0, 0, width, height);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            _ret = RenderTile ();
                        }

                    }

                    return _ret;
                }

            private :

// TODO:  Add extention support
                valid_t Initialize () {
                    valid_t _ret = false;
                    _ret = true;
                    return _ret;
                }

                valid_t Deinitialize () {
                    valid_t _ret = false;

                    if (_tex != InvalidTex ()) {
                        glDeleteTextures (1, &_tex);
                        _ret = glGetError () == GL_NO_ERROR;
                    }

                    return _ret;
                }

// TODO: precompile programs at initialization stage

                valid_t SetupProgram (/* some identifier for a precompiled program */) {
                    auto LoadShader = [] (GLuint type, GLchar const code []) -> GLuint {
                        valid_t _ret = glGetError () == GL_NO_ERROR;

                        GLuint _shader = 0;
                        if (_ret != false) {
                            _shader = glCreateShader (type);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false && _shader != 0) {
                            glShaderSource (_shader, 1, &code, nullptr);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glCompileShader (_shader);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        return _shader;
                    };

                    auto ShadersToProgram = [] (GLuint vertex, GLuint fragment) -> bool {
                        valid_t _ret = glGetError () == GL_NO_ERROR;

                        GLuint _prog = 0;

                        if (_ret != false) {
                            glGetIntegerv (GL_CURRENT_PROGRAM, reinterpret_cast <GLint *> (&_prog));
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false && _prog != 0) {
                            glDeleteProgram (_prog);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        _prog = 0;

                        if (_ret != false) {
                            _prog = glCreateProgram ();
                            _ret = _prog != 0;
                        }

                        if (_ret != false) {
                            glAttachShader (_prog, vertex);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glAttachShader (_prog, fragment);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glBindAttribLocation (_prog, 0, "position");
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glLinkProgram (_prog);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glUseProgram (_prog);
                            _ret = glGetError () == GL_NO_ERROR;
                        }
                        else {
                            glDeleteProgram (_prog);
                            _ret = glGetError () == GL_NO_ERROR;

                            glDeleteShader (vertex);
                            _ret = glGetError () == GL_NO_ERROR && _ret;

                            glDeleteShader (fragment);
                            _ret = glGetError () == GL_NO_ERROR && _ret;
                        }

                        return _ret;
                    };


                    bool _ret = glGetError () == GL_NO_ERROR;

                    constexpr char const _vtx_src [] =
                        "#version 100                               \n"
                        "attribute vec3 position;                   \n"
                        "varying vec2 coordinates;                  \n"
                        "void main () {                             \n"
                            "gl_Position = vec4 (position.xyz, 1);  \n"
                            "coordinates = position.xy;             \n"
                        "}                                          \n"
                    ;

                    constexpr char  const _frag_src [] =
                        "#version 100                                                           \n"
                        "#extension GL_OES_EGL_image_external : require                         \n"
                        "precision mediump float;                                               \n"
                        "uniform samplerExternalOES sampler;                                    \n"
                        "varying vec2 coordinates;                                              \n"
                        "void main () {                                                         \n"
                            "gl_FragColor = vec4 (texture2D (sampler, coordinates).rgb, 1.0f);  \n"
                        "}                                                                      \n"
                    ;

                    GLuint _vtxShader = LoadShader (GL_VERTEX_SHADER, _vtx_src);
                    GLuint _fragShader = LoadShader (GL_FRAGMENT_SHADER, _frag_src);

// TODO: inefficient on every call, reuse compiled program
                    _ret = ShadersToProgram(_vtxShader, _fragShader);

                    // Blend pixels with pixels already present in the frame buffer

                    if (_ret != false) {
                         glEnable (GL_BLEND);
                        _ret = glGetError () == GL_NO_ERROR;
                    }

                    if (_ret != false) {
                        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                        _ret = glGetError () == GL_NO_ERROR;
                    }

                    // Color on error
                    if (_ret != true) {
                        glClearColor (1.0f, 0.0f, 0.0f, 0.5f);
                        _ret = glGetError () == GL_NO_ERROR;
                    }

                    return _ret;
                }

                valid_t RenderTile () {
                    valid_t _ret = glGetError () == GL_NO_ERROR;

                    if (_ret != false) {
                        _ret = SetupProgram ();
                    }

// TODO: range
                    static_assert (std::is_same <GLfloat, GLES::offset::coordinate_t>:: value != false);
                    std::array <GLfloat, 4 * VerticeDimensions> const _vert = {-0.5f + _offset._x, -0.5f + _offset._y, 0.0f + _offset._z /* v0 */, -0.5f + _offset._x, 0.5f + _offset._y, 0.0f + _offset._z /* v1 */, 0.5f + _offset._x, -0.5f + _offset._y, 0.0f + _offset._z /* v2 */, 0.5f + _offset._x, 0.5f + _offset._y, 0.0f + _offset._z /* v3 */};

                    if (_ret != false) {
                        GLuint _prog = 0;

                        if (_ret != false) {
                            glGetIntegerv (GL_CURRENT_PROGRAM, reinterpret_cast <GLint *> (&_prog));
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        GLint _loc = 0;
                        if (_ret != false) {
                            _loc = glGetAttribLocation (_prog, "position");
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glVertexAttribPointer (_loc, VerticeDimensions, GL_FLOAT, GL_FALSE, 0, _vert.data ());
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            glEnableVertexAttribArray (_loc);
                            _ret = glGetError () == GL_NO_ERROR;
                        }
                    }

                    if (_ret != false) {
                        glDrawArrays (GL_TRIANGLE_STRIP, 0, _vert.size () / VerticeDimensions);
                        _ret = glGetError () == GL_NO_ERROR;
                    }

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
                EGLConfig _conf = EGL_NO_CONFIG;
                EGLContext _ctx = EGL_NO_CONTEXT;
                EGLSurface _surf = EGL_NO_SURFACE;

                WPEFramework::Plugin::EGL::width_t  _width = 0;
                WPEFramework::Plugin::EGL::height_t _height = 0;

                Natives const & _natives;

                bool _valid = false;

            public :

                using dpy_t = decltype (_dpy);
                using surf_t = decltype (_surf);
                using ctx_t = decltype (_ctx);
                using height_t = decltype (_height);
                using width_t = decltype (_width);
                using valid_t = decltype (_valid);

                using img_t = WPEFramework::Plugin::EGL::img_t;

                EGL () = delete;
                EGL (Natives const & natives) : _natives { natives }, _valid { Initialize () } {}
                ~EGL () {
                    _valid = false;
                    DeInitialize ();
                }

                static constexpr img_t InvalidImage () { return WPEFramework::Plugin::EGL::InvalidImage (); }

                dpy_t Display () const { return _dpy; }
                surf_t Surface () const { return _surf; }

                height_t Height () const { return _height; }
                width_t Width () const { return _width; }

                static img_t CreateImage (EGL const & egl, ClientSurface::surf_t const & surf) {
                    img_t _ret = InvalidImage ();

                    if (egl.Valid () != false) {

                            static_assert ((std::is_same <dpy_t, EGLDisplay> :: value && std::is_same <ctx_t, EGLContext> :: value && std::is_same <img_t, EGLImageKHR> :: value ) != false);
                        static EGLImageKHR (* _eglCreateImageKHR) (EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, EGLint const * ) = reinterpret_cast < EGLImageKHR (*) (EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, EGLint const * ) > (eglGetProcAddress ("eglCreateImageKHR"));

                        if (_eglCreateImageKHR != nullptr) {

                            auto _width = gbm_bo_get_width (surf._buf);
                            auto _height = gbm_bo_get_height (surf._buf);

                            using width_t =  decltype (_width);
                            using height_t = decltype (_height);

                            // Narrowing detection

                            // Enable narrowing detecttion
// TODO:
                            constexpr bool _enable = false;

                            // (Almost) all will fail!
                            if (_narrowing < width_t, EGLint, _enable > :: value != false
                                && _narrowing < height_t, EGLint, _enable > :: value != false) {
                                TRACE_WITHOUT_THIS (Trace::Information, (_T ("Possible narrowing detected!")));
                            }

                            EGLint const _attrs [] = {
                                EGL_WIDTH, static_cast <EGLint> (_width),
                                EGL_HEIGHT, static_cast <EGLint> (_height),
                                EGL_NONE
                            };

                            static_assert (std::is_convertible < decltype (surf._buf), EGLClientBuffer > :: value != false);
                            _ret = _eglCreateImageKHR (egl.Display (), EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, surf._buf, _attrs);
                        }
                        else {
                            // Error
                            TRACE_WITHOUT_THIS (Trace::Error, (_T ("eglCreateImageKHR is unavailable or invalid parameters.")));
                        }
                    }
                    else {
                        TRACE_WITHOUT_THIS (Trace::Error, (_T ("EGL is not properly initialized.")));
                    }

                    return _ret;
                }

                static img_t DestroyImage (EGL const & egl, ClientSurface::surf_t const & surf) {
                    img_t _ret = surf._khr;

                    if (egl.Valid () != false ) {
                        static EGLBoolean (* _eglDestroyImageKHR) (EGLDisplay, EGLImageKHR) = reinterpret_cast < EGLBoolean (*) (EGLDisplay, EGLImageKHR) > (eglGetProcAddress ("eglDestroyImageKHR"));

                        if (_eglDestroyImageKHR != nullptr && surf.RenderComplete () != false) {
// TODO: Leak?
                            _ret = _eglDestroyImageKHR (egl.Display (), surf._khr) != EGL_FALSE ? EGL::InvalidImage () : _ret;
                        }
                        else {
                            // Error
                            TRACE_WITHOUT_THIS (Trace::Error, (_T ("eglDestroyImageKHR is unavailablei or invalid paramters.")));
                        }
                    }
                    else {
                        TRACE_WITHOUT_THIS (Trace::Error, (_T ("EGL is not properly initialized.")));
                    }

                    return _ret;
                }

                valid_t Valid () const { return _valid; }

            private :

// TODO: extension support
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
                        static_assert ((std::is_integral <decltype (_major)> :: value) != false);
                        static_assert ((std::is_integral <decltype (_minor)> :: value) != false);
                        TRACE (Trace::Information, (_T ("EGL version : %d.%d"), static_cast <int> (_major), static_cast <int> (_minor)));
                    }

                    if (_ret != false) {
                        constexpr EGLint const _attr [] = {
                            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
// TODO: magic constants
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
// TODO: magic constant, GLESv2
                            EGL_CONTEXT_CLIENT_VERSION, 2,
                            EGL_NONE
                        };

                        _ctx = eglCreateContext (_dpy, _conf, EGL_NO_CONTEXT, _attr);
                        _ret = _ctx != EGL_NO_CONTEXT;
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
                    bool _ret = Valid () != false && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE;

                    if (_ret != false) {
                        _ret = eglSwapBuffers (_dpy, _surf) != EGL_FALSE;

                        // Avoid any memory leak if the local thread is stopped (by another thread)
                        _ret = eglMakeCurrent (_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_FALSE && _ret;
                    }

                    return _ret;
                }

                bool Render (std::function < GLES::valid_t () > prefunc, std::function < GLES::valid_t () > postfunc) {
                    bool _ret = Valid () != false && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE;

                    if (_ret != false) {
                        _ret = prefunc () != false && eglSwapBuffers (_dpy, _surf) != EGL_FALSE && postfunc () != false;

                        // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
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
            , _dma { nullptr }
            , _natives (_platform)
            , _egl (_natives)
        {
        }
        ~CompositorImplementation()
        {
            if (_dma != nullptr) {
                delete _dma;
            }

            _dma = nullptr;

#ifdef _0 // Only if destructors are not called
            _clients.Visit (
                [this] (string const & name, const Core::ProxyType<ClientSurface>& client) {
                    if ( (client.IsValid () && _egl.Valid () ) != false) {
                        /* surf_t */ client->Surface ( EGL::DestroyImage ( _egl, client->Surface () ));
                    }
                }
            );
#else
            _clients.Clear();
#endif

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

        bool FDFor (std::string const & name, int & fd) {
            std::lock_guard < decltype (_clientLock) > const lock (_clientLock);

            bool _ret = false;

            TRACE (Trace::Information, (_T ("%s requested a DMA transfer."), name.c_str ()));

            /* ProxyType <> */ auto _client = _clients.Find (name);

            if (_client.IsValid () != true ||  name.compare (_client->Name ()) != 0) {
                TRACE (Trace::Error, (_T ("%s does not appear to be a valid client."), name.length () > 0 ? name.c_str () : "'<no name provided>'"));
            }
            else {
                if (_dma != nullptr) {
                    using fd_t = std::remove_reference < decltype (fd) > :: type;
                    using class_t = decltype (_client);
                    using return_t = decltype ( std::declval < class_t > ().operator-> ()->Native () );

                    static_assert ((std::is_convertible < return_t, fd_t > :: value) != false);
                    // Likely to almost always fail
//                    static_assert ((sizeof ( return_t ) <= sizeof ( fd_t )) != false);

                    fd = static_cast < fd_t > ( _client->Native () );

                    _ret = fd > -1;
                }
            }

            if (_ret != false) {
                /* surf_t */ _client->Surface ( EGL::CreateImage (_egl, _client->Surface () ));
            }

            return _ret;
        }

        // The (remote) caller should not continue to render to any shared resource until this completes
        bool CompositeFor (std::string const & name) {
            std::lock_guard < decltype (_clientLock) > const lock (_clientLock);

            bool _ret = false;

            /* ProxyType <> */ auto _client = _clients.Find (name);

            if (_client.IsValid () != true ||  name.compare (_client->Name ()) != 0) {
                TRACE (Trace::Error, (_T ("%s does not appear to be a valid client."), name.c_str ()));
            }
            else {
                static GLES _gles;

                ClientSurface::surf_t const & _surf = _client->Surface ();

                _ret = ( _surf.RenderComplete () && _egl.Valid () && _gles.Valid () ) != false;

                if (_ret != false) {
                    TRACE (Trace::Information, (_T ("Client has an associated EGL image.")));

                    auto _width = gbm_bo_get_width (_surf._buf);
                    auto _height = gbm_bo_get_height (_surf._buf);

                    using width_t = decltype (_width);
                    using height_t = decltype (_height);

                    // Narrowing detection

                    // Enable narrowing detection
// TODO:
                    constexpr bool _enable = false;

                    // (Almost) all will fail!
                    if (_narrowing < width_t, EGLint, _enable > :: value != true
                        && _narrowing < height_t, EGLint, _enable > :: value != true) {
                        TRACE (Trace::Information, (_T ("Possible narrowing detected!")));
                    }

                    // Update including some GLES preparations
//                    _ret = ( _gles.UpdateOffset ( GLES::InitialOffset () ) && _egl.Render (std::bind (&GLES::RenderColor, &_gles ), std::bind (&GLES::Render, &_gles) ) ) != false;
                    _ret = ( _gles.UpdateOffset ( GLES::InitialOffset () ) && _egl.Render (std::bind (&GLES::RenderEGLImage, &_gles, std::cref (_surf._khr), _width, _height ), [] () -> GLES::valid_t { GLES::valid_t _ret = true; return _ret; } ) ) != false;
                }

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

            // Clean up client that leaves prematurely

            /* ProxyType <> */ auto _client = _clients.Find (name);

            if (_client.IsValid () != false) {
                /* surf_t */ _client->Surface ( EGL::DestroyImage ( _egl, _client->Surface () ));
            }

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

            if (client == nullptr) {
                TRACE(Trace::Error, (_T("Unable to create the ClientSurface with name %s"), name.c_str()));
            }
            else {
                _dma = new DMATransfer (* this);

                if (_dma == nullptr || _dma->Valid () != true) {
                    TRACE (Trace::Error, (_T ("DMA transfers are not supported.")));

                    delete _dma;
                    _dma = nullptr;
                }
                else {
                    _dma->Run ();
                }
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

        DMATransfer * _dma;
        Natives _natives;
        EGL _egl;
        std::mutex _clientLock;
    };

    // ODR-use
    /* static */ constexpr void * const CompositorImplementation::EGL::EGL_NO_CONFIG;

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

    ClientSurface::ClientSurface(ModeSet& modeSet, CompositorImplementation& compositor, const string& name, const uint32_t width, const uint32_t height)
        : _nativeSurface { nullptr, -1, WPEFramework::Plugin::EGL::InvalidImage () }
        , _modeSet(modeSet)
        , _compositor(compositor)
        , _name(name)
        , _opacity(Exchange::IComposition::maxOpacity)
        , _layer(0)
        , _destination( { 0, 0, width, height } ) {

        using surface_t = decltype (_nativeSurface._buf);
        using class_t = std::remove_reference < decltype (_modeSet) > ::type;
        using return_t = decltype ( std::declval < class_t > ().CreateBufferObject (width, height) );

        static_assert (std::is_same < surface_t, return_t> :: value != false);

// TODO: The internal scan out flag might not be appropriate
        _nativeSurface._buf = _modeSet.CreateBufferObject (width, height);

        if (_nativeSurface.Valid () != true) {
            TRACE (Trace::Error, (_T ("A ClientSurface cannot be created for %s"), name.c_str ()));
        }
        else {
            _nativeSurface._fd = gbm_bo_get_fd (_nativeSurface._buf);

            if (_nativeSurface.DMAComplete () != true) {
                TRACE (Trace::Error, (_T ("The created ClientSurface for %s is not suitable for DMA."), name.c_str ()));
            }
        }
    }

    ClientSurface::~ClientSurface() {

        // Part of the client is cleaned up via the detached (hook)

        _compositor.Detached(_name);

        if (_nativeSurface._fd != -1) {
            /* int */ close (_nativeSurface._fd);
        }

        if (_nativeSurface.Valid () != false) {
            _modeSet.DestroyBufferObject (_nativeSurface._buf);
        }

        _nativeSurface = { nullptr, -1 , WPEFramework::Plugin::EGL::InvalidImage () };

    }

    void ClientSurface::ScanOut () {
        _compositor.CompositeFor (_name);
    }

} // namespace Plugin
} // namespace WPEFramework
