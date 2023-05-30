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
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <EGL/egl.h>

#ifdef __cplusplus
}
#endif

#include "Module.h"
#include <interfaces/IComposition.h>

#include <WPEFramework/compositorclient/traits.h>


struct gbm_surface;
struct gbm_bo;

namespace WPEFramework {

    class ModeSet {
    public :

        class DRM {
        public :

            DRM();
            ~DRM();

            DRM(const DRM&) = delete;
            DRM& operator=(const DRM&) = delete;

            DRM(DRM&&) = delete;
            DRM& operator=(DRM&&) = delete;

            static constexpr int InvalidFileDescriptor() { return -1; }

            static constexpr uint32_t InvalidFramebuffer() { return 0; }
            static constexpr uint32_t InvalidCrtc() { return 0; }
            static constexpr uint32_t InvalidEncoder() { return 0; }
            static constexpr uint32_t InvalidConnector() { return 0; }

            static constexpr uint32_t InvalidWidth() { return 0; }
            static constexpr uint32_t InvalidHeight() { return 0; }

            static_assert(   !narrowing<decltype(DRM_FORMAT_INVALID), uint32_t, true>::value
                          || (   DRM_FORMAT_INVALID >= 0
                              && in_unsigned_range<uint32_t, DRM_FORMAT_INVALID>::value
                             )
                          );
            static constexpr uint32_t InvalidFormat() { return static_cast<uint32_t>(DRM_FORMAT_INVALID); }

            static_assert(   !narrowing<decltype(DRM_FORMAT_MOD_INVALID), uint64_t, true>::value
                          || (   DRM_FORMAT_MOD_INVALID >= 0
                              && in_unsigned_range<uint64_t, DRM_FORMAT_MOD_INVALID>::value
                             )
                          );
            static constexpr uint64_t InvalidModifier() { return static_cast<uint64_t>(DRM_FORMAT_MOD_INVALID); }

            static_assert(   !narrowing<decltype(DRM_FORMAT_MOD_LINEAR), uint64_t, true>::value
                          || (   DRM_FORMAT_MOD_LINEAR >= 0
                              && in_unsigned_range<uint64_t, DRM_FORMAT_MOD_LINEAR>::value
                             )
                          );
            static constexpr uint64_t FormatModifier() { return static_cast<uint64_t>(DRM_FORMAT_MOD_LINEAR); }

            // An odd duck to expose
            int FileDescriptor() const { return _fd; }

            uint32_t DisplayWidth() const{ return _width; }
            uint32_t DisplayHeight() const { return _height; }
            uint32_t DisplayRefreshRate() const { return _vrefresh; }

            bool Open(uint32_t type);

            bool AddFramebuffer(struct gbm_bo*);
            bool RemoveFramebuffer(struct gbm_bo* bo);

            bool EnableDisplay() const;
            bool DisableDisplay() const;

            bool ShowFramebuffer(struct gbm_bo*) const;

            bool IsValid() const;

        private :

            void AvailableNodes(uint32_t type, std::vector<std::string>& list) const;
            bool Close();

            bool InitializeDisplayProperties();
            bool PreferredDisplayMode();

            int _fd;

            uint32_t _crtc;
            uint32_t _encoder;
            uint32_t _connector;
            uint32_t _fb;
            uint32_t _mode;

            uint32_t _width;
            uint32_t _height;
            uint32_t _vrefresh;

            std::map<uint32_t, uint32_t> _handle2fb;
        };

        class GBM {
        public :

            GBM(DRM& drm);
            ~GBM();

            GBM(const GBM&) = delete;
            GBM& operator=(const GBM&) = delete;

            GBM(GBM&&) = delete;
            GBM& operator=(GBM&&) = delete;

            static constexpr struct gbm_device* InvalidDevice() { return nullptr; }
            static constexpr struct gbm_surface* InvalidSurface() { return nullptr; }
            static constexpr struct gbm_bo* InvalidBuffer() { return nullptr ; }
            static constexpr int InvalidFileDescriptor() { return -1; }

            static constexpr uint32_t InvalidWidth() { return DRM::InvalidWidth(); }
            static constexpr uint32_t InvalidHeight() { return DRM::InvalidHeight(); }
            static constexpr uint32_t InvalidStride() { return 0; }

            static constexpr uint32_t InvalidFormat() { return DRM::InvalidFormat(); }
            static constexpr uint64_t InvalidModifier() { return DRM::InvalidModifier(); }

            static constexpr uint64_t FormatModifier() { return DRM::FormatModifier(); }

            static_assert(   !narrowing<decltype(DRM_FORMAT_ARGB8888), uint32_t, true>::value
                          || (   DRM_FORMAT_ARGB8888 >= 0
                              && in_unsigned_range<uint32_t, DRM_FORMAT_ARGB8888>::value
                             )
                          );
            static constexpr uint32_t SupportedBufferType () { return static_cast<uint32_t>(DRM_FORMAT_ARGB8888); }

            struct gbm_device* Device() const { return _device; }

            bool CreateSurface(uint32_t width, uint32_t height, struct gbm_surface*& surface) const;
            bool DestroySurface(struct gbm_surface*& surface) const;

            bool CreateBuffer(struct gbm_bo*&);
            bool CreateBufferFromSurface(struct gbm_surface*, struct gbm_bo*&);
            bool DestroyBuffer(struct gbm_bo* bo);
            bool DestroyBufferFromSurface(struct gbm_surface*, struct gbm_bo*&);

            bool IsValid() const { return _device != InvalidDevice(); }

        private :

            bool Device(struct gbm_device*& device) const;

            bool Lock(struct gbm_surface*, struct gbm_bo*&);
            bool Unlock(struct gbm_surface*, struct gbm_bo*&);

            DRM& _drm;

            mutable struct gbm_device* _device;

            std::vector<struct gbm_bo*> _bos;
            mutable std::vector<struct gbm_surface*> _surfaces;
            mutable std::map<struct gbm_bo*, struct gbm_surface*> _bo2surface;
        };

        ModeSet();
        ~ModeSet();

        ModeSet(const ModeSet&) = delete;
        ModeSet& operator=(const ModeSet&) = delete;

        ModeSet(ModeSet&&) = delete;
        ModeSet& operator=(ModeSet&&) = delete;

        uint32_t Open();
        uint32_t Close();

        struct gbm_device* UnderlyingHandle() const;

        int Descriptor() const;

        uint32_t Width() const;
        uint32_t Height() const;

        uint32_t RefreshRate() const;

        bool Interlaced() const;

        struct gbm_surface* CreateRenderTarget(uint32_t, uint32_t) const;

        void DestroyRenderTarget(struct gbm_surface*&) const;

        struct gbm_bo* CreateBufferObject(uint32_t, uint32_t);
        void DestroyBufferObject(struct gbm_bo*&);

        void Swap(struct gbm_surface*);

    private :

        DRM _drm;
        GBM _gbm;

        struct gbm_bo* _bo;
    };

}
