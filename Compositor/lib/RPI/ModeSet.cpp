#include "ModeSet.h"

#include <vector>
#include <list>
#include <string>
#include <cassert>
#include <limits>
#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#ifdef __cplusplus
}
#endif


// TODO: match return type to class types

namespace WPEFramework {
    static void GetNodes ( uint32_t type , std::vector < std::string > & list ) {
        // Arbitrary value
        /* static */ constexpr uint8_t DrmMaxDevices = 16;

        drmDevicePtr devices [ DrmMaxDevices ];

        static_assert( sizeof ( DrmMaxDevices ) <= sizeof ( int ) );
        static_assert( std::numeric_limits < decltype ( DrmMaxDevices ) > :: max () <= std::numeric_limits < int > :: max () );

        int device_count = drmGetDevices2 ( 0 /* flags */ , & devices [ 0 ] , static_cast < int > ( DrmMaxDevices ) );

        if ( device_count > 0 ) {
            for ( decltype ( device_count ) i = 0 ; i < device_count ; i++ ) {
                switch ( type ) {
                    case DRM_NODE_PRIMARY   :   // card<num>, always created, KMS, privileged
                    case DRM_NODE_CONTROL   :   // ControlD<num>, currently unused
                    case DRM_NODE_RENDER    :   // Solely for render clients, unprivileged
                                                {
                                                    if ( ( 1 << type ) == ( devices [ i ] -> available_nodes & ( 1 << type ) ) ) {
                                                        list . push_back ( std::string ( devices [ i ] -> nodes [ type ] ) );
                                                    }
                                                    break;
                                                }
                    case DRM_NODE_MAX       :
                    default                 :   // Unknown (new) node type
                                            ;
                }
            }

            drmFreeDevices ( & devices [ 0 ] , device_count );
        }
    }

    static uint32_t GetConnectors ( WPEFramework::ModeSet::DRM::fd_t fd , uint32_t type ) {
        uint32_t bitmask = 0;

        drmModeResPtr resources = drmModeGetResources ( fd );

        if ( nullptr != resources ) {
            for ( decltype ( resources -> count_connectors ) i = 0 ; i < resources -> count_connectors ; i++ ) {
                drmModeConnectorPtr connector = drmModeGetConnector ( fd , resources -> connectors [ i ] );

                if ( nullptr != connector ) {
                    if (    ( type == connector -> connector_type )
                         && ( static_cast < decltype ( connector -> connection ) > ( DRM_MODE_CONNECTED ) == connector -> connection )
                       ) {

                        bitmask = bitmask | ( 1 << i );
                    }

                    drmModeFreeConnector ( connector );
                }
            }

            drmModeFreeResources ( resources );
        }

        return bitmask;
    }

    static uint32_t GetCRTCS ( WPEFramework::ModeSet::DRM::fd_t fd , bool valid ) {
        uint32_t bitmask = 0;

        drmModeResPtr resources = drmModeGetResources ( fd );

        if ( nullptr != resources ) {
            for ( decltype (resources -> count_crtcs ) i = 0 ; i < resources -> count_crtcs ; i++ ) {
                drmModeCrtcPtr crtc = drmModeGetCrtc ( fd , resources -> crtcs [ i ] );

                if ( nullptr != crtc ) {
                    bool currentSet = ( crtc -> mode_valid == 1 );

                    if ( valid == currentSet ) {
                        bitmask = bitmask | ( 1 << i );
                    }
                    drmModeFreeCrtc ( crtc );
                }
            }

            drmModeFreeResources ( resources );
        }

        return bitmask;
    }

