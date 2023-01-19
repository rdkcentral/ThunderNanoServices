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

#ifndef MODULE_NAME
#define MODULE_NAME CompositorRendererGLES2
#endif

#include <tracing/tracing.h>

#include <IRenderer.h>
#include <compositorbuffer/IBuffer.h>

#include "EGL.h"
#include "RenderAPI.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>

#include <core/core.h>

// these headers are generated
#include <fragment-shader.h>
#include <vertex-shader.h>

MODULE_NAME_ARCHIVE_DECLARATION

namespace {
namespace Trace {
    class GL {
    public:
        ~GL() = default;
        GL() = delete;
        GL(const GL&) = delete;
        GL& operator=(const GL&) = delete;
        GL(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            WPEFramework::Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit GL(const string& text)
            : _text(WPEFramework::Core::ToString(text))
        {
        }

    public:
        const char* Data() const
        {
            return (_text.c_str());
        }
        uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    }; // class GL
}
}

namespace Compositor {
namespace Renderer {
    class GLES : public Interfaces::IRenderer {
        struct Buffer {
            // struct wlr_gles2_renderer* renderer;

            EGLImageKHR image;
            GLuint rbo;
            GLuint fbo;

            WPEFramework::Core::ProxyType<Interfaces::IBuffer> realBuffer;
        };

        static constexpr GLfloat verts[] = {
            1, 0, // top right
            0, 0, // top left
            1, 1, // bottom right
            0, 1, // bottom left
        };

#define PushDebug() RealPushDebug(__FILE__, __func__, __LINE__)
        void RealPushDebug(const std::string& file, const std::string& func, const uint32_t line)
        {
            if (_api.glPushDebugGroupKHR == nullptr) {
                return;
            }

            std::stringstream message;
            message << file << ":" << line << "[" << func << "]";

            _api.glPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION_KHR, 1, -1, message.str().c_str());
        }

        void PopDebug()
        {
            if (_api.glPopDebugGroupKHR != nullptr) {
                _api.glPopDebugGroupKHR();
            }
        }

        static void DebugSink(GLenum src, GLenum type, GLuint id, GLenum severity,
            GLsizei len, const GLchar* msg, const void* user)
        {
            std::stringstream line;
            line << ", source: " << Compositor::API::GL::SourceString(src)
                 << ", type: " << Compositor::API::GL::TypeString(type)
                 << ", message: \"" << msg << "\"";

            switch (severity) {

            case GL_DEBUG_SEVERITY_HIGH_KHR: {
                TRACE_GLOBAL(WPEFramework::Trace::Error, ("%s", line.str().c_str()));
                break;
            }
            case GL_DEBUG_SEVERITY_MEDIUM_KHR: {
                TRACE_GLOBAL(WPEFramework::Trace::Warning, ("%s", line.str().c_str()));
                break;
            }
            case GL_DEBUG_SEVERITY_LOW_KHR: {
                TRACE_GLOBAL(WPEFramework::Trace::Information, ("%s", line.str().c_str()));
                break;
            }
            case GL_DEBUG_SEVERITY_NOTIFICATION_KHR:
            default: {
                TRACE_GLOBAL(Trace::GL, ("%s", line.str().c_str()));
                break;
            }
            }
        }

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
                // PushDebug();

                GLint status(GL_FALSE);
                GLuint handle = glCreateShader(type);

                // TRACE(Trace::GL, ("Shader[0x%02X]=%d source: \n%s", type, handle, source));

                glShaderSource(handle, 1, &source, NULL);
                glCompileShader(handle);

                glGetShaderiv(handle, GL_COMPILE_STATUS, &status);

                if (status == GL_FALSE) {
                    TRACE(WPEFramework::Trace::Error, ("Shader %d compilation failed: %s", handle, API::GL::ShaderInfoLog(handle).c_str()));
                    glDeleteShader(handle);
                    handle = GL_FALSE;
                }

                // PopDebug();
                return handle;
            }

