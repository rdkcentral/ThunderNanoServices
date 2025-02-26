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

#include "EGL.h"
#include "RenderAPI.h"

// these headers are generated
#include <fragment-shader.h>
#include <vertex-shader.h>

#include "Format.h"
#include <png.h>

#include <DRM.h>

namespace Thunder {
namespace Compositor {
    namespace Renderer {
        class GLES : public IRenderer {
        private:
            using PointCoordinates = std::array<GLfloat, 8>;

            static constexpr PointCoordinates Vertices = {
                1, 0, // top right
                0, 0, // top left
                1, 1, // bottom right
                0, 1, // bottom left
            };

#ifdef __DEBUG__
            static void GLPushDebug(const std::string& file, const std::string& func, const uint32_t line)
            {
                if (_gles.glPushDebugGroupKHR == nullptr) {
                    return;
                }

                std::stringstream message;
                message << file << ":" << line << "[" << func << "]";

                _gles.glPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION_KHR, 1, -1, message.str().c_str());
            }

            static void GLPopDebug()
            {
                if (_gles.glPopDebugGroupKHR != nullptr) {
                    _gles.glPopDebugGroupKHR();
                }
            }

            static void DebugSink(GLenum src, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*len*/, const GLchar* msg,
                const void* /*user*/)
            {
                std::stringstream line;
                line << "[" << Compositor::API::GL::SourceString(src)
                     << "](" << Compositor::API::GL::TypeString(type)
                     << "): " << msg;

                switch (severity) {
                case GL_DEBUG_SEVERITY_HIGH_KHR: {
                    TRACE_GLOBAL(Trace::Error, ("%s", line.str().c_str()));
                    break;
                }
                case GL_DEBUG_SEVERITY_MEDIUM_KHR: {
                    TRACE_GLOBAL(Trace::Warning, ("%s", line.str().c_str()));
                    break;
                }
                case GL_DEBUG_SEVERITY_LOW_KHR: {
                    TRACE_GLOBAL(Trace::Information, ("%s", line.str().c_str()));
                    break;
                }
                case GL_DEBUG_SEVERITY_NOTIFICATION_KHR:
                default: {
                    TRACE_GLOBAL(Trace::GL, ("%s", line.str().c_str()));
                    break;
                }
                }
            }

#define PushDebug() GLPushDebug(__FILE__, __func__, __LINE__)
#define PopDebug() GLPopDebug()
#else
#define PushDebug()
#define PopDebug()
#endif
            // Framebuffers are usually connected to an output, it's the buffer for the composition to be rendered on.
            class FrameBuffer {
            public:
                FrameBuffer() = delete;
                FrameBuffer(FrameBuffer&&) = delete;
                FrameBuffer(const FrameBuffer&) = delete;
                FrameBuffer& operator=(FrameBuffer&&) = delete;
                FrameBuffer& operator=(const FrameBuffer&) = delete;

                FrameBuffer(API::GL& api, EGLImage eglImage, const bool external)
                    : _api(api)
                    , _eglImage(eglImage)
                    , _glFrameBuffer(0)
                    , _glRenderBuffer(0)
                    , _external(external)
                {
                    ASSERT(_eglImage != EGL_NO_IMAGE);

                    // If this triggers the platform is very old (pre-2008) and not supporting OpenGL 3.0 or higher.
                    ASSERT(_api.glEGLImageTargetRenderbufferStorageOES != nullptr);

                    PushDebug();

                    glGenRenderbuffers(1, &_glRenderBuffer);
                    glGenFramebuffers(1, &_glFrameBuffer);

                    glBindRenderbuffer(GL_RENDERBUFFER, _glRenderBuffer);
                    _api.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, _eglImage);
                    glBindRenderbuffer(GL_RENDERBUFFER, 0);

                    PopDebug();

                    TRACE(Trace::GL, ("Created FrameBuffer"));
                }
                ~FrameBuffer()
                {
                    Unbind();

                    PushDebug();
                    glDeleteFramebuffers(1, &_glFrameBuffer);
                    glDeleteRenderbuffers(1, &_glRenderBuffer);
                    PopDebug();

                    TRACE(Trace::GL, ("FrameBuffer %p destructed", this));
                }

