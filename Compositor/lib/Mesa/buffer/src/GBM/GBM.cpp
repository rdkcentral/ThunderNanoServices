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

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <DRM.h>

MODULE_NAME_ARCHIVE_DECLARATION

namespace WPEFramework {

namespace Compositor {

    namespace Renderer {

        class GBM : public Exchange::ICompositionBuffer {
#if HAVE_GBM_MODIFIERS
            static constexpr uint8_t DmaBufferMaxPlanes = GBM_MAX_PLANES;
#else
            static constexpr uint8_t DmaBufferMaxPlanes = 1;
#endif

            // using Iterator = Core::IteratorType<Planes, Exchange::ICompositionBuffer*, Planes::iterator>;
            class Plane : public Exchange::ICompositionBuffer::IPlane {
            public:
                Plane() = delete;
                Plane(Plane&&) = delete;
                Plane(const Plane&) = delete;
                Plane& operator=(const Plane&) = delete;

                Plane(const GBM& parent, const uint8_t index)
                    : _parent(parent)
                    , _index(index)
                    , _descriptor(InvalidFileDescriptor)
                {
                    _descriptor = parent.Export(_index);
                    TRACE(Trace::Buffer, (_T("Plane[%d] %p constructed [%d, %p]"), _index, this, _descriptor, &_parent));
                }
                ~Plane() override
                {
                    if (_descriptor > 0) {
                        close(_descriptor);
                        _descriptor = InvalidFileDescriptor;
                    }

                    TRACE(Trace::Buffer, (_T("Plane[%d] %p destructed"), _index, this));
                }

            public:
                Exchange::ICompositionBuffer::buffer_id Accessor() const override
                {
                    return (static_cast<Exchange::ICompositionBuffer::buffer_id>(_descriptor));
                }

                uint32_t Offset() const override
                {
                    uint32_t result = _parent.Offset(_index);
                    return (result);
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
            class PlaneIterator : public Exchange::ICompositionBuffer::IIterator {
            public:
                PlaneIterator(PlaneIterator&&) = delete;
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

            public:
                uint8_t Init(GBM& buffer)
                {
                    _planeCount = buffer.PlaneCount();

                    for (uint8_t index = 0; index < _planeCount; index++) {
                        _planes.at(index) = Core::ProxyType<GBM::Plane>::Create(buffer, index);
                    }

                    TRACE(Trace::Buffer, (_T("GBM buffer %p constructed with %d plane%s"), this, _planeCount, (_planeCount == 1) ? "" : "s"));

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
                Exchange::ICompositionBuffer::IPlane* Plane() override
                {
                    ASSERT(IsValid() == true);
                    return _planes.at(_index - 1).operator->();
                }

            private:
                uint32_t _index;
                uint32_t _planeCount;
                std::array<Core::ProxyType<GBM::Plane>, MAX_PLANES> _planes;
            }; // class PlaneIterator

            GBM() = delete;
            GBM(GBM&&) = delete;
            GBM(const GBM&) = delete;
            GBM& operator=(const GBM&) = delete;

            GBM(const int drmFd, const uint32_t width, const uint32_t height, const PixelFormat& format)
                : _adminLock()
                , _device(nullptr)
                , _bo(nullptr)
                , _iterator()
                , _mutex()
                , _drmFd(drmFd)
            {
                ASSERT(drmFd != InvalidFileDescriptor);

                if (drmFd != InvalidFileDescriptor) {
                    _device = gbm_create_device(drmFd);

                    ASSERT(_device != nullptr);
                }

                if (_device != nullptr) {
                    if (::pthread_mutex_init(&_mutex, nullptr) != 0) {
                        // That will be the day, if this fails...
                        ASSERT(false);
                    }

                    _bo = gbm_bo_create_with_modifiers(_device, width, height, format.Type(), format.Modifiers().data(), format.Modifiers().size());

                    if (_bo == nullptr) {
                        uint32_t usage = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;

                        if ((format.Modifiers().size() == 1) && (format.Modifiers()[0] == DRM_FORMAT_MOD_LINEAR)) {
                            usage |= GBM_BO_USE_LINEAR;
                        }

                        _bo = gbm_bo_create(_device, width, height, format.Type(), usage);
                    }
                }

                ASSERT(_bo != nullptr);

                if (_bo != nullptr) {
                    if (_iterator.Init(*this) == 0) {
                        TRACE(Trace::Buffer, (_T("Failed to allocate GBM buffer object.")));
                        gbm_bo_destroy(_bo);
                        _bo = nullptr;
                    }
                }
            }
            ~GBM() override
            {
                if (_bo != nullptr) {
                    gbm_bo_destroy(_bo);
                    _bo = nullptr;
                }

                if (_device != nullptr) {
                    gbm_device_destroy(_device);
                    _device = nullptr;
                }

                TRACE(Trace::Buffer, (_T("GBM buffer %p destructed"), this));
            }

        public:
            bool IsValid() const
            {
                return (_bo != nullptr);
            }
            //
            // Implementation Exchange::ICompositionBuffer
            // ---------------------------------------------------------------------------------------
            uint32_t Identifier() const override
            {
                // can be used to re-open the device again where this buffer was created.
                return _drmFd;
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
            Exchange::ICompositionBuffer::IIterator* Planes(const uint32_t timeoutMs) override
            {
                if (Lock(timeoutMs) == Core::ERROR_NONE) {
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
            void Render() override
            {
            }

            Exchange::ICompositionBuffer::DataType Type() const
            {
                return Exchange::ICompositionBuffer::TYPE_DMA;
            }

        private:
            uint32_t Offset(const uint8_t planeId) const
            {
                uint32_t result = 0;

                ASSERT(IsValid() == true);

#ifdef HAVE_GBM_MODIFIERS
                if ((IsValid() == true) && (planeId < DmaBufferMaxPlanes)) {
                    result = gbm_bo_get_offset(_bo, planeId);
                }
#endif

                return result;
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

            int Export(const uint8_t planeId) const
            {
                ASSERT(IsValid() == true);

                int handle(InvalidFileDescriptor);

#if HAS_GBM_BO_GET_FD_FOR_PLANE
                handle = gbm_bo_get_fd_for_plane(_bo, planeId);
#else
                handle = gbm_bo_get_fd(_bo);
#endif

                TRACE(Trace::Buffer, (_T("GBM plane %d exported to fd=%d"), planeId, handle));

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
                return (result == 0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
            }
            uint32_t Unlock()
            {
                return (pthread_mutex_unlock(&_mutex) == 0) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
            }

        private:
            mutable Core::CriticalSection _adminLock;
            mutable uint32_t _referenceCount;
            gbm_device* _device;
            gbm_bo* _bo;
            PlaneIterator<DmaBufferMaxPlanes> _iterator;
            pthread_mutex_t _mutex;
            int _drmFd;
        }; // class GBM

    } // namespace Renderer

    /* extern */ Core::ProxyType<Exchange::ICompositionBuffer> CreateBuffer(const Compositor::Identifier identifier, const uint32_t width, const uint32_t height, const PixelFormat& format)
    {
        // ASSERT(drmAvailable() == 1);

        Core::ProxyType<Exchange::ICompositionBuffer> result;

        int drmFd = DRM::ReopenNode(identifier, true);

        ASSERT(drmFd >= 0);

        if (drmFd >= 0) {
            Core::ProxyType<Renderer::GBM> entry = Core::ProxyType<Renderer::GBM>::Create(drmFd, width, height, format);

            if (entry->IsValid() == true) {
                result = Core::ProxyType<Exchange::ICompositionBuffer>(entry);
            }
        }

        return (result);
    }

} // namespace Compositor

} // namespace WPEFramework