            GLuint Create(const uint8_t variant)
            {
                // PushDebug();

                GLuint handle(GL_FALSE);

                const char* VariantStr[] = {
                    "NONE",
                    "SOLID",
                    "TEXTURE_EXTERNAL",
                    "TEXTURE_RGBA",
                    "TEXTURE_RGBX",
                    "TEXTURE_Y_U_V",
                    "TEXTURE_Y_UV",
                    "TEXTURE_Y_XUXV",
                    "TEXTURE_XYUV"
                };

                TRACE(WPEFramework::Trace::Error, ("Creating Program for %s", VariantStr[variant]));

                std::stringstream fragmentShaderSource;
                fragmentShaderSource
                    << "#define VARIANT " << static_cast<uint16_t>(variant) << std::endl
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
                        TRACE(WPEFramework::Trace::Error, ("Program linking failed: %s", API::GL::ProgramInfoLog(handle).c_str()));
                        glDeleteProgram(handle);
                        handle = GL_FALSE;
                    }

                } else {
                    TRACE(WPEFramework::Trace::Error, ("Error Creating Program"));
                }

                // PopDebug();

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

        class ColourProgram : public Program {
        public:
            enum { ID = static_cast<std::underlying_type<VariantType>::type>(Variant::SOLID) };

        public:
            ColourProgram()
                : Program(ID)
                , _color(glGetUniformLocation(Id(), "color"))
            {
            }

            virtual ~ColourProgram() = default;

            GLint Color() const
            {
                return _color;
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
                    char parameter[len];
                    snprintf(parameter, len, "tex%d", index++);

                    id = glGetUniformLocation(Id(), parameter);
                }
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

        private:
            std::array<GLint, TEXTURE_COUNT> _textureIds;
            GLint _coordinates;
            GLuint _alpha;
        };

