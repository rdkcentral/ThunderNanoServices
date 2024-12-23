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

namespace {
    static int CreateDumbBuffer(const int fd, const uint32_t width, const uint32_t height, const uint32_t bpp, uint32_t& pitch, uint32_t& handle, uint64_t& size)
    {
        #ifndef LIBDRM_DUMB_BUFFER_HELPERS
        int result(0);

        struct drm_mode_create_dumb create;
        memset(&create, 0, sizeof(create));

        create.width = width;
        create.height = height;
        create.bpp = bpp;

        if (result = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) == 0) {
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

    static int MapDumbBuffer(const int fd, uint32_t handle, uint32_t& offset)
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
        return drmModeMapDumbBuffer(fd, handle, &offset);
        #endif
    }

    static int DestroyDumbBuffer(const int fd, const uint32_t handle)
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
}

namespace Thunder {
    namespace Compositor {
        class DRMDumb : public LocalBuffer {
        public:
            DRMDumb() = delete;
            DRMDumb(DRMDumb&&) = delete;
            DRMDumb(const DRMDumb&) = delete;
            DRMDumb& operator=(DRMDumb&&) = delete;
            DRMDumb& operator=(const DRMDumb&) = delete;

            DRMDumb(const int drmFd, uint32_t width, uint32_t height, const PixelFormat& format)
                : LocalBuffer(width, height, format.Type(), DRM_FORMAT_MOD_LINEAR, Exchange::ICompositionBuffer::TYPE_DMA)
            {
                ASSERT(drmFd != -1);
                ASSERT(drmGetNodeTypeFromFd(drmFd) == DRM_NODE_PRIMARY);

                if (drmFd != -1) {
                    uint64_t dumbAvailable(0);

                    drmGetCap(drmFd, DRM_CAP_DUMB_BUFFER, &dumbAvailable);

                    ASSERT(dumbAvailable != 0);

                    if (dumbAvailable != 0) {
                        uint32_t pitch;
                        uint64_t size;
                        const unsigned int bpp(32); // TODO: derive from _format...

                        if (CreateDumbBuffer(drmFd, width, height, bpp, pitch, _handle, size) == 0) {

                            uint32_t offset;

                            // Severly changed the order, assume that the _fd can be used 
                            // for exchange and use from now on..
                            drmPrimeHandleToFD(drmFd, _handle, DRM_CLOEXEC, &_fd);

                            if (_fd != -1) {
                                // TODO: use Core stuff and error handling here...
                                MapDumbBuffer(_fd, _handle, offset);

                                Compositor::Add(_fd, 0, offset);
                            }
                            TRACE(Trace::Buffer, ("DRMDumb buffer %p constructed fd=%d [%dbytes]", this, _fd, size));
                        }
                    }
                }

            }
            ~DRMDumb() override
            {
                if (_fd != -1) {
                    DestroyDumbBuffer(_fd, _handle);
                    _fd = -1;
                    _handle = 0;
                }

                TRACE(Trace::Buffer, ("DRMDumb buffer %p destructed", this));
            }

        public:
            bool IsValid() const {
                return (_fd != -1);
            }

        private:
            uint32_t _handle;
            int _fd;
        }; // class DRMDumb

        Core::ProxyType<CompositorBuffer> CreateBuffer (
            DRM::Identifier identifier, 
            const uint32_t width, 
            const uint32_t height, 
            const PixelFormat& format)
        {
            ASSERT(drmAvailable() > 0);

            Core::ProxyType<Exchange::ICompositionBuffer> result;
            int fd = DRM::ReopenNode(static_cast<int>(identifier), false);

            ASSERT(fd >= 0);

            if (fd >= 0) {
                Core::ProxyType<DRMDumb> entry = Core::ProxyType<DRMDumb>::Create(fd, width, height, format, callback);
                if (entry->IsValid() == false) {
                    result = Core::ProxyType<Exchange::ICompositionBuffer>(entry);
                }
                ::close(fd);
            }

            return (result);
        }
    } // namespace Compositor
} //namespace Thunder
