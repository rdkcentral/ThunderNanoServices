#pragma once

#include <interfaces/ICompositionBuffer.h>

#include "../../src/GL/RenderAPI.h"
#include "../../src/GL/EGL.h"

#include "Textures.h"

#include "../../src/GL/Format.h"

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

namespace Thunder {
namespace Compositor {
    class DmaBuffer : public Exchange::ICompositionBuffer {

        class Iterator : public Exchange::ICompositionBuffer::IIterator {
        public:
            Iterator(DmaBuffer& parent)
                : _parent(parent)
                , _index(0)
            {
            }

            virtual ~Iterator() = default;

            bool IsValid() const override
            {
                return ((_index > 0) && (_index <= 1));
            }
            void Reset() override
            {
                _index = 0;
            }

            bool Next() override
            {
                if (_index <= 1) {
                    _index++;
                }
                return (IsValid());
            }

            int Descriptor() const override
            {
                return static_cast<int>(_parent.Descriptor());
            }

            uint32_t Stride() const override
            {
                return _parent.Stride();
            }

            uint32_t Offset() const override
            {
                return _parent.Offset();
            }

        private:
            DmaBuffer& _parent;
            uint8_t _index;
        };

    public:
        DmaBuffer() = delete;

        DmaBuffer(int gpuFd, const Texture::PixelData& source)
            : _id(0)
            , _api()
            , _egl(gpuFd)
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

            if (!queried) {
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

        DmaBuffer(const DmaBuffer&) = delete;
        DmaBuffer& operator=(const DmaBuffer&) = delete;

        virtual ~DmaBuffer(){
            Renderer::EGL::ContextBackup backup;

            _egl.SetCurrent();

            glDeleteTextures(1, &_textureId);

            if (_image != EGL_NO_IMAGE) {
                _api.eglDestroyImage(_egl.Display(), _image);
            }

            if (_id > 0) {
                close(_id);
            }
        }

        Exchange::ICompositionBuffer::IIterator* Acquire(const uint32_t /*timeoutMs*/) override
        {
            return &_iterator;
        }

        void Relinquish() override
        {
        }

        uint32_t Width() const override
        {
            return _width;
        }

        uint32_t Height() const override
        {
            return _height;
        }

        uint32_t Format() const override
        {
            return _format;
        }

        uint32_t Stride() const
        {
            return _stride;
        }

        uint64_t Offset() const
        {
            return _offset;
        }

        uint64_t Modifier() const override
        {
            return _modifiers;
        }

        Exchange::ICompositionBuffer::DataType Type() const override
        {
            return Exchange::ICompositionBuffer::TYPE_DMA;
        }

    protected:
        int Descriptor() const
        {
            return _id;
        }

    private:
        int _id;
        API::EGL _api;
        Renderer::EGL _egl;

        Iterator _iterator;

        EGLContext _context;
        GLenum _target;

        const uint32_t _width;
        const uint32_t _height;
        const uint32_t _format;
        int _planes;

        GLuint _textureId;
        EGLImage _image;
        int _fourcc;
        EGLuint64KHR _modifiers;
        EGLint _stride;
        EGLint _offset;

    }; // class DmaBuffer
}
}