    static bool FindProperDisplay ( WPEFramework::ModeSet::DRM::fd_t fd , WPEFramework::ModeSet::DRM::crtc_id_t & crtc , WPEFramework::ModeSet::DRM::enc_id_t & encoder , WPEFramework::ModeSet::DRM::conn_id_t & connector , WPEFramework::ModeSet::DRM::fb_id_t & fb ) {
        bool found = false;

        assert ( fd != WPEFramework::ModeSet::DRM::InvalidFd () );

        // Only connected connectors are considered
        uint32_t connectorMask = GetConnectors ( fd , DRM_MODE_CONNECTOR_HDMIA );

        // All CRTCs are considered for the given mode (valid / not valid)
        uint32_t crtcs = GetCRTCS ( fd , true );

        drmModeResPtr resources = drmModeGetResources ( fd );

        if ( nullptr != resources ) {
            int32_t connectorIndex = 0;

            while ( ( found == false ) && ( connectorIndex < resources -> count_connectors ) ) {
                if ( ( connectorMask & ( 1 << connectorIndex ) ) != 0 ) {
                    drmModeConnectorPtr connectors = drmModeGetConnector ( fd , resources -> connectors [ connectorIndex ] );

                    if ( nullptr != connectors ) {
                        int32_t encoderIndex = 0;

                        while ( ( found == false ) && ( encoderIndex < connectors -> count_encoders ) ) {
                            drmModeEncoderPtr encoders = drmModeGetEncoder ( fd , connectors -> encoders [ encoderIndex ] );

                            if ( nullptr != encoders ) {
                                uint32_t matches = ( encoders -> possible_crtcs & crtcs );
                                WPEFramework::ModeSet::DRM::crtc_id_t * pcrtc = resources -> crtcs;

                                while ( ( found == false ) && ( matches > 0 ) ) {
                                    if ( ( matches & 1 ) != 0 ) {
                                        drmModeCrtcPtr modePtr = drmModeGetCrtc ( fd , * pcrtc );

                                        if ( nullptr != modePtr ) {
                                            // A viable set found
                                            crtc = * pcrtc;
                                            encoder = encoders -> encoder_id;
                                            connector = connectors -> connector_id;
                                            fb = modePtr -> buffer_id;

                                            drmModeFreeCrtc ( modePtr );
                                            found = true;
                                        }
                                    }
                                    matches >>= 1;
                                    pcrtc ++;
                                }
                                drmModeFreeEncoder ( encoders );
                            }
                            encoderIndex ++;
                        }
                        drmModeFreeConnector ( connectors );
                    }
                }
                connectorIndex ++;
            }

            drmModeFreeResources ( resources );
        }

        return found;
    }

    static bool CreateBuffer ( WPEFramework::ModeSet::DRM::fd_t fd , const WPEFramework::ModeSet::DRM::conn_id_t connector , WPEFramework::ModeSet::GBM::dev_t & device , uint32_t & modeIndex , WPEFramework::ModeSet::DRM::fb_id_t & id , WPEFramework::ModeSet::GBM::buf_t & buffer , uint32_t & vrefresh ) {
        assert ( fd != WPEFramework::ModeSet::DRM::InvalidFd () );

        bool created = false;
        buffer = WPEFramework::ModeSet::GBM::InvalidBuf ();
        modeIndex = 0;
        id = WPEFramework::ModeSet::DRM::InvalidFb ();
        device = gbm_create_device ( fd );

        if ( device != WPEFramework::ModeSet::GBM::InvalidDev () ) {
            drmModeConnectorPtr pconnector = drmModeGetConnector ( fd , connector );

            if ( nullptr != pconnector ) {
                bool found = false;
                uint32_t index = 0;
                uint64_t area = 0;

                while ( ( found == false ) && ( index < static_cast < uint32_t > ( pconnector -> count_modes ) ) ) {
                    uint32_t type = pconnector -> modes [ index ] . type;

                    // At least one preferred mode should be set by the driver, but dodgy EDID parsing might not provide it
                    if ( DRM_MODE_TYPE_PREFERRED == ( DRM_MODE_TYPE_PREFERRED & type ) ) {
                        modeIndex = index;
                        vrefresh = pconnector -> modes [ modeIndex ] . vrefresh;

                        // Found a suitable mode; break the loop
                        found = true;
                    }
                    else
                        if ( DRM_MODE_TYPE_DRIVER == ( DRM_MODE_TYPE_DRIVER & type ) ) {
                            // Calculate screen area
                            uint64_t size = pconnector -> modes [ index ] . hdisplay * pconnector -> modes [ index ] . vdisplay;
                            if ( area < size ) {
                                area = size;

                                // Use another selection criterium
                                // Select highest clock and vertical refresh rate

                                if ( ( pconnector -> modes [ index ] . clock > pconnector -> modes [ modeIndex ] . clock )
                                        ||
                                            (     ( pconnector -> modes [ index ] . clock == pconnector -> modes [ modeIndex ] . clock )
                                               && ( pconnector -> modes [ index ] . vrefresh > pconnector -> modes [ modeIndex ] . vrefresh )
                                            )
                                   )
                                {
                                    modeIndex = index;
                                    vrefresh = pconnector -> modes [ modeIndex ] . vrefresh;
                                }
                            }
                        }

                    index ++;
                }

                ModeSet::GBM::modifier_t modifiers [ 1 ] = { ModeSet::FormatModifier () };
                ModeSet::GBM::buf_t bo = gbm_bo_create_with_modifiers ( device , pconnector -> modes [ modeIndex ] . hdisplay , pconnector -> modes [ modeIndex ] . vdisplay , ModeSet::SupportedBufferType () , & modifiers [ 0 ] , 1 );

                drmModeFreeConnector ( pconnector );

                if ( ModeSet::GBM::InvalidBuf () != bo ) {
                    // Associate a frame buffer with this bo
                    ModeSet::GBM::fd_t fb_fd = gbm_device_get_fd( device );

                    ModeSet::GBM::width_t width = gbm_bo_get_width ( bo );
                    ModeSet::GBM::height_t height = gbm_bo_get_height ( bo );
                    ModeSet::GBM::frmt_t format = gbm_bo_get_format ( bo );
                    ModeSet::GBM::handle_t handle = gbm_bo_get_handle ( bo ) . u32;
                    ModeSet::GBM::stride_t stride = gbm_bo_get_stride ( bo );
                    ModeSet::GBM::modifier_t modifier = gbm_bo_get_modifier ( bo );

                    static_assert ( static_cast < ModeSet::GBM::plane_t > ( GBM_MAX_PLANES ) == static_cast < ModeSet::GBM::plane_t > ( 4 ) );
                    ModeSet::DRM::handle_t const handles [ GBM_MAX_PLANES ] = { static_cast < ModeSet::DRM::handle_t > ( handle ) , 0 , 0 , 0 };
                    ModeSet::DRM::pitch_t const pitches [ GBM_MAX_PLANES ] = { static_cast < ModeSet::DRM::pitch_t > ( stride ), 0 , 0 , 0 };
                    ModeSet::DRM::offset_t  const offsets [ GBM_MAX_PLANES ] = { static_cast < ModeSet::DRM::offset_t > ( 0 ) , 0 , 0 , 0 };
                    ModeSet::DRM::modifier_t const modifiers [ GBM_MAX_PLANES ] = { static_cast < ModeSet::DRM::modifier_t > ( modifier ) , 0 , 0 , 0 };

                    int32_t ret = drmModeAddFB2WithModifiers ( fb_fd , width , height , format , & handles [ 0 ] , & pitches [ 0 ] , & offsets [ 0 ] , & modifiers [ 0 ] , & id , 0 /* flags */ );

                    if ( 0 == ret ) {
                        buffer = bo;
                        created = true;
                    }
                }
            }
        }

        return created;
    }



    ModeSet::BufferInfo::BufferInfo () : _surface { ModeSet::GBM::InvalidSurf () } , _bo { ModeSet::GBM::InvalidBuf () } , _id { ModeSet::DRM::InvalidFb () } {
    }

    ModeSet::BufferInfo::BufferInfo ( ModeSet::GBM::surf_t surface , ModeSet::GBM::buf_t buffer , ModeSet::DRM::fb_id_t id ) : _surface { surface } , _bo { buffer } , _id { id } {
    }

