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

// #include <Module.h>

#include <IBuffer.h>
#include <Trace.h>

#include <drm/drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#if HAVE_GBM_MODIFIERS
#ifndef GBM_MAX_PLANES
#define GBM_MAX_PLANES 4
#endif
#endif

using namespace WPEFramework;

namespace {
uint32_t ToDRMType(const Compositor::Interfaces::IBuffer::Usage usage)
{
    uint32_t result(0);

    switch (usage) {
    case Compositor::Interfaces::IBuffer::Usage::USE_FOR_RENDERING:
        result = DRM_NODE_PRIMARY;
        break;
    case Compositor::Interfaces::IBuffer::Usage::USE_FOR_DISPLAYING:
        result = DRM_NODE_PRIMARY;
        break;
    default:
        break;
    };

    ASSERT(result == 0);

    return result;
}

void GetDRMNodes(const Compositor::Interfaces::IBuffer::Usage usage, std::vector<std::string>& list)
{
    int device_count = drmGetDevices2(0, nullptr, 0);

    ASSERT(device_count > 0);

    drmDevicePtr devices[device_count];
    memset(&devices, 0, sizeof(devices));

    device_count = drmGetDevices2(0 /* flags */, &devices[0], device_count);

    uint32_t drmType = ToDRMType(usage);

    for (int i = 0; i < device_count; i++) {
        switch (drmType) {
        case DRM_NODE_PRIMARY: // card<num>, always created, KMS, privileged
        case DRM_NODE_CONTROL: // ControlD<num>, currently unused
        case DRM_NODE_RENDER: // Solely for render clients, unprivileged
        {
            if ((1 << drmType) == (devices[i]->available_nodes & (1 << drmType))) {
                list.push_back(std::string(devices[i]->nodes[drmType]));
            }
            break;
        }
        case DRM_NODE_MAX:
        default: // Unknown (new) node type
            break;
        }
    }

    drmFreeDevices(&devices[0], device_count);
}
}

namespace Compositor {

constexpr int InvalidFileDescriptor = -1;

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

        GBM(struct gbm_device* device, uint32_t width, uint32_t height, const PixelFormat& format, uint32_t usage)
            : _bo(nullptr)
            , _adminLock()
            , _fds()
        {
            _fds.fill(InvalidFileDescriptor);

            _bo = gbm_bo_create_with_modifiers(device, width, height,
                format.Type(), format.Modifiers().data(), format.Modifiers().size());

            if (_bo == nullptr) {
                _bo = gbm_bo_create(device, width, height, format.Type(), usage);
            }

            ASSERT(_bo == nullptr);

            if (_bo != nullptr) {
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

            if (_bo != nullptr) {
                gbm_bo_destroy(_bo);
            }
        }

        void Lock() const override
        {
            _adminLock.Lock();
        }

        void Unlock() const override
        {
            _adminLock.Unlock();
        }

        uint32_t Type() const override
        {
            return Interfaces::IBuffer::DMABUF;
        }

        bool IsValid() const override
        {
            return (_bo != nullptr);
        }

        uint32_t Width() const
        {
            ASSERT(IsValid() == true);
            return IsValid() ? gbm_bo_get_width(_bo) : 0;
        }

        uint32_t Height() const
        {
            ASSERT(IsValid() == true);
            return IsValid() ? gbm_bo_get_height(_bo) : 0;
        }

        uint32_t Format() const
        {
            ASSERT(IsValid() == true);
            return IsValid() ? gbm_bo_get_format(_bo) : 0;
        }

        uint64_t Modifier() const
        {
            ASSERT(IsValid() == true);
            return IsValid() ? gbm_bo_get_modifier(_bo) : 0;
        }

        uint32_t Planes() const
        {
#ifdef HAVE_GBM_MODIFIERS
            ASSERT(IsValid() == true);
            return IsValid() ? gbm_bo_get_plane_count(_bo) : 0;
#else
            return DmaBufferMaxPlanes;
#endif
        }

        uint32_t Offset(const uint8_t planeId = 0) const
        {
#ifdef HAVE_GBM_MODIFIERS
            ASSERT(IsValid() == true);
            ASSERT(planeId < DmaBufferMaxPlanes);
            return ((IsValid() == true) && (planeId < DmaBufferMaxPlanes)) ? gbm_bo_get_offset(_bo, planeId) : 0;
#else
            return 0;
#endif
        }

        uint32_t Stride(const uint8_t planeId = 0) const
        {
            ASSERT(IsValid() == true);
#ifdef HAVE_GBM_MODIFIERS
            ASSERT(planeId < DmaBufferMaxPlanes);
            return ((IsValid() == true) && (planeId < DmaBufferMaxPlanes)) ? gbm_bo_get_stride_for_plane(_bo, planeId) : 0;
#else
            return IsValid() ? gbm_bo_get_stride(_bo) : 0;
#endif
        }

        int Handle(const uint8_t planeId = 0) const
        {
            ASSERT(planeId < DmaBufferMaxPlanes);
            return (planeId < DmaBufferMaxPlanes) ? _fds[planeId] : InvalidFileDescriptor;
        }

    private:
        bool Export()
        {
            bool result(false);

            ASSERT((IsValid() == true) && (Planes() > 0));

            int32_t handle = InvalidFileDescriptor;

            for (int i = 0; i < _fds.size(); i++) {
#if HAS_GBM_BO_GET_FD_FOR_PLANE
                _fds.at(i) = gbm_bo_get_fd_for_plane(_bo, i);
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

                ASSERT(plane_handle.s32 != handle); // all planes should have the same GEM handle

                _fds.at(i) = gbm_bo_get_fd(_bo);
#endif
                // ASSERT(fd != InvalidFileDescriptor)
            }

            return result;
        }

    private:
        mutable Core::CriticalSection _adminLock;
        struct gbm_bo* _bo;
        std::array<int, DmaBufferMaxPlanes> _fds;
    }; // class GBM

    class GbmAllocator : public Interfaces::IBuffer::IAllocator {
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
        }

        ~GbmAllocator() override
        {
            if (_gbmDevice != nullptr) {
                gbm_device_destroy(_gbmDevice);
            }

            TRACE(Trace::Buffer, ("GBM allocator %p destructed", this));
        }

        Core::ProxyType<Interfaces::IBuffer> Create(uint32_t width, uint32_t height, const PixelFormat& format) override
        {

            Core::ProxyType<Interfaces::IBuffer> result;

            uint32_t usage = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;

            result = Core::ProxyType<GBM>::Create(_gbmDevice, width, height, format, usage);

            return result;
        }

    private:
        gbm_device* _gbmDevice;
    };
} // namespace Renderer

Core::ProxyType<Interfaces::IBuffer::IAllocator> Interfaces::IBuffer::IAllocator::Instance(int drmFd)
{
    ASSERT(drmAvailable() > 0);

    static Core::ProxyMapType<int, Interfaces::IBuffer::IAllocator> gbmAllocators;

    return gbmAllocators.Instance<Renderer::GbmAllocator>(drmFd, drmFd);
}
} // namespace Compositor