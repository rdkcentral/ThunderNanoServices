#pragma once

#include <interfaces/ICompositionBuffer.h>

#include "../../src/GL/RenderAPI.h"
#include "../../src/GL/EGL.h"
#include "../../src/GL/Format.h"

#include "Textures.h"
#include <drm_fourcc.h>

namespace Thunder {
    namespace Compositor {
        class DmaBuffer : public SimpleBuffer {
        public:
            DmaBuffer() = delete;
            DmaBuffer(DmaBuffer&&) = delete;
            DmaBuffer(const DmaBuffer&) = delete;
            DmaBuffer& operator=(DmaBuffer&&) = delete;
            DmaBuffer& operator=(const DmaBuffer&) = delete;

            DmaBuffer(int gpuFd, const Texture::PixelData& source)
                : SimpleBuffer(source.width, source.height, DRM_FORMAT_ABGR8888, DRM_FORMAT_MOD_LINEAR, Exchange::ICompositionBuffer::TYPE_RAW)
                , _api()
                , _egl(gpuFd)
                , _context(EGL_NO_CONTEXT)
                , _target(GL_TEXTURE_2D)
                , _textureId(EGL_NO_TEXTURE)
                , _image(EGL_NO_IMAGE) 
                , _fd(-1) {

                ASSERT(source.data.data() != nullptr);
                ASSERT(_api.eglCreateImage != nullptr);
                ASSERT(_api.eglExportDmaBufImageQueryMesa != nullptr);
                ASSERT(_api.eglExportDmaBufImageMesa != nullptr);

                Renderer::GLPixelFormat glFormat = Renderer::ConvertFormat(DRM_FORMAT_ABGR8888);

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

                int planes = 0;
                int fourcc = 0;
                EGLint stride;
                EGLint offset;
                EGLuint64KHR modifiers = DRM_FORMAT_MOD_LINEAR;

                EGLBoolean queried = _api.eglExportDmaBufImageQueryMesa(_egl.Display(),
                    _image,
                    &fourcc,
                    &planes,
                    &modifiers);

                if(!queried){
                    printf("Failed to query EGL Image.\n");
                }

                ASSERT(queried);
                ASSERT(planes == 1);

                if (_api.eglExportDmaBufImageMesa(_egl.Display(), _image, &_fd, &stride, &offset) ) {
                    printf("DMA buffer available on descriptor %d.\n", _fd);

                    glBindTexture(_target, 0);

                    SimpleBuffer::Add(_fd, stride, offset);
                }
                else {
                    ASSERT(false);
                }
            }
            ~DmaBuffer() override {
                Renderer::EGL::ContextBackup backup;

                _egl.SetCurrent();

                glDeleteTextures(1, &_textureId);

                if (_image != EGL_NO_IMAGE) {
                    _api.eglDestroyImage(_egl.Display(), _image);
                }

                if (_fd > 0) {
                    close(_fd);
                }
            }

        private:
            API::EGL _api;
            Renderer::EGL _egl;
            EGLContext _context;
            GLenum _target;
            GLuint _textureId;
            EGLImage _image;
            int _fd;
        }; // class DmaBuffer
    }
}
