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

namespace {
    #ifdef ENABLE_DUMP
        static void WritePNG(const std::string& filename, uint32_t formatIn, int width, int height, int stride, bool invertY, png_bytep data)
    {
        const struct format {
            uint32_t format;
            bool is_bgr;
        };
        const struct format formats[] = {
            { DRM_FORMAT_XRGB8888, true },
            { DRM_FORMAT_ARGB8888, true },
            { DRM_FORMAT_XBGR8888, false },
            { DRM_FORMAT_ABGR8888, false },
        };
        const struct format* fmt = NULL;

        for (size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
            if (formats[i].format == formatIn) {
                fmt = &formats[i];
                break;
            }
        }

        if (fmt == NULL) {
            fprintf(stderr, "unsupported format %" PRIu32 "\n", formatIn);
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
    static uint32_t Dump(gbm_bo* bo, const uint32_t width, const uint32_t height, const uint32_t format)
    {
        uint32_t stride = 0;
        void* map_data = NULL;

        png_bytep data = reinterpret_cast<png_bytep>(gbm_bo_map(bo, 0, 0, width, height, GBM_BO_TRANSFER_READ, &stride, &map_data));

        if (data) {
            std::stringstream filename;
            filename << "gbm-snapshot-" << Core::Time::Now().Ticks() << ".png" << std::ends;

            Core::File snapshot(filename.str());
            WritePNG(filename.str(), format, width, height, stride, false, data);

            gbm_bo_unmap(_bo, map_data);
        } else {
            TRACE(Thunder::Trace::Buffer, (_T("Dump: failed to map gbm bo")));
        }

        return 0;
    }
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

            TRACE_GLOBAL(Thunder::Trace::Buffer, (_T("GBM buffer %dx%d, format %s (0x%08" PRIX32 "), modifier %s (0x%016" PRIX64 ")"), width, height, formatName ? formatName : "<Unknown>", format, modifierName ? modifierName : "<Unknown>", modifier));

            if (formatName) {
                free(formatName);
            }

            if (modifierName) {
                free(modifierName);
            }
        }
    }
}

namespace Thunder {

    namespace Compositor {

        class GBM : public LocalBuffer {
        public:
            GBM() = delete;
            GBM(GBM&&) = delete;
            GBM(const GBM&) = delete;
            GBM& operator=(GBM&&) = delete;
            GBM& operator=(const GBM&) = delete;

            GBM(const int drmFd, const uint32_t width, const uint32_t height, const PixelFormat& format)
                : LocalBuffer(width, height, format.Type(), 0, Exchange::ICompositionBuffer::TYPE_DMA)
                , _device(nullptr)
                , _bo(nullptr) {
                ASSERT(drmFd != InvalidFileDescriptor);

                if (drmFd != -1) {
                    _device = gbm_create_device(drmFd);

                    ASSERT(_device != nullptr);

                    if (_device != nullptr) {

                        _bo = gbm_bo_create_with_modifiers(_device, width, height, format.Type(), format.Modifiers().data(), format.Modifiers().size());

                        if (_bo == nullptr) {
                            uint32_t usage = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;

                            if ((format.Modifiers().size() == 1) && (format.Modifiers()[0] == DRM_FORMAT_MOD_LINEAR)) {
                                usage |= GBM_BO_USE_LINEAR;
                            }

                            _bo = gbm_bo_create(_device, width, height, format.Type(), usage);
                        }

                        ASSERT(_bo != nullptr);

                        if (_bo != nullptr) {
                            uint8_t maxPlanes = gbm_bo_get_plane_count(_bo);

                            ASSERT (maxPlanes < /*PLANE_COUNT*/ 4);

                            for (uint8_t index = 0; (index < gbm_bo_get_plane_count(_bo)); index++) {
                                #if HAS_GBM_BO_GET_FD_FOR_PLANE
                                int fd = gbm_bo_get_fd_for_plane(_bo, index);
                                #else
                                int fd = gbm_bo_get_fd(_bo);
                                #endif

                                uint32_t offset = gbm_bo_get_offset(_bo, index);
                                uint32_t stride = gbm_bo_get_stride_for_plane(_bo, index);

                                LocalBuffer::Add(fd, stride, offset);

                                ::close(fd);
                            }

                            if (maxPlanes > 0) {
                                // Set Modifier..
                                gbm_bo_get_modifier(_bo);
                                PrintBufferInfo(_bo);
                            }
                            else {
                                TRACE(Trace::Buffer, (_T("Failed to allocate GBM buffer object.")));
                                gbm_bo_destroy(_bo);
                                _bo = nullptr;
                            }
                        }
                    }
                }
            }
            ~GBM() override {
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
            bool IsValid() const {
                return (_bo != nullptr);
            }

        private:
            gbm_device* _device;
            gbm_bo* _bo;
        }; // class GBM

        /* extern */ Core::ProxyType<Exchange::ICompositionBuffer> CreateBuffer(
            Identifier identifier, 
            const uint32_t width, 
            const uint32_t height, 
            const PixelFormat& format)
        {
            // ASSERT(drmAvailable() == 1);

            Core::ProxyType<Exchange::ICompositionBuffer> result;

            int drmFd = DRM::ReopenNode(identifier, true);

            ASSERT(drmFd >= 0);

            if (drmFd >= 0) {
                Core::ProxyType<GBM> entry = Core::ProxyType<GBM>::Create(drmFd, width, height, format);

                if (entry->IsValid() == true) {
                    result = Core::ProxyType<Exchange::ICompositionBuffer>(entry);
                }
            }

            return (result);
        }

    } // namespace Compositor

} // namespace Thunder
