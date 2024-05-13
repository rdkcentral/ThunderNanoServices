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

#ifdef ENABLE_DUMP
#include <png.h>
#endif

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

            static void PrintBufferInfo(gbm_bo* buffer)
            {
                if (buffer != nullptr) {
                    const uint32_t height(gbm_bo_get_height(buffer));
                    const uint32_t width(gbm_bo_get_width(buffer));
                    const uint32_t format(gbm_bo_get_format(buffer));
                    const uint64_t modifier(gbm_bo_get_modifier(buffer));

                    char* formatName = drmGetFormatName(format);
                    char* modifierName = drmGetFormatModifierName(modifier);

                    TRACE_GLOBAL(Trace::Buffer, (_T("GBM buffer %dx%d, format %s (0x%08" PRIX32 "), modifier %s (0x%016" PRIX64 ")"), width, height, formatName ? formatName : "<Unknown>", format, modifierName ? modifierName : "<Unknown>", modifier));

                    if (formatName) {
                        free(formatName);
                    }

                    if (modifierName) {
                        free(modifierName);
                    }
                }
            }

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
                    TRACE(Trace::Buffer, (_T("Plane[%d] %p constructed [%d, %p]: stride: %d offset: %d"), _index, this, _descriptor, &_parent, _parent.Stride(_index), _parent.Offset(_index)));
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

                PrintBufferInfo(_bo);
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

            PUSH_WARNING(DISABLE_WARNING_UNUSED_PARAMETERS)
            uint32_t Completed(const bool changed) override
            {
                Unlock();
                // TODO, Implement me!
#ifdef ENABLE_DUMP
                if (changed == true) {
                    Dump();
                }
#endif
                return ~0;
            }
            POP_WARNING()

            void Render() override
            {
                // Nothing to render here.
                ASSERT(false);
            }

            Exchange::ICompositionBuffer::DataType Type() const
            {
                return Exchange::ICompositionBuffer::TYPE_DMA;
            }

        private:
#ifdef ENABLE_DUMP
            struct format {
                uint32_t format;
                bool is_bgr;
            };

            uint32_t Dump()
            {
                uint32_t stride = 0;
                void* map_data = NULL;

                png_bytep data = reinterpret_cast<png_bytep>(gbm_bo_map(_bo, 0, 0, Width(), Height(), GBM_BO_TRANSFER_READ, &stride, &map_data));

                if (data) {
                    std::stringstream filename;
                    filename << "gbm-snapshot-" << Core::Time::Now().Ticks() << ".png" << std::ends;

                    Core::File snapshot(filename.str());
                    WritePNG(filename.str(), Format(), Width(), Height(), stride, false, data);

                    gbm_bo_unmap(_bo, map_data);
                } else {
                    TRACE(Trace::Buffer, (_T("Dump: failed to map gbm bo")));
                }

                return 0;
            }

            void WritePNG(const std::string& filename, uint32_t format, int width, int height, int stride, bool invertY, png_bytep data)
            {

                const struct format formats[] = {
                    { DRM_FORMAT_XRGB8888, true },
                    { DRM_FORMAT_ARGB8888, true },
                    { DRM_FORMAT_XBGR8888, false },
                    { DRM_FORMAT_ABGR8888, false },
                };

                const struct format* fmt = NULL;

                for (size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
                    if (formats[i].format == format) {
                        fmt = &formats[i];
                        break;
                    }
                }

                if (fmt == NULL) {
                    fprintf(stderr, "unsupported format %" PRIu32 "\n", format);
                    return;
                }

                FILE* filePointer = fopen(filename.c_str(), "wb");

                if (filePointer == NULL) {
                    fprintf(stderr, "failed to open output file\n");
                    return;
                }

                png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                png_infop info = png_create_info_struct(png);

                png_init_io(png, filePointer);

                png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA,
                    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                    PNG_FILTER_TYPE_DEFAULT);

                if (fmt->is_bgr) {
                    png_set_bgr(png);
                }

                png_write_info(png, info);

                for (size_t i = 0; i < (size_t)height; ++i) {
                    png_bytep row;
                    if (invertY) {
                        row = data + (height - i - 1) * stride;
                    } else {
                        row = data + i * stride;
                    }
                    png_write_row(png, row);
                }

                png_write_end(png, NULL);

                png_destroy_write_struct(&png, &info);

                // Close stream to flush and release allocated buffers
                fclose(filePointer);
            }
#endif
            uint32_t Offset(const uint8_t planeId) const
            {
                ASSERT(IsValid() == true);
                ASSERT(planeId < DmaBufferMaxPlanes);

                return gbm_bo_get_offset(_bo, planeId);
            }

            uint32_t Stride(const uint8_t planeId) const
            {
                ASSERT(IsValid() == true);
                ASSERT(planeId < DmaBufferMaxPlanes);

                return gbm_bo_get_stride_for_plane(_bo, planeId);
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