    ModeSet::BufferInfo::BufferInfo ( ModeSet::BufferInfo && other ) : _surface { other . _surface } , _bo { other . _bo } , _id { other . _id } {
        if ( this != & other ) {
            other . _surface = GBM::InvalidSurf ();
            other. _bo = GBM::InvalidBuf ();
            other . _id = DRM::InvalidFb ();
        }
    }

    ModeSet::BufferInfo & ModeSet::BufferInfo::operator= ( ModeSet::BufferInfo && other ) {
        if ( this != & other ) {
            _surface = other . _surface;
            _bo = other . _bo;
            _id = other . _id;

            other . _surface = GBM::InvalidSurf ();
            other. _bo = GBM::InvalidBuf ();
            other . _id = DRM::InvalidFb ();
        }

        return * this;
    }



    ModeSet::ModeSet ()
        : _crtc { DRM::InvalidCrtc () }
        , _encoder { DRM::InvalidEncoder () }
        , _connector { DRM::InvalidConnector () }
        , _mode ( 0 )
        , _vrefresh ( 0 )
        , _device { GBM::InvalidDev () }
        , _buffer { GBM::InvalidBuf () }
        , _fd { DRM::InvalidFd () } {
    }

    ModeSet::~ModeSet () {
        if ( _buffer != GBM::InvalidBuf () ) {
            Close ();
        }
    }

    uint32_t ModeSet::Open ( string const & ) {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if ( drmAvailable () > 0 ) {

            std::vector < std::string > nodes;

            GetNodes ( DRM_NODE_PRIMARY , nodes );

            std::vector < std::string > :: iterator index ( nodes . begin () );

            while ( ( index != nodes . end () ) && ( _fd == DRM::InvalidFd () ) ) {
                // Select the first from the list
                if ( index -> empty () == false ) {
                    // The node might be priviliged and the call will fail.
                    // Do not close fd with exec functions! No O_CLOEXEC!
                    _fd = open ( index -> c_str () , O_RDWR );

                    if ( _fd != DRM::InvalidFd () ) {
                        bool success = false;

                        printf ( "Test Card: %s\n" , index -> c_str () );
                        if (       ( FindProperDisplay ( _fd , _crtc , _encoder , _connector , _fb ) == true )
                                   /* TODO: Changes the original fb which might not be what is intended */
                                && ( CreateBuffer ( _fd , _connector , _device , _mode , _fb , _buffer , _vrefresh ) == true )
                                && ( drmSetMaster ( _fd ) == 0 )
                           )
                        {
                            drmModeConnectorPtr pconnector = drmModeGetConnector ( _fd , _connector );

                            /* At least one mode has to be set */
                            if ( pconnector != nullptr ) {
                                success = ( 0 == drmModeSetCrtc ( _fd , _crtc , _fb , 0 , 0 , & _connector , 1 , & ( pconnector -> modes [ _mode ] ) ) );
                                drmModeFreeConnector ( pconnector );
                            }
                        }

                        if ( success == true ) {
                            printf ( "Opened Card: %s\n" , index -> c_str () );
                        }
                        else {
                            Close ();
                        }
                    }
                }

                index ++;
            }

            result = ( _fd != DRM::InvalidFd () ? Core::ERROR_NONE : Core::ERROR_GENERAL );
        }

        return result;
    }

    uint32_t ModeSet::Close () {
        // Destroy the initial buffer if one exists
        if ( _buffer != GBM::InvalidBuf () ) {
            gbm_bo_destroy ( _buffer );
            _buffer = GBM::InvalidBuf ();
        }

        if ( _device != GBM::InvalidDev () ) {
            gbm_device_destroy ( _device );
            _device = GBM::InvalidDev ();
        }

        if ( _fd != DRM::InvalidFd () ) {
            close ( _fd );
            _fd = DRM::InvalidFd ();
        }

        _crtc = DRM::InvalidCrtc ();
        _encoder = DRM::InvalidEncoder ();
        _connector = DRM::InvalidConnector ();

        return Core::ERROR_NONE;
    }

    ModeSet::GBM::dev_t /* const */ ModeSet::UnderlyingHandle () const {
        return _device;
    }

