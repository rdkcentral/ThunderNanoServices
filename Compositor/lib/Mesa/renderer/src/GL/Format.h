#pragma once

#include <stdint.h>
#include <map>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <drm_fourcc.h>

namespace Thunder {
namespace Compositor {
    namespace Renderer {
        struct GLPixelFormat {
            GLPixelFormat(GLenum format, GLenum type, uint32_t bitperpixel, bool alpha)
                : Format(format)
                , Type(type)
                , BitPerPixel(bitperpixel)
                , Alpha(alpha)
            {
            }
            const GLenum Format;
            const GLenum Type;
            const uint32_t BitPerPixel;
            const bool Alpha;
        };

        static const inline GLPixelFormat ConvertFormat(uint32_t drmFormat)
        {
            /*
             * Note: DRM formats are little endian and GL formats are big endian
             */
            static const std::map<uint32_t, GLPixelFormat> formatsRegister{
                { DRM_FORMAT_ARGB8888, GLPixelFormat(GL_BGRA_EXT, GL_UNSIGNED_BYTE, 32, true) },
                { DRM_FORMAT_XRGB8888, GLPixelFormat(GL_BGRA_EXT, GL_UNSIGNED_BYTE, 32, false) },
                { DRM_FORMAT_ABGR8888, GLPixelFormat(GL_RGBA, GL_UNSIGNED_BYTE, 32, true) },
                { DRM_FORMAT_XBGR8888, GLPixelFormat(GL_RGBA, GL_UNSIGNED_BYTE, 32, false) },
                { DRM_FORMAT_BGR888, GLPixelFormat(GL_RGB, GL_UNSIGNED_BYTE, 24, false) }
            };

            const auto& index = formatsRegister.find(drmFormat);

            return (index != formatsRegister.end()) ? index->second : GLPixelFormat(0, 0, 0, false);
        }

    } // namespace Renderer
} // namespace Compositor
} // namespace Thunder