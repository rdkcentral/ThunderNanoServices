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

#include <drm/drm_fourcc.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

MODULE_NAME_ARCHIVE_DECLARATION

namespace Compositor {

constexpr int InvalidFileDescriptor = -1;

namespace Renderer {
    class DRMDumb : public Interfaces::IBuffer {
    public:
        DRMDumb() = delete;
        DRMDumb(const DRMDumb&) = delete;
        DRMDumb& operator=(const DRMDumb&) = delete;

        DRMDumb(const int fdPrimary, uint32_t width, uint32_t height, const PixelFormat& format)
            : _adminLock()
            , _fd(InvalidFileDescriptor)
            , _ptr(nullptr)
            , _size(0)
            , _pitch(0)
            , _handle(0)
            , _offset(0)
            , _width(width)
            , _height(height)
            , _format(format.Type())
        {
            ASSERT(fdPrimary != InvalidFileDescriptor);

            const unsigned int bpp(32); // TODO: derive from _format...

            if (CreateDumbBuffer(fdPrimary, _width, _height, bpp, _handle, _pitch, _size) == 0) {

                // TODO: use Core stuff and error handling here...

                MapDumbBuffer(fdPrimary, _handle, _offset);

                _ptr = mmap(nullptr, _size, PROT_READ | PROT_WRITE, MAP_SHARED, fdPrimary, _offset);
                memset(_ptr, 0, _size);

                drmPrimeHandleToFD(fdPrimary, _handle, DRM_CLOEXEC, &_fd);
            }

            ASSERT(_fd > 0);
            ASSERT(_ptr != nullptr);

            TRACE(Trace::Buffer, ("DRMDumb buffer %p constructed fd=%d data=%p [%dbytes]", this, _fd, _ptr, _size));
        }

