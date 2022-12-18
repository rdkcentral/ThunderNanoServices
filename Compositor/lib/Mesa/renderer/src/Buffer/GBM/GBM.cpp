/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
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

#include "../Trace.h"

#include <IAllocator.h>
#include <IBuffer.h>
#include <CompositorTypes.h>

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <DrmCommon.h>

#if HAVE_GBM_MODIFIERS
#ifndef GBM_MAX_PLANES
#define GBM_MAX_PLANES 4
#endif
#endif

MODULE_NAME_ARCHIVE_DECLARATION

namespace Compositor {

namespace Renderer {
    class GBM : public Interfaces::IBuffer {
#if HAVE_GBM_MODIFIERS
        static constexpr uint8_t DmaBufferMaxPlanes = GBM_MAX_PLANES;
#else
        static constexpr uint8_t DmaBufferMaxPlanes = 1;
#endif

    public:
        GBM() = delete;
        GBM(const GBM&) = delete;
        GBM& operator=(const GBM&) = delete;

        GBM(gbm_device* device, uint32_t width, uint32_t height, const PixelFormat& format, uint32_t usage)
            : _bo(nullptr)
            , _adminLock()
            , _fds()
        {
            _bo = gbm_bo_create_with_modifiers(device, width, height,
                format.Type(), format.Modifiers().data(), format.Modifiers().size());

            if (_bo == nullptr) {
                _bo = gbm_bo_create(device, width, height, format.Type(), usage);
            }

            ASSERT(_bo != nullptr);

            if (_bo != nullptr) {
                TRACE(Trace::Buffer, ("GBM buffer %p constructed with %d plane%s", this, Planes(), (Planes() == 1) ? "" : "s"));

                _fds.clear();

                if (Export() == true) {
                    gbm_bo_destroy(_bo);
                    _bo = nullptr;
                    TRACE(WPEFramework::Trace::Error, (_T("Failed to export buffer object.")));
                }
            } else {
                TRACE(WPEFramework::Trace::Error, (_T("Failed to allocate buffer object.")));
            }
        }

        virtual ~GBM()
        {
            for (auto& fd : _fds) {
                if (fd > 0) {
                    close(fd);
                    fd = InvalidFileDescriptor;
                }
            }

            _fds.clear();

            if (_bo != nullptr) {
                gbm_bo_destroy(_bo);
                _bo = nullptr;
            }

            TRACE(Trace::Buffer, ("GBM buffer %p destructed", this));
        }

        uint32_t Lock(uint32_t timeout) const override
        {
            _adminLock.Lock();
            return WPEFramework::Core::ERROR_NONE;
        }

        uint32_t Unlock(uint32_t timeout) const override
        {
            _adminLock.Unlock();
            return WPEFramework::Core::ERROR_NONE;
        }

        bool IsValid() const
        {
            return (_bo != nullptr);
        }

        uint32_t Width() const override
        {
            ASSERT(IsValid() == true);
            return IsValid() ? gbm_bo_get_width(_bo) : 0;
        }

        uint32_t Height() const override
        {
            ASSERT(IsValid() == true);
            return IsValid() ? gbm_bo_get_height(_bo) : 0;
        }

        uint32_t Format() const override
        {
            ASSERT(IsValid() == true);
            return IsValid() ? gbm_bo_get_format(_bo) : DRM_FORMAT_INVALID;
        }

        uint64_t Modifier() const override
        {
            ASSERT(IsValid() == true);
            return IsValid() ? gbm_bo_get_modifier(_bo) : DRM_FORMAT_MOD_INVALID;
        }

        uint8_t Planes() const override
        {
#ifdef HAVE_GBM_MODIFIERS
            ASSERT(IsValid() == true);
            return IsValid() ? gbm_bo_get_plane_count(_bo) : 0;
#else
            return 1;
#endif
        }

        uint32_t Offset(const uint8_t planeId = 0) const override
        {
#ifdef HAVE_GBM_MODIFIERS
            ASSERT(IsValid() == true);
            ASSERT(planeId < Planes());
            return ((IsValid() == true) && (planeId < DmaBufferMaxPlanes)) ? gbm_bo_get_offset(_bo, planeId) : 0;
#else
            return 0;
#endif
        }

        uint32_t Stride(const uint8_t planeId = 0) const override
        {
            ASSERT(IsValid() == true);
#ifdef HAVE_GBM_MODIFIERS
            ASSERT(planeId < Planes());
            return ((IsValid() == true) && (planeId < DmaBufferMaxPlanes)) ? gbm_bo_get_stride_for_plane(_bo, planeId) : 0;
#else
            return IsValid() ? gbm_bo_get_stride(_bo) : 0;
#endif
        }

        int Handle()
        {
            return InvalidFileDescriptor;
        }

        WPEFramework::Core::instance_id Accessor(const uint8_t planeId)
        {
            ASSERT(planeId < Planes());
            return (planeId < Planes()) ? _fds[planeId] : InvalidFileDescriptor;
        }

    private:
        bool Export()
        {
            bool result(false);

            ASSERT((IsValid() == true) && (Planes() > 0));

            int32_t handle = InvalidFileDescriptor;

            const uint8_t maxPlanes(Planes());

            for (int i = 0; i < maxPlanes; i++) {
#if HAS_GBM_BO_GET_FD_FOR_PLANE
                _fds.push_back(gbm_bo_get_fd_for_plane(_bo, i));
#else
                // GBM is lacking a function to get a FD for a given plane. Instead,
                // check all planes have the same handle. We can't use
                // drmPrimeHandleToFD because that messes up handle ref'counting in
                // the user-space driver.
                union gbm_bo_handle plane_handle = gbm_bo_get_handle_for_plane(_bo, i);

                ASSERT(plane_handle.s32 > 0);

                if (i == 0) {
                    handle = plane_handle.s32;
                }

                ASSERT(plane_handle.s32 == handle); // all planes should have the same GEM handle

                _fds.push_back(gbm_bo_get_fd(_bo));
#endif
                TRACE(Trace::Buffer, ("GBM plane %d exported to fd=%d", i, _fds.at(i)));
            }

            return result;
        }

    private:
        mutable WPEFramework::Core::CriticalSection _adminLock;
        gbm_bo* _bo;
        std::vector<int> _fds;
    }; // class GBM

    class GbmAllocator : public Interfaces::IAllocator {
    public:
        GbmAllocator() = delete;
        GbmAllocator(const GbmAllocator&) = delete;
        GbmAllocator& operator=(const GbmAllocator&) = delete;

        GbmAllocator(const int drmFd)
            : _gbmDevice(nullptr)
        {
            ASSERT(drmFd != InvalidFileDescriptor);

            _gbmDevice = gbm_create_device(drmFd);

            ASSERT(_gbmDevice != nullptr);

            TRACE(Trace::Buffer, ("GBM allocator %p constructed gbmDevice=%p fd=%d", this, _gbmDevice, drmFd));
        }

        ~GbmAllocator() override
        {
            if (_gbmDevice != nullptr) {
                gbm_device_destroy(_gbmDevice);
            }

            TRACE(Trace::Buffer, ("GBM allocator %p destructed", this));
        }

        WPEFramework::Core::ProxyType<Interfaces::IBuffer> Create(const uint32_t width, const uint32_t height, const PixelFormat& format) override
        {

            WPEFramework::Core::ProxyType<Interfaces::IBuffer> result;

            uint32_t usage = GBM_BO_USE_RENDERING;

            result = WPEFramework::Core::ProxyType<GBM>::Create(_gbmDevice, width, height, format, usage);

            return result;
        }

    private:
        gbm_device* _gbmDevice;
    };

} // namespace Renderer

WPEFramework::Core::ProxyType<Interfaces::IAllocator> Interfaces::IAllocator::Instance(WPEFramework::Core::instance_id identifier)
{
    ASSERT(drmAvailable() > 0);

    static WPEFramework::Core::ProxyMapType<WPEFramework::Core::instance_id, Interfaces::IAllocator> gbmAllocators;

    int fd = DRM::ReopenNode(static_cast<int>(identifier), true);

    ASSERT(fd > 0);

    return gbmAllocators.Instance<Renderer::GbmAllocator>(identifier, fd);
}
} // namespace Compositor