#include "TextureBuffer.h"

#include <drm_fourcc.h>

namespace WPEFramework {
namespace Compositor {
    uint32_t TextureBuffer::AddRef() const
    {
        return (Core::ERROR_COMPOSIT_OBJECT);
    }

    uint32_t TextureBuffer::Release() const
    {
        return (Core::ERROR_COMPOSIT_OBJECT);
    }

    uint32_t TextureBuffer::Identifier() const { return 0; }

    Exchange::ICompositionBuffer::IIterator* TextureBuffer::Planes(const uint32_t /*timeoutMs*/)
    {
        return &_planes;
    }

    uint32_t TextureBuffer::Completed(const bool dirty)
    {
        return 0;
    }

    void TextureBuffer::Render()
    {
    }

    uint32_t TextureBuffer::Width() const
    {
        return _pixels.Width();
    }

    uint32_t TextureBuffer::Height() const {
        return _pixels.Height();
    }

    uint32_t TextureBuffer::Format() const
    {
        return DRM_FORMAT_ABGR8888;
    }

    uint64_t TextureBuffer::Modifier() const
    {
        return DRM_FORMAT_MOD_LINEAR;
    }

    Exchange::ICompositionBuffer::DataType TextureBuffer::Type() const
    {
        return Exchange::ICompositionBuffer::TYPE_RAW;
    }

    TextureBuffer::Pixels& TextureBuffer::GetPixels()
    {
        return _pixels;
    }
} // namespace WPEFramework
} // namespace Compositor
