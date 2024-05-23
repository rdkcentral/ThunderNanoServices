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

#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

namespace WPEFramework {

namespace Exchange {
    struct ICompositionBuffer;
}

namespace Compositor {

    namespace DRM {
        using Identifier = uintptr_t;
        using Value = uint64_t;
        using PropertyRegister = std::map<std::string, Identifier>;
        using IdentifierRegister = std::vector<Identifier>;

        constexpr Identifier InvalidIdentifier = static_cast<Identifier>(~0);

        enum class PlaneType : uint8_t {
            Cursor = DRM_PLANE_TYPE_CURSOR,
            Primary = DRM_PLANE_TYPE_PRIMARY,
            Overlay = DRM_PLANE_TYPE_OVERLAY,
        };

        extern std::string PropertyString(const PropertyRegister& properties, const bool pretty = false);
        extern std::string IdentifierString(const std::vector<Identifier>& ids, const bool pretty = false);
        extern uint32_t GetPropertyId(PropertyRegister& registry, const std::string& name);
        extern uint32_t GetProperty(const int cardFd, const Identifier object, const Identifier property, Value& value);
        extern uint16_t GetBlobProperty(const int cardFd, const Identifier object, const Identifier property, const uint16_t blobSize, uint8_t blob[]);

        extern void GetNodes(const uint32_t type, std::vector<std::string>& list);
        extern bool HasCapability(const int cardFd, const uint64_t option);
        /*
         * Re-open the DRM node to avoid GEM handle ref'counting issues.
         * See: https://gitlab.freedesktop.org/mesa/drm/-/merge_requests/110
         *
         */
        extern int ReopenNode(int fd, bool openRenderNode);
        extern uint32_t CreateFrameBuffer(const int cardFd, Exchange::ICompositionBuffer* buffer);
        extern void DestroyFrameBuffer(const int cardFd, const uint32_t frameBufferId);
        extern bool HasNode(const drmDevice* device, const char* deviceName);
        extern int OpenGPU(const std::string& gpuNode);
        extern std::string GetNode(const uint32_t type, drmDevice* device);

/* simple stringification operator to make errorcodes human readable */
#define CASE_TO_STRING(value) \
    case value:               \
        return #value;