    /* static */ std::list < ModeSet::DRM::frmt_t > ModeSet::AvailableFormats ( ModeSet::DRM::fd_t fd ) {
        std::list < DRM::frmt_t > ret;

        if ( fd != DRM::InvalidFd () ) {

            drmModePlaneResPtr res = drmModeGetPlaneResources ( fd );

            if ( res != nullptr ) {

                decltype ( drmModePlaneRes::count_planes ) count = res -> count_planes;
                decltype ( drmModePlaneRes::planes ) planes = res -> planes;

                for ( ; count > 0 ; count-- ) {

                    drmModePlanePtr plane = drmModeGetPlane ( fd , planes [ count - 1 ] );

                    if ( plane != nullptr ) {

                        decltype ( drmModePlane::count_formats ) number = plane -> count_formats;
                        decltype ( drmModePlane::formats ) formats = plane -> formats;

                        // Uses the iterator constructor
                        std::list < DRM::frmt_t > sub ( & formats [ 0 ] , & formats [ number ] );

                        sub . sort ();
                        sub . unique ();

                        ret . merge ( sub );

                        drmModeFreePlane ( plane );

                    }

                }

                drmModeFreePlaneResources ( res );
            }

        }

        ret . sort ();
        ret . unique ();

        return ret;
    }

    ModeSet::DRM::fd_t ModeSet::Descriptor () const {
        return ( _fd );
    }

    uint32_t ModeSet::Width () const {
        // Derived from modinfo if CreateBuffer was called prior to this
        uint32_t width = 0;

        if ( GBM::InvalidBuf () != _buffer ) {
            width = gbm_bo_get_width ( _buffer );
        }

        return width;
    }

    uint32_t ModeSet::Height () const {
        // Derived from modinfo if CreateBuffer was called prior to this
        uint32_t height = 0;

        if ( GBM::InvalidBuf () != _buffer ) {
            height = gbm_bo_get_height ( _buffer );
        }

        return height;
    }

    uint32_t ModeSet::RefreshRate () const {
        // Derived from modinfo if CreateBuffer was called prior to this

        return _vrefresh;
    }

    bool ModeSet::Interlaced () const {
// TODO: For now assume interlaced is always false
        bool ret = false;

        return ret;
    }

    // These created resources are automatically destroyed if gbm_device is destroyed
    ModeSet::GBM::surf_t ModeSet::CreateRenderTarget ( const uint32_t width , const uint32_t height ) const {
        GBM::surf_t result = GBM::InvalidSurf ();

        if ( GBM::InvalidDev () != _device ) {
            GBM::modifier_t modifiers [ 1 ] = { static_cast < GBM::modifier_t > ( FormatModifier () ) };
            result = gbm_surface_create_with_modifiers ( _device , width , height , SupportedBufferType () , & modifiers [ 0 ] , 1 );
        }

        return result;
    }

    void ModeSet::DestroyRenderTarget ( struct BufferInfo & buffer ) {
        if ( GBM::InvalidBuf () != buffer . _bo ) {
            if ( static_cast < uint32_t > ( ~0 ) == buffer . _id ) {
                drmModeRmFB ( _fd , buffer . _id );
            }

            gbm_surface_release_buffer ( buffer . _surface , buffer . _bo );
            buffer . _bo = GBM::InvalidBuf ();
        }

        if ( GBM::InvalidSurf () != buffer . _surface ) {
            DestroyRenderTarget ( buffer . _surface );
        }
    }

    void ModeSet::DestroyRenderTarget ( ModeSet::GBM::surf_t surface ) const {
        if ( surface != GBM::InvalidSurf () ) {
            /* void */ gbm_surface_destroy ( surface );
        }

        surface = GBM::InvalidSurf ();
    }

    ModeSet::GBM::buf_t ModeSet::CreateBufferObject ( uint32_t const width , uint32_t const height ) {
        return _device != GBM::InvalidDev () ? gbm_bo_create ( _device , width , height , SupportedBufferType () , GBM_BO_USE_SCANOUT /* presented on a screen */ | GBM_BO_USE_RENDERING /* used for rendering */ ) : GBM::InvalidBuf ();
    }

    void ModeSet::DestroyBufferObject ( ModeSet::GBM::buf_t bo ) {
        if ( bo != GBM::InvalidBuf () ) {
            /* void */ gbm_bo_destroy ( bo );
        }
    }

    using data_t = std::atomic < bool >;

    static void PageFlip ( int , unsigned int , unsigned int , unsigned int , void * data ) {
        assert ( data != nullptr );

        if ( data != nullptr ) {
            data_t * _data = reinterpret_cast < data_t * > ( data );
            * _data = false;
        }
        else {
//            TRACE ( Trace::Error , ( _T ( "Invalid callback data." ) ) );
        }
    }

    void ModeSet::Swap ( struct BufferInfo & buffer ) {
        assert ( _fd != DRM::InvalidFd () );

        // Lock the a new buffer so we can use it
        buffer . _bo = gbm_surface_lock_front_buffer ( buffer . _surface );

        if ( buffer . _bo != GBM::InvalidBuf () ) {
            ModeSet::GBM::fd_t fb_fd = gbm_device_get_fd ( _device );

            assert ( _fd == fb_fd );

            ModeSet::GBM::width_t width = gbm_bo_get_width ( buffer . _bo );
            ModeSet::GBM::height_t height = gbm_bo_get_height ( buffer . _bo );
            ModeSet::GBM::frmt_t format = gbm_bo_get_format ( buffer . _bo );
            ModeSet::GBM::handle_t handle = gbm_bo_get_handle ( buffer . _bo ) . u32;
            ModeSet::GBM::stride_t stride = gbm_bo_get_stride ( buffer . _bo );
            ModeSet::GBM::modifier_t modifier = gbm_bo_get_modifier ( buffer . _bo );

            static_assert ( static_cast < ModeSet::GBM::plane_t > ( GBM_MAX_PLANES ) == static_cast < ModeSet::GBM::plane_t > ( 4 ));
            ModeSet::DRM::handle_t const handles [ GBM_MAX_PLANES ] = { static_cast < ModeSet::DRM::handle_t > ( handle ) ,  static_cast < ModeSet::DRM::handle_t > ( 0 ) ,  static_cast < ModeSet::DRM::handle_t > ( 0 ) ,  static_cast < ModeSet::DRM::handle_t > ( 0 ) };
            ModeSet::DRM::pitch_t const pitches [ GBM_MAX_PLANES ] = { static_cast < ModeSet::DRM::pitch_t > ( stride ) , static_cast < ModeSet::DRM::pitch_t > ( 0 ) , static_cast < ModeSet::DRM::pitch_t > ( 0 ) , static_cast < ModeSet::DRM::pitch_t > ( 0 ) };
            ModeSet::DRM::offset_t  const offsets [ GBM_MAX_PLANES ] = { static_cast < ModeSet::DRM::offset_t > ( 0 ) , static_cast < ModeSet::DRM::offset_t > ( 0 ) ,  static_cast < ModeSet::DRM::offset_t > ( 0 ) , static_cast < ModeSet::DRM::offset_t > ( 0) };
            ModeSet::DRM::modifier_t const modifiers [ GBM_MAX_PLANES ] = { static_cast < ModeSet::DRM::modifier_t > ( modifier ) , static_cast < ModeSet::DRM::modifier_t > ( 0 ) , static_cast < ModeSet::DRM::modifier_t > ( 0 ) , static_cast < ModeSet::DRM::modifier_t > ( 0 ) };

            if ( drmModeAddFB2WithModifiers ( _fd , width , height , format , & handles [ 0 ] , & pitches [ 0 ] , & offsets [ 0 ] , & modifiers [ 0 ] , & ( buffer . _id ) , 0 /* flags */ ) != 0 ) {
                buffer . _id = DRM::InvalidFb ();

                TRACE ( Trace::Error , ( _T ( "Unable to contruct a frame buffer for scan out." ) ) );
            }
            else {
                static data_t cdata ( true );
                cdata = true;

                int err = drmModePageFlip ( _fd , _crtc , buffer . _id , DRM_MODE_PAGE_FLIP_EVENT , & cdata );
                // Many causes, but the most obvious is a busy resource or a missing drmModeSetCrtc
                // Probably a missing drmModeSetCrtc or an invalid _crtc
                // See ModeSet::Create, not recovering here
                assert ( err != -EINVAL );
                assert ( err != -EBUSY );

                if ( err == 0 ) {
                    // No error
                    // Asynchronous, but never called more than once, waiting in scope
                    // Use the magic constant here because the struct is versioned!

                    static_assert ( DRM_EVENT_CONTEXT_VERSION >= static_cast < decltype ( DRM_EVENT_CONTEXT_VERSION ) > ( 2 ) && DRM_EVENT_CONTEXT_VERSION <= static_cast < decltype ( DRM_EVENT_CONTEXT_VERSION ) > ( 4 ) );

                    drmEventContext context = {
                          . version = 2
                        , . vblank_handler = nullptr
                        , . page_flip_handler = PageFlip
#if DRM_EVENT_CONTEXT_VERSION > 2
                        , . page_flip_handler2 = nullptr
#endif
#if DRM_EVENT_CONTEXT_VERSION > 3
                        , . sequence_handler = nullptr
#endif
                    };


                    struct timespec timeout = { . tv_sec = 1 , . tv_nsec = 0 };
                    fd_set fds;

                    // Going fast could trigger an (unrecoverable) EBUSY
                    bool waiting = cdata;

                    while ( waiting != false ) {
                        FD_ZERO ( & fds );
                        FD_SET(  _fd , & fds );

                        // Race free
                        if ( ( err = pselect ( _fd + 1 , & fds , nullptr , nullptr , & timeout , nullptr ) ) < 0 ) {
                            // Error; break the loop
                            TRACE ( Trace::Error , ( _T ( "Event processing for page flip failed." ) ) );
                            break;
                        }
                        else {
                            if ( err == 0 ) {
                                // Timeout; retry
                                // TODO: add an additional condition to break the loop to limit the
                                // number of retries, but then deal with the asynchronous nature of
                                // the callback

                                TRACE ( Trace::Information , ( _T ( "Unable to execute a timely page flip. Trying again." ) ) );
                            }
                            else {
                                if ( FD_ISSET ( _fd , & fds ) != 0 ) {
                                    // Node is readable
                                    if ( drmHandleEvent ( _fd , & context ) != 0 ) {
                                        // Error; break the loop
                                        break;
                                    }
                                    // Flip probably occured already otherwise it loops again
                                }
                            }

                            waiting = cdata;

                            if ( waiting != true ) {
                                // Do not prematurely remove the FB to prevent an EBUSY
                                static BufferInfo binfo ( GBM::InvalidSurf () , GBM::InvalidBuf () , DRM::InvalidFb () );
                                std::swap ( binfo , buffer );
                            }
                        }
                    }
                }
                else {
                     TRACE ( Trace::Error , ( _T ( "Unable to execute a page flip." ) ) );
                }

                if ( _fd != DRM::InvalidFd () && buffer . _id == DRM::InvalidFb () && drmModeRmFB ( _fd , buffer . _id ) != 0) {
                    TRACE ( Trace::Error , ( _T ( "Unable to remove (old) frame buffer." ) ) );
                }
            }

            if ( buffer . _surface != GBM::InvalidSurf () && buffer . _bo != GBM::InvalidBuf () ) {
                    gbm_surface_release_buffer ( buffer . _surface, buffer . _bo );
                    buffer . _bo = GBM::InvalidBuf () ;
            }
            else {
                    TRACE ( Trace::Error , ( _T ( "Unable to release (old) buffer." ) ) );
            }

        }
        else {
            TRACE ( Trace::Error , ( _T ( "Unable to obtain a buffer to support scan out." ) ) );
        }
    }
}
