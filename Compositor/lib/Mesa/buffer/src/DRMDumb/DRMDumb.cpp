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

#include "../Module.h"

#include <IBuffer.h>
#include <interfaces/ICompositionBuffer.h>
#include <CompositorTypes.h>
#include <DrmCommon.h>

#include <drm_fourcc.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

MODULE_NAME_ARCHIVE_DECLARATION

namespace Thunder {
namespace Compositor {
namespace Renderer {
    class DRMDumb : public Exchange::ICompositionBuffer {
    public:
        DRMDumb() = delete;
        DRMDumb(DRMDumb&&) = delete;
        DRMDumb(const DRMDumb&) = delete;
        DRMDumb& operator=(const DRMDumb&) = delete;

        DRMDumb(const int drmFd, uint32_t width, uint32_t height, const PixelFormat& format)
            : _adminLock()
            , _fd(InvalidFileDescriptor)
            , _fdPrimary(drmFd)
            , _ptr(nullptr)
            , _size(0)
            , _pitch(0)
            , _handle(0)
            , _offset(0)
            , _width(width)
            , _height(height)
            , _format(format.Type())
        {
            ASSERT(drmFd != InvalidFileDescriptor);
            ASSERT(drmGetNodeTypeFromFd(drmFd) == DRM_NODE_PRIMARY);

            if (_fdPrimary != InvalidFileDescriptor) {
                uint64_t dumbAvailable(0);

                drmGetCap(_fdPrimary, DRM_CAP_DUMB_BUFFER, &dumbAvailable);

                ASSERT(dumbAvailable != 0);

                if (dumbAvailable != 0) {
                    const unsigned int bpp(32); // TODO: derive from _format...

                    if (CreateDumbBuffer(_fdPrimary, _width, _height, bpp, _handle, _pitch, _size) == 0) {

                        // TODO: use Core stuff and error handling here...

                        MapDumbBuffer(_fdPrimary, _handle, _offset);

                        _ptr = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fdPrimary, _offset);
                        memset(_ptr, 0, _size);

                        drmPrimeHandleToFD(_fdPrimary, _handle, DRM_CLOEXEC, &_fd);
                    }
                }
            }

            ASSERT(_fd > 0);
            ASSERT(_ptr != nullptr);

            TRACE(Trace::Buffer, ("DRMDumb buffer %p constructed fd=%d data=%p [%dbytes]", this, _fd, _ptr, _size));
        }
        ~DRMDumb() override
        {
            if (_ptr != nullptr) {
                munmap(_ptr, _size);
                _ptr = nullptr;
            }

            if (_fd != InvalidFileDescriptor) {
                DestroyDumbBuffer(_fd, _handle);
                _fd = InvalidFileDescriptor;
                _handle = 0;
            }

            if (_fdPrimary != InvalidFileDescriptor) {
                close(_fdPrimary);
                _fdPrimary = InvalidFileDescriptor;
            }
 
            TRACE(Trace::Buffer, ("DRMDumb buffer %p destructed", this));
        }

    public:
        bool IsValid() const {
            return (_ptr != nullptr);
        }
        uint32_t Lock(uint32_t timeout) const override
        {
            _adminLock.Lock();
            return Core::ERROR_NONE;
        }

        uint32_t Unlock(uint32_t timeout) const override
        {
            _adminLock.Unlock();
            return Core::ERROR_NONE;
        }

        bool IsValid() const
        {
            return (_fd != InvalidFileDescriptor);
        }

        uint32_t Width() const override
        {
            ASSERT(IsValid() == true);
            return IsValid() ? _width : 0;
        }

        uint32_t Height() const override
        {
            ASSERT(IsValid() == true);
            return IsValid() ? _height : 0;
        }

        uint32_t Format() const override
        {
            ASSERT(IsValid() == true);
            return IsValid() ? _format : DRM_FORMAT_INVALID;
        }

        uint64_t Modifier() const override
        {
            ASSERT(IsValid() == true);
            return IsValid() ? DRM_FORMAT_MOD_LINEAR : DRM_FORMAT_MOD_INVALID;
        }

        uint8_t Planes() const override
        {
            return 1;
        }

        uint32_t Offset(const uint8_t) const override
        {
            return _offset;
        }

        uint32_t Stride(const uint8_t) const override
        {
            ASSERT(IsValid() == true);
            return IsValid() ? _pitch : 0;
        }

        int Handle()
        {
            return InvalidFileDescriptor;
        }

        Core::instance_id Accessor(const uint8_t)
        {
            return reinterpret_cast<Core::instance_id>(_ptr);
        }

        Exchange::ICompositionBuffer::DataType Type() const
        {
            return Exchange::ICompositionBuffer::TYPE_DMA;
        }

    private:
        int CreateDumbBuffer(const int fd,
            const uint32_t width, const uint32_t height, const uint32_t bpp,
            uint32_t& handle, uint32_t& pitch, uint64_t& size)
        {
#ifndef LIBDRM_DUMB_BUFFER_HELPERS
            int result(0);

            struct drm_mode_create_dumb create;
            memset(&create, 0, sizeof(create));

            create.width = width;
            create.height = height;
            create.bpp = bpp;

            if (result = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) != 0) {
                ASSERT(width == create.width);
                ASSERT(height == create.height);

                pitch = create.pitch;
                handle = create.handle;
                size = create.size;
            }

            return result;
#else
            return drmModeCreateDumbBuffer(fd, width, height, bpp, 0, &handle, &pitch, &size);
#endif
        }

        int MapDumbBuffer(const int fd, const uint32_t handle, uint64_t& offset)
        {
#ifndef LIBDRM_DUMB_BUFFER_HELPERS
            int result(0);

            struct drm_mode_map_dumb map = { 0 };
            memset(&map, 0, sizeof(map));

            map.handle = handle;

            if (result = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) != 0) {
                offset = map.offset;
            }
            return result;
#else
            return drmModeMapDumbBuffer(fd, handle, offset);
#endif
        }

        int DestroyDumbBuffer(const int fd, const uint32_t handle)
        {
#ifndef LIBDRM_DUMB_BUFFER_HELPERS
            struct drm_mode_destroy_dumb destroy;
            memset(&destroy, 0, sizeof(destroy));

            destroy.handle = handle;

            return drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
#else
            return drmModeDestroyDumbBuffer(fd, handle);
#endif
        }

        mutable Core::CriticalSection _adminLock;

        int _fd;
        int _fdPrimary;
        void* _ptr;
        uint64_t _size;
        uint32_t _pitch;
        uint32_t _handle;
        uint64_t _offset;

        const uint32_t _width;
        const uint32_t _height;
        const uint32_t _format;
    }; // class DRMDumb

} // namespace Renderer

Core::ProxyType<Exchange::ICompositionBuffer> CreateBuffer (DRM::Identifier identifier, const uint32_t width, const uint32_t height, const PixelFormat& format)
{
    ASSERT(drmAvailable() > 0);

    Core::ProxyType<Exchange::ICompositionBuffer> result;
    int fd = DRM::ReopenNode(static_cast<int>(identifier), false);

    ASSERT(fd >= 0);

    if (fd >= 0) {
        Core::ProxyType<DRMDumb> entry = Core::ProxyType<DRMDumb>::Create(fd, width, height, format);
        if (entry->IsValid() == false) {
            result = Core::ProxyType<Exchange::ICompositionBuffer>(entry);
        }
    }

    return (result);
}

} // namespace Compositor
} //namespace Thunder
