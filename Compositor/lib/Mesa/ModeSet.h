#pragma once

#include <cstdint>
#include <ctime>
#include <limits>
#include <list>

#define __GBM__

#ifdef __cplusplus
extern "C" {
#endif

#include <EGL/egl.h>
#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drmMode.h>

#ifdef __cplusplus
}
#endif

#include "Module.h"
#include <interfaces/IComposition.h>

struct gbm_surface;
struct gbm_bo;

namespace WPEFramework {

class ModeSet {
public:
    class DRM {
    public:
        DRM() = delete;
        DRM(DRM const&) = delete;
        DRM& operator=(DRM const&) = delete;

        using fd_t = int;

        using fb_id_t = decltype(drmModeFB::fb_id);
        using crtc_id_t = decltype(drmModeCrtc::crtc_id);
        using enc_id_t = decltype(drmModeEncoder::encoder_id);
        using conn_id_t = decltype(drmModeConnector::connector_id);

        using width_t = decltype(drmModeFB2::width);
        using height_t = decltype(drmModeFB2::height);
        using format_t = decltype(drmModeFB2::pixel_format);
        using modifier_t = decltype(drmModeFB2::modifier);

        using handle_t = std::remove_reference<decltype(*drmModeFB2::handles)>::type;
        using pitch_t = std::remove_reference<decltype(*drmModeFB2::pitches)>::type;
        using offset_t = std::remove_reference<decltype(*drmModeFB2::offsets)>::type;

        using x_t = decltype(drmModeCrtc::x);
        using y_t = decltype(drmModeCrtc::y);

        using duration_t = decltype(timespec::tv_sec);

        static constexpr fd_t InvalidFd() { return static_cast<fd_t>(-1); }

        static constexpr fb_id_t InvalidFb() { return static_cast<fb_id_t>(0); }
        static constexpr crtc_id_t InvalidCrtc() { return static_cast<crtc_id_t>(0); }
        static constexpr enc_id_t InvalidEncoder() { return static_cast<enc_id_t>(0); }
        static constexpr conn_id_t InvalidConnector() { return static_cast<conn_id_t>(0); }

        static constexpr width_t InvalidWidth() { return static_cast<width_t>(0); }
        static constexpr height_t InvalidHeight() { return static_cast<height_t>(0); }

        static constexpr format_t InvalidFormat() { return static_cast<format_t>(DRM_FORMAT_INVALID); }

        static constexpr modifier_t InvalidModifier() { return static_cast<modifier_t>(DRM_FORMAT_MOD_INVALID); }
    };

    class GBM {
    public:
        GBM() = delete;
        GBM(GBM const&) = delete;
        GBM& operator=(GBM const&) = delete;

        // See xf86drmMode.h for magic constant
        using plane_t = decltype(GBM_MAX_PLANES);

        using width_t = decltype(gbm_import_fd_modifier_data::width);
        using height_t = decltype(gbm_import_fd_modifier_data::height);
        using format_t = decltype(gbm_import_fd_modifier_data::format);

        using fd_t = std::remove_reference<decltype(*gbm_import_fd_modifier_data::fds)>::type;
        using stride_t = std::remove_reference<decltype(*gbm_import_fd_modifier_data::strides)>::type;
        using offset_t = std::remove_reference<decltype(*gbm_import_fd_modifier_data::offsets)>::type;
        using modifier_t = decltype(gbm_import_fd_modifier_data::modifier);

        using dev_t = struct gbm_device*;
        using surf_t = struct gbm_surface*;
        using buf_t = struct gbm_bo*;

        using handle_t = decltype(gbm_bo_handle::u32);

        static constexpr dev_t InvalidDev() { return static_cast<dev_t>(nullptr); }
        static constexpr surf_t InvalidSurf() { return static_cast<surf_t>(nullptr); }
        static constexpr buf_t InvalidBuf() { return static_cast<buf_t>(nullptr); }
        static constexpr fd_t InvalidFd() { return static_cast<fd_t>(-1); }

        static constexpr width_t InvalidWidth() { return static_cast<width_t>(0); }
        static constexpr height_t InvalidHeight() { return static_cast<height_t>(0); }
        static constexpr stride_t InvalidStride() { return static_cast<stride_t>(0); }
        static constexpr format_t InvalidFormat() { return static_cast<format_t>(DRM::InvalidFormat()); }
        static constexpr modifier_t InvalidModifier() { return static_cast<modifier_t>(DRM::InvalidModifier()); }
    };

    struct BufferInfo {
        BufferInfo(BufferInfo const&) = delete;
        BufferInfo& operator=(BufferInfo const&) = delete;

        BufferInfo();
        explicit BufferInfo(GBM::surf_t const, GBM::buf_t const, DRM::fb_id_t const);
        // TODO: clean up members
        ~BufferInfo() = default;

        BufferInfo(BufferInfo&&);
        BufferInfo& operator=(BufferInfo&&);

        GBM::surf_t _surface;
        GBM::buf_t _bo;
        DRM::fb_id_t _id;
    };

    ModeSet(ModeSet const&) = delete;
    ModeSet& operator=(ModeSet const&) = delete;

    ModeSet();
    ~ModeSet();

    uint32_t Open(string const&);
    uint32_t Close();

    GBM::dev_t /* const */ UnderlyingHandle() const;

    static std::list<DRM::format_t> AvailableFormats(DRM::fd_t);

    static constexpr GBM::format_t SupportedBufferType() { return static_cast<GBM::format_t>(DRM_FORMAT_ARGB8888); }
    static constexpr GBM::modifier_t FormatModifier() { return static_cast<GBM::modifier_t>(DRM_FORMAT_MOD_LINEAR); }

    DRM::fd_t Descriptor() const;

    uint32_t Width() const;
    uint32_t Height() const;

    uint32_t RefreshRate() const;

    bool Interlaced() const;

    GBM::surf_t CreateRenderTarget(uint32_t const, uint32_t const) const;
    void DestroyRenderTarget(struct BufferInfo&);

    // For objects not included in Swap ()
    void DestroyRenderTarget(GBM::surf_t&) const;

    GBM::buf_t CreateBufferObject(uint32_t const, uint32_t const);
    void DestroyBufferObject(GBM::buf_t&);

    void Swap(struct BufferInfo&);

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
