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

#include <CompositorTypes.h>
#include <DrmCommon.h>
#include <IAllocator.h>

#include <compositorbuffer/IBuffer.h>

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

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
        // using Iterator = WPEFramework::Core::IteratorType<Planes, Interfaces::IBuffer*, Planes::iterator>;
        class Plane : public Interfaces::IBuffer::IPlane {
        public:
            Plane() = delete;
            Plane(const Plane&) = delete;
            Plane& operator=(const Plane&) = delete;

            Plane(const GBM& parent, const uint8_t index)
                : _parent(parent)
                , _index(index)
                , _descriptor(InvalidFileDescriptor)
            {
                _descriptor = parent.Export(_index);
                TRACE(Trace::Buffer, ("Plane[%d] %p constructed", _index, this));
            }

            virtual ~Plane()
            {
                if (_descriptor > 0) {
                    close(_descriptor);
                    _descriptor = InvalidFileDescriptor;
                }

                TRACE(Trace::Buffer, ("Plane[%d] %p destructed", _index, this));
            }

            WPEFramework::Core::instance_id Accessor() const override
            {
                return static_cast<WPEFramework::Core::instance_id>(_descriptor);
            }

            uint32_t Offset() const override
            {
                return _parent.Offset(_index);
            }

            uint32_t Stride() const override
            {
                return _parent.Stride(_index);
            }

        private:
            const GBM& _parent;
            const uint8_t _index;
            int _descriptor;
        };

    public:
        template <const uint8_t MAX_PLANES>
        class PlaneIterator : public Interfaces::IBuffer::IIterator {

        public:
            PlaneIterator(const PlaneIterator&) = delete;
            PlaneIterator& operator=(const PlaneIterator&) = delete;

            PlaneIterator()
                : _index(0)
                , _planeCount(0)
                , _planes()
            {
            }

            ~PlaneIterator() override
            {
                for (auto plane : _planes) {
                    if (plane.IsValid()) {
                        plane.Release();
                    }
                }
            }

            uint8_t Init(GBM& buffer)
            {
                _planeCount = buffer.PlaneCount();

                for (uint8_t index = 0; index < _planeCount; index++) {
                    _planes.at(index) = WPEFramework::Core::ProxyType<GBM::Plane>::Create(buffer, index);
                }

                TRACE(Trace::Buffer, ("GBM buffer %p constructed with %d plane%s", this, _planeCount, (_planeCount == 1) ? "" : "s"));

                return _planeCount;
            }

        public:
            void Reset() override
            {
                _index = 0;
            }
            bool IsValid() const override
            {
                return ((_index != 0) && (_index <= _planeCount));
            }
            bool Next() override
            {
                if (_index == 0) {
                    _index = 1;
                } else if (_index <= _planeCount) {
                    _index++;
                }
                return (IsValid());
            }
            Interfaces::IBuffer::IPlane* Plane() override
            {
                ASSERT(IsValid() == true);
                return _planes.at(_index - 1).operator->();
            }

        private:
            uint32_t _index;
            uint32_t _planeCount;
            std::array<WPEFramework::Core::ProxyType<GBM::Plane>, MAX_PLANES> _planes;
        }; // class PlaneIterator

        GBM() = delete;
        GBM(const GBM&) = delete;
        GBM& operator=(const GBM&) = delete;

        GBM(gbm_device* device, uint32_t width, uint32_t height, const PixelFormat& format, uint32_t usage)
            : _adminLock()
            , _referenceCount(0)
            , _bo(nullptr)
            , _iterator()
        {

            if (::pthread_mutex_init(&_mutex, nullptr) != 0) {
                // That will be the day, if this fails...
                ASSERT(false);
            }

            AddRef();

            _bo = gbm_bo_create_with_modifiers(device, width, height, format.Type(), format.Modifiers().data(), format.Modifiers().size());

            if (_bo == nullptr) {
                _bo = gbm_bo_create(device, width, height, format.Type(), usage);
            }

            ASSERT(_bo != nullptr);

            if ((_bo == nullptr) || (_iterator.Init(*this)) == 0) {
                TRACE(WPEFramework::Trace::Error, (_T("Failed to allocate GBM buffer object.")));
                Release();
            }
        }

        virtual ~GBM()
        {

            if (_bo != nullptr) {
                gbm_bo_destroy(_bo);
                _bo = nullptr;
            }

            TRACE(Trace::Buffer, ("GBM buffer %p destructed", this));
        }

    public:
        Interfaces::IBuffer::IIterator* Planes(const uint32_t timeoutMs) override
        {
            if (Lock(timeoutMs) == WPEFramework::Core::ERROR_NONE) {
                _iterator.Reset();
                return (&_iterator);
            }
            return (nullptr);
        }

        uint32_t Completed(const bool /*changed*/) override
        {
            Unlock();
            // TODO, Implement me!
            return ~0;
        }

        void AddRef() const override
        {
            WPEFramework::Core::InterlockedIncrement(_referenceCount);
        }

        uint32_t Release() const override
        {
            ASSERT(_referenceCount > 0);
            uint32_t result(WPEFramework::Core::ERROR_NONE);

            if (WPEFramework::Core::InterlockedDecrement(_referenceCount) == 0) {
                delete this;
                result = WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
            }

            return result;
        }

        uint32_t Identifier() const override
        {
            // TODO, Implement me!
            return ~0;
        }

        void Render() override { }

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

    private:
        bool IsValid() const
        {
            return (_bo != nullptr);
        }

        uint32_t Offset(const uint8_t planeId) const
        {
            ASSERT(IsValid() == true);
#ifdef HAVE_GBM_MODIFIERS
            return ((IsValid() == true) && (planeId < DmaBufferMaxPlanes)) ? gbm_bo_get_offset(_bo, planeId) : 0;
#else
            return 0;
#endif
        }

        uint32_t Stride(const uint8_t planeId) const
        {
            ASSERT(IsValid() == true);
#ifdef HAVE_GBM_MODIFIERS
            return ((IsValid() == true) && (planeId < DmaBufferMaxPlanes)) ? gbm_bo_get_stride_for_plane(_bo, planeId) : 0;
#else
            return IsValid() ? gbm_bo_get_stride(_bo) : 0;
#endif
        }

        int Export(uint8_t planeId) const
        {
            ASSERT(IsValid() == true);

            int handle(InvalidFileDescriptor);

#if HAS_GBM_BO_GET_FD_FOR_PLANE
            handle = gbm_bo_get_fd_for_plane(_bo, planeId);
#else
            handel = gbm_bo_get_fd(_bo);
#endif
            TRACE(Trace::Buffer, ("GBM plane %d exported to fd=%d", planeId, handle));

            return handle;
        }

        uint32_t PlaneCount() const
        {
            ASSERT(_bo != nullptr);
            return (_bo != nullptr) ? gbm_bo_get_plane_count(_bo) : 0;
        }

        uint32_t Lock(uint32_t timeout)
        {
            timespec structTime;

            clock_gettime(CLOCK_MONOTONIC, &structTime);
            structTime.tv_nsec += ((timeout % 1000) * 1000 * 1000); /* remainder, milliseconds to nanoseconds */
            structTime.tv_sec += (timeout / 1000) + (structTime.tv_nsec / 1000000000); /* milliseconds to seconds */
            structTime.tv_nsec = structTime.tv_nsec % 1000000000;
            int result = pthread_mutex_timedlock(&_mutex, &structTime);
            return (result == 0 ? WPEFramework::Core::ERROR_NONE : WPEFramework::Core::ERROR_TIMEDOUT);
        }
        uint32_t Unlock()
        {
            return (pthread_mutex_unlock(&_mutex) == 0) ? WPEFramework::Core::ERROR_NONE : WPEFramework::Core::ERROR_GENERAL;
        }

    private:
        mutable WPEFramework::Core::CriticalSection _adminLock;
        mutable uint32_t _referenceCount;
        gbm_bo* _bo;
        PlaneIterator<DmaBufferMaxPlanes> _iterator;
        pthread_mutex_t _mutex;
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

        // Interfaces::IBuffer*
        WPEFramework::Core::ProxyType<Interfaces::IBuffer> Create(const uint32_t width, const uint32_t height, const PixelFormat& format) override
        {
            WPEFramework::Core::ProxyType<Interfaces::IBuffer> result;

            uint32_t usage = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT;

            result = WPEFramework::Core::ProxyType<GBM>::Create(_gbmDevice, width, height, format, usage);

            // Interfaces::IBuffer* result = new GBM(_gbmDevice, width, height, format, usage);

            return result;
        }

    private:
        gbm_device* _gbmDevice;
    };

} // namespace Renderer

WPEFramework::Core::ProxyType<Interfaces::IAllocator> Interfaces::IAllocator::Instance(WPEFramework::Core::instance_id identifier)
{
    ASSERT(drmAvailable() == 1);

    static WPEFramework::Core::ProxyMapType<WPEFramework::Core::instance_id, Interfaces::IAllocator> gbmAllocators;

    int fd = DRM::ReopenNode(static_cast<int>(identifier), true);

    ASSERT(fd > 0);

    return gbmAllocators.Instance<Renderer::GbmAllocator>(identifier, fd);
}
} // namespace Compositor