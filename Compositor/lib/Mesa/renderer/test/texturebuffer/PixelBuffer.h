#pragma once

#include <interfaces/ICompositionBuffer.h>
#include <compositorbuffer/CompositorBufferType.h>

#include "Textures.h"
#include <drm_fourcc.h>

namespace Thunder {
namespace Compositor {
    class PixelBuffer : public SimpleBuffer {
    public:
        PixelBuffer() = delete;
        PixelBuffer(PixelBuffer&&) = delete;
        PixelBuffer(const PixelBuffer&) = delete;
        PixelBuffer& operator=(PixelBuffer&&) = delete;
        PixelBuffer& operator=(const PixelBuffer&) = delete;

        PixelBuffer (const Texture::PixelData& source)
            : SimpleBuffer(source.width, source.height, DRM_FORMAT_ABGR8888, DRM_FORMAT_MOD_LINEAR, Exchange::ICompositionBuffer::TYPE_RAW) {
            SimpleBuffer::Add(reinterpret_cast<int>(source.data.data()), source.width * source.bytes_per_pixel, 0);
        }
        ~PixelBuffer() override = default;
    }; // class PixelBuffer

} // namespace Thunder
} // namespace Compositor