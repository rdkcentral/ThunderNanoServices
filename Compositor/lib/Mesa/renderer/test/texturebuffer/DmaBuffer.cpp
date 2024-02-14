#include "DmaBuffer.h"

#include "../../src/GL/Format.h"

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

#include <png.h>

namespace WPEFramework {
namespace Compositor {
    DmaBuffer::DmaBuffer(int gpuFd, const Texture::PixelData& source)
        : _id(0)
        , _api()
        , _egl(gpuFd)
        , _plane(*this)
        , _iterator(*this)
        , _context(EGL_NO_CONTEXT)
        , _target(GL_TEXTURE_2D)
        , _width(source.width)
        , _height(source.height)
        , _format(DRM_FORMAT_ABGR8888)
        , _planes(0)
        , _textureId(EGL_NO_TEXTURE)
        , _image(EGL_NO_IMAGE)
        , _fourcc(0)
        , _modifiers(DRM_FORMAT_MOD_LINEAR)
        , _stride(0)
        , _offset(0)
    {
        ASSERT(source.data.data() != nullptr);
        ASSERT(_api.eglCreateImage != nullptr);
        ASSERT(_api.eglExportDmaBufImageQueryMesa != nullptr);
        ASSERT(_api.eglExportDmaBufImageMesa != nullptr);

        Renderer::GLPixelFormat glFormat = Renderer::ConvertFormat(_format);

        Renderer::EGL::ContextBackup backup;

        _egl.SetCurrent();

        glGenTextures(1, &_textureId);
        glBindTexture(_target, _textureId);

        glTexParameteri(_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, (source.width * source.bytes_per_pixel) / (glFormat.BitPerPixel / 8));

        glTexImage2D(_target, 0, glFormat.Format, source.width, source.height, 0, glFormat.Format, glFormat.Type, source.data.data());

        glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);

        // EGL: Create EGL image from the GL texture
        _image = _api.eglCreateImage(_egl.Display(),
            _egl.Context(),
            EGL_GL_TEXTURE_2D,
            (EGLClientBuffer)(uint64_t)_textureId,
            NULL);

        ASSERT(_image != EGL_NO_IMAGE);

        // glFlush();

        EGLBoolean queried = _api.eglExportDmaBufImageQueryMesa(_egl.Display(),
            _image,
            &_fourcc,
            &_planes,
            &_modifiers);

        if(!queried){
            printf("Failed to query EGL Image.\n");
        }

        ASSERT(queried);
        ASSERT(_planes == 1);

        EGLBoolean exported = _api.eglExportDmaBufImageMesa(_egl.Display(),
            _image,
            &_id,
            &_stride,
            &_offset);

        if (!exported) {
            printf("Failed to export EGL Image to a DMA buffer.\n");
        }

        ASSERT(exported);

        printf("DMA buffer available on descriptor %d.\n", _id);

        glBindTexture(_target, 0);
    }

    DmaBuffer::~DmaBuffer()
    {
        Renderer::EGL::ContextBackup backup;

        _egl.SetCurrent();

        glDeleteTextures(1, &_textureId);

        if (_image != EGL_NO_IMAGE) {
            _api.eglDestroyImage(_egl.Display(), _image);
        }

        if (_id > 0) {
            close(_id);
        }
    };

    void DmaBuffer::AddRef() const
    {
        // return _source.AddRef();
    }

    uint32_t DmaBuffer::Release() const
    {
        return 0; //_source.Release();
    }

    uint32_t DmaBuffer::Identifier() const
    {
        return _id;
    }

    Exchange::ICompositionBuffer::IIterator* DmaBuffer::Planes(const uint32_t /*timeoutMs*/)
    {
        return &_iterator;
    }

    uint32_t DmaBuffer::Completed(const bool /*dirty*/)
    {
        return 0;
    }

    void DmaBuffer::Render()
    {
    }

    uint32_t DmaBuffer::Width() const
    {
        return _width;
    }

    uint32_t DmaBuffer::Height() const
    {
        return _height;
    }

    uint32_t DmaBuffer::Format() const
    {
        return _format;
    }

    uint32_t DmaBuffer::Stride() const
    {
        return _stride;
    }

    uint64_t DmaBuffer::Offset() const
    {
        return _offset;
    }

    uint64_t DmaBuffer::Modifier() const
    {
        return _modifiers;
    }

    Exchange::ICompositionBuffer::DataType DmaBuffer::Type() const
    {
        return Exchange::ICompositionBuffer::TYPE_DMA;
    }

    DmaBuffer::Plane& DmaBuffer::GetPlanes()
    {
        return _plane;
    }
}
}