        static inline const char* FormatString(uint32_t format)
        {
            switch (format) {
                CASE_TO_STRING(DRM_FORMAT_INVALID)
                CASE_TO_STRING(DRM_FORMAT_C8)
                CASE_TO_STRING(DRM_FORMAT_R8)
                CASE_TO_STRING(DRM_FORMAT_R16)
                CASE_TO_STRING(DRM_FORMAT_RG88)
                CASE_TO_STRING(DRM_FORMAT_GR88)
                CASE_TO_STRING(DRM_FORMAT_RG1616)
                CASE_TO_STRING(DRM_FORMAT_GR1616)
                CASE_TO_STRING(DRM_FORMAT_RGB332)
                CASE_TO_STRING(DRM_FORMAT_BGR233)
                CASE_TO_STRING(DRM_FORMAT_XRGB4444)
                CASE_TO_STRING(DRM_FORMAT_XBGR4444)
                CASE_TO_STRING(DRM_FORMAT_RGBX4444)
                CASE_TO_STRING(DRM_FORMAT_BGRX4444)
                CASE_TO_STRING(DRM_FORMAT_ARGB4444)
                CASE_TO_STRING(DRM_FORMAT_ABGR4444)
                CASE_TO_STRING(DRM_FORMAT_RGBA4444)
                CASE_TO_STRING(DRM_FORMAT_BGRA4444)
                CASE_TO_STRING(DRM_FORMAT_XRGB1555)
                CASE_TO_STRING(DRM_FORMAT_XBGR1555)
                CASE_TO_STRING(DRM_FORMAT_RGBX5551)
                CASE_TO_STRING(DRM_FORMAT_BGRX5551)
                CASE_TO_STRING(DRM_FORMAT_ARGB1555)
                CASE_TO_STRING(DRM_FORMAT_ABGR1555)
                CASE_TO_STRING(DRM_FORMAT_RGBA5551)
                CASE_TO_STRING(DRM_FORMAT_BGRA5551)
                CASE_TO_STRING(DRM_FORMAT_RGB565)
                CASE_TO_STRING(DRM_FORMAT_BGR565)
                CASE_TO_STRING(DRM_FORMAT_RGB888)
                CASE_TO_STRING(DRM_FORMAT_BGR888)
                CASE_TO_STRING(DRM_FORMAT_XRGB8888)
                CASE_TO_STRING(DRM_FORMAT_XBGR8888)
                CASE_TO_STRING(DRM_FORMAT_RGBX8888)
                CASE_TO_STRING(DRM_FORMAT_BGRX8888)
                CASE_TO_STRING(DRM_FORMAT_ARGB8888)
                CASE_TO_STRING(DRM_FORMAT_ABGR8888)
                CASE_TO_STRING(DRM_FORMAT_RGBA8888)
                CASE_TO_STRING(DRM_FORMAT_BGRA8888)
                CASE_TO_STRING(DRM_FORMAT_XRGB2101010)
                CASE_TO_STRING(DRM_FORMAT_XBGR2101010)
                CASE_TO_STRING(DRM_FORMAT_RGBX1010102)
                CASE_TO_STRING(DRM_FORMAT_BGRX1010102)
                CASE_TO_STRING(DRM_FORMAT_ARGB2101010)
                CASE_TO_STRING(DRM_FORMAT_ABGR2101010)
                CASE_TO_STRING(DRM_FORMAT_RGBA1010102)
                CASE_TO_STRING(DRM_FORMAT_BGRA1010102)
                CASE_TO_STRING(DRM_FORMAT_XRGB16161616)
                CASE_TO_STRING(DRM_FORMAT_XBGR16161616)
                CASE_TO_STRING(DRM_FORMAT_ARGB16161616)
                CASE_TO_STRING(DRM_FORMAT_ABGR16161616)
                CASE_TO_STRING(DRM_FORMAT_XRGB16161616F)
                CASE_TO_STRING(DRM_FORMAT_XBGR16161616F)
                CASE_TO_STRING(DRM_FORMAT_ARGB16161616F)
                CASE_TO_STRING(DRM_FORMAT_ABGR16161616F)
                CASE_TO_STRING(DRM_FORMAT_AXBXGXRX106106106106)
                CASE_TO_STRING(DRM_FORMAT_YUYV)
                CASE_TO_STRING(DRM_FORMAT_YVYU)
                CASE_TO_STRING(DRM_FORMAT_UYVY)
                CASE_TO_STRING(DRM_FORMAT_VYUY)
                CASE_TO_STRING(DRM_FORMAT_AYUV)
                CASE_TO_STRING(DRM_FORMAT_XYUV8888)
                CASE_TO_STRING(DRM_FORMAT_VUY888)
                CASE_TO_STRING(DRM_FORMAT_VUY101010)
                CASE_TO_STRING(DRM_FORMAT_Y210)
                CASE_TO_STRING(DRM_FORMAT_Y212)
                CASE_TO_STRING(DRM_FORMAT_Y216)
                CASE_TO_STRING(DRM_FORMAT_Y410)
                CASE_TO_STRING(DRM_FORMAT_Y412)
                CASE_TO_STRING(DRM_FORMAT_Y416)
                CASE_TO_STRING(DRM_FORMAT_XVYU2101010)
                CASE_TO_STRING(DRM_FORMAT_XVYU12_16161616)
                CASE_TO_STRING(DRM_FORMAT_XVYU16161616)
                CASE_TO_STRING(DRM_FORMAT_Y0L0)
                CASE_TO_STRING(DRM_FORMAT_X0L0)
                CASE_TO_STRING(DRM_FORMAT_Y0L2)
                CASE_TO_STRING(DRM_FORMAT_X0L2)
                CASE_TO_STRING(DRM_FORMAT_YUV420_8BIT)
                CASE_TO_STRING(DRM_FORMAT_YUV420_10BIT)
                CASE_TO_STRING(DRM_FORMAT_XRGB8888_A8)
                CASE_TO_STRING(DRM_FORMAT_XBGR8888_A8)
                CASE_TO_STRING(DRM_FORMAT_RGBX8888_A8)
                CASE_TO_STRING(DRM_FORMAT_BGRX8888_A8)
                CASE_TO_STRING(DRM_FORMAT_RGB888_A8)
                CASE_TO_STRING(DRM_FORMAT_BGR888_A8)
                CASE_TO_STRING(DRM_FORMAT_RGB565_A8)
                CASE_TO_STRING(DRM_FORMAT_BGR565_A8)
                CASE_TO_STRING(DRM_FORMAT_NV12)
                CASE_TO_STRING(DRM_FORMAT_NV21)
                CASE_TO_STRING(DRM_FORMAT_NV16)
                CASE_TO_STRING(DRM_FORMAT_NV61)
                CASE_TO_STRING(DRM_FORMAT_NV24)
                CASE_TO_STRING(DRM_FORMAT_NV42)
                CASE_TO_STRING(DRM_FORMAT_NV15)
                CASE_TO_STRING(DRM_FORMAT_P210)
                CASE_TO_STRING(DRM_FORMAT_P010)
                CASE_TO_STRING(DRM_FORMAT_P012)
                CASE_TO_STRING(DRM_FORMAT_P016)
#ifdef DRM_FORMAT_P030
                CASE_TO_STRING(DRM_FORMAT_P030)
#endif
                CASE_TO_STRING(DRM_FORMAT_Q410)
                CASE_TO_STRING(DRM_FORMAT_Q401)
                CASE_TO_STRING(DRM_FORMAT_YUV410)
                CASE_TO_STRING(DRM_FORMAT_YVU410)
                CASE_TO_STRING(DRM_FORMAT_YUV411)
                CASE_TO_STRING(DRM_FORMAT_YVU411)
                CASE_TO_STRING(DRM_FORMAT_YUV420)
                CASE_TO_STRING(DRM_FORMAT_YVU420)
                CASE_TO_STRING(DRM_FORMAT_YUV422)
                CASE_TO_STRING(DRM_FORMAT_YVU422)
                CASE_TO_STRING(DRM_FORMAT_YUV444)
                CASE_TO_STRING(DRM_FORMAT_YVU444)

            default:
                return "Unknown Format";
            }
        }

        static inline const char* ModifierVendorString(uint64_t modifier)
        {
            uint32_t vendor = (modifier & 0xff00000000000000ULL) << 56;

            switch (vendor) {
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_NONE)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_INTEL)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_AMD)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_NVIDIA)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_SAMSUNG)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_QCOM)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_VIVANTE)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_BROADCOM)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_ARM)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_ALLWINNER)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_AMLOGIC)
            default:
                return "Unknown Vendor";
            }
        }
#undef CASE_TO_STRING

        static inline bool HasAlpha(uint32_t drmFormat)
        {
            return (
                   (drmFormat == DRM_FORMAT_ARGB4444)
                || (drmFormat == DRM_FORMAT_ABGR4444)
                || (drmFormat == DRM_FORMAT_RGBA4444)
                || (drmFormat == DRM_FORMAT_BGRA4444)
                || (drmFormat == DRM_FORMAT_ARGB1555)
                || (drmFormat == DRM_FORMAT_ABGR1555)
                || (drmFormat == DRM_FORMAT_RGBA5551)
                || (drmFormat == DRM_FORMAT_BGRA5551)
                || (drmFormat == DRM_FORMAT_ARGB8888)
                || (drmFormat == DRM_FORMAT_ABGR8888)
                || (drmFormat == DRM_FORMAT_RGBA8888)
                || (drmFormat == DRM_FORMAT_BGRA8888)
                || (drmFormat == DRM_FORMAT_ARGB2101010)
                || (drmFormat == DRM_FORMAT_ABGR2101010)
                || (drmFormat == DRM_FORMAT_RGBA1010102)
                || (drmFormat == DRM_FORMAT_BGRA1010102)
                || (drmFormat == DRM_FORMAT_ARGB16161616)
                || (drmFormat == DRM_FORMAT_ABGR16161616)
                || (drmFormat == DRM_FORMAT_ARGB16161616F)
                || (drmFormat == DRM_FORMAT_ABGR16161616F));
        }

    } // namespace DRM
} // namespace Compositor
} // namespace WPEFramework