        using ExternalProgram = TextureProgramType<Program::Variant::TEXTURE_EXTERNAL, 1>;
        using RGBAProgram = TextureProgramType<Program::Variant::TEXTURE_RGBA, 1>;
        using RGBXProgram = TextureProgramType<Program::Variant::TEXTURE_RGBX, 1>;
        using XYUVProgram = TextureProgramType<Program::Variant::TEXTURE_XYUV, 1>;
        using Y_UVProgram = TextureProgramType<Program::Variant::TEXTURE_Y_UV, 2>;
        using Y_U_VProgram = TextureProgramType<Program::Variant::TEXTURE_Y_U_V, 3>;
        using Y_XUXVProgram = TextureProgramType<Program::Variant::TEXTURE_Y_XUXV, 2>;

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
                    // ASSERT(dynamic_cast<PROGRAM*>(base) != nullptr);
                    result = static_cast<PROGRAM*>(base);
                }
                return (result);
            }

        private:
            Programs _programs;
        };

    public:
        GLES() = delete;
        GLES(GLES const&) = delete;
        GLES& operator=(GLES const&) = delete;

        GLES(int fd)
            : _fd(fd)
            , _egl(fd)
            , _current_buffer(nullptr)
            , _viewport_width(0)
            , _viewport_height(0)
            , _programs()
        {
            _egl.SetCurrent();

            TRACE(Trace::GL, ("%s - build: %s\n version: %s\n renderer: %s\n vendor: %s", __func__, __TIMESTAMP__, glGetString(GL_VERSION), glGetString(GL_RENDERER), glGetString(GL_VENDOR)));

            ASSERT(_egl.IsCurrent() == true);

            if (API::GL::HasExtension("GL_KHR_debug")) {
                glEnable(GL_DEBUG_OUTPUT_KHR);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);

                _api.glDebugMessageCallbackKHR(DebugSink, this);

                // Silence unwanted message types
                _api.glDebugMessageControlKHR(GL_DONT_CARE,
                    GL_DEBUG_TYPE_POP_GROUP_KHR, GL_DONT_CARE, 0, nullptr, GL_FALSE);
                _api.glDebugMessageControlKHR(GL_DONT_CARE,
                    GL_DEBUG_TYPE_PUSH_GROUP_KHR, GL_DONT_CARE, 0, nullptr, GL_FALSE);
            }

            PushDebug();

            _programs.Announce<ColourProgram>();
            _programs.Announce<ExternalProgram>();
            _programs.Announce<RGBAProgram>();
            _programs.Announce<RGBXProgram>();
            _programs.Announce<XYUVProgram>();
            _programs.Announce<Y_UVProgram>();
            _programs.Announce<Y_U_VProgram>();
            _programs.Announce<Y_XUXVProgram>();

            PopDebug();

            _egl.ResetCurrent();
        }

        virtual ~GLES()
        {
            if (API::GL::HasExtension("GL_KHR_debug")) {
                glDisable(GL_DEBUG_OUTPUT_KHR);
                _api.glDebugMessageCallbackKHR(nullptr, nullptr);
            }
        };

        uint32_t Configure(const string& config)
        {
            return WPEFramework::Core::ERROR_NONE;
        }

        uint32_t Bind(Interfaces::IBuffer* buffer) override
        {

            if (_current_buffer != nullptr) {
                ASSERT(_egl.IsCurrent() == true);
                PushDebug();

                glFlush();
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                PopDebug();

                _current_buffer->realBuffer->Completed(false);
                _current_buffer = nullptr;
            }

            if (buffer == nullptr) {
                _egl.ResetCurrent();
                return true;
            }

            _egl.SetCurrent();

            Buffer* glBuffer = dynamic_cast<Buffer*>(buffer);

            // if (glBuffer == NULL) {
            //     // glBuffer = CreateBuffer(buffer);
            // }

            if (glBuffer == NULL) {
                return false;
            }

            // buffer->realBuffer->Lock(100);

            _current_buffer = glBuffer;

            PushDebug();

            glBindFramebuffer(GL_FRAMEBUFFER, _current_buffer->fbo);

            PopDebug();
            return true;
        }

        bool Begin(uint32_t width, uint32_t height) override
        {
            PushDebug();
            // gles2_get_renderer_in_context(wlr_renderer);

            // if (renderer->procs.glGetGraphicsResetStatusKHR) {
            //     GLenum status = renderer->procs.glGetGraphicsResetStatusKHR();
            //     if (status != GL_NO_ERROR) {
            //         wlr_log(WLR_ERROR, "GPU reset (%s)", reset_status_str(status));
            //         wl_signal_emit_mutable(&wlr_renderer->events.lost, NULL);
            //         return false;
            //     }
            // }

            glViewport(0, 0, width, height);

            _viewport_width = width;
            _viewport_height = height;

            // refresh projection matrix
            //_projection = matrix_projection(width, height, WL_OUTPUT_TRANSFORM_FLIPPED_180);

            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            // XXX: maybe we should save output projection and remove some of the need
            // for users to sling matricies themselves

            PopDebug();
            return true;
        }

        void End() override
        {
            // nothing to do
            _current_buffer->realBuffer->Completed(true);
        }

        void Clear(const Color color) override
        {
            PushDebug();
            glClearColor(color[0], color[1], color[2], color[3]);
            glClear(GL_COLOR_BUFFER_BIT);
            PopDebug();
        }

        void Scissor(const Box* box) override
        {
            PushDebug();
            if (box != nullptr) {
                glScissor(box->x, box->y, box->width, box->height);
                glEnable(GL_SCISSOR_TEST);
            } else {
                glDisable(GL_SCISSOR_TEST);
            }
            PopDebug();
        }

        uint32_t Render(Interfaces::IBuffer* texture, const Box region, const Matrix transform, float alpha) override
        {
            PushDebug();
            PopDebug();
            return 0;
        }

        uint32_t Quadrangle(const Color color, const Matrix transformation) override
        {
            PushDebug();
            PopDebug();
            return 0;
        }

        Interfaces::IBuffer* Bound() const
        {
            return nullptr;
        }

        int Handle() const
        {
            return _fd;
        }

        const std::vector<PixelFormat>& RenderFormats() const override
        {
            return _egl.Formats();
        }

        const std::vector<PixelFormat>& TextureFormats() const override
        {
            return _egl.Formats();
        }

    private:
        bool HasContext() const
        {
            return ((_egl.IsCurrent() == true) && (_current_buffer != nullptr));
        }

        void CreateBuffer()
        {
        }

        static API::GL _api;

    private:
        int _fd;
        EGL _egl;
        Buffer* _current_buffer;
        uint32_t _viewport_width;
        uint32_t _viewport_height;
        Matrix _projection;
        ProgramRegistry _programs;
    }; // class GLES

    API::GL GLES::_api;

} // namespace Renderer

WPEFramework::Core::ProxyType<Interfaces::IRenderer>
Interfaces::IRenderer::Instance(WPEFramework::Core::instance_id identifier)
{
    static WPEFramework::Core::ProxyMapType<WPEFramework::Core::instance_id, Interfaces::IRenderer> glRenderers;

    return glRenderers.Instance<Renderer::GLES>(identifier, static_cast<int>(identifier));
}

} // namespace Compositor
