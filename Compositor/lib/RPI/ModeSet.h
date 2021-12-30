#pragma once

#include <cstdint>
#include <limits>
#include <ctime>
#include <list>

#ifdef VC6
#define __GBM__
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <gbm.h>
#include <drm/drm_fourcc.h>
#include <xf86drmMode.h>
#include <EGL/egl.h>

#ifdef __cplusplus
}
#endif

#include "Module.h"
#include <interfaces/IComposition.h>

struct gbm_surface;
struct gbm_bo;

namespace WPEFramework {

class ModeSet
{
    template <class T>
    struct remove_pointer {
        typedef T type;
    };

    template <class T>
    struct remove_pointer <T*> {
        typedef T type;
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

    public:

        class DRM {
            public:
                using fd_t = int;

                using fb_id_t = remove_pointer < decltype (drmModeFB::fb_id) >::type;
                using crtc_id_t = remove_pointer < decltype (drmModeCrtc::crtc_id) >::type;
                using enc_id_t = remove_pointer < decltype (drmModeEncoder::encoder_id) >::type;
                using conn_id_t = remove_pointer < decltype (drmModeConnector::connector_id) >::type;

                using width_t = remove_pointer < decltype (drmModeFB2::width) >::type;
                using height_t = remove_pointer < decltype (drmModeFB2::height) >::type;
                using frmt_t = remove_pointer < decltype (drmModeFB2::pixel_format) >::type;
                using modifier_t = remove_pointer < decltype (drmModeFB2::modifier) >::type;

                using handle_t = remove_pointer < std::decay < decltype (drmModeFB2::handles) >::type >::type;
                using pitch_t = remove_pointer < std::decay < decltype (drmModeFB2::pitches) >::type >::type;
                using offset_t = remove_pointer < std::decay < decltype (drmModeFB2::offsets) >::type >::type;

                using x_t = remove_pointer < decltype (drmModeCrtc::x) >::type;
                using y_t = remove_pointer < decltype (drmModeCrtc::y) >::type;

                using duration_t = remove_pointer < decltype (timespec::tv_sec) >::type;

                static constexpr fd_t InvalidFd () { return -1; }
                static constexpr fb_id_t  InvalidFb () { return  0; }
                static constexpr crtc_id_t InvalidCrtc () { return 0; }
                static constexpr enc_id_t InvalidEncoder () { return 0; }
                static constexpr conn_id_t InvalidConnector () { return 0; }

                static constexpr width_t InvalidWidth () { return 0; }
                static constexpr height_t InvalidHeight () { return 0; }
                static constexpr frmt_t InvalidFrmt () { return DRM_FORMAT_INVALID; }
                static constexpr modifier_t InvalidModifier () { return DRM_FORMAT_MOD_INVALID; }
        };

        class GBM {
            public :
                using width_t = remove_pointer < decltype (gbm_import_fd_modifier_data::width) >::type;
                using height_t = remove_pointer < decltype (gbm_import_fd_modifier_data::height) >::type;
                using frmt_t = remove_pointer < decltype (gbm_import_fd_modifier_data::format) >::type;

                // See xf86drmMode.h for magic constant
                static_assert (GBM_MAX_PLANES == 4);

                using fd_t = remove_pointer < std::decay < decltype (gbm_import_fd_modifier_data::fds) > :: type >::type;
                using stride_t = remove_pointer < std::decay < decltype (gbm_import_fd_modifier_data::strides) > :: type >::type;
                using offset_t = remove_pointer < std::decay < decltype (gbm_import_fd_modifier_data::offsets) > :: type >::type;
                using modifier_t = remove_pointer < decltype (gbm_import_fd_modifier_data::modifier) >::type;

                using dev_t = struct gbm_device *;
                using surf_t = struct  gbm_surface *;
                using buf_t = struct gbm_bo *;

                using handle_t = decltype (gbm_bo_handle::u32);

                static constexpr dev_t InvalidDev () { return nullptr; }
                static constexpr surf_t InvalidSurf () { return nullptr; }
                static constexpr buf_t InvalidBuf () { return nullptr; }
                static constexpr fd_t InvalidFd () { return -1; }

                static constexpr width_t InvalidWidth () { return 0; }
                static constexpr height_t InvalidHeight () { return 0; }
                static constexpr stride_t InvalidStride () { return 0; }
                static constexpr frmt_t InvalidFrmt () { return DRM::InvalidFrmt (); }
                static constexpr modifier_t InvalidModifier () { return DRM::InvalidModifier (); }
        };

        struct BufferInfo {
            GBM::surf_t _surface;
            GBM::buf_t _bo;
            uint32_t _id;
        };

    public:
        ModeSet(const ModeSet&) = delete;
        ModeSet& operator= (const ModeSet&) = delete;

        ModeSet();
        ~ModeSet();

    public:
        uint32_t Open(const string& name);
        uint32_t Close();

        const GBM::dev_t UnderlyingHandle() const {
            return _device;
        }

        static std::list <DRM::frmt_t> AvailableFormats (DRM::fd_t fd) {
            std::list <DRM::frmt_t> _ret;

            if ( fd >= DRM::InvalidFd ()) {

                drmModePlaneResPtr _res = drmModeGetPlaneResources (fd);

                if (_res != nullptr) {

                    decltype (drmModePlaneRes::count_planes) _count = _res->count_planes;
                    decltype (drmModePlaneRes::planes) _planes = _res->planes;

                    for ( ; _count > 0; _count--) {

                        drmModePlanePtr _plane = drmModeGetPlane (fd, _planes [_count - 1]);

                        if (_plane != nullptr) {

                            decltype (drmModePlane::count_formats) _number = _plane->count_formats;
                            decltype (drmModePlane::formats) _formats = _plane->formats;

                            // Uses the iterator constructor
                            std::list <DRM::frmt_t> _sub (&_formats [0], &_formats [_number]);

                            _sub.sort ();
                            _sub.unique ();

                            _ret.merge (_sub);

                            drmModeFreePlane (_plane);
                        }

                    }

                    drmModeFreePlaneResources (_res);
                }

            }

            _ret.sort ();
            _ret.unique ();

            return _ret;
        }

        static_assert (_narrowing < decltype (DRM_FORMAT_ARGB8888), GBM::frmt_t, true > :: value != false);
        static constexpr GBM::frmt_t SupportedBufferType() { return static_cast <GBM::frmt_t> (DRM_FORMAT_ARGB8888); }

        static_assert (_narrowing < decltype (DRM_FORMAT_MOD_LINEAR), GBM::modifier_t, true > :: value != false);
        static constexpr GBM::modifier_t FormatModifier () { return static_cast <GBM::modifier_t> (DRM_FORMAT_MOD_LINEAR); }

        DRM::fd_t Descriptor () const {
            return (_fd);
        }
        uint32_t Width() const;
        uint32_t Height() const;
        uint32_t RefreshRate () const;
        bool Interlaced () const;
        GBM::surf_t CreateRenderTarget(const uint32_t width, const uint32_t height) const;
        void DestroyRenderTarget(struct BufferInfo& buffer);

        // For objects not included in Swap ()
        void DestroyRenderTarget(struct gbm_surface * surface) const;

        GBM::buf_t CreateBufferObject (uint32_t const width, uint32_t const height);
        void DestroyBufferObject (struct gbm_bo * bo);

        void Swap(struct BufferInfo& buffer);

    private:
        DRM::crtc_id_t _crtc;
        DRM::enc_id_t _encoder;
        DRM::conn_id_t _connector;

        DRM::fb_id_t _fb;
        uint32_t _mode;
        uint32_t _vrefresh;

        GBM::dev_t _device;
        GBM::buf_t _buffer;
        DRM::fd_t _fd;
};

}