        virtual ~DRMDumb()
        {
            if (_ptr != nullptr) {
                munmap(_ptr, _size);
                _ptr = nullptr;
            }

            DestroyDumbBuffer(_fd, _handle);

            _fd = InvalidFileDescriptor;
            _handle = 0;

            TRACE(Trace::Buffer, ("DRMDumb buffer %p destructed", this));
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

        WPEFramework::Core::instance_id Accessor(const uint8_t)
        {
            return reinterpret_cast<WPEFramework::Core::instance_id>(_ptr);
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

        mutable WPEFramework::Core::CriticalSection _adminLock;

        int _fd;
        void* _ptr;
        uint64_t _size;
        uint32_t _pitch;
        uint32_t _handle;
        uint64_t _offset;

        const uint32_t _width;
        const uint32_t _height;
        const uint32_t _format;
    }; // class DRMDumb

    class GbmAllocator : public Interfaces::IAllocator {
    public:
        GbmAllocator() = delete;
        GbmAllocator(const GbmAllocator&) = delete;
        GbmAllocator& operator=(const GbmAllocator&) = delete;

        GbmAllocator(const int drmFd)
            : _fdPrimary(-1)
        {
            ASSERT(drmFd != InvalidFileDescriptor);
            ASSERT(drmGetNodeTypeFromFd(drmFd) == DRM_NODE_PRIMARY);

            uint64_t dumbAvailable(0);

            drmGetCap(drmFd, DRM_CAP_DUMB_BUFFER, &dumbAvailable);

            ASSERT(dumbAvailable == 0);

            _fdPrimary = drmFd;

            TRACE(Trace::Buffer, ("DRMDumb allocator %p constructed _fdPrimary=%d", this, _fdPrimary));
        }

        ~GbmAllocator() override
        {
            if (_fdPrimary != InvalidFileDescriptor) {
                close(_fdPrimary);
                _fdPrimary = InvalidFileDescriptor;
            }

            TRACE(Trace::Buffer, ("DRMDumb allocator %p destructed", this));
        }

        WPEFramework::Core::ProxyType<Interfaces::IBuffer> Create(const uint32_t width, const uint32_t height, const PixelFormat& format) override
        {
            WPEFramework::Core::ProxyType<Interfaces::IBuffer> result;

            result = WPEFramework::Core::ProxyType<DRMDumb>::Create(_fdPrimary, width, height, format);

            return result;
        }

    private:
        int _fdPrimary;
    };

    /*
     * Re-open the DRM node to avoid GEM handle ref'counting issues.
     * See: https://gitlab.freedesktop.org/mesa/drm/-/merge_requests/110
     */

    static int ReopenNode(int fd, bool openRenderNode)
    {
        if (drmIsMaster(fd)) {
            // Only recent kernels support empty leases
            uint32_t lessee_id;
            int lease_fd = drmModeCreateLease(fd, nullptr, 0, O_CLOEXEC, &lessee_id);

            if (lease_fd >= 0) {
                return lease_fd;
            } else if (lease_fd != -EINVAL && lease_fd != -EOPNOTSUPP) {
                TRACE_GLOBAL(WPEFramework::Trace::Error, ("drmModeCreateLease failed"));
                return InvalidFileDescriptor;
            }
            TRACE_GLOBAL(WPEFramework::Trace::Information, ("drmModeCreateLease failed, falling back to plain open"));
        } else {
            TRACE_GLOBAL(WPEFramework::Trace::Information, ("DRM is not in master mode"));
        }

        char* name = nullptr;

        if (openRenderNode) {
            name = drmGetRenderDeviceNameFromFd(fd);
        }

        if (name == nullptr) {
            // Either the DRM device has no render node, either the caller wants a primary node
            name = drmGetDeviceNameFromFd2(fd);

            if (name == nullptr) {
                TRACE_GLOBAL(WPEFramework::Trace::Error, ("drmGetDeviceNameFromFd2 failed"));
                return InvalidFileDescriptor;
            }
        } else {
            TRACE_GLOBAL(WPEFramework::Trace::Information, ("DRM Render Node: %s", name));
        }

        int newFd = open(name, O_RDWR | O_CLOEXEC);

        if (newFd < 0) {
            TRACE_GLOBAL(WPEFramework::Trace::Error, ("Failed to open DRM node '%s'", name));
            free(name);
            return InvalidFileDescriptor;
        } else {
            TRACE_GLOBAL(WPEFramework::Trace::Information, ("DRM Render Node opened: %s", name));
        }

        free(name);

        // If we're using a DRM primary node (e.g. because we're running under the
        // DRM backend, or because we're on split render/display machine), we need
        // to use the legacy DRM authentication mechanism to have the permission to
        // manipulate buffers.
        if (drmGetNodeTypeFromFd(newFd) == DRM_NODE_PRIMARY) {
            drm_magic_t magic;

            int ret(0);

            if (ret = drmGetMagic(newFd, &magic) < 0) {
                TRACE_GLOBAL(WPEFramework::Trace::Error, ("drmGetMagic failed: %s", strerror(ret)));
                close(newFd);
                return InvalidFileDescriptor;
            }

            if (ret = drmAuthMagic(fd, magic) < 0) {
                TRACE_GLOBAL(WPEFramework::Trace::Error, ("drmAuthMagic failed: %s", strerror(ret)));
                close(newFd);
                return InvalidFileDescriptor;
            }
        }

        return newFd;
    }
} // namespace Renderer

WPEFramework::Core::ProxyType<Interfaces::IAllocator>
Interfaces::IAllocator::Instance(WPEFramework::Core::instance_id identifier)
{
    ASSERT(drmAvailable() > 0);

    static WPEFramework::Core::ProxyMapType<WPEFramework::Core::instance_id, Interfaces::IAllocator> gbmAllocators;

    int fd = Renderer::ReopenNode(static_cast<int>(identifier), false);

    ASSERT(fd > 0);

    return gbmAllocators.Instance<Renderer::GbmAllocator>(identifier, fd);
}
} // namespace Compositor