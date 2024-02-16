#include "PixelBuffer.h"

#include <drm_fourcc.h>

namespace WPEFramework {
namespace Compositor {
    uint32_t PixelBuffer::AddRef() const
    {
        return Core::ERROR_NONE;
    }

    uint32_t PixelBuffer::Release() const
    {
        return Core::ERROR_NONE;
    }

    uint32_t PixelBuffer::Identifier() const { return 0; }

    Exchange::ICompositionBuffer::IIterator* PixelBuffer::Planes(const uint32_t /*timeoutMs*/)
    {
        return &_planes;
    }

    uint32_t PixelBuffer::Completed(const bool /*dirty*/)
    {
        return 0;
    }

    void PixelBuffer::Render()
    {
    }

    uint32_t PixelBuffer::Width() const
    {
        return _pixels.Width();
    }

    uint32_t PixelBuffer::Height() const {
        return _pixels.Height();
    }

    uint32_t PixelBuffer::Format() const
    {
        return DRM_FORMAT_ABGR8888;
    }

    uint64_t PixelBuffer::Modifier() const
    {
        return DRM_FORMAT_MOD_LINEAR;
    }

    Exchange::ICompositionBuffer::DataType PixelBuffer::Type() const
    {
        return Exchange::ICompositionBuffer::TYPE_RAW;
    }

    PixelBuffer::Pixels& PixelBuffer::GetPixels()
    {
        return _pixels;
    }
} // namespace WPEFramework
} // namespace Compositor