#include "DmaBuffer.h"

#include "../../src/GL/Format.h"

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <messaging/messaging.h>

#include <png.h>

namespace WPEFramework {
namespace Compositor {
    bool DumpTex(const Box& box, const uint32_t format, std::vector<uint8_t>& pixels, GLuint textureId)
    {
        const Renderer::GLPixelFormat formatGL(Renderer::ConvertFormat(format));

        glFinish();
        glGetError();

        GLuint fbo, bound_fbo;
        GLint viewport[4];

        // Backup framebuffer
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&bound_fbo));
        glGetIntegerv(GL_VIEWPORT, viewport);

        // Generate framebuffer
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Bind the texture to your FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

        // Test if everything failed
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            printf("failed to make complete framebuffer object %x", status);
        }

        // set the viewport as the FBO won't be the same dimension as the screen
        glViewport(box.x, box.y, box.width, box.height);

        pixels.clear();
        pixels.resize(box.width * box.height * (formatGL.BitPerPixel / 8));

        glReadPixels(box.x, box.y, box.width, box.height, formatGL.Format, formatGL.Type, pixels.data());

        // Restore framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, bound_fbo);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

        EGLint error = glGetError();

        printf("Snapshot result: %s\n", API::GL::ErrorString(error));

        return error == GL_NO_ERROR;
    }

    bool WritePNG(const std::string& filename, const std::vector<uint8_t> buffer, const unsigned int width, const unsigned int height)
    {

        png_structp pngPointer = nullptr;

        pngPointer = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (pngPointer == nullptr) {

            return false;
        }

        png_infop infoPointer = nullptr;
        infoPointer = png_create_info_struct(pngPointer);
        if (infoPointer == nullptr) {

            png_destroy_write_struct(&pngPointer, &infoPointer);
            return false;
        }

        // Set up error handling.
        if (setjmp(png_jmpbuf(pngPointer))) {

            png_destroy_write_struct(&pngPointer, &infoPointer);
            return false;
        }

        // Set image attributes.
        png_set_IHDR(pngPointer,
            infoPointer,
            width,
            height,
            8,
            PNG_COLOR_TYPE_RGBA,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);

        // Initialize rows of PNG.
        png_byte** rowLines = static_cast<png_byte**>(png_malloc(pngPointer, height * sizeof(png_byte*)));

        const int pixelSize = 4; // RGBA

        for (unsigned int i = 0; i < height; ++i) {

            png_byte* rowLine = static_cast<png_byte*>(png_malloc(pngPointer, sizeof(png_byte) * width * pixelSize));
            const uint8_t* rowSource = buffer.data() + (sizeof(png_byte) * i * width * pixelSize);
            rowLines[i] = rowLine;
            for (unsigned int j = 0; j < width * pixelSize; j += pixelSize) {
                *rowLine++ = rowSource[j + 0]; // Blue
                *rowLine++ = rowSource[j + 1]; // Green
                *rowLine++ = rowSource[j + 2]; // Red
                *rowLine++ = rowSource[j + 3]; // Alpha
            }
        }

        bool result = false;

        // Duplicate file descriptor and create File stream based on it.
        FILE* filePointer = fopen(filename.c_str(), "wb");

        if (nullptr != filePointer) {
            // Write the image data to "file".
            png_init_io(pngPointer, filePointer);
            png_set_rows(pngPointer, infoPointer, rowLines);
            png_write_png(pngPointer, infoPointer, PNG_TRANSFORM_IDENTITY, nullptr);
            // All went well.
            result = true;
        }

        // Close stream to flush and release allocated buffers
        fclose(filePointer);

        for (unsigned int i = 0; i < height; i++) {
            png_free(pngPointer, rowLines[i]);
        }

        png_free(pngPointer, rowLines);
        png_destroy_write_struct(&pngPointer, &infoPointer);

        return result;
    }

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

        glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, (source.width * source.bytes_per_pixel) / (glFormat.BitPerPixel / 8));

        glTexImage2D(_target, 0, glFormat.Format, source.width, source.height, 0, glFormat.Format, glFormat.Type, source.data.data());

        glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);

        std::vector<uint8_t> pixels;
        Box box = { 0, 0, static_cast<int>(_width), static_cast<int>(_height) };
        if (DumpTex(box, _format, pixels, _textureId) == true) {
            std::stringstream ss;
            ss << "gl-tex-snapshot-" << Core::Time::Now().Ticks() << ".png" << std::ends;
            Core::File snapshot(ss.str());

            WritePNG(ss.str(), pixels, _width, _height);
        }

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