            public:
                void Unbind()
                {
                    PushDebug();
                    glFlush();
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glBindRenderbuffer(GL_RENDERBUFFER, 0);
                    PopDebug();
                }
                void Bind()
                {
                    PushDebug();
                    glBindRenderbuffer(GL_RENDERBUFFER, _glRenderBuffer);
                    glBindFramebuffer(GL_FRAMEBUFFER, _glFrameBuffer);
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _glRenderBuffer);
                    ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
                    PopDebug();
                }
                bool External() const {
                    return (_external);
                }

            private:
                API::GL& _api;
                EGLImage _eglImage;
                GLuint _glFrameBuffer;
                GLuint _glRenderBuffer;
                bool _external;
            };
            class Program {
            public:
                enum class Variant : uint8_t {
                    NONE = 0,
                    /* Keep the following in sync with fragment.glsl. */
                    SOLID = 1,
                    TEXTURE_EXTERNAL = 2, // 1 texture
                    TEXTURE_RGBA = 3, // 1 texture
                    TEXTURE_RGBX = 4, // 1 texture
                    TEXTURE_Y_U_V = 5, // 3 textures
                    TEXTURE_Y_UV = 6, // 2 textures
                    TEXTURE_Y_XUXV = 7, // 2 textures
                    TEXTURE_XYUV = 8 // 1 texture
                };

                using VariantType = Variant;

                Program(const uint8_t variant)
                    : _id(Create(variant))
                    , _projection(glGetUniformLocation(_id, "projection"))
                    , _position(glGetAttribLocation(_id, "position"))
                {
                    ASSERT(_id != GL_FALSE);
                }
                virtual ~Program()
                {
                    ASSERT(_id != GL_FALSE);

                    if (_id != GL_FALSE) {
                        glDeleteProgram(_id);
                    }
                };

            public:
                virtual uint32_t Draw(const GLuint id, GLenum target, const bool hasAlpha, const float& alpha, const Matrix& matrix, const PointCoordinates& coordinates) const = 0;

            private:
                GLuint Compile(GLenum type, const char* source)
                {
                    PushDebug();

                    GLint status(GL_FALSE);
                    GLuint handle = glCreateShader(type);

                    // TRACE(Trace::GL, ("Shader[0x%02X]=%d source: \n%s", type, handle, source));

                    glShaderSource(handle, 1, &source, NULL);
                    glCompileShader(handle);

                    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);

                    if (status == GL_FALSE) {
                        TRACE(Trace::Error,
                            ("Shader %d compilation failed: %s", handle, API::GL::ShaderInfoLog(handle).c_str()));
                        glDeleteShader(handle);
                        handle = GL_FALSE;
                    }

                    PopDebug();
                    return handle;
                }

                GLuint Create(const uint8_t variant)
                {
                    PushDebug();

                    GLuint handle(GL_FALSE);

                    const char* VariantStr[] = { "NONE", "SOLID", "TEXTURE_EXTERNAL",
                        "TEXTURE_RGBA", "TEXTURE_RGBX", "TEXTURE_Y_U_V",
                        "TEXTURE_Y_UV", "TEXTURE_Y_XUXV", "TEXTURE_XYUV" };

                    TRACE(Trace::Information, ("Creating Program for %s", VariantStr[variant]));

                    std::stringstream fragmentShaderSource;
                    fragmentShaderSource << "#define VARIANT " << static_cast<uint16_t>(variant) << std::endl
                                         << ::GLES::fragment_shader;

                    GLuint vs = Compile(GL_VERTEX_SHADER, ::GLES::vertex_shader);
                    GLuint fs = Compile(GL_FRAGMENT_SHADER, fragmentShaderSource.str().c_str());

                    if ((vs != GL_FALSE) && (fs != GL_FALSE)) {
                        handle = glCreateProgram();

                        glAttachShader(handle, vs);
                        glAttachShader(handle, fs);

                        glLinkProgram(handle);

                        glDetachShader(handle, vs);
                        glDetachShader(handle, fs);
                        glDeleteShader(vs);
                        glDeleteShader(fs);

                        GLint status(GL_FALSE);
                        glGetProgramiv(handle, GL_LINK_STATUS, &status);

                        if (status == GL_FALSE) {
                            TRACE(Trace::Error, ("Program linking failed: %s", API::GL::ProgramInfoLog(handle).c_str()));
                            glDeleteProgram(handle);
                            handle = GL_FALSE;
                        }

                    } else {
                        TRACE(Trace::Error, ("Error Creating Program"));
                    }

                    PopDebug();

                    return handle;
                }

            public:
                GLuint Id() const
                {
                    return _id;
                }
                GLint Projection() const
                {
                    return _projection;
                }
                GLint Position() const
                {
                    return _position;
                }

            private:
                GLuint _id;
                GLint _projection;
                GLint _position;
            };
            class ColorProgram : public Program {
            public:
                enum { ID = static_cast<std::underlying_type<VariantType>::type>(Variant::SOLID) };

            public:
                ColorProgram()
                    : Program(ID)
                    , _color(glGetUniformLocation(Id(), "color"))
                    , _colorValue ({ 0.0, 0.0, 0.0, 0.0 })
                {
                    TRACE(Trace::Information, ("Created Color Program: id=%d, projection=%d, position=%d color=%d", Id(), Projection(), Position(), Color()));
                }
                ~ColorProgram() override = default;

            public:
                GLint Color() const
                {
                    return _color;
                }
                void Color(const Compositor::Color& color)
                {
                    _colorValue = color;
                }
                uint32_t Draw(const Compositor::Color& color, const Matrix& matrix) const {
                    PushDebug();

                    if (_colorValue[3] >= 1.0) {
                        glDisable(GL_BLEND);
                    } else {
                        glEnable(GL_BLEND);
                    }

                    glUseProgram(Id());

                    glUniformMatrix3fv(Projection(), 1, GL_FALSE, &matrix[0]);

                    glUniform4f(_color, _colorValue[0], _colorValue[1], _colorValue[2], _colorValue[3]);

                    glVertexAttribPointer(Position(), 2, GL_FLOAT, GL_FALSE, 0, &Vertices[0]);

                    glEnableVertexAttribArray(Position());

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                    glDisableVertexAttribArray(Position());

                    PopDebug();

                    return (glGetError() == GL_NO_ERROR ? Core::ERROR_NONE : Core::ERROR_GENERAL);
 
                }
                uint32_t Draw(const GLuint, GLenum, const bool, const float&, const Matrix& matrix, const PointCoordinates&) const
                {
                    return (Draw(_colorValue, matrix));
                }

            private:
                GLint _color;
                Compositor::Color _colorValue;
            };
            template <const Program::VariantType VARIANT, const uint8_t TEXTURE_COUNT>
            class TextureProgramType : public Program {
            public:
                enum { ID = static_cast<std::underlying_type<VariantType>::type>(VARIANT) };

            public:
                TextureProgramType()
                    : Program(ID)
                    , _textureIds()
                    , _coordinates(glGetAttribLocation(Id(), "texcoord"))
                    , _alpha(glGetUniformLocation(Id(), "alpha"))
                {
                    uint8_t index(0);

                    for (auto& id : _textureIds) {
                        int len = snprintf(NULL, 0, "tex%d", index) + 1;

                        char* parameter = static_cast<char*>(ALLOCA(len));

                        snprintf(parameter, len, "tex%d", index++);

                        id = glGetUniformLocation(Id(), parameter);
                    }

                    TRACE(Trace::Information, ("Created Texture Program: id=%d, projection=%d, position=%d coordinates=%d, alpha=%d", Id(), Projection(), Position(), Coordinates(), Alpha()));
                }
                ~TextureProgramType() override = default;

            public:
                GLint TextureId(const uint8_t id) const
                {
                    return _textureIds[id];
                }
                GLint TextureCount() const
                {
                    return _textureIds.size();
                }
                GLint Coordinates() const
                {
                    return _coordinates;
                }
                GLuint Alpha() const
                {
                    return _alpha;
                }
                uint32_t Draw(const GLuint id, GLenum target, const bool hasAlpha, const float& alpha, const Matrix& matrix, const PointCoordinates& coordinates) const override
                {
                    PushDebug();

                    if ((hasAlpha == false) && (alpha >= 1.0)) {
                        glDisable(GL_BLEND);
                    } else {
                        glEnable(GL_BLEND);
                    }

                    glActiveTexture(GL_TEXTURE0);

                    glBindTexture(target, id);
                    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

                    glUseProgram(Id());

                    glUniformMatrix3fv(Projection(), 1, GL_FALSE, &matrix[0]);
                    glUniform1i(TextureId(0), 0);
                    glUniform1f(Alpha(), alpha);

                    glVertexAttribPointer(Position(), 2, GL_FLOAT, GL_FALSE, 0, &Vertices[0]);
                    glVertexAttribPointer(Coordinates(), 2, GL_FLOAT, GL_FALSE, 0, &coordinates[0]);

                    glEnableVertexAttribArray(Position());
                    glEnableVertexAttribArray(Coordinates());

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                    glDisableVertexAttribArray(Position());
                    glDisableVertexAttribArray(Coordinates());

                    glBindTexture(target, 0);

                    PopDebug();

                    return (glGetError() == GL_NO_ERROR ? Core::ERROR_NONE : Core::ERROR_GENERAL);
                }

            private:
                std::array<GLint, TEXTURE_COUNT> _textureIds;
                GLint _coordinates;
                GLuint _alpha;
            };

            using ExternalProgram = TextureProgramType<Program::Variant::TEXTURE_EXTERNAL, 1>;
            using RGBAProgram = TextureProgramType<Program::Variant::TEXTURE_RGBA, 1>;
            using RGBXProgram = TextureProgramType<Program::Variant::TEXTURE_RGBX, 1>;
            // using XYUVProgram = TextureProgramType<Program::Variant::TEXTURE_XYUV, 1>;
            // using Y_UVProgram = TextureProgramType<Program::Variant::TEXTURE_Y_UV, 2>;
            // using Y_U_VProgram = TextureProgramType<Program::Variant::TEXTURE_Y_U_V, 3>;
            // using Y_XUXVProgram = TextureProgramType<Program::Variant::TEXTURE_Y_XUXV, 2>;

            class ProgramRegistry {
            private:
                using Programs = std::unordered_map<Program::VariantType, Program*>;

            public:
                ProgramRegistry(ProgramRegistry&&) = delete;
                ProgramRegistry(const ProgramRegistry&) = delete;
                ProgramRegistry& operator=(ProgramRegistry&&) = delete;
                ProgramRegistry& operator=(const ProgramRegistry&) = delete;

                ProgramRegistry() = default;
                ~ProgramRegistry()
                {
                    for (auto& entry : _programs) {
                        delete entry.second;
                    }
                }

                template <typename PROGRAM>
                void Announce()
                {
                    _programs.emplace(std::piecewise_construct,
                        std::forward_as_tuple(static_cast<Program::VariantType>(PROGRAM::ID)),
                        std::forward_as_tuple(new PROGRAM()));
                }

                template <typename PROGRAM>
                PROGRAM* QueryType()
                {
                    PROGRAM* result = nullptr;
                    Programs::iterator index(_programs.find(static_cast<Program::VariantType>(PROGRAM::ID)));
                    if (index != _programs.end()) {
                        Program* base = index->second;
                        result = static_cast<PROGRAM*>(base);
                    }
                    return (result);
                }

            private:
                Programs _programs;
            };

            static bool WritePNG(const std::string& filename, const std::vector<uint8_t> buffer, const unsigned int width, const unsigned int height)
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
                        // PNG has ARGB
                        *rowLine++ = rowSource[j + 3]; // Alpha
                        *rowLine++ = rowSource[j + 2]; // Red
                        *rowLine++ = rowSource[j + 1]; // Green
                        *rowLine++ = rowSource[j + 0]; // Blue
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

            static bool DumpTex(const Exchange::IComposition::Rectangle& box, const uint32_t format, std::vector<uint8_t>& pixels, GLuint textureId)
            {
                const Renderer::GLPixelFormat formatGL(Renderer::ConvertFormat(format));

                glFinish();
                glGetError();

                GLuint fbo, bound_fbo;
                GLint viewport[4];

                // Backup framebuffer
                glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&bound_fbo));
                glGetIntegerv(GL_VIEWPORT, viewport);

                glActiveTexture(GL_TEXTURE0);

                // Generate framebuffer
                glGenFramebuffers(1, &fbo);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo);

                // Bind the texture to your FBO
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

                // Test if everything failed
                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

                if (status != GL_FRAMEBUFFER_COMPLETE) {
                    TRACE_GLOBAL(Trace::Error, ("DumpTex: failed to make complete framebuffer object %x", status));
                    // return false;
                }

                // set the viewport as the FBO won't be the same dimension as the screen
                glViewport(box.x, box.y, box.width, box.height);

                pixels.clear();
                pixels.resize(box.width * box.height * (formatGL.BitPerPixel / 8));

                glReadPixels(box.x, box.y, box.width, box.height, formatGL.Format, formatGL.Type, pixels.data());

                // Restore framebuffer
                glBindFramebuffer(GL_FRAMEBUFFER, bound_fbo);
                glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

                return glGetError() == GL_NO_ERROR;
            }

            class GLESTexture : public IRenderer::ITexture {
            public:
                GLESTexture() = delete;
                GLESTexture(GLESTexture&&) = delete;
                GLESTexture(const GLESTexture&) = delete;
                GLESTexture& operator=(GLESTexture&&) = delete;
                GLESTexture& operator=(const GLESTexture&) = delete;

                GLESTexture(GLES& parent, const Core::ProxyType<Exchange::ICompositionBuffer>& buffer)
                    : _parent(parent)
                    , _target(GL_TEXTURE_2D)
                    , _textureId(0)
                    , _image(EGL_NO_IMAGE)
                    , _buffer(buffer)
                    , _program(nullptr)
                    , _hasAlpha(DRM::HasAlpha(_buffer->Format()))
                {
                    ASSERT(_buffer != nullptr);

                    if (buffer->Type() == Exchange::ICompositionBuffer::TYPE_DMA) {
                        ImportDMABuffer();
                    }

                    if (buffer->Type() == Exchange::ICompositionBuffer::TYPE_RAW) {
                        ImportPixelBuffer();
                    }

                    ASSERT(_textureId != 0);

                    if ((_target == GL_TEXTURE_EXTERNAL_OES)) {
                        _program = _parent.Programs().QueryType<ExternalProgram>();
                    }
                    else if ((_target == GL_TEXTURE_2D)) {
                        if (_hasAlpha == true) {
                            _program = _parent.Programs().QueryType<RGBAProgram>();
                        } else {
                            _program = _parent.Programs().QueryType<RGBXProgram>();
                        }
                    }

                    ASSERT(_program != nullptr);
 
                    // Snapshot();
                }

                ~GLESTexture() override
                {
                    Renderer::EGL::ContextBackup backup;
                    _parent.Egl().SetCurrent();

                    glDeleteTextures(1, &_textureId);

                    if (_image != EGL_NO_IMAGE) {
                        _parent.Egl().DestroyImage(_image);
                    }
                }

            public:
                /**
                 *  IRenderer::ITexture
                 */
                bool IsValid() const override
                {
                    return (_textureId != 0);
                }
                uint32_t Width() const override { return _buffer->Width(); }
                uint32_t Height() const override { return _buffer->Height(); }
                GLenum Target() const { return _target; }
                GLuint Id() const { return _textureId; }

                uint32_t Draw(const float& alpha, const Matrix& matrix, const Exchange::IComposition::Rectangle& region) const override
                {
                    const GLfloat x1 = region.x / _buffer->Width();
                    const GLfloat y1 = region.y / _buffer->Height();
                    const GLfloat x2 = (region.x + region.width) / _buffer->Width();
                    const GLfloat y2 = (region.y + region.height) / _buffer->Height();

                    const PointCoordinates coordinates = {
                        x2, y1, // top right
                        x1, y1, // top left
                        x2, y2, // bottom right
                        x1, y2, // bottom left
                    };

                    return (_program->Draw(_textureId, _target, _hasAlpha, alpha, matrix, coordinates));
                }

            private:
                void Snapshot() const
                {
                    std::vector<uint8_t> pixels;
                    Exchange::IComposition::Rectangle box = { 0, 0, _buffer->Width(), _buffer->Height() };

                    if (DumpTex(box, _buffer->Format(), pixels, _textureId) == true) {
                        std::stringstream ss;
                        ss << "gles2-rgba-texture-snapshot-" << Core::Time::Now().Ticks() << ".png" << std::ends;
                        Core::File snapshot(ss.str());

                        WritePNG(ss.str(), pixels, box.width, box.height);
                    }
                }

                void ImportDMABuffer()
                {
                    bool external(false);

                    Renderer::EGL::ContextBackup backup;

                    _parent.Egl().SetCurrent();
                    ASSERT(eglGetError() == EGL_SUCCESS);

                    _image = _parent.Egl().CreateImage(&(*_buffer), external);
                    ASSERT(_image != EGL_NO_IMAGE);

                    _target = (external == true) ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;

                    glGenTextures(1, &_textureId);
                    glBindTexture(_target, _textureId);

                    glTexParameteri(_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    // glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    // glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    _parent.Gles().glEGLImageTargetTexture2DOES(_target, _image);

                    glBindTexture(_target, 0);

                    TRACE(Trace::GL, ("Imported dma buffer texture id=%d, width=%d, height=%d external=%s", _textureId, _buffer->Width(), _buffer->Height(), external ? "true" : "false"));
                }

                void ImportPixelBuffer()
                {
                    Exchange::ICompositionBuffer::IIterator* planes = _buffer->Acquire(Compositor::DefaultTimeoutMs);

                    // uint8_t index(0);

                    planes->Next(); // select first plane.

                    int data(planes->Descriptor());

                    ASSERT(data != -1);

                    GLPixelFormat glFormat = ConvertFormat(_buffer->Format());

                    Renderer::EGL::ContextBackup backup;

                    _parent.Egl().SetCurrent();

                    glGenTextures(1, &_textureId);
                    glBindTexture(_target, _textureId);

                    glTexParameteri(_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, planes->Stride() / (glFormat.BitPerPixel / 8));

                    glTexImage2D(_target, 0, glFormat.Format, _buffer->Width(), _buffer->Height(), 0, glFormat.Format, glFormat.Type, reinterpret_cast<uint8_t*>(data));

                    glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);

                    glBindTexture(_target, 0);

                    _buffer->Relinquish();

                    TRACE(Trace::GL, ("Imported pixel buffer texture id=%d, width=%d, height=%d glformat=0x%04x, gltype=0x%04x glbitperpixel=%d glAlpha=0x%x", _textureId, _buffer->Width(), _buffer->Height(), glFormat.Format, glFormat.Type, glFormat.BitPerPixel, glFormat.Alpha));
                }

            private:
                GLES& _parent;
                GLenum _target;
                GLuint _textureId;
                EGLImageKHR _image;
                Core::ProxyType<Exchange::ICompositionBuffer> _buffer;
                Program* _program;
                bool _hasAlpha;

            }; //  class GLESTexture

        public:
            GLES() = delete;
            GLES(GLES&&) = delete;
            GLES(const GLES&) = delete;
            GLES& operator=(GLES&&) = delete;
            GLES& operator=(const GLES&) = delete;

            GLES(const int drmDevFd, const Core::ProxyType<Exchange::ICompositionBuffer>& buffer)
                : _egl(drmDevFd)
                , _frameBuffer(nullptr)
                , _viewportWidth(0)
                , _viewportHeight(0)
                , _programs()
                , _rendering(false)
            {
                _egl.SetCurrent();

                TRACE(Trace::GL,
                    ("%s Creating GLES2 renderer - build: %s\n GL version: %s\n GL renderer: %s\n GL vendor: %s", __func__,
                        __TIMESTAMP__, glGetString(GL_VERSION), glGetString(GL_RENDERER), glGetString(GL_VENDOR)));

                ASSERT(_egl.IsCurrent() == true);
#ifdef __DEBUG__
                const std::string glExtensions(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));

                if (API::HasExtension(glExtensions, "GL_KHR_debug")) {
                    glEnable(GL_DEBUG_OUTPUT_KHR);
                    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);

                    _gles.glDebugMessageCallbackKHR(DebugSink, this);

                    // Silence unwanted message types
                    _gles.glDebugMessageControlKHR(GL_DONT_CARE, GL_DEBUG_TYPE_POP_GROUP_KHR, GL_DONT_CARE, 0, nullptr, GL_FALSE);
                    _gles.glDebugMessageControlKHR(GL_DONT_CARE, GL_DEBUG_TYPE_PUSH_GROUP_KHR, GL_DONT_CARE, 0, nullptr, GL_FALSE);
                }
#endif

                if (_gles.glGetGraphicsResetStatusKHR != nullptr) {
                    GLenum status = _gles.glGetGraphicsResetStatusKHR();
                    if (status != GL_NO_ERROR) {
                        TRACE(Trace::Error, ("GPU reset (%s)", API::GL::ResetStatusString(status)));
                    }
                }

                PushDebug();

                _programs.Announce<ColorProgram>();
                _programs.Announce<ExternalProgram>();
                _programs.Announce<RGBAProgram>();
                _programs.Announce<RGBXProgram>();
                // _programs.Announce<XYUVProgram>();
                // _programs.Announce<Y_UVProgram>();
                // _programs.Announce<Y_U_VProgram>();
                // _programs.Announce<Y_XUXVProgram>();

                PopDebug();


fprintf(stdout, "-------------------------- %s ------------------------- %d -----------------\n", __FUNCTION__, __LINE__); fflush(stdout);
                bool external(false);
                _frameBuffer.reset(new FrameBuffer(_gles, _egl.CreateImage(buffer.operator->(),external), external));
fprintf(stdout, "-------------------------- %s ------------------------- %d -----------------\n", __FUNCTION__, __LINE__); fflush(stdout);
                _egl.ResetCurrent();
            }

            virtual ~GLES()
            {
                _egl.SetCurrent();
#ifdef __DEBUG__

                const std::string glExtensions(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));

                if (API::HasExtension(glExtensions, "GL_KHR_debug")) {
                    glDisable(GL_DEBUG_OUTPUT_KHR);
                    _gles.glDebugMessageCallbackKHR(nullptr, nullptr);
                }

#endif

                // glFinish();
                // glFlush();
                // glGetError();

                _egl.ResetCurrent();

                TRACE(Trace::GL, ("GLES2 renderer %p destructed", this));
            }

            void Unbind()
            {
                _egl.ResetCurrent();
                _rendering = false;
            }

            uint32_t Bind() override
            {
                ASSERT(_rendering == false);

                _rendering = true;
                _egl.SetCurrent();

                _frameBuffer->Bind();
                return (Core::ERROR_NONE);
           }

            uint32_t ViewPort(const uint32_t width, const uint32_t height) override
            {
                PushDebug();

                _egl.SetCurrent();
                glViewport(0, 0, width, height);

                _viewportWidth = width;
                _viewportHeight = height;

                // refresh projection matrix
                Transformation::Projection(_projection, _viewportWidth, _viewportHeight, Transformation::TRANSFORM_NORMAL);

                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                _egl.ResetCurrent();
                PopDebug();

                return (glGetError() == GL_NO_ERROR);
 
            }
            void Dump () 
            {
                    std::vector<uint8_t> pixels;
                    Exchange::IComposition::Rectangle box = { 0, 0, _viewportWidth, _viewportHeight };
                    if (Snapshot(box, DRM_FORMAT_ARGB8888, pixels) == true) {
                        std::stringstream ss;
                        ss << "renderer-end-snapshot-" << Core::Time::Now().Ticks() << ".png" << std::ends;
                        Core::File snapshot(ss.str());

                        WritePNG(ss.str(), pixels, _viewportWidth, _viewportHeight);
                    }
            }

            void Clear(const Color color) override
            {
                ASSERT((_rendering == true) && (_egl.IsCurrent() == true));

                PushDebug();
                glClearColor(color[0], color[1], color[2], color[3]);
                glClear(GL_COLOR_BUFFER_BIT);
                PopDebug();
            }

            void Scissor(const Exchange::IComposition::Rectangle* box) override
            {
                ASSERT((_rendering == true) && (_egl.IsCurrent() == true));

                PushDebug();
                if (box != nullptr) {
                    glScissor(box->x, box->y, box->width, box->height);
                    glEnable(GL_SCISSOR_TEST);
                } else {
                    glDisable(GL_SCISSOR_TEST);
                }
                PopDebug();
            }

            Core::ProxyType<ITexture> Texture(const Core::ProxyType<Exchange::ICompositionBuffer>& buffer) override
            {
                return (Core::ProxyType<ITexture>(Core::ProxyType<GLESTexture>::Create(*this, buffer)));
            };

            uint32_t Render(const Core::ProxyType<ITexture>& texture, const Exchange::IComposition::Rectangle& region, const Matrix transformation, const float alpha) override
            {
                ASSERT((_rendering == true) && (_egl.IsCurrent() == true) && (texture != nullptr));

                uint32_t result(Core::ERROR_BAD_REQUEST);

                Matrix gl_matrix;
                Transformation::Multiply(gl_matrix, _projection, transformation);
                Transformation::Multiply(gl_matrix, Transformation::Transformations[Transformation::TRANSFORM_NORMAL], transformation);
                Transformation::Transpose(gl_matrix, gl_matrix);

                return ((texture->Draw(alpha, gl_matrix, region) == true) ? Core::ERROR_NONE : Core::ERROR_GENERAL);
            }

            uint32_t Quadrangle(const Color color, const Matrix transformation) override
            {
                ASSERT((_rendering == true) && (_egl.IsCurrent() == true));

                Matrix gl_matrix;
                Transformation::Multiply(gl_matrix, _projection, transformation);
                Transformation::Multiply(gl_matrix, Transformation::Transformations[Transformation::TRANSFORM_FLIPPED_180], transformation);
                Transformation::Transpose(gl_matrix, gl_matrix);

                ColorProgram* program = _programs.QueryType<ColorProgram>();

                ASSERT(program != nullptr);

                return (program->Draw(color, gl_matrix) == true) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
            }

            const std::vector<PixelFormat>& RenderFormats() const override
            {
                return _egl.Formats();
            }

            const std::vector<PixelFormat>& TextureFormats() const override
            {
                return _egl.Formats();
            }
            const Matrix& Projection() const
            {
                return _projection;
            }

        private:
            const EGL& Egl() const
            {
                return _egl;
            }

            const API::GL& Gles() const
            {
                return _gles;
            }

            bool InContext() const
            {
                return ((_egl.IsCurrent() == true) && (_frameBuffer != nullptr));
            }

            bool Snapshot(const Exchange::IComposition::Rectangle& box, const uint32_t format, std::vector<uint8_t>& pixels)
            {
                PushDebug();

                const GLPixelFormat formatGL(ConvertFormat(format));

                glFinish();
                glGetError();

                pixels.clear();
                pixels.resize(_viewportWidth * _viewportHeight * (formatGL.BitPerPixel / 8));

                glReadPixels(box.x, box.y, box.width, box.height, formatGL.Format, formatGL.Type, pixels.data());

                PopDebug();

                return glGetError() == GL_NO_ERROR;
            }
            ProgramRegistry& Programs()
            {
                return _programs;
            }

        private:
            static API::GL _gles;

        private:
            EGL _egl;
            std::unique_ptr<FrameBuffer> _frameBuffer;
            uint32_t _viewportWidth;
            uint32_t _viewportHeight;
            Matrix _projection;
            ProgramRegistry _programs;
            mutable bool _rendering;
        }; // class GLES

        API::GL GLES::_gles;

        constexpr GLES::PointCoordinates GLES::Vertices;

    } // namespace Renderer

    Core::ProxyType<IRenderer> IRenderer::Instance(const int identifier, const Core::ProxyType<Exchange::ICompositionBuffer>& buffer)
    {
        ASSERT(identifier >= 0); // this should be a valid file descriptor.

        return (Core::ProxyType<IRenderer>(Core::ProxyType<Renderer::GLES>::Create(identifier, buffer)));
    }

} // namespace Compositor
} // namespace Thunder
