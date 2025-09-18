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

#include <sys/mman.h>

namespace Thunder {
namespace Compositor {
    namespace Renderer {

#ifdef __DEBUG__
        // Stack to track debug group context (GLES doesn't provide this automatically)
        namespace {
            thread_local std::vector<std::string> g_debugGroupStack;
        }
#endif // __DEBUG__

        class SHMMapper {
        public:
            SHMMapper() = delete;
            SHMMapper(const SHMMapper&) = delete;
            SHMMapper& operator=(const SHMMapper&) = delete;
            SHMMapper(SHMMapper&&) = delete;
            SHMMapper& operator=(SHMMapper&&) = delete;

            SHMMapper(int fd, size_t size)
                : _mappedMemory(nullptr)
                , _size(size)
                , _isValid(false)
            {
                if (fd < 0 || size == 0) {
                    return;
                }

                _mappedMemory = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);

                if (_mappedMemory == MAP_FAILED) {
                    TRACE(Trace::Error, ("SHM mmap failed (fd=%d, size=%zu): %s", fd, size, strerror(errno)));
                    _mappedMemory = nullptr;
                    return;
                }

                _isValid = true;
            }

            ~SHMMapper()
            {
                if (_isValid && _mappedMemory != nullptr) {
                    munmap(_mappedMemory, _size);
                }
            }

            bool IsValid() const { return _isValid; }

            const uint8_t* GetData(size_t offset) const
            {
                if (!_isValid || offset >= _size) {
                    return nullptr;
                }
                return static_cast<const uint8_t*>(_mappedMemory) + offset;
            }

        private:
            void* _mappedMemory;
            size_t _size;
            bool _isValid;
        };

        class GLES : public IRenderer {
            using PointCoordinates = std::array<GLfloat, 8>;

            static constexpr PointCoordinates Vertices = {
                1, 0, // top right
                0, 0, // top left
                1, 1, // bottom right
                0, 1, // bottom left
            };

#ifdef __DEBUG__
            static std::string GetDebugGroupContext()
            {
                if (g_debugGroupStack.empty()) {
                    return "";
                }

                std::stringstream context;
                for (size_t i = 0; i < g_debugGroupStack.size(); ++i) {
                    if (i > 0)
                        context << " -> ";
                    // Extract just the function name from "file:line[func]"
                    const std::string& group = g_debugGroupStack[i];
                    size_t start = group.find('[');
                    size_t end = group.find(']');
                    if (start != std::string::npos && end != std::string::npos) {
                        context << group.substr(start + 1, end - start - 1);
                    } else {
                        context << group;
                    }
                }
                return context.str();
            }

            static void DebugSink(GLenum src, GLenum type, GLuint id, GLenum severity, GLsizei /*len*/, const GLchar* msg,
                const void* /*user*/)
            {
                std::stringstream line;
                line << "[" << Compositor::API::GL::SourceString(src)
                     << "](" << Compositor::API::GL::TypeString(type)
                     << ")";

                // Add debug group context if available
                std::string context = GetDebugGroupContext();
                if (!context.empty()) {
                    line << " in " << context;
                }

                // Add message ID for more specific error tracking
                if (id != 0) {
                    line << " (ID:" << id << ")";
                }

                line << ": " << msg;

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

            class GLESDebugScope {
            public:
                GLESDebugScope(const char* name)
                    : _pushed(false)
                {
                    ASSERT(eglGetCurrentContext() != EGL_NO_CONTEXT);

                    if (_gles.glPushDebugGroupKHR != nullptr) {
                        _gles.glPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION_KHR, 2, -1, name);

                        if (glGetError() == GL_NO_ERROR) {
                            g_debugGroupStack.push_back(name);
                            _pushed = true;
                        } else {
                            TRACE_GLOBAL(Trace::Error, ("Failed to push GL debug group '%s'", name));
                        }
                    }
                }

                ~GLESDebugScope()
                {
                    if ((_pushed == true) && (g_debugGroupStack.empty() == false) && _gles.glPopDebugGroupKHR != nullptr) {
                        g_debugGroupStack.pop_back();
                        _gles.glPopDebugGroupKHR();
                    }
                }

            private:
                bool _pushed;
            };

#define GLES_DEBUG_SCOPE(name) GLESDebugScope _debug_scope(name)

#else
#define GLES_DEBUG_SCOPE(name)
#endif

            // Framebuffers are usually connected to an output, it's the buffer for the composition to be rendered on.
            class GLESFrameBuffer : public IFrameBuffer {
            public:
                GLESFrameBuffer() = delete;
                GLESFrameBuffer(const GLESFrameBuffer&) = delete;
                GLESFrameBuffer& operator=(const GLESFrameBuffer&) = delete;

                GLESFrameBuffer(const GLES& parent, const Core::ProxyType<Exchange::IGraphicsBuffer>& buffer)
                    : _parent(parent)
                    , _eglImage()
                    , _glFrameBuffer(0)
                    , _glRenderBuffer(0)
                {
                    _parent.Egl().SetCurrent();
                    ASSERT(eglGetCurrentContext() != EGL_NO_CONTEXT);

                    GLES_DEBUG_SCOPE("GLESFrameBuffer Constructor");

                    TRACE_GLOBAL(Trace::GL, ("Creating framebuffer for buffer: %dx%d, format=0x%x, modifier=0x%" PRIx64 ", type=%d", buffer->Width(), buffer->Height(), buffer->Format(), buffer->Modifier(), buffer->Type()));

                    bool external(false);

                    {
                        GLES_DEBUG_SCOPE("CreateEGLImage");

                        _eglImage = _parent.Egl().CreateImage(buffer.operator->(), external);

                        if (_eglImage == EGL_NO_IMAGE) {
                            EGLint eglError = eglGetError();
                            TRACE_GLOBAL(Trace::Error, ("Failed to create EGL image: 0x%x", eglError));
                            ASSERT(false);
                        }
                    }

                    if (external) {
                        TRACE_GLOBAL(Trace::Error, ("Usage of external EGLImages in GLESFrameBuffers is not supported, please use a different buffer format or modifier"));
                        ASSERT(false);
                    }

                    ASSERT(_parent.Gles().glEGLImageTargetRenderbufferStorageOES != nullptr);

                    {
                        GLES_DEBUG_SCOPE("CreateRenderbuffer");

                        glGenRenderbuffers(1, &_glRenderBuffer);
                        glBindRenderbuffer(GL_RENDERBUFFER, _glRenderBuffer);

                        _parent.Gles().glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, _eglImage);

                        GLenum glError = glGetError();

                        if (glError != GL_NO_ERROR) {
                            TRACE_GLOBAL(Trace::Error, ("glEGLImageTargetRenderbufferStorageOES failed: 0x%x", glError));

                            // Check EGL error too
                            EGLint eglError = eglGetError();
                            if (eglError != EGL_SUCCESS) {
                                TRACE_GLOBAL(Trace::Error, ("EGL error after renderbuffer operation: 0x%x", eglError));
                            }
                        } else {
                            TRACE_GLOBAL(Trace::GL, ("glEGLImageTargetRenderbufferStorageOES succeeded"));
                        }

                        glBindRenderbuffer(GL_RENDERBUFFER, 0);
                    }

                    {
                        GLES_DEBUG_SCOPE("CreateFramebuffer");

                        glGenFramebuffers(1, &_glFrameBuffer);

                        Bind();

                        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _glRenderBuffer);

                        GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                        TRACE_GLOBAL(Trace::GL, ("Framebuffer status: 0x%x", fb_status));

                        if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
                            TRACE_GLOBAL(Trace::Error, ("Framebuffer incomplete: 0x%x", fb_status));

                            // Detailed framebuffer status logging
                            switch (fb_status) {
                            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                                TRACE_GLOBAL(Trace::Error, ("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"));
                                break;
                            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                                TRACE_GLOBAL(Trace::Error, ("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"));
                                break;
                            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                                TRACE_GLOBAL(Trace::Error, ("Framebuffer error: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS"));
                                break;
                            case GL_FRAMEBUFFER_UNSUPPORTED:
                                TRACE_GLOBAL(Trace::Error, ("Framebuffer error: GL_FRAMEBUFFER_UNSUPPORTED"));
                                break;
                            default:
                                TRACE_GLOBAL(Trace::Error, ("Framebuffer error: Unknown status 0x%x", fb_status));
                                break;
                            }
                        }

                        Unbind();

                        ASSERT(fb_status == GL_FRAMEBUFFER_COMPLETE);
                    }

                    TRACE(Trace::GL, ("Created GLESFrameBuffer %dpx x %dpx", buffer->Width(), buffer->Height()));

                    _parent.Egl().ResetCurrent();
                }

                ~GLESFrameBuffer()
                {
                    Renderer::EGL::ContextBackup backup;

                    _parent.Egl().SetCurrent();

                    // Unbind will be handled by glDeleteFramebuffers

                    glDeleteFramebuffers(1, &_glFrameBuffer);
                    glDeleteRenderbuffers(1, &_glRenderBuffer);

                    _parent.Egl().ResetCurrent();

                    TRACE(Trace::GL, ("GLESFrameBuffer %p destructed", this));
                }

                bool IsValid() const override
                {
                    return ((_glFrameBuffer != 0) && (_glRenderBuffer != 0) && (_eglImage != EGL_NO_IMAGE));
                }

                void Unbind() const override
                {
                    GLES_DEBUG_SCOPE("FrameBuffer::Unbind");
                    ASSERT(eglGetCurrentContext() != EGL_NO_CONTEXT);

                    glFlush();
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                }

                void Bind() const override
                {
                    GLES_DEBUG_SCOPE("FrameBuffer::Bind");

                    ASSERT(eglGetCurrentContext() != EGL_NO_CONTEXT);

                    glBindFramebuffer(GL_FRAMEBUFFER, _glFrameBuffer);
                }

            private:
                const GLES& _parent;
                EGLImage _eglImage;
                GLuint _glFrameBuffer;
                GLuint _glRenderBuffer;
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

            private:
                GLuint Compile(GLenum type, const char* source)
                {
                    GLES_DEBUG_SCOPE("Compile");
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
                    return handle;
                }

                GLuint Create(const uint8_t variant)
                {
                    GLES_DEBUG_SCOPE("Create");

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
                {
                    TRACE(Trace::Information, ("Created Color Program: id=%d, projection=%d, position=%d color=%d", Id(), Projection(), Position(), Color()));
                }

                virtual ~ColorProgram() = default;

                GLint Color() const
                {
                    return _color;
                }

                bool Draw(const Compositor::Color& color, const Matrix& matrix)
                {
                    GLES_DEBUG_SCOPE("ColorProgram::Draw");

                    if (color[3] >= 1.0) {
                        glDisable(GL_BLEND);
                    } else {
                        glEnable(GL_BLEND);
                    }

                    glUseProgram(Id());

                    glUniformMatrix3fv(Projection(), 1, GL_FALSE, &matrix[0]);

                    glUniform4f(_color, color[0], color[1], color[2], color[3]);

                    glVertexAttribPointer(Position(), 2, GL_FLOAT, GL_FALSE, 0, &Vertices[0]);

                    glEnableVertexAttribArray(Position());

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                    glDisableVertexAttribArray(Position());

                    return glGetError() == GL_NO_ERROR;
                }

            private:
                GLint _color;
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

                virtual ~TextureProgramType() = default;

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

                bool Draw(const GLuint id, GLenum target, const bool hasAlpha, const float& alpha, const Matrix& matrix, const PointCoordinates& coordinates) const
                {
                    GLES_DEBUG_SCOPE("TextureProgram::Draw");

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

                    return glGetError() == GL_NO_ERROR;
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

                GLESTexture(GLES& parent, const Core::ProxyType<Exchange::IGraphicsBuffer>& buffer)
                    : _parent(parent)
                    , _target(GL_TEXTURE_2D)
                    , _textureId(0)
                    , _image(EGL_NO_IMAGE)
                    , _buffer(buffer)
                {
                    ASSERT(_buffer != nullptr);

                    _parent.Add(this);

                    if (buffer->Type() == Exchange::IGraphicsBuffer::TYPE_DMA) {
                        ImportDMABuffer();
                    }

                    if (buffer->Type() == Exchange::IGraphicsBuffer::TYPE_RAW) {
                        ImportSHMBuffer();
                    }

                    ASSERT(_textureId != 0);

                    // Snapshot();
                }

                ~GLESTexture() override
                {
                    _parent.Remove(this);

                    Renderer::EGL::ContextBackup backup;
                    _parent.Egl().SetCurrent();

                    glDeleteTextures(1, &_textureId);

                    if (_image != EGL_NO_IMAGE) {
                        _parent.Egl().DestroyImage(_image);
                    }
                }

                /**
                 *  IRenderer::ITexture
                 */
                virtual bool IsValid() const
                {
                    return (_textureId != 0);
                }

                bool Invalidate()
                {
                    bool result(
                        (_buffer->Type() != Exchange::IGraphicsBuffer::TYPE_DMA)
                        || ((_image != EGL_NO_IMAGE) && (_target == GL_TEXTURE_EXTERNAL_OES)));

                    if ((_image != EGL_NO_IMAGE) && (_target != GL_TEXTURE_EXTERNAL_OES)) {
                        Renderer::EGL::ContextBackup backup;

                        _parent.Egl().SetCurrent();

                        glBindTexture(_target, _textureId);
                        _parent.Gles().glEGLImageTargetTexture2DOES(_target, _image);
                        glBindTexture(_target, 0);
                    }
                    return result;
                }

                uint32_t Width() const override { return _buffer->Width(); }
                uint32_t Height() const override { return _buffer->Height(); }
                GLenum Target() const { return _target; }
                GLuint Id() const { return _textureId; }

                bool Draw(const float& alpha, const Matrix& matrix, const PointCoordinates& coordinates) const
                {
                    GLES_DEBUG_SCOPE("GLESTexture::Draw");
                    bool result(false);
                    bool hasAlpha(DRM::HasAlpha(_buffer->Format()));

                    if ((_target == GL_TEXTURE_EXTERNAL_OES)) {
                        ExternalProgram* program = _parent.Programs().QueryType<ExternalProgram>();
                        ASSERT(program != nullptr);
                        result = program->Draw(_textureId, _target, hasAlpha, alpha, matrix, coordinates);
                    }

                    if ((_target == GL_TEXTURE_2D)) {
                        if (hasAlpha == true) {
                            RGBAProgram* program = _parent.Programs().QueryType<RGBAProgram>();
                            ASSERT(program != nullptr);
                            result = program->Draw(_textureId, _target, hasAlpha, alpha, matrix, coordinates);
                        } else {
                            RGBXProgram* program = _parent.Programs().QueryType<RGBXProgram>();
                            ASSERT(program != nullptr);
                            result = program->Draw(_textureId, _target, false, alpha, matrix, coordinates);
                        }
                    }

                    return result;
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

                    GLES_DEBUG_SCOPE("ImportDMABuffer");

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

                void ImportSHMBuffer()
                {
                    Exchange::IGraphicsBuffer::IIterator* planes = _buffer->Acquire(Compositor::DefaultTimeoutMs);
                    ASSERT(planes != nullptr);

                    planes->Next();

                    do {
                        int fd = planes->Descriptor();
                        const uint32_t stride = planes->Stride();
                        const uint32_t offset = planes->Offset();
                        const size_t totalSize = offset + (stride * _buffer->Height());

                        // One-time mapping with automatic cleanup
                        SHMMapper mapper(fd, totalSize);

                        if (!mapper.IsValid()) {
                            TRACE(Trace::Error, ("Failed to map SHM buffer for plane"));
                            continue;
                        }

                        const uint8_t* data = mapper.GetData(offset);
                        if (data == nullptr) {
                            TRACE(Trace::Error, ("Invalid offset %d for SHM buffer", offset));
                            continue;
                        }

                        GLPixelFormat glFormat = ConvertFormat(_buffer->Format());

                        Renderer::EGL::ContextBackup backup;
                        _parent.Egl().SetCurrent();

                        GLES_DEBUG_SCOPE("ImportPixelBuffer");

                        glGenTextures(1, &_textureId);
                        glBindTexture(_target, _textureId);

                        glTexParameteri(_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                        const uint32_t bytesPerPixel = glFormat.BitPerPixel / 8;
                        glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, stride / bytesPerPixel);
                        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                        glTexImage2D(_target, 0, glFormat.Format, _buffer->Width(), _buffer->Height(),
                            0, glFormat.Format, glFormat.Type, data);

                        glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);
                        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

                        glBindTexture(_target, 0);

                        TRACE(Trace::GL, ("Imported SHM texture: %dx%d, stride=%d, offset=%d", _buffer->Width(), _buffer->Height(), stride, offset));

                    } while (planes->Next());

                    _buffer->Relinquish();
                }

            private:
                GLES& _parent;
                GLenum _target;
                GLuint _textureId;
                EGLImageKHR _image;
                Core::ProxyType<Exchange::IGraphicsBuffer> _buffer;
            }; //  class GLESTexture

        public:
            GLES() = delete;
            GLES(GLES const&) = delete;
            GLES& operator=(GLES const&) = delete;

            GLES(int drmDevFd)
                : _egl(drmDevFd)
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

                _programs.Announce<ColorProgram>();
                _programs.Announce<ExternalProgram>();
                _programs.Announce<RGBAProgram>();
                _programs.Announce<RGBXProgram>();
                // _programs.Announce<XYUVProgram>();
                // _programs.Announce<Y_UVProgram>();
                // _programs.Announce<Y_U_VProgram>();
                // _programs.Announce<Y_XUXVProgram>();

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

            uint32_t Unbind(const Core::ProxyType<IFrameBuffer>& frameBuffer) override
            {
                if (frameBuffer.IsValid() == true) {
                    _egl.SetCurrent();
                    frameBuffer->Unbind();
                } else {
                    return Core::ERROR_BAD_REQUEST;
                }

                _egl.ResetCurrent();

                return (eglGetError() == EGL_SUCCESS) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
            }

            uint32_t Bind(const Core::ProxyType<IFrameBuffer>& frameBuffer) override
            {
                ASSERT(_rendering == false);

                _egl.SetCurrent();

                GLES_DEBUG_SCOPE("GLES::Bind");

                if (frameBuffer.IsValid() == true) {
                    frameBuffer->Bind();
                } else {
                    return Core::ERROR_BAD_REQUEST;
                }

                return (eglGetError() == EGL_SUCCESS) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
            }

            bool Begin(uint32_t width, uint32_t height) override
            {
                ASSERT((_rendering == false) && (_egl.IsCurrent() == true));

                GLES_DEBUG_SCOPE("GLES::Begin");

                _rendering = true;

                if (_gles.glGetGraphicsResetStatusKHR != nullptr) {
                    GLenum status = _gles.glGetGraphicsResetStatusKHR();
                    if (status != GL_NO_ERROR) {
                        TRACE(Trace::Error, ("GPU reset (%s)", API::GL::ResetStatusString(status)));
                        return false;
                    }
                }

                glViewport(0, 0, width, height);

                _viewportWidth = width;
                _viewportHeight = height;

                // refresh projection matrix
                Transformation::Projection(_projection, _viewportWidth, _viewportHeight, Transformation::TRANSFORM_NORMAL);

                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                return (glGetError() == GL_NO_ERROR);
            }

            void End(bool dump) override
            {
                ASSERT((_rendering == true) && (_egl.IsCurrent() == true));

                GLES_DEBUG_SCOPE("GLES::End");

                if (dump == true) {
                    std::vector<uint8_t> pixels;
                    Exchange::IComposition::Rectangle box = { 0, 0, _viewportWidth, _viewportHeight };
                    if (Snapshot(box, DRM_FORMAT_ARGB8888, pixels) == true) {
                        std::stringstream ss;
                        ss << "renderer-end-snapshot-" << Core::Time::Now().Ticks() << ".png" << std::ends;
                        Core::File snapshot(ss.str());

                        WritePNG(ss.str(), pixels, _viewportWidth, _viewportHeight);
                    }
                }

                // nothing to do

                _rendering = false;
            }

            PUSH_WARNING(DISABLE_WARNING_OVERLOADED_VIRTUALS)
            void Clear(const Color color) override
            {
                ASSERT((_rendering == true) && (_egl.IsCurrent() == true));

                GLES_DEBUG_SCOPE("GLES::Clear");

                glClearColor(color[0], color[1], color[2], color[3]);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            POP_WARNING()

            void Scissor(const Exchange::IComposition::Rectangle* box) override
            {
                ASSERT((_rendering == true) && (_egl.IsCurrent() == true));

                GLES_DEBUG_SCOPE("GLES::Scissor");

                if (box != nullptr) {
                    glScissor(box->x, box->y, box->width, box->height);
                    glEnable(GL_SCISSOR_TEST);
                } else {
                    glDisable(GL_SCISSOR_TEST);
                }
            }

            Core::ProxyType<IFrameBuffer> FrameBuffer(const Core::ProxyType<Exchange::IGraphicsBuffer>& buffer) override
            {
                return (Core::ProxyType<IFrameBuffer>(Core::ProxyType<GLESFrameBuffer>::Create(*this, buffer)));
            };

            Core::ProxyType<ITexture> Texture(const Core::ProxyType<Exchange::IGraphicsBuffer>& buffer) override
            {
                return (Core::ProxyType<ITexture>(Core::ProxyType<GLESTexture>::Create(*this, buffer)));
            };

            uint32_t Render(const Core::ProxyType<ITexture>& texture, const Exchange::IComposition::Rectangle& region, const Matrix transformation, const float alpha) override
            {
                GLES_DEBUG_SCOPE("GLES::Render");
                ASSERT((_rendering == true) && (_egl.IsCurrent() == true) && (texture != nullptr));

                const auto index = std::find(_textures.begin(), _textures.end(), &(*texture));

                uint32_t result(Core::ERROR_BAD_REQUEST);

                if (index != _textures.end()) {
                    const GLESTexture* glesTexture = reinterpret_cast<const GLESTexture*>(*index);

                    Matrix gl_matrix;
                    Transformation::Multiply(gl_matrix, _projection, transformation);
                    Transformation::Multiply(gl_matrix, Transformation::Transformations[Transformation::TRANSFORM_NORMAL], transformation);
                    Transformation::Transpose(gl_matrix, gl_matrix);

                    const GLfloat x1 = region.x / glesTexture->Width();
                    const GLfloat y1 = region.y / glesTexture->Height();
                    const GLfloat x2 = (region.x + region.width) / glesTexture->Width();
                    const GLfloat y2 = (region.y + region.height) / glesTexture->Height();

                    const PointCoordinates coordinates = {
                        x2, y1, // top right
                        x1, y1, // top left
                        x2, y2, // bottom right
                        x1, y2, // bottom left
                    };

                    result = (glesTexture->Draw(alpha, gl_matrix, coordinates) == true) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
                }

                return result;
            }

            uint32_t Quadrangle(const Color color, const Matrix transformation) override
            {
                GLES_DEBUG_SCOPE("GLES::Quadrangle");
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
                return _egl.RenderFormats();
            }

            const std::vector<PixelFormat>& TextureFormats() const override
            {
                return _egl.TextureFormats();
            }
            Core::ProxyType<Exchange::IGraphicsBuffer> Bound() const override
            {
                return (Core::ProxyType<Exchange::IGraphicsBuffer>());
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
                return (_egl.IsCurrent() == true);
            }

            bool Snapshot(const Exchange::IComposition::Rectangle& box, const uint32_t format, std::vector<uint8_t>& pixels)
            {
                GLES_DEBUG_SCOPE("GLES::Snapshot");

                const GLPixelFormat formatGL(ConvertFormat(format));

                glFinish();
                glGetError();

                pixels.clear();
                pixels.resize(_viewportWidth * _viewportHeight * (formatGL.BitPerPixel / 8));

                glReadPixels(box.x, box.y, box.width, box.height, formatGL.Format, formatGL.Type, pixels.data());

                return glGetError() == GL_NO_ERROR;
            }

            void Add(IRenderer::ITexture* texture)
            {
                auto index = std::find(_textures.begin(), _textures.end(), texture);

                ASSERT(index == _textures.end());

                _textures.emplace_back(texture);
            }

            void Remove(IRenderer::ITexture* texture)
            {
                auto index = std::find(_textures.begin(), _textures.end(), texture);

                if (index != _textures.end()) {
                    _textures.erase(index);
                }
            }

            ProgramRegistry& Programs()
            {
                return _programs;
            }

            using TextureRegister = std::list<IRenderer::ITexture*>;

        private:
            static API::GL _gles;

        private:
            EGL _egl;
            uint32_t _viewportWidth;
            uint32_t _viewportHeight;
            Matrix _projection;
            ProgramRegistry _programs;
            mutable bool _rendering;
            TextureRegister _textures;
        }; // class GLES

        API::GL GLES::_gles;

        constexpr GLES::PointCoordinates GLES::Vertices;

    } // namespace Renderer

    Core::ProxyType<IRenderer> IRenderer::Instance(Core::instance_id identifier)
    {
        ASSERT(int(identifier) >= 0); // this should be a valid file descriptor.

        static Core::ProxyMapType<Core::instance_id, IRenderer> glRenderers;

        return glRenderers.Instance<Renderer::GLES>(identifier, static_cast<int>(identifier));
    }

} // namespace Compositor
} // namespace Thunder
