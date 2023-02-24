/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "Module.h"

#include <com/com.h>
#include <core/core.h>
#include <interfaces/IComposition.h>

#include "CompositorTrace.h"
#include "ModeSet.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <fcntl.h>
#include <gbm.h>
#include <sys/socket.h>

#ifdef __cplusplus
}
#endif

#include <chrono>
#include <cstring>
#include <functional>
#include <inttypes.h>
#include <mutex>
#include <queue>
#include <set>
#include <stack>
#include <stdlib.h>
#include <type_traits>

#ifndef EGL_VERSION_1_5
using EGLImage = EGLImageKHR;
using EGL_NO_IMAGE = EGL_NO_IMAGE_KHR;
#endif

#ifndef GL_KHR_debug
#error "No GL_KHR_debug"
#endif

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {
    constexpr char DmaFdConnector[] = "/tmp/Compositor/DmaFdConnector";
    constexpr int InvalidFd = ModeSet::GBM::InvalidFd();

    class CompositorImplementation : public Exchange::IComposition, public Exchange::IComposition::IDisplay {
    public:
        class Client : public Exchange::IComposition::IClient,
                       public Exchange::IComposition::IRender,
                       public Exchange::IComposition::IClient::IProperties {
        public:
            // The buffer acts as a surface for the remote site
            struct SurfaceType {
                struct gbm_bo* _buf;
                int _fd;
                EGLImage _image;

                SurfaceType()
                    : _buf(nullptr)
                    , _fd(InvalidFd)
                    , _image(EGL_NO_IMAGE)
                {
                }

                bool IsValid() const
                {
                    return (_buf != nullptr) && (_fd != InvalidFd) && (_image != EGL_NO_IMAGE);
                }
            };

        public:
            Client() = delete;
            Client(const Client&) = delete;
            Client& operator=(const Client&) = delete;

            Client(ModeSet& modeSet, CompositorImplementation& compositor, const string& name, const uint32_t width, const uint32_t height)
                : _nativeSurface()
                , _modeSet(modeSet)
                , _compositor(compositor)
                , _name(name)
                , _opacity(Exchange::IComposition::maxOpacity)
                , _layer(0)
                , _destination({ 0, 0, width, height })
            {
                // The compositor should not be destroyed before any existing client surface
                _compositor.AddRef();

                // TODO: The internal scan out flag might not be appropriate
                _nativeSurface._buf = _modeSet.CreateBufferObject(width, height);

                if (_nativeSurface._buf != nullptr) {
                    _nativeSurface._fd = gbm_bo_get_fd(_nativeSurface._buf); // acts

                    TRACE(CompositorTrace::Client, (_T("Constructed Client gbm_bo_fd=%d, width=%d, height=%d, stride=%d, format=0x%04X, modifier=0x%" PRIX64), _nativeSurface._fd, gbm_bo_get_width(_nativeSurface._buf), gbm_bo_get_height(_nativeSurface._buf), gbm_bo_get_stride(_nativeSurface._buf), gbm_bo_get_format(_nativeSurface._buf), gbm_bo_get_modifier(_nativeSurface._buf)));
                } else {
                    TRACE(Trace::Error, (_T ( "A Client cannot be created for %s" ), name.c_str()));
                }
            }

            ~Client() override
            {
                // Part of the client is cleaned up via the detached (hook)
                if (_nativeSurface._buf != ModeSet::GBM::InvalidBuf()) {
                    _modeSet.DestroyBufferObject(_nativeSurface._buf);
                }

                if (_nativeSurface._fd != InvalidFd) {
                    int ret = close(_nativeSurface._fd);
                    ASSERT(ret != -1);
                }

                _compositor.Release();
            }

            // Underlying native surface handle
            Core::instance_id Native() const override
            {
                return static_cast<Core::instance_id>(_nativeSurface._fd);
            }

            // Just me
            string Name() const override
            {
                return _name;
            }

            // Opacity
            void Opacity(const uint32_t value) override { _opacity = value; }
            uint32_t Opacity() const override { return _opacity; }

            // Geometry
            uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override
            {
                _destination = rectangle;
                TRACE(CompositorTrace::Client, (_T("Set %s Geometry x=%d y=%d width=%d height=%d is set for client surface %s"), _name.c_str(), _destination.x, _destination.y, _destination.width, _destination.height));
                return static_cast<uint32_t>(Core::ERROR_NONE);
            }
            Exchange::IComposition::Rectangle Geometry() const override
            {
                return _destination;
            }

            // Z-ordering within multiple surfaces
            uint32_t ZOrder(const uint16_t zorder) override
            {
                _layer = zorder;
                return static_cast<uint32_t>(Core::ERROR_NONE);
            }
            uint32_t ZOrder() const override { return _layer; }

            // Buffer to display flipping
            void ScanOut() override
            {
                bool composed = _compositor.CompositeFor(_name);
                TRACE(CompositorTrace::Client, (_T("%s:%d %s for %s %s"), __FILE__, __LINE__, __FUNCTION__, _name.c_str(), (composed) ? "OK" : "Failed"));
            }

            const SurfaceType& Surface(const EGLImage& khr = EGL_NO_IMAGE)
            {
                TRACE(CompositorTrace::Client, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
                if (khr != EGL_NO_IMAGE) {
                    _nativeSurface._image = khr;
                }

                return _nativeSurface;
            }

            // IComposition::IClient::IProperties
            uint32_t Stride()
            {
                return gbm_bo_get_stride(_nativeSurface._buf);
            }
            uint32_t Format()
            {
                return gbm_bo_get_format(_nativeSurface._buf);
            }
            uint32_t Offset()
            {
                return gbm_bo_get_offset(_nativeSurface._buf, 0);
            }
            uint64_t Modifier()
            {
                return gbm_bo_get_modifier(_nativeSurface._buf);
            }

            BEGIN_INTERFACE_MAP(Client)
            INTERFACE_ENTRY(Exchange::IComposition::IClient)
            INTERFACE_ENTRY(Exchange::IComposition::IRender)
            INTERFACE_ENTRY(Exchange::IComposition::IClient::IProperties)
            END_INTERFACE_MAP

        private:
            SurfaceType _nativeSurface;
            ModeSet& _modeSet;
            CompositorImplementation& _compositor;
            std::string const _name;

            uint32_t _opacity;
            uint32_t _layer;

            Exchange::IComposition::Rectangle _destination;
        }; // class Client

        using ClientContainer = Core::ProxyMapType<string, Client>;

        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

    private:
        class DmaFdServer : public Core::PrivilegedRequest {
        public:
            DmaFdServer() = delete;
            DmaFdServer(const DmaFdServer&) = delete;
            DmaFdServer& operator=(const DmaFdServer&) = delete;

            DmaFdServer(CompositorImplementation& parent)
                : _parent(parent)
            {
            }

            ~DmaFdServer()
            {
                Core::PrivilegedRequest::Close();
            }

            int Service(const uint32_t id) override
            {
                return _parent.SurfaceDmaFd(id);
            }

        private:
            CompositorImplementation& _parent;
        };

        // The communication channel with the (remote) other site
        class ExternalAccess : public RPC::Communicator {
        public:
            ExternalAccess() = delete;
            ExternalAccess(const ExternalAccess&) = delete;
            ExternalAccess& operator=(const ExternalAccess&) = delete;

            ExternalAccess(CompositorImplementation&, const Core::NodeId&, const string&, const Core::ProxyType<RPC::InvokeServer>&);
            ~ExternalAccess() override = default;

        private:
            void* Acquire(const string&, uint32_t const, uint32_t const) override;

            CompositorImplementation& _parent;
        };

        class Natives final {
        public:
            static constexpr ModeSet::GBM::dev_t InvalidDisplay = nullptr;
            static constexpr ModeSet::GBM::surf_t InvalidSurface = nullptr;

            Natives() = delete;
            Natives(const Natives&) = delete;
            Natives& operator=(const Natives&) = delete;

            explicit Natives(ModeSet& modeSet)
                : _set(modeSet)
                , _gbmSurface(InvalidSurface)
            {
                Initialize();
            }

            ~Natives()
            {
                Deinitialize();
            }

            ModeSet::GBM::dev_t Display() const { return _set.UnderlyingHandle(); }
            ModeSet::GBM::surf_t Surface() const { return _gbmSurface; }

            void Invalidate()
            {
                Deinitialize();
            }

            bool Valid() const
            {
                return (_gbmSurface != InvalidSurface);
            }

        private:
            bool Initialize()
            {
                bool ret = false;

                ret = _set.Open("") == Core::ERROR_NONE && Display() != InvalidDisplay;

                if (ret == true) {
                    _gbmSurface = _set.CreateRenderTarget(_set.Width(), _set.Height());
                    ret = _gbmSurface != InvalidSurface;
                }

                if (ret != true) {
                    TRACE(Trace::Error, (_T ( "Unable to create a compositor surface of dimensions: %u x %u [width, height]))." ), _set.Width(), _set.Height()));
                }

                return ret;
            }
            void Deinitialize()
            {
                if (_gbmSurface != InvalidSurface) {
                    _set.DestroyRenderTarget(_gbmSurface);
                }

                _set.Close();
            }

        private:
            ModeSet& _set;
            ModeSet::GBM::surf_t _gbmSurface;
        };

        class GLES final {
        private:
            // x, y, z
            static constexpr const uint8_t VerticeDimensions = 3;

            struct offset {
                // using GLfloat = GLfloat;

                // Each coordinate in the range [-1.0f, 1.0f]
                static constexpr const GLfloat _left = static_cast<GLfloat>(-1.0f);
                static constexpr const GLfloat _right = static_cast<GLfloat>(1.0f);
                static constexpr const GLfloat _bottom = static_cast<GLfloat>(-1.0f);
                static constexpr const GLfloat _top = static_cast<GLfloat>(1.0f);
                static constexpr const GLfloat _near = static_cast<GLfloat>(-1.0f);
                static constexpr const GLfloat _far = static_cast<GLfloat>(1.0f);

                GLfloat _x;
                GLfloat _y;
                GLfloat _z;

                offset();
                explicit offset(const GLfloat&, const GLfloat&, const GLfloat&);
            } _offset;

            struct scale {
                static constexpr const GLclampf _identity = static_cast<GLclampf>(1.0f);
                static constexpr const GLclampf _min = static_cast<GLclampf>(0.0f);
                static constexpr const GLclampf _max = static_cast<GLclampf>(1.0f);

                GLclampf _horiz;
                GLclampf _vert;

                scale();
                explicit scale(const GLclampf&, const GLclampf&);
            } _scale;

            struct opacity {
                static constexpr const GLfloat _min = static_cast<GLfloat>(0.0f);
                static constexpr const GLfloat _max = static_cast<GLfloat>(1.0f);

                GLfloat _alpha;

                opacity();
                explicit opacity(const GLfloat&);
                explicit opacity(const opacity&);

                opacity& operator=(const GLfloat& alpha)
                {
                    _alpha = alpha;
                    return *this;
                }

                opacity& operator=(const opacity& rhs)
                {
                    _alpha = rhs._alpha;
                    return *this;
                }

                static const opacity& InitialOpacity()
                {
                    static const opacity op(_max);
                    return op;
                }
            } _opacity;

            bool _valid;

            static constexpr GLuint InvalidProg() { return static_cast<GLuint>(0); }

        public:
            struct texture {
                GLuint _tex;
                GLuint _target;

                GLES::opacity _opacity;

                GLuint _x;
                GLuint _y;
                GLuint _z;

                GLuint _width;
                GLuint _height;

                texture();
                explicit texture(GLuint const, const GLES::opacity&);
                explicit texture(const texture&);

                texture& operator=(const texture& rhs)
                {
                    _tex = rhs._tex;
                    _target = rhs._target;

                    _opacity = rhs._opacity;

                    _x = rhs._x;
                    _y = rhs._y;
                    _z = rhs._z;

                    _width = rhs._width;
                    _height = rhs._height;

                    return *this;
                }
            };

        private:
            std::map<EGLImage, texture> _scene;

            std::stack<texture> _preallocated;

            std::mutex _token;

        public:
            GLES(GLES const&) = delete;
            GLES& operator=(GLES const&) = delete;

            GLES();
            ~GLES();

            static const offset InitialOffset() { return offset(); }
            bool UpdateOffset(const offset&);

            static const scale InitialScale() { return scale(); }
            bool UpdateScale(const scale&);

            static const opacity& InitialOpacity() { return CompositorImplementation::GLES::opacity::InitialOpacity(); }
            bool UpdateOpacity(const opacity&);

            static constexpr GLuint InvalidTex() { return static_cast<GLuint>(0); }

            static constexpr GLuint MajorVersion() { return 2; }
            static constexpr GLuint MinorVersion() { return 0; }

            bool Valid() const { return _valid; }

            bool Render() { return Valid(); }
            bool RenderColor(const bool, const bool, const bool, const bool alpha = true);
            bool RenderEGLImage(const EGLImage&, EGLint const, EGLint const, EGLint const, EGLint const, EGLint, EGLint const);
            bool RenderScene(GLuint const, GLuint const, std::function<bool(const texture&, const texture&)>);

            bool SkipEGLImageFromScene(const EGLImage&);

        private:
            bool Initialize();
            bool Deinitialize();

            // TODO: precompile programs at initialization stage

            bool SetupProgram(const char[], const char[], GLuint&);

            bool RenderTileOES();
            bool RenderTile();

            template <size_t N>
            bool RenderPolygon(const std::array<const GLfloat, N>&);

            bool Supported(const std::string&);

            bool SetupViewport(EGLint const, EGLint const, EGLint const, EGLint const);
        };

        class EGL {
        private:
#define XSTRINGIFY(X) STRINGIFY(X)
#define STRINGIFY(X) #X

#ifdef EGL_VERSION_1_5
#define KHRFIX(name) name
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE
#define _EGL_NO_SYNC EGL_NO_SYNC
#define _EGL_FOREVER EGL_FOREVER
#define _EGL_NO_IMAGE EGL_NO_IMAGE
#define _EGL_NATIVE_PIXMAP EGL_NATIVE_PIXMAP_KHR
#define _EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED
#define _EGL_SYNC_STATUS EGL_SYNC_STATUS
#define _EGL_SIGNALED EGL_SIGNALED
#define _EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT
#else
#define _KHRFIX(left, right) left##right
#define KHRFIX(name) _KHRFIX(name, KHR)
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE_KHR
#define _EGL_NO_SYNC EGL_NO_SYNC_KHR
#define _EGL_FOREVER EGL_FOREVER_KHR
#define _EGL_NO_IMAGE EGL_NO_IMAGE_KHR
#define _EGL_NATIVE_PIXMAP EGL_NATIVE_PIXMAP_KHR
#define _EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED_KHR
#define _EGL_SYNC_STATUS EGL_SYNC_STATUS_KHR
#define _EGL_SIGNALED EGL_SIGNALED_KHR
#define _EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT_KHR

            using EGLAttrib = EGLint;
#endif
            using EGLuint64KHR = khronos_uint64_t;

            EGLContext _ctx = EGL_NO_CONTEXT;

#define CASE_STR(value) \
    case value:         \
        return #value;

            static const char* EGLErrorString(EGLint error)
            {
                switch (error) {
                    CASE_STR(EGL_SUCCESS)
                    CASE_STR(EGL_NOT_INITIALIZED)
                    CASE_STR(EGL_BAD_ACCESS)
                    CASE_STR(EGL_BAD_ALLOC)
                    CASE_STR(EGL_BAD_ATTRIBUTE)
                    CASE_STR(EGL_BAD_CONTEXT)
                    CASE_STR(EGL_BAD_CONFIG)
                    CASE_STR(EGL_BAD_CURRENT_SURFACE)
                    CASE_STR(EGL_BAD_DISPLAY)
                    CASE_STR(EGL_BAD_SURFACE)
                    CASE_STR(EGL_BAD_MATCH)
                    CASE_STR(EGL_BAD_PARAMETER)
                    CASE_STR(EGL_BAD_NATIVE_PIXMAP)
                    CASE_STR(EGL_BAD_NATIVE_WINDOW)
                    CASE_STR(EGL_CONTEXT_LOST)
                default:
                    return "Unknown";
                }
            }
#undef CASE_STR

        public:
            static EGLint CheckEGLError()
            {
                EGLint error = eglGetError();

                if (error != EGL_SUCCESS) {
                    TRACE_GLOBAL(Trace::Error, (_T("EGL ERROR[%d]: %s\n"), error, EGLErrorString(error)));
                }

                return error;
            }

            class Sync final {
            public:
                Sync() = delete;
                Sync(const Sync&) = delete;
                Sync(Sync&&) = delete;

                Sync& operator=(const Sync&) = delete;
                Sync& operator=(Sync&&) = delete;

                void* operator new(size_t) = delete;
                void* operator new[](size_t) = delete;
                void operator delete(void*) = delete;
                void operator delete[](void*) = delete;

                explicit Sync(EGLDisplay&);
                ~Sync();

            private:
                using sync_t = KHRFIX(EGLSync);

                static constexpr KHRFIX(EGLSync) InvalidSync() { return static_cast<sync_t>(_EGL_NO_SYNC); }

                sync_t _sync;
                EGLDisplay& _dpy;
            };

            class RenderThread : public Core::Thread {
            protected:
                // Shared texture access
                Core::CriticalSection& _sharing;

                EGL& _egl;
                GLES& _gles;

                explicit RenderThread(EGL&, GLES&);

            public:
                RenderThread(const RenderThread&) = delete;
                RenderThread& operator=(const RenderThread&) = delete;

                virtual ~RenderThread();

                uint32_t Worker() override;

            private:
                Core::CriticalSection& LockInstance()
                {
                    static Core::CriticalSection lock;
                    return lock;
                }
            };

            class SceneRenderer final : public RenderThread {
            private:
                using SurfaceType = Client;

                CompositorImplementation& _compositor;

                // Local
                std::mutex _access;

            public:
                SceneRenderer() = delete;
                SceneRenderer(const SceneRenderer&) = delete;
                SceneRenderer& operator=(const SceneRenderer&) = delete;

                explicit SceneRenderer(EGL&, GLES&, CompositorImplementation&);
                ~SceneRenderer() override;

                uint32_t Worker() override;

            private:
                bool Render();
            };

            class TextureRenderer final : public RenderThread {
            private:
                using SurfaceType = Client;

                // Local
                std::mutex _access;

                CompositorImplementation::ClientContainer& _clients;

                struct element {
                public:
                    const std::string& _name;

                    element() = delete;

                    explicit element(const std::string& name)
                        : _name(name)
                    {
                    }
                    ~element() = default;
                };

                using element_t = struct element;

                using function_t = std::function<bool(const element_t&, const element_t&)>;

                // Unique elements, to prevent queue from growing beyond N
                std::set<element_t, function_t> set;
                // FIFO
                std::queue<element_t> queue;

            public:
                TextureRenderer() = delete;
                TextureRenderer(const TextureRenderer&) = delete;
                TextureRenderer& operator=(const TextureRenderer&) = delete;

                explicit TextureRenderer(EGL&, GLES&, CompositorImplementation::ClientContainer&);
                ~TextureRenderer();

                void SetClientName(const std::string&);

                uint32_t Worker() override;

            private:
                bool Render();
            };

        public:
            EGL() = delete;
            EGL(EGL const&) = delete;
            EGL& operator=(EGL const&) = delete;

            EGL(const Natives& natives)
                : _dpy(EGL_NO_DISPLAY)
                , _conf(EGL_NO_CONFIG)
                , _surf(EGL_NO_SURFACE)
                , _width(InvalidSize)
                , _height(InvalidSize)
                , _natives(natives)
            {
                Initialize();
            }

            ~EGL()
            {
                Deinitialize();
            }

            static constexpr EGLint RedBufferSize = 8;
            static constexpr EGLint GreenBufferSize = 8;
            static constexpr EGLint BlueBufferSize = 8;
            static constexpr EGLint AlphaBufferSize = 8;
            // For OpenGL ES 2.0 the only possible value is 16
            static constexpr EGLint DepthBufferSize() { return (GLES::MajorVersion() == 2) ? 16 : 0; }

            static constexpr EGLint InvalidSize = -1;

            static constexpr void* const EGL_NO_CONFIG = nullptr;

            EGLDisplay Display() const { return _dpy; }
            EGLSurface Surface() const { return _surf; }
            EGLContext Context() const { return _ctx; }

            EGLint Height() const { return _height; }
            EGLint Width() const { return _width; }

            static EGLImage CreateImage(EGL const&, const Client::SurfaceType&);
            static EGLImage DestroyImage(EGL const&, const Client::SurfaceType&);

            bool Invalidate();
            bool IsValid() const
            {
                return ((_dpy != EGL_NO_DISPLAY) && (_conf != EGL_NO_CONFIG) && (_surf != EGL_NO_SURFACE) && _natives.Valid());
            }

            static void DebugCallback(GLenum source, GLenum type, GLuint id,
                GLenum severity, GLsizei length, const GLchar* msg, const void* param)
            {

                string sourceStr;
                switch (source) {
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM_KHR:
                    sourceStr = "WindowSys";
                    break;
                case GL_DEBUG_SOURCE_APPLICATION_KHR:
                    sourceStr = "App";
                    break;
                case GL_DEBUG_SOURCE_API_KHR:
                    sourceStr = "OpenGL";
                    break;
                case GL_DEBUG_SOURCE_SHADER_COMPILER_KHR:
                    sourceStr = "ShaderCompiler";
                    break;
                case GL_DEBUG_SOURCE_THIRD_PARTY_KHR:
                    sourceStr = "3rdParty";
                    break;
                case GL_DEBUG_SOURCE_OTHER_KHR:
                    sourceStr = "Other";
                    break;
                default:
                    sourceStr = "Unknown";
                }

                string typeStr;
                switch (type) {
                case GL_DEBUG_TYPE_ERROR_KHR:
                    typeStr = "Error";
                    break;
                case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_KHR:
                    typeStr = "Deprecated";
                    break;
                case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_KHR:
                    typeStr = "Undefined";
                    break;
                case GL_DEBUG_TYPE_PORTABILITY_KHR:
                    typeStr = "Portability";
                    break;
                case GL_DEBUG_TYPE_PERFORMANCE_KHR:
                    typeStr = "Performance";
                    break;
                case GL_DEBUG_TYPE_MARKER_KHR:
                    typeStr = "Marker";
                    break;
                case GL_DEBUG_TYPE_PUSH_GROUP_KHR:
                    typeStr = "PushGrp";
                    break;
                case GL_DEBUG_TYPE_POP_GROUP_KHR:
                    typeStr = "PopGrp";
                    break;
                case GL_DEBUG_TYPE_OTHER_KHR:
                    typeStr = "Other";
                    break;
                default:
                    typeStr = "Unknown";
                }

                string sevStr;
                switch (severity) {
                case GL_DEBUG_SEVERITY_HIGH_KHR:
                    sevStr = "HIGH";
                    break;
                case GL_DEBUG_SEVERITY_MEDIUM_KHR:
                    sevStr = "MED";
                    break;
                case GL_DEBUG_SEVERITY_LOW_KHR:
                    sevStr = "LOW";
                    break;
                case GL_DEBUG_SEVERITY_NOTIFICATION_KHR:
                    sevStr = "NOTIFY";
                    break;
                default:
                    sevStr = "UNK";
                }

                TRACE_GLOBAL(CompositorTrace::Render, (_T ( "%s:%s[%s](%d): %s" ), sourceStr.c_str(), typeStr.c_str(), sevStr.c_str(), id, msg));
            }

        private:
            bool Initialize();
            void Deinitialize();

            template <typename FUNC, typename... ARG>
            bool Render(std::false_type const, FUNC&&, const bool, ARG&&...);

            template <typename FUNC, typename ARG0, typename... ARG>
            bool Render(std::true_type const, FUNC&&, const bool, ARG0&&, ARG&&...);

            // TODO: two different signatures for callables
            template <typename FUNC, typename... ARG>
            bool Render(std::false_type const, FUNC&&, FUNC&&, ARG&&...);

            template <typename FUNC, typename ARG0, typename... ARG>
            bool Render(std::true_type const, FUNC&&, FUNC&&, ARG0&&, ARG&&...);

            template <typename FUNC, typename... ARG>
            bool RenderWithoutSwap(std::false_type const, FUNC&&, ARG&&...);

            template <typename FUNC, typename ARG0, typename... ARG>
            bool RenderWithoutSwap(std::true_type const, FUNC&&, ARG0&&, ARG&&...);

        public:
            // Although compile / build time may succeed, runtime checks are also mandatory
            static bool Supported(EGLDisplay const, const std::string&);

            bool Render(GLES&);

            template <typename FUNC, typename... ARG>
            bool Render(FUNC&&, const bool, ARG&&...);

            template <typename FUNC, typename... ARG>
            bool Render(FUNC&&, FUNC&&, ARG&&...);

            template <typename FUNC, typename... ARG>
            bool RenderWithoutSwap(FUNC&&, ARG&&...);

        private:
            EGLDisplay _dpy;
            EGLConfig _conf;
            EGLSurface _surf;
            EGLint _width;
            EGLint _height;
            const Natives& _natives;

#undef STRINGIFY
#ifdef _KHRFIX
#undef _KHRFIX
#endif
#undef KHRFIX
#undef _EGL_SYNC_FENCE
#undef _EGL_NO_SYNC
#undef _EGL_FOREVER
#undef _EGL_NO_IMAGE
#undef _EGL_NATIVE_PIXMAP
#undef _EGL_CONDITION_SATISFIED
#undef _EGL_SYNC_STATUS
#undef _EGL_SIGNALED
#undef _EGL_SYNC_FLUSH_COMMANDS_BIT
        };

    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config();
            ~Config();

            Core::JSON::String _connector;
            Core::JSON::String _port;
        };

        void PlatformReady();

        int SurfaceDmaFd(const uint32_t id)
        {
            class Find : public ClientContainer::IFind {
            public:
                Find(const uint32_t id, const EGL& egl)
                    : _id(id)
                    , _result(-1)
                    , _egl(egl)
                {
                }

                bool Check(const string& /*key*/, const Core::ProxyType<Client>& client) const override
                {
                    bool isValid(false);

                    if (client->Native() == _id) {
                        Client::SurfaceType surf = client->Surface(EGL::CreateImage(_egl, client->Surface()));
                        _result = surf.IsValid() ? client->Native() : -1;
                        isValid = surf.IsValid();
                    }

                    return isValid;
                };

                int Result() const
                {
                    return _result;
                }

            private:
                const uint32_t _id;
                mutable int _result;
                const EGL& _egl;
            } callback(id, _egl);

            return (_clients.Find(callback) == true) ? callback.Result() : -1;
        }

        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Core::ProxyType<RPC::InvokeServer> _engine;
        ExternalAccess* _externalAccess;
        std::list<Exchange::IComposition::INotification*> _observers;
        ClientContainer _clients;
        string _port;
        ModeSet _modeSet;

        Natives _natives;
        EGL _egl;
        GLES _gles;

        DmaFdServer _dmaFdServer;

        Core::CriticalSection _clientLock;

        EGL::TextureRenderer _textureRenderer;
        EGL::SceneRenderer _sceneRenderer;

    public:
        CompositorImplementation();
        ~CompositorImplementation();

        bool CompositeFor(const std::string&);

        bool FrameFlip();

        //
        // Exchange::IComposition
        // ==================================================================================================================

        static uint32_t WidthFromResolution(ScreenResolution const);
        static uint32_t HeightFromResolution(ScreenResolution const);
        static ScreenResolution ResolutionFromHeightWidth(uint32_t const, uint32_t const);

        uint32_t Configure(PluginHost::IShell* service) override;

        void Register(Exchange::IComposition::INotification*) override;
        void Unregister(Exchange::IComposition::INotification*) override;

        void Attached(const string&, IClient*);
        void Detached(const string&);

        //
        // Exchange::IComposition::IDisplay
        // ==================================================================================================================
        Core::instance_id Native() const override;

        string Port() const override;

        IClient* CreateClient(const string&, uint32_t const, uint32_t const) override;
        void InvalidateClient(const string&) /*override*/;

        Exchange::IComposition::ScreenResolution Resolution() const override;
        uint32_t Resolution(Exchange::IComposition::ScreenResolution const) override;

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        INTERFACE_ENTRY(Exchange::IComposition::IDisplay)
        END_INTERFACE_MAP
    };

    CompositorImplementation::ExternalAccess::ExternalAccess(CompositorImplementation& parent, const Core::NodeId& source, const string& proxyStubPath, const Core::ProxyType<RPC::InvokeServer>& handler)
        : RPC::Communicator(source, proxyStubPath.empty() == false ? Core::Directory::Normalize(proxyStubPath) : proxyStubPath, Core::ProxyType<Core::IIPCServer>(handler))
        , _parent(parent)
    {

        uint32_t result = RPC::Communicator::Open(RPC::CommunicationTimeOut);

        handler->Announcements(Announcement());

        if (result != Core::ERROR_NONE) {
            TRACE(Trace::Error, (_T ( "Could not open RPI Compositor RPCLink server. Error: %s" ), Core::NumberType<uint32_t>(result).Text()));
        } else {
            // We need to pass the communication channel NodeId via an environment variable, for process,
            // not being started by the rpc process...
            Core::SystemInfo::SetEnvironment(_T ( "COMPOSITOR" ), RPC::Communicator::Connector(), true);
        }
    }

    void* CompositorImplementation::ExternalAccess::Acquire(const string& /*className*/, const uint32_t interfaceId, uint32_t const /*version*/)
    {
        // Use the className to check for multiple HDMI's.
        return (_parent.QueryInterface(interfaceId));
    }

#define GL_ERROR_WITH_RETURN() (glGetError() == GL_NO_ERROR)
#ifdef NDEBUG
#define GL_ERROR() \
    do {           \
    } while (0)
#else
#define GL_ERROR() ASSERT(GL_ERROR_WITH_RETURN())
#endif

    CompositorImplementation::GLES::offset::offset()
        : offset((_right - _left) / static_cast<GLfloat>(2.0f) + _left, (_top - _bottom) / static_cast<GLfloat>(2.0f) + _bottom, (_far - _near) / static_cast<GLfloat>(2.0f) + _near)
    {
    }

    CompositorImplementation::GLES::offset::offset(const GLfloat& x, const GLfloat& y, const GLfloat& z)
        : _x(x)
        , _y(y)
        , _z(z)
    {
    }

    // ODR use

    /* static */ constexpr const GLfloat CompositorImplementation::GLES::offset::_left;
    /* static */ constexpr const GLfloat CompositorImplementation::GLES::offset::_right;
    /* static */ constexpr const GLfloat CompositorImplementation::GLES::offset::_bottom;
    /* static */ constexpr const GLfloat CompositorImplementation::GLES::offset::_top;
    /* static */ constexpr const GLfloat CompositorImplementation::GLES::offset::_near;
    /* static */ constexpr const GLfloat CompositorImplementation::GLES::offset::_far;

    /* static */ constexpr const GLclampf CompositorImplementation::GLES::scale::_identity;
    /* static */ constexpr const GLclampf CompositorImplementation::GLES::scale::_min;
    /* static */ constexpr const GLclampf CompositorImplementation::GLES::scale::_max;

    CompositorImplementation::GLES::scale::scale()
        : scale(CompositorImplementation::GLES::scale::_identity, CompositorImplementation::GLES::scale::_identity)
    {
    }

    CompositorImplementation::GLES::scale::scale(const GLclampf& horiz, const GLclampf& vert)
        : _horiz(horiz)
        , _vert(vert)
    {
    }

    /* static */ constexpr const GLfloat CompositorImplementation::GLES::opacity::_min;
    /* static */ constexpr const GLfloat CompositorImplementation::GLES::opacity::_max;

    CompositorImplementation::GLES::opacity::opacity()
        : opacity(_max)
    {
    }

    CompositorImplementation::GLES::opacity::opacity(const GLfloat& alpha)
        : _alpha(alpha)
    {
    }

    CompositorImplementation::GLES::opacity::opacity(const CompositorImplementation::GLES::opacity& other)
        : _alpha(other._alpha)
    {
    }

    CompositorImplementation::GLES::texture::texture()
        : texture(static_cast<GLuint>(GL_INVALID_ENUM), CompositorImplementation::GLES::opacity::InitialOpacity())
    {
    }

    CompositorImplementation::GLES::texture::texture(const GLuint target, const CompositorImplementation::GLES::opacity& opacity)
        : _tex(0)
        , _target(target)
        , _opacity(opacity)
        , _x(0)
        , _y(0)
        , _z(0)
        , _width(0)
        , _height(0)
    {
    }

    CompositorImplementation::GLES::texture::texture(const CompositorImplementation::GLES::texture& other)
        : _tex(other._tex)
        , _target(other._target)
        , _opacity(other._opacity)
        , _x(other._x)
        , _y(other._y)
        , _z(other._z)
        , _width(other._width)
        , _height(other._height)
    {
    }

    CompositorImplementation::GLES::GLES()
        : _offset(InitialOffset())
        , _scale(InitialScale())
        , _opacity(InitialOpacity())
        , _valid(Initialize())
    {
    }

    CompositorImplementation::GLES::~GLES()
    {
        _valid = false;
        Deinitialize();
    }

    bool CompositorImplementation::GLES::UpdateOffset(const CompositorImplementation::GLES::offset& off)
    {
        bool ret = false;

        // Range check without taking into account rounding errors
        if (off._x >= offset::_left
            && off._x <= offset::_right
            && off._y >= offset::_bottom
            && off._y <= offset::_top
            && off._z >= offset::_near
            && off._z <= offset::_far) {
            _offset = off;

            TRACE(CompositorTrace::GL, (_T("Updated Offset: x=%f, y=%f z=%f"), _offset._x, _offset._y, _offset._z));
            ret = true;
        }

        return ret;
    }

    bool CompositorImplementation::GLES::UpdateScale(const CompositorImplementation::GLES::scale& scale)
    {
        bool ret = false;

        // Range check without taking into account rounding errors
        if (scale._horiz >= scale::_min
            && scale._horiz <= scale::_max
            && scale._vert >= scale::_min
            && scale._vert <= scale::_max) {
            _scale = scale;

            TRACE(CompositorTrace::GL, (_T("Updated Scale: h=%f v=%f."), _scale._horiz, _scale._vert));
            ret = true;
        }

        return ret;
    }

    bool CompositorImplementation::GLES::UpdateOpacity(const CompositorImplementation::GLES::opacity& opacity)
    {
        bool ret = false;

        if (opacity._alpha >= GLES::opacity::_min && _opacity._alpha <= GLES::opacity::_max) {
            _opacity = opacity;

            TRACE(CompositorTrace::GL, (_T("Updated Opacity: a=%f."), _opacity._alpha));
            ret = true;
        }

        return ret;
    }

    bool CompositorImplementation::GLES::RenderColor(const bool red, const bool green, const bool blue, const bool alpha)
    {
        static uint16_t degree = 0;

        constexpr decltype(degree) const ROTATION = 360;

        constexpr const float OMEGA = 3.14159265 / 180;

        bool ret = Valid();

        if (ret != false) {
            GLfloat rad = static_cast<GLfloat>(cos(static_cast<float>(degree) * OMEGA));

            constexpr GLfloat default_color = 0.0f;

            // The function clamps the input to [0.0f, 1.0f]
            /* void */ glClearColor(red != false ? rad : default_color, green != false ? rad : default_color, blue != false ? rad : default_color, alpha != false ? default_color : 1.0f);
            GL_ERROR();

            /* void */ glClear(GL_COLOR_BUFFER_BIT);
            GL_ERROR();

            degree = (degree + 1) % ROTATION;
        }

        ret = ret && GL_ERROR_WITH_RETURN();

        return ret;
    }

    bool CompositorImplementation::GLES::RenderEGLImage(const EGLImage& img,
        const EGLint x, const EGLint y,
        const EGLint width, const EGLint height,
        const EGLint zorder, const EGLint opacity)
    {
        EGLDisplay dpy = EGL_NO_DISPLAY;
        EGLDisplay ctx = EGL_NO_CONTEXT;

#ifdef _RESIZE_TEXTURE
        auto DestroyTexture = [this](texture& tex) -> bool {
            bool ret = tex._tex != InvalidTex();

            // Delete the previously created texture
            glDeleteTextures(1, &(tex._tex));

            tex._tex = InvalidTex();

            ret = ret
                && GL_ERROR_WITH_RETURN();

            return ret;
        };
#endif
        auto SetupTexture = [this, &dpy, &ctx](texture& tex, const EGLImage& img, const EGLint width, const EGLint height, const bool quick) -> bool {
            bool ret = GL_ERROR_WITH_RETURN();

            if (quick != true) {
                glGenTextures(1, &(tex._tex));
                GL_ERROR();
            }

            glBindTexture(tex._target, (tex._tex));
            GL_ERROR();

            glTexParameteri(tex._target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GL_ERROR();

            glTexParameteri(tex._target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GL_ERROR();

            glTexParameteri(tex._target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GL_ERROR();

            glTexParameteri(tex._target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GL_ERROR();

            tex._width = width;
            tex._height = height;

            switch (tex._target) {
            case GL_TEXTURE_EXTERNAL_OES: {
                // A valid GL context should exist for GLES::Supported ()

                ret = dpy != EGL_NO_DISPLAY
                    && ctx != EGL_NO_CONTEXT;

                static bool supported = (GLES::Supported("GL_OES_EGL_image")
                                            && (EGL::Supported(dpy, "EGL_KHR_image")
                                                || EGL::Supported(dpy, "EGL_KHR_image_base")))
                    != false;

                if ((ret && supported) != false) {
                    // Take storage for the texture from the EGLImage; Pixel data becomes undefined
                    static void (*pEGLImageTargetTexture2DOES)(GLenum, GLeglImageOES) = reinterpret_cast<void (*)(GLenum, GLeglImageOES)>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

                    // Assume nullptr represents invalid / not found
                    ret = pEGLImageTargetTexture2DOES != nullptr;

                    if (ret != false) {
                        pEGLImageTargetTexture2DOES(tex._target, reinterpret_cast<GLeglImageOES>(img));
                        GL_ERROR();
                    }
                }
            }; break;

            case GL_TEXTURE_2D: {
                if (quick != true) {
                    glTexImage2D(tex._target, 0, GL_RGBA, tex._width, tex._height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                    GL_ERROR();
                } else {
                    // Clear the content
                    glTexSubImage2D(tex._target, 0, 0, 0, tex._width, tex._height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                }
            }; break;

            default: {
                ret = false;
            }
            }

            glBindTexture(tex._target, InvalidTex());

            ret = ret && GL_ERROR_WITH_RETURN();

            return ret;
        };

        bool ret = GL_ERROR_WITH_RETURN()
            && img != EGL_NO_IMAGE
            && width > 0
            && height > 0;

        // A valid GL context should exist for GLES::Supported ()
        /* EGL::EGLContext */ ctx = eglGetCurrentContext();
        /* EGL::EGLDisplay */ dpy = ctx != EGL_NO_CONTEXT ? eglGetCurrentDisplay() : EGL_NO_DISPLAY;

        // OES dimensions should match the underlying buffer dimensions
        // Estimate using the EGL surface

        EGLSurface surf = EGL_NO_SURFACE;

        if (ret != false) {
            surf = eglGetCurrentSurface(EGL_DRAW);

            ret = (EGL::CheckEGLError() == EGL_SUCCESS) && (surf != EGL_NO_SURFACE);
        }

        TRACE(CompositorTrace::Render, (_T("Rendering EGLImage\n"
                                           "- Display=%p\n"
                                           "- Context=%p\n"
                                           "- Image=%p:\n"
                                           "- Surface=%p:\n"
                                           "- Position: x=%d y=%d z=%d \n"
                                           "- Size: width=%d height=%d\n"
                                           "- Opacity=%d"),
                                           dpy, ctx, img, surf, x, y, zorder, width, height, opacity));

        EGLint e_width = 0, e_height = 0;

        if (ret != false) {
            ret = (eglQuerySurface(dpy, surf, EGL_WIDTH, &e_width) != EGL_FALSE)
                && (eglQuerySurface(dpy, surf, EGL_HEIGHT, &e_height) != EGL_FALSE)
                && (EGL::CheckEGLError() == EGL_SUCCESS);
        }

        ret = ret && (EGL::CheckEGLError() == EGL_SUCCESS) && (ctx != EGL_NO_CONTEXT);

        // Set up the required textures

        // The  'shared' texture
        static texture tex_oes(GL_TEXTURE_EXTERNAL_OES, CompositorImplementation::GLES::opacity::InitialOpacity());

        static bool reuse = false;

        // The 'scene' texture
        texture tex_fbo(GL_TEXTURE_2D, CompositorImplementation::GLES::opacity::InitialOpacity());

        if (ret != false) {
            // Just an arbitrarily selected texture unit
            glActiveTexture(GL_TEXTURE0);
            GL_ERROR();

            // Using the EGL surface dimensions

#ifdef _RESIZE_TEXTURE
            // Did dimensions change ?
            using common_w_t = std::common_type<decltype(tex_oes._width), decltype(e_width)>::type;
            using common_h_t = std::common_type<decltype(tex_oes._height), decltype(e_height)>::type;

            if (static_cast<common_w_t>(tex_oes._width) != static_cast<common_w_t>(e_width) || static_cast<common_h_t>(tex_oes._height) != static_cast<common_h_t>(e_height)) {

                reuse = false;

                if (tex_oes._width > 0 && tex_oes._height > 0) {
                    reuse = DestroyTexture(tex_oes) != true;
                }
            }
#endif

            ret = SetupTexture(tex_oes, img, e_width, e_height, reuse) != false;
            reuse = ret;

            {
                const std::lock_guard<decltype(_token)> lock(_token);

                auto it = _scene.find(img);

                ret = it != _scene.end();

                if (ret != false) {
                    // Found, just update values
                    tex_fbo = it->second;

                    // Did dimensions change ?

                    using common_w_t = std::common_type<decltype(width), decltype(it->second._width)>::type;
                    using common_h_t = std::common_type<decltype(height), decltype(it->second._height)>::type;

                    if (static_cast<common_w_t>(width) != static_cast<common_w_t>(it->second._width)
                        || static_cast<common_h_t>(height) != static_cast<common_h_t>(it->second._height)) {
                        TRACE(Trace::Error, (_T("Unsupported texture dimensions change detected!")));
                    }

                } else {
                    if (_preallocated.size() > 0) {
                        tex_fbo = _preallocated.top();

                        _preallocated.pop();

                        // Did dimensions change ?

                        using common_w_t = std::common_type<decltype(width), decltype(tex_fbo._width)>::type;
                        using common_h_t = std::common_type<decltype(height), decltype(tex_fbo._height)>::type;

                        if (static_cast<common_w_t>(width) != static_cast<common_w_t>(tex_fbo._width)
                            || static_cast<common_h_t>(height) != static_cast<common_h_t>(tex_fbo._height)) {

                            TRACE(Trace::Error, (_T ( "Unsupported texture dimensions change detected!" )));

                            ASSERT(false);
                        }

                        ret = SetupTexture(tex_fbo, img, width, height, true) != false;
                    } else {
                        ret = SetupTexture(tex_fbo, img, width, height, false) != false;
                    }

                    if (ret != false) {
                        auto it = _scene.insert(std::pair<EGLImage, texture>(img, tex_fbo));
                        ret = it.second != false;
                    }
                }

                // Update
                if (ret != false) {
                    using common_opacity_t = std::common_type<decltype(opacity), decltype(GLES::opacity::_alpha), decltype(Exchange::IComposition::minOpacity), decltype(Exchange::IComposition::maxOpacity)>::type;

                    tex_fbo._opacity = (static_cast<GLfloat>(static_cast<common_opacity_t>(opacity) / (static_cast<common_opacity_t>(Exchange::IComposition::maxOpacity) - static_cast<common_opacity_t>(Exchange::IComposition::minOpacity))));
                    tex_fbo._x = x;
                    tex_fbo._y = y;
                    tex_fbo._z = zorder;

                    _scene[img] = tex_fbo;
                }
            }

            GLuint fbo;

            glGenFramebuffers(1, &fbo);
            GL_ERROR();

            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            GL_ERROR();

            glBindTexture(tex_oes._target, tex_oes._tex);
            GL_ERROR();

            glBindTexture(tex_fbo._target, tex_fbo._tex);
            GL_ERROR();

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_fbo._tex, 0);
            GL_ERROR();

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            ret = ret && GL_ERROR_WITH_RETURN() && status == GL_FRAMEBUFFER_COMPLETE;

            glDisable(GL_DEPTH_TEST);
            GL_ERROR();

            glDisable(GL_BLEND);
            GL_ERROR();

            TRACE(CompositorTrace::GL, (_T("%s:%d %s RenderEGLImage BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

            ret = (ret
                      && UpdateScale(GLES::InitialScale())
                      && UpdateOffset(GLES::InitialOffset())
                      && UpdateOpacity(tex_fbo._opacity)
                      && SetupViewport(0, 0, width, height)
                      && RenderTileOES())
                != false;

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, InvalidTex(), 0);
            GL_ERROR();

            glBindTexture(tex_oes._target, InvalidTex());
            GL_ERROR();

            glBindTexture(tex_fbo._target, InvalidTex());
            GL_ERROR();

            glDeleteFramebuffers(1, &fbo);
            GL_ERROR();
        }

        ret = ret
            && GL_ERROR_WITH_RETURN();

        // Do not destroy tex-fbo and _tex_oes !

        return ret;
    }

    bool CompositorImplementation::GLES::RenderScene(const GLuint width, const GLuint height, std::function<bool(const CompositorImplementation::GLES::texture& left, const CompositorImplementation::GLES::texture& right)> sortfunc)
    {
        bool ret = GL_ERROR_WITH_RETURN();

        if (ret != false) {

            // TODO: very inefficient way to get z-order sorted textures
            std::list<texture> sorted;

            {
                const std::lock_guard<decltype(_token)> lock(_token);

                for (auto begin = _scene.begin(), it = begin, end = _scene.end(); it != end; it++) {
                    sorted.push_back(it->second);
                }
            }

            sorted.sort(sortfunc);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            GL_ERROR();

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

            ret = ret
                && GL_ERROR_WITH_RETURN()
                && status == GL_FRAMEBUFFER_COMPLETE;

            // Blend pixels with pixels already present in the frame buffer
            glEnable(GL_BLEND);
            GL_ERROR();

            glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
            GL_ERROR();

            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            GL_ERROR();

            GLint bits = 0;

            glGetIntegerv(GL_DEPTH_BITS, &bits);

            ret = ret
                && GL_ERROR_WITH_RETURN()
                && bits > static_cast<GLint>(0);

            glEnable(GL_DEPTH_TEST);
            GL_ERROR();

            glDepthMask(GL_TRUE);
            GL_ERROR();

            glDepthFunc(GL_LESS);
            GL_ERROR();

            // Fully utilize the depth buffer range
            glDepthRangef(GLES::offset::_near, GLES::offset::_far);
            GL_ERROR();

            // TODO: magic number
            glClearDepthf(1.0f);
            GL_ERROR();

            glClear(GL_DEPTH_BUFFER_BIT);
            GL_ERROR();

            // Start with an empty (solid) background
            ret = (ret && RenderColor(false, false, false, false)) != false;

            // For all textures in map
            if (ret != false) {
                offset off(_offset);
                scale scl(_scale);
                GLES::opacity op(_opacity);

                for (auto begin = sorted.begin(), it = begin, end = sorted.end(); it != end; it++) {
                    texture& tex = *it;

                    ASSERT(tex._target == static_cast<GLuint>(GL_TEXTURE_2D));

                    glBindTexture(tex._target, tex._tex);
                    GL_ERROR();

                    GLES::scale g_scale(
                        static_cast<GLclampf>(static_cast<GLfloat>(tex._width) / static_cast<GLfloat>(width)),
                        static_cast<GLclampf>(static_cast<GLfloat>(tex._height) / static_cast<GLfloat>(height)));

                    GLES::offset g_offset(
                        static_cast<GLfloat>(static_cast<GLfloat>(g_scale._horiz) * static_cast<GLfloat>(tex._x) / static_cast<GLfloat>(tex._width)),
                        static_cast<GLfloat>(static_cast<GLfloat>(g_scale._vert) * static_cast<GLfloat>(tex._y) / static_cast<GLfloat>(tex._height)),
                        static_cast<GLfloat>(static_cast<GLuint>(tex._z) / (static_cast<GLuint>(Exchange::IComposition::maxZOrder - Exchange::IComposition::minZOrder))));

                    TRACE(CompositorTrace::GL, (_T("%s:%d %s RenderScene BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

                    // Width and height are screen dimensions, eg the geometry values are in this space
                    ret = (ret
                              && UpdateScale(g_scale)
                              && UpdateOffset(g_offset)
                              && UpdateOpacity(tex._opacity)
                              && SetupViewport(0, 0, width, height)
                              && RenderTile())
                        != false;

                    if (ret != true) {
                        break;
                    }
                }

                ret = (ret && UpdateScale(scl) && UpdateOffset(off) && UpdateOpacity(op)) != false;
            }

            glBindTexture(GL_TEXTURE_2D, InvalidTex());
            GL_ERROR();

            // Unconditionally
            glDisable(GL_DEPTH_TEST);
            GL_ERROR();

            glDisable(GL_BLEND);
            GL_ERROR();

            ret = ret
                && GL_ERROR_WITH_RETURN();
        }

        return ret;
    }

    bool CompositorImplementation::GLES::SkipEGLImageFromScene(const EGLImage& img)
    {
        bool ret = false;

        const std::lock_guard<decltype(_token)> lock(_token);

        auto it = _scene.find(img);

        ret = it != _scene.end();

        if (ret != false) {
            // TODO: Apparently it is hard to release (GPU) memory allocated so reuse it
            // Move texture to the stack for reuse
            _preallocated.push(it->second);

            using scene_t = decltype(_scene);

            scene_t::size_type size = _scene.size();

            _scene.erase(it);

            ret = (size - _scene.size()) == static_cast<scene_t::size_type>(1);
        }

        return ret;
    }

    bool CompositorImplementation::GLES::Initialize()
    {
        bool ret = GL_ERROR_WITH_RETURN();
        return ret;
    }

    bool CompositorImplementation::GLES::Deinitialize()
    {
        bool ret = GL_ERROR_WITH_RETURN();

        glBindTexture(GL_TEXTURE_2D, InvalidTex());
        GL_ERROR();

        glBindTexture(GL_TEXTURE_EXTERNAL_OES, InvalidTex());
        GL_ERROR();

        const std::lock_guard<decltype(_token)> lock(_token);

        for (auto begin = _scene.begin(), it = begin, end = _scene.end(); it != end; it++) {
            GLuint& tex = it->second._tex;
            glDeleteTextures(1, &tex);
            GL_ERROR();
        }

        ret = ret && GL_ERROR_WITH_RETURN();

        return ret;
    }

    // TODO: precompile programs at initialization stage

    bool CompositorImplementation::GLES::SetupProgram(const char vtx_src[], const char frag_src[], GLuint& prog)
    {
        auto LoadShader = [](const GLuint type, const GLchar code[]) -> GLuint {
            GLuint shader = glCreateShader(type);
            GL_ERROR();

            bool ret = (GL_ERROR_WITH_RETURN() && shader != 0) != false;

            if (ret != false) {
                glShaderSource(shader, 1, &code, nullptr);
                GL_ERROR();

                glCompileShader(shader);
                GL_ERROR();

                GLint status = GL_FALSE;

                glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
                GL_ERROR();

                ret = (GL_ERROR_WITH_RETURN()
                          && status != GL_FALSE)
                    != false;

#ifndef NDEBUG
                if (ret != true) {
                    GLint size = 0;

                    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
                    GL_ERROR();

                    if (GL_ERROR_WITH_RETURN() != false) {
                        GLchar info[size];
                        GLsizei length = 0;

                        glGetShaderInfoLog(shader, static_cast<GLsizei>(size), &length, &info[0]);
                        GL_ERROR();

                        info[size] = '\0';

                        TRACE_GLOBAL(Trace::Error, (_T ( "Error: shader log: %s" ), static_cast<char*>(&info[0])));
                    }
                }
#endif

                ret = ret && GL_ERROR_WITH_RETURN();
            }

            return shader;
        };

        auto ShadersToProgram = [](const GLuint vertex, const GLuint fragment, GLuint& prog) -> bool {
            prog = glCreateProgram();
            GL_ERROR();

            bool ret = (GL_ERROR_WITH_RETURN() && prog != InvalidProg()) != false;

            if (ret != false) {
                glAttachShader(prog, vertex);
                GL_ERROR();

                glAttachShader(prog, fragment);
                GL_ERROR();

                glBindAttribLocation(prog, 0, "position");
                GL_ERROR();

                glLinkProgram(prog);
                GL_ERROR();

                GLint status = GL_FALSE;

                glGetProgramiv(prog, GL_LINK_STATUS, &status);
                GL_ERROR();

                ret = (GL_ERROR_WITH_RETURN() && status != GL_FALSE) != false;

#ifndef NDEBUG
                if (ret != true) {
                    GLint size = 0;

                    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &size);
                    GL_ERROR();

                    if (GL_ERROR_WITH_RETURN() != false) {
                        GLchar info[size];
                        GLsizei length = 0;

                        glGetProgramInfoLog(prog, static_cast<GLsizei>(size), &length, &info[0]);
                        GL_ERROR();

                        info[size] = '\0';

                        TRACE_GLOBAL(Trace::Error, (_T("Error: program log: %s"), static_cast<char*>(&info[0])));
                    }
                }
#endif

                ret = ret && GL_ERROR_WITH_RETURN();
            }

            return ret;
        };

        auto DeleteCurrentProgram = []() -> bool {
            GLuint prog = InvalidProg();

            glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&prog));
            GL_ERROR();

            bool ret = (GL_ERROR_WITH_RETURN()
                           && prog != InvalidProg())
                != false;

            if (ret != false) {
                GLint count = 0;

                glGetProgramiv(prog, GL_ATTACHED_SHADERS, &count);
                GL_ERROR();

                ret = (GL_ERROR_WITH_RETURN() && count > 0) != false;

                if (ret != false) {
                    GLuint shaders[count];

                    glGetAttachedShaders(prog, count, static_cast<GLsizei*>(&count), &shaders[0]);

                    if (GL_ERROR_WITH_RETURN() != false) {
                        for (count--; count >= 0; count--) {
                            glDetachShader(prog, shaders[count]);
                            GL_ERROR();

                            glDeleteShader(shaders[count]);
                            GL_ERROR();
                        }

                        glDeleteProgram(prog);
                        GL_ERROR();
                    }
                }

                ret = ret && GL_ERROR_WITH_RETURN();
            }

            return ret;
        };

        bool ret = GL_ERROR_WITH_RETURN();

        if (ret != false
            && prog == InvalidProg()) {

            GLuint vtxShader = LoadShader(GL_VERTEX_SHADER, vtx_src);
            GLuint fragShader = LoadShader(GL_FRAGMENT_SHADER, frag_src);

            ret = ShadersToProgram(vtxShader, fragShader, prog);
        }

        if (ret != false) {
            glUseProgram(prog);
            GL_ERROR();

            ret = GL_ERROR_WITH_RETURN();

            if (ret != true) {
                /* bool */ DeleteCurrentProgram();
                GL_ERROR();

                prog = InvalidProg();

                // Color on error
                glClearColor(1.0f, 0.0f, 0.0f, 0.5f);
                GL_ERROR();
            }
        }

        ret = ret && GL_ERROR_WITH_RETURN();

        return ret;
    }

    bool CompositorImplementation::GLES::RenderTileOES()
    {
        bool ret = GL_ERROR_WITH_RETURN();

        TRACE(CompositorTrace::GL, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        constexpr const char vtx_src[] = "#version 100                                  \n"
                                         "attribute vec3 position;                      \n"
                                         "varying vec2 coordinates;                     \n"
                                         "void main () {                                \n"
                                         "gl_Position = vec4 ( position . xyz , 1); \n"
                                         "coordinates = position . xy;              \n"
                                         "}                                             \n";

        constexpr const char frag_src[] = "#version 100                                                                     \n"
                                          "#extension GL_OES_EGL_image_external : require                                   \n"
                                          "precision mediump float;                                                         \n"
                                          "uniform samplerExternalOES sampler;                                              \n"
                                          "uniform float opacity;                                                           \n"
                                          "varying vec2 coordinates;                                                        \n"
                                          "void main () {                                                                   \n"
                                          "gl_FragColor = vec4 ( texture2D ( sampler , coordinates ) . rgb , opacity ); \n"
                                          "}                                                                                \n";

        const std::array<const GLfloat, 4 * VerticeDimensions> vert = {
            0.0f, 0.0f, 0.0f /* v0 */,
            1.0f, 0.0f, 0.0f /* v1 */,
            0.0f, 1.0f, 0.0f /* v2 */,
            1.0f, 1.0f, 0.0f /* v3 */
        };

        static void (*pglDebugMessageCallback)(GLDEBUGPROCKHR, const void*) = reinterpret_cast<void (*)(GLDEBUGPROCKHR, const void*)>(eglGetProcAddress("glDebugMessageCallbackKHR"));
        static void (*pglDebugMessageControl)(GLenum, GLenum, GLenum, GLsizei, const GLuint*, GLboolean) = reinterpret_cast<void (*)(GLenum, GLenum, GLenum, GLsizei, const GLuint*, GLboolean)>(eglGetProcAddress("glDebugMessageControlKHR"));
        static void (*pglDebugMessageInsert)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*) = reinterpret_cast<void (*)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*)>(eglGetProcAddress("glDebugMessageInsertKHR"));

        if (ret != false) {
            ASSERT((pglDebugMessageCallback != nullptr) && (pglDebugMessageControl != nullptr) && (pglDebugMessageInsert != nullptr));

            pglDebugMessageCallback(EGL::DebugCallback, nullptr);
            pglDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            pglDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION_KHR, GL_DEBUG_TYPE_MARKER_KHR, 0, GL_DEBUG_SEVERITY_NOTIFICATION_KHR, -1, "Start debugging");

            glDisable(GL_BLEND);
            GL_ERROR();

            static GLuint prog = InvalidProg();

            static bool supported = Supported("GL_OES_EGL_image_external") != false;

            ret = (ret
                      && RenderColor(false, false, false)
                      && supported
                      && SetupProgram(vtx_src, frag_src, prog)
                      && RenderPolygon(vert))
                != false;
        }

        return ret;
    }

    bool CompositorImplementation::GLES::RenderTile()
    {
        bool ret = GL_ERROR_WITH_RETURN();

        TRACE(CompositorTrace::GL, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        constexpr const char vtx_src[] = "#version 100                                   \n"
                                         "attribute vec3 position;                       \n"
                                         "varying vec2 coordinates;                      \n"
                                         "void main () {                                 \n"
                                         "gl_Position = vec4 ( position . xyz , 1 ); \n"
                                         "coordinates = position . xy;               \n"
                                         "}                                              \n";

        constexpr const char frag_src[] = "#version 100                                                            \n"
                                          "precision mediump float;                                                \n"
                                          "uniform sampler2D sampler;                                              \n"
                                          // Required by RenderPolygon
                                          "uniform float opacity;                                                  \n"
                                          "varying vec2 coordinates;                                               \n"
                                          "void main () {                                                          \n"
                                          "gl_FragColor = vec4 ( texture2D ( sampler , coordinates ) . rgba ); \n"
                                          "}                                                                       \n";

        const std::array<const GLfloat, 4 * VerticeDimensions> vert = {
            0.0f, 0.0f, _offset._z /* v0 */,
            1.0f, 0.0f, _offset._z /* v1 */,
            0.0f, 1.0f, _offset._z /* v2 */,
            1.0f, 1.0f, _offset._z /* v3 */
        };

        if (ret != false) {
// TODO: version match
#ifdef _0
            // TODO: type check GL_SHADING_LANGUAGE_VERSION
            using string_t = std::string::value_type;
            const string_t* ext = reinterpret_cast<const string_t*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

            bool ret = GL_ERROR_WITH_RETURN();
#endif

            static GLuint prog = InvalidProg();

            ret = (ret && SetupProgram(vtx_src, frag_src, prog) && RenderPolygon(vert)) != false;
        }

        return ret;
    }

    template <size_t N>
    bool CompositorImplementation::GLES::RenderPolygon(const std::array<const GLfloat, N>& vert)
    {
        GLuint prog = InvalidProg();

        TRACE(CompositorTrace::GL, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&prog));
        GL_ERROR();

        bool ret = (GL_ERROR_WITH_RETURN() && prog != InvalidProg()) != false;

        if (ret != false) {
            GLint loc_vert = 0, loc_op = 0;

            loc_op = glGetUniformLocation(prog, "opacity");
            GL_ERROR();

            glUniform1f(loc_op, _opacity._alpha);
            GL_ERROR();

            loc_vert = glGetAttribLocation(prog, "position");
            GL_ERROR();

            glVertexAttribPointer(loc_vert, VerticeDimensions, GL_FLOAT, GL_FALSE, 0, vert.data());
            GL_ERROR();

            glEnableVertexAttribArray(loc_vert);
            GL_ERROR();

            glDrawArrays(GL_TRIANGLE_STRIP, 0, vert.size() / VerticeDimensions);
            GL_ERROR();

            glDisableVertexAttribArray(loc_vert);
            GL_ERROR();

            ret = ret && GL_ERROR_WITH_RETURN();
        }

        return ret;
    }

    bool CompositorImplementation::GLES::Supported(const std::string& name)
    {
        using string_t = std::string::value_type;

        const string_t* ext = reinterpret_cast<const string_t*>(glGetString(GL_EXTENSIONS));
        GL_ERROR();

        return ((GL_ERROR_WITH_RETURN() != false) && (ext != nullptr) && (name.size() > 0) && (std::string(ext).find(name) != std::string::npos)) != false;
    }

    bool CompositorImplementation::GLES::SetupViewport(EGLint const /*x*/, EGLint const /*y*/, const EGLint width, const EGLint height)
    {
        bool ret = GL_ERROR_WITH_RETURN();

        GLint dims[2] = { 0, 0 };

        glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &dims[0]);
        GL_ERROR();

        GLint x = 0;
        GLint y = 0;
        GLsizei w = 0;
        GLsizei h = 0;

// #define _QUIRKS
#ifdef _QUIRKS
        // glViewport (x, y, width, height)
        //
        // Applied width = width / 2
        // Applied height = height / 2
        // Applied origin's x = width / 2 + x
        // Applied origin's y = height / 2 + y
        //
        // Compensate to origin bottom left and true size by
        // glViewport (-width, -height, width * 2, height * 2)
        //
        // _offset is in the range -1..1 wrt to origin, so the effective value maps to -width to width, -height to height

        constexpr const uint8_t mult = 2;

        using common_t = std::common_type<
            decltype(width), decltype(height), decltype(mult),
            decltype(_scale._horiz), decltype(_scale._vert),
            decltype(_offset._x), decltype(_offset._y), GLint>::type;

        common_t quirk_width = static_cast<common_t>(width) * static_cast<common_t>(mult) * static_cast<common_t>(_scale._horiz);
        common_t quirk_height = static_cast<common_t>(height) * static_cast<common_t>(mult) * static_cast<common_t>(_scale._vert);

        common_t quirk_x = (static_cast<common_t>(-width) * static_cast<common_t>(_scale._horiz)) + (static_cast<common_t>(_offset._x) * static_cast<common_t>(width));
        common_t quirk_y = (static_cast<common_t>(-height) * static_cast<common_t>(_scale._vert)) + (static_cast<common_t>(_offset._y) * static_cast<common_t>(height));

        if (quirk_x < (-quirk_width / static_cast<common_t>(mult))
            || quirk_y < (-quirk_height / static_cast<common_t>(mult))
            || quirk_x > static_cast<common_t>(0)
            || quirk_y > static_cast<common_t>(0)
            || quirk_width > (static_cast<common_t>(width) * static_cast<common_t>(mult))
            || quirk_height > (static_cast<common_t>(height) * static_cast<common_t>(mult))
            || static_cast<common_t>(width) > static_cast<common_t>(dims[0])
            || static_cast<common_t>(height) > static_cast<common_t>(dims[1])) {
            // Clipping, or undefined / unknown behavior
            std::cout << "Warning: possible clipping or unknown behavior detected. [" << quirk_x << ", " << quirk_y << ", " << quirk_width << ", " << quirk_height << ", " << width << ", " << height << ", " << dims[0] << ", " << dims[1] << "]" << std::endl;
        }

        //      If common type typically is unsigned then checking min values is only allowed on unsigned
        //        ASSERT ( static_cast < common_t > ( quirk_x ) >= static_cast < common_t > ( std::numeric_limits < GLint > :: min () ) );
        ASSERT(static_cast<common_t>(quirk_x) <= static_cast<common_t>(std::numeric_limits<GLint>::max()));
        //        ASSERT ( static_cast < common_t > ( quirk_y ) >= static_cast < common_t > ( std::numeric_limits < GLint > :: min () ) );
        ASSERT(static_cast<common_t>(quirk_y) <= static_cast<common_t>(std::numeric_limits<GLint>::max()));
        //        ASSERT ( static_cast < common_t > ( quirk_width ) >= static_cast < common_t > ( std::numeric_limits < GLint > :: min () ) );
        ASSERT(static_cast<common_t>(quirk_width) <= static_cast<common_t>(std::numeric_limits<GLint>::max()));
        //        ASSERT ( static_cast < common_t > ( quirk_height ) >= static_cast < common_t > ( std::numeric_limits < GLint > :: min () ) );
        ASSERT(static_cast<common_t>(quirk_height) <= static_cast<common_t>(std::numeric_limits<GLint>::max()));

        x = static_cast<GLint>(quirk_x);
        y = static_cast<GLint>(quirk_y);
        w = static_cast<GLsizei>(quirk_width);
        h = static_cast<GLsizei>(quirk_height);
#else
        w = static_cast<GLsizei>(width);
        h = static_cast<GLsizei>(height);
#endif
        TRACE_GLOBAL(CompositorTrace::GL, (_T("Set viewport: x=%d, y=%d w=%d h=%d"), x, y, w, h));

        glViewport(x, y, w, h);
        GL_ERROR();

        ret = (ret && GL_ERROR_WITH_RETURN()) != false;

        return ret;
    }

#undef GL_ERROR_WITH_RETURN
#undef GL_ERROR

// These ate just a copy of the previously defined macros
#define XSTRINGIFY(X) STRINGIFY(X)
#define STRINGIFY(X) #X

#ifdef EGL_VERSION_1_5
#define KHRFIX(name) name
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE
#define _EGL_NO_SYNC EGL_NO_SYNC
#define _EGL_FOREVER EGL_FOREVER
#define _EGL_NO_IMAGE EGL_NO_IMAGE
#define _EGL_NATIVE_PIXMAP EGL_NATIVE_PIXMAP_KHR
#define _EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED
#define _EGL_SYNC_STATUS EGL_SYNC_STATUS
#define _EGL_SIGNALED EGL_SIGNALED
#define _EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT
#else
#define _KHRFIX(left, right) left##right
#define KHRFIX(name) _KHRFIX(name, KHR)
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE_KHR
#define _EGL_NO_SYNC EGL_NO_SYNC_KHR
#define _EGL_FOREVER EGL_FOREVER_KHR
#define _EGL_NO_IMAGE EGL_NO_IMAGE_KHR
#define _EGL_NATIVE_PIXMAP EGL_NATIVE_PIXMAP_KHR
#define _EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED_KHR
#define _EGL_SYNC_STATUS EGL_SYNC_STATUS_KHR
#define _EGL_SIGNALED EGL_SIGNALED_KHR
#define _EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT_KHR
#endif

    // ODR use
    /* static */ constexpr void* const CompositorImplementation::EGL::EGL_NO_CONFIG;

    CompositorImplementation::EGL::Sync::Sync(EGLDisplay& dpy)
        : _dpy(dpy)
    {
        static bool supported = EGL::Supported(dpy, "EGL_KHR_fence_sync") != false;

        ASSERT(supported != false);
        ASSERT(_dpy != EGL_NO_DISPLAY);

        _sync = (supported && _dpy != EGL_NO_DISPLAY) != false ? KHRFIX(eglCreateSync)(_dpy, _EGL_SYNC_FENCE, nullptr) : InvalidSync();
    }

    CompositorImplementation::EGL::Sync::~Sync()
    {
        if (_sync == InvalidSync()) {
            // Error creating sync object or unable to create one
            glFinish();
        } else {
            // Mandatory
            glFlush();

            // .. but still execute, when needed, an additional flush to be on the safe side, and avoid a dreaded  deadlock
            EGLint val = static_cast<EGLint>(KHRFIX(eglClientWaitSync)(_dpy, _sync, _EGL_SYNC_FLUSH_COMMANDS_BIT, _EGL_FOREVER));

            if (val == static_cast<EGLint>(EGL_FALSE)
                || val != static_cast<EGLint>(_EGL_CONDITION_SATISFIED)) {

                EGLAttrib status;

                bool ret = KHRFIX(eglGetSyncAttrib)(_dpy, _sync, _EGL_SYNC_STATUS, &status) != EGL_FALSE;

                ret = ret
                    && status == _EGL_SIGNALED;

                // Assert on error
                if (ret != true) {
                    TRACE(Trace::Error, (_T ( "EGL: synchronization primitive" )));
                }
            }

            /* EGLBoolean */ val = static_cast<EGLint>(KHRFIX(eglDestroySync)(_dpy, _sync));

            if (val != EGL_TRUE) {
                // Error
            }

            // Consume the (possible) error(s)
            glGetError();
            CheckEGLError();
        }
    }

    CompositorImplementation::EGL::RenderThread::RenderThread(EGL& egl, GLES& gles)
        : Core::Thread()
        , _sharing(LockInstance())
        , _egl(egl)
        , _gles(gles)
    {
    }

    CompositorImplementation::EGL::RenderThread::~RenderThread()
    {
        Stop();

        /* bool */ Wait(Core::Thread::STOPPED, Core::infinite);
    }

    uint32_t CompositorImplementation::EGL::RenderThread::Worker()
    {
        return static_cast<uint32_t>(Core::infinite);
    }

    CompositorImplementation::EGL::SceneRenderer::SceneRenderer(EGL& egl, GLES& gles, CompositorImplementation& compositor)
        : RenderThread(egl, gles)
        , _compositor(compositor)
    {
    }

    CompositorImplementation::EGL::SceneRenderer::~SceneRenderer()
    {
        Stop();
        Wait(Core::Thread::STOPPED, Core::infinite);
    }

    uint32_t CompositorImplementation::EGL::SceneRenderer::Worker()
    {
        TRACE(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        bool status = (Render() && _compositor.FrameFlip()) != false;

        if (status != false) {
            Block();
        } else {
            Stop();
        }

        return 0;
    }

    bool CompositorImplementation::EGL::SceneRenderer::Render()
    {
        // No resize supported
        static Exchange::IComposition::ScreenResolution resolution = _compositor.Resolution();

        TRACE(CompositorTrace::Render, (_T("Render scene %dwx%dh "), WidthFromResolution(resolution), HeightFromResolution(resolution)));

        _sharing.Lock();

        bool ret = _egl.Render(std::bind(&GLES::RenderScene, &_gles, static_cast<GLuint>(WidthFromResolution(resolution)), static_cast<GLuint>(HeightFromResolution(resolution)), [](const GLES::texture& left, const GLES::texture& right) -> bool { bool ret = left . _z > right . _z ; return ret ; }), true) != false;

        _sharing.Unlock();

        TRACE(CompositorTrace::Render, (_T("Scene render done...")));

        return ret;
    }

    CompositorImplementation::EGL::TextureRenderer::TextureRenderer(EGL& egl, GLES& gles, CompositorImplementation::ClientContainer& clients)
        : RenderThread(egl, gles)
        , _clients(clients)
        , set { [](const element_t& lhs, const element_t& rhs) -> bool {
            bool ret = !(!(lhs._name < rhs._name) && !(lhs._name > rhs._name));
            return ret;
        } }
    {
    }

    CompositorImplementation::EGL::TextureRenderer::~TextureRenderer()
    {
        Stop();
        Wait(Core::Thread::STOPPED, Core::infinite);
    }

    void CompositorImplementation::EGL::TextureRenderer::SetClientName(const std::string& name)
    {
        const std::lock_guard<decltype(_access)> lock(_access);

        auto result = set.insert(element_t(name));

        if (result.second != true) {
            // Probably the element exist
        } else {
            // Add element to the queue
            queue.push(element_t(name));
            TRACE(CompositorTrace::Render, (_T("%s:%d %s Queued: %s"), __FILE__, __LINE__, __FUNCTION__, name.c_str()));
        }
    }

    uint32_t CompositorImplementation::EGL::TextureRenderer::Worker()
    {
        TRACE(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        uint32_t ret = Core::infinite;

        bool status = Render() != false;

        Block();

        if (status != false) {
            // TODO: do not exceed a single frame time for multiple

            const std::lock_guard<decltype(_access)> lock(_access);
            if (queue.size() > 0) {
                ret = 0;
            }

        } else {
            // TODO: Stop () implies no state change possible anymore.
            //            Stop ();
        }
        TRACE(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        return ret;
    }

    bool CompositorImplementation::EGL::TextureRenderer::Render()
    {
        bool ret = false;

        Core::ProxyType<Client> client;

        {
            const std::lock_guard<decltype(_access)> lock(_access);

            if (queue.size() > 0) {
                client = _clients.Find(queue.front()._name);
            }
        }

        if (client.IsValid() != true) {
            TRACE(Trace::Error, (_T ( "%s does not appear to be a valid client." ), client->Name()));
        } else {
            const Client::SurfaceType& surf = client->Surface();

            if ((surf.IsValid() && _egl.IsValid() && _gles.Valid())) {
                auto geom = client->Geometry();
                auto opa = client->Opacity();
                auto zorder = client->ZOrder();

                TRACE(CompositorTrace::Render, (_T("Rendering %s:\n- Position: x=%d y=%d z=%d \n- Size: width=%d height=%d\n- Opacity=%d"), client->Name().c_str(), geom.x, geom.y, zorder, geom.width, geom.height, opa));

                _sharing.Lock();

                ret = (_egl.RenderWithoutSwap(std::bind(&GLES::RenderEGLImage, &_gles, std::cref(surf._image), geom.x, geom.y, geom.width, geom.height, static_cast<EGLint>(zorder), static_cast<EGLint>(opa)))) != false;

                _sharing.Unlock();

                TRACE(CompositorTrace::Render, (_T("Rendering %s Done..."), client->Name().c_str()));
            }

            client.Release();
        }

        {
            const std::lock_guard<decltype(_access)> lock(_access);
            set.erase(queue.front());
            queue.pop();
        }

        return ret;
    }

    EGLImage CompositorImplementation::EGL::CreateImage(EGL const& egl, const Client::SurfaceType& surf)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        EGLImage ret = EGL_NO_IMAGE;

        static bool supported = (Supported(egl.Display(), "EGL_KHR_image")
                                    && Supported(egl.Display(), "EGL_KHR_image_base")
                                    && Supported(egl.Display(), "EGL_EXT_image_dma_buf_import")
                                    && Supported(egl.Display(), "EGL_EXT_image_dma_buf_import_modifiers"))
            != false;

        if ((egl.IsValid() && supported) != false) {
            constexpr char methodName[] = XSTRINGIFY(KHRFIX(eglCreateImage));

            static KHRFIX(EGLImage) (*peglCreateImage)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLAttrib*) = reinterpret_cast<KHRFIX(EGLImage) (*)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLAttrib*)>(eglGetProcAddress(methodName));

            if (peglCreateImage != nullptr) {
                auto width = gbm_bo_get_width(surf._buf);
                auto height = gbm_bo_get_height(surf._buf);
                auto stride = gbm_bo_get_stride(surf._buf);
                auto format = gbm_bo_get_format(surf._buf);
                auto modifier = gbm_bo_get_modifier(surf._buf);

                // Does it already exist ?
                ASSERT(surf._fd != InvalidFd);

                // Test our initial assumption
                ASSERT(format == ModeSet::SupportedBufferType());
                ASSERT(modifier == ModeSet::FormatModifier());

                // EGL may report differently than DRM
                ModeSet::GBM::dev_t dev = gbm_bo_get_device(surf._buf);
                ModeSet::GBM::fd_t fd = gbm_device_get_fd(dev);

                // EGL may report differently than DRM
                auto list_d_for = ModeSet::AvailableFormats(static_cast<ModeSet::DRM::fd_t>(fd));
                auto it_d_for = std::find(list_d_for.begin(), list_d_for.end(), format);

                bool valid = it_d_for != list_d_for.end();

                // Query EGL
                static EGLBoolean (*peglQueryDmaBufFormatsEXT)(EGLDisplay, EGLint, EGLint*, EGLint*) = reinterpret_cast<EGLBoolean (*)(EGLDisplay, EGLint, EGLint*, EGLint*)>(eglGetProcAddress("eglQueryDmaBufFormatsEXT"));
                static EGLBoolean (*peglQueryDmaBufModifiersEXT)(EGLDisplay, EGLint, EGLint, EGLuint64KHR*, EGLBoolean*, EGLint*) = reinterpret_cast<EGLBoolean (*)(EGLDisplay, EGLint, EGLint, EGLuint64KHR*, EGLBoolean*, EGLint*)>(eglGetProcAddress("eglQueryDmaBufModifiersEXT"));

                valid = valid
                    && peglQueryDmaBufFormatsEXT != nullptr
                    && peglQueryDmaBufModifiersEXT != nullptr;

                EGLint count = 0;

                valid = valid && peglQueryDmaBufFormatsEXT(egl.Display(), 0, nullptr, &count) != EGL_FALSE;

                EGLint formats[count];

                valid = valid && peglQueryDmaBufFormatsEXT(egl.Display(), count, &formats[0], &count) != EGL_FALSE;

                // format should be listed as supported
                if (valid != false) {
                    std::list<EGLint> list_e_for(&formats[0], &formats[count]);

                    auto it_e_for = std::find(list_e_for.begin(), list_e_for.end(), format);

                    valid = it_e_for != list_e_for.end();
                }

                valid = valid && peglQueryDmaBufModifiersEXT(egl.Display(), format, 0, nullptr, nullptr, &count) != EGL_FALSE;

                EGLuint64KHR modifiers[count];
                EGLBoolean external[count];

                // External is required to exclusive use withGL_TEXTURE_EXTERNAL_OES
                valid = valid && peglQueryDmaBufModifiersEXT(egl.Display(), format, count, &modifiers[0], &external[0], &count) != FALSE;

                // _modifier should be listed as supported, and _external should be true
                if (valid != false) {
                    std::list<EGLuint64KHR> list_e_mod(&modifiers[0], &modifiers[count]);

                    auto it_e_mod = std::find(list_e_mod.begin(), list_e_mod.end(), static_cast<EGLuint64KHR>(modifier));

                    valid = it_e_mod != list_e_mod.end();
                }

                if (valid != false) {
                    const EGLAttrib _attrs[] = {
                        EGL_WIDTH, static_cast<EGLAttrib>(width),
                        EGL_HEIGHT, static_cast<EGLAttrib>(height),
                        EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLAttrib>(format),
                        EGL_DMA_BUF_PLANE0_FD_EXT, static_cast<EGLAttrib>(surf._fd),
                        // TODO: magic constant
                        EGL_DMA_BUF_PLANE0_OFFSET_EXT, static_cast<EGLAttrib>(0),
                        EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLAttrib>(stride),
                        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, static_cast<EGLAttrib>(static_cast<EGLuint64KHR>(modifier) & 0xFFFFFFFF),
                        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, static_cast<EGLAttrib>(static_cast<EGLuint64KHR>(modifier) >> 32),
                        EGL_IMAGE_PRESERVED_KHR, static_cast<EGLAttrib>(EGL_TRUE),
                        EGL_NONE
                    };

                    ret = peglCreateImage(egl.Display(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, _attrs);
                }
            } else {
                // Error
                TRACE_GLOBAL(Trace::Error, (_T ( "%s is unavailable or invalid parameters." ), methodName));
            }
        } else {
            TRACE_GLOBAL(Trace::Error, (_T("EGL is not properly initialized.")));
        }

        return ret;
    }

    EGLImage CompositorImplementation::EGL::DestroyImage(EGL const& egl, const Client::SurfaceType& surf)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        EGLImage ret = surf._image;

        static bool supported = (Supported(egl.Display(), "EGL_KHR_image") && Supported(egl.Display(), "EGL_KHR_image_base")) != false;

        if ((egl.IsValid() && supported) != false) {
            constexpr const char methodName[] = XSTRINGIFY(KHRFIX(eglDestroyImage));

            static EGLBoolean (*peglDestroyImage)(EGLDisplay, KHRFIX(EGLImage)) = reinterpret_cast<EGLBoolean (*)(EGLDisplay, KHRFIX(EGLImage))>(eglGetProcAddress(KHRFIX("eglDestroyImage")));

            if (peglDestroyImage != nullptr && surf.IsValid() != false) {
                ret = peglDestroyImage(egl.Display(), surf._image) != EGL_FALSE ? EGL_NO_IMAGE : ret;
            } else {
                TRACE_GLOBAL(Trace::Error, (_T ( "%s is unavailable or invalid parameters are provided." ), methodName));
            }
        } else {
            TRACE_GLOBAL(Trace::Error, (_T ( "EGL is not properly initialized." )));
        }

        return ret;
    }

    bool CompositorImplementation::EGL::Invalidate()
    {
        bool ret = true;

        Deinitialize();

        return ret;
    }

    bool CompositorImplementation::EGL::Initialize()
    {
        bool ret = _natives.Valid();

        if (ret != false) {
            if (_dpy != EGL_NO_DISPLAY) {
                ret = false;

                if (eglTerminate(_dpy) != EGL_FALSE) {
                    ret = true;
                }
            }
        }

        if (ret != false) {
            _dpy = eglGetDisplay(_natives.Display());
            ret = _dpy != EGL_NO_DISPLAY;
        }

        if (ret != false) {
            EGLint major = 0, minor = 0;
            ret = eglInitialize(_dpy, &major, &minor) != EGL_FALSE;

            TRACE(Trace::Information, (_T ( "EGL version : %d.%d" ), static_cast<int>(major), static_cast<int>(minor)));
        }

        if (ret != false) {
            constexpr const EGLint attr[] = {
                EGL_SURFACE_TYPE, static_cast<EGLint>(EGL_WINDOW_BIT),
                EGL_RED_SIZE, RedBufferSize,
                EGL_GREEN_SIZE, GreenBufferSize,
                EGL_BLUE_SIZE, BlueBufferSize,
                EGL_ALPHA_SIZE, AlphaBufferSize,
                EGL_BUFFER_SIZE, RedBufferSize + GreenBufferSize + BlueBufferSize + AlphaBufferSize,
                EGL_RENDERABLE_TYPE, static_cast<EGLint>(EGL_OPENGL_ES2_BIT),
                EGL_DEPTH_SIZE, DepthBufferSize(),
                EGL_NONE
            };

            EGLint count = 0;

            if (eglGetConfigs(_dpy, nullptr, 0, &count) != EGL_TRUE) {
                count = 1;
            }

            std::vector<EGLConfig> conf(count, EGL_NO_CONFIG);

            /* EGLBoolean */ eglChooseConfig(_dpy, &attr[0], conf.data(), conf.size(), &count);

            _conf = conf[0];

            ret = _conf != EGL_NO_CONFIG;
        }

        if (ret != false) {
            EGLenum api = eglQueryAPI();

            ret = api == EGL_OPENGL_ES_API;

            if (ret != true) {
                /* EGLBoolean */ eglBindAPI(EGL_OPENGL_ES_API);
                ret = (CheckEGLError() == EGL_SUCCESS);
            }
        }

        if (ret != false) {
            constexpr const EGLint attr[] = {
                EGL_CONTEXT_CLIENT_VERSION, static_cast<EGLint>(GLES::MajorVersion()),
                EGL_NONE
            };

            _ctx = eglCreateContext(_dpy, _conf, EGL_NO_CONTEXT, attr);
            ret = _ctx != EGL_NO_CONTEXT;
        }

        if (ret != false) {
            constexpr const EGLint attr[] = {
                EGL_NONE
            };

            _surf = eglCreateWindowSurface(_dpy, _conf, _natives.Surface(), &attr[0]);
            ret = _surf != EGL_NO_SURFACE;
        }

        if (ret != true) {
            Deinitialize();
        }

        return ret;
    }

    void CompositorImplementation::EGL::Deinitialize()
    {
        eglMakeCurrent(_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (_surf != EGL_NO_SURFACE) {
            eglDestroySurface(_dpy, _surf);
            _surf = EGL_NO_SURFACE;
        }

        if (_ctx != EGL_NO_CONTEXT) {
            eglDestroyContext(_dpy, _ctx);
            _ctx = EGL_NO_CONTEXT;
        }

        if (_dpy != EGL_NO_DISPLAY) {
            eglTerminate(_dpy);
            _dpy = EGL_NO_DISPLAY;
        }
    }

    // Although compile / build time may succeed, runtime checks are also mandatory
    bool CompositorImplementation::EGL::Supported(const EGLDisplay dpy, const std::string& name)
    {
        bool ret = false;

#ifdef EGL_VERSION_1_5
        // KHR extension that have become part of the standard

        // Sync capability
        ret = name.find("EGL_KHR_fence_sync") != std::string::npos
            /* CreateImage / DestroyImage */
            || name.find("EGL_KHR_image") != std::string::npos
            || name.find("EGL_KHR_image_base") != std::string::npos;
#endif

        if (ret != true) {
            const char* ext = eglQueryString(dpy, EGL_EXTENSIONS);

            ret = ext != nullptr
                && name.size() > 0
                && (std::string(ext).find(name)
                    != std::string::npos);
        }

        return ret;
    }

    bool CompositorImplementation::EGL::Render(CompositorImplementation::GLES& /*gles*/)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        // Ensure the client API is set per thread basis
        bool ret = IsValid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            ret = eglSwapBuffers(_dpy, _surf) != EGL_FALSE;

            // Guarantee all (previous) effects of client API and frame buffer state are realized
            {
                Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Avoid any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_FALSE && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename... ARG>
    bool CompositorImplementation::EGL::Render(std::false_type const, FUNC&& func, const bool post, ARG&&... arg)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        // Ensure the client API is set per thread basis
        bool ret = IsValid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            if (post != false) {
                ret = func(std::forward<ARG>(arg)...) != false;

                {
                    Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
                }

                ret = ret
                    && eglSwapBuffers(_dpy, _surf) != EGL_FALSE;
            } else {
                ret = eglSwapBuffers(_dpy, _surf) != EGL_FALSE
                    && func(std::forward<ARG>(arg)...) != false;
            }

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but it avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename ARG0, typename... ARG>
    bool CompositorImplementation::EGL::Render(std::true_type const, FUNC&& func, const bool post, ARG0&& arg0, ARG&&... arg)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        // Ensure the client API is set per thread basis
        bool ret = IsValid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            if (post != false) {
                ret = (std::forward<ARG0>(arg0).*func)(std::forward<ARG>(arg)...) != false;

                {
                    Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
                }

                ret = ret
                    && eglSwapBuffers(_dpy, _surf) != EGL_FALSE;
            } else {
                ret = eglSwapBuffers(_dpy, _surf) != EGL_FALSE
                    && (std::forward<ARG0>(arg0).*func)(std::forward<ARG>(arg)...) != false;
            }

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but it avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename... ARG>
    bool CompositorImplementation::EGL::Render(FUNC&& func, const bool post, ARG&&... arg)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        bool ret = Render(typename std::is_member_pointer<FUNC>::type(), func, post, std::forward<ARG>(arg)...);
        return ret;
    }

    template <typename FUNC, typename... ARG>
    bool CompositorImplementation::EGL::Render(std::false_type const, FUNC&& prefunc, FUNC&& postfunc, ARG&&... arg)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        // Ensure the client API is set per thread basis
        bool ret = IsValid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            ret = prefunc(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            ret = ret
                && eglSwapBuffers(_dpy, _surf) != EGL_FALSE
                && postfunc(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename ARG0, typename... ARG>
    bool CompositorImplementation::EGL::Render(std::true_type const, FUNC&& prefunc, FUNC&& postfunc, ARG0&& arg0, ARG&&... arg)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        // Ensure the client API is set per thread basis
        bool ret = IsValid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            ret = (std::forward<ARG0>(arg0).*prefunc)(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            ret = ret
                && eglSwapBuffers(_dpy, _surf) != EGL_FALSE
                && (std::forward<ARG0>(arg0).*postfunc)(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename... ARG>
    bool CompositorImplementation::EGL::Render(FUNC&& prefunc, FUNC&& postfunc, ARG&&... arg)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        bool ret = Render(typename std::is_member_pointer<FUNC>::type(), prefunc, postfunc, std::forward<ARG>(arg)...);
        return ret;
    }

    template <typename FUNC, typename... ARG>
    bool CompositorImplementation::EGL::RenderWithoutSwap(std::false_type const, FUNC&& func, ARG&&... arg)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        // Ensure the client API is set per thread basis
        bool ret = IsValid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            ret = func(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename ARG0, typename... ARG>
    bool CompositorImplementation::EGL::RenderWithoutSwap(std::true_type const, FUNC&& func, ARG0&& arg0, ARG&&... arg)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        // Ensure the client API is set per thread basis
        bool ret = IsValid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            ret = (std::forward<ARG0>(arg0).*func)(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename... ARG>
    bool CompositorImplementation::EGL::RenderWithoutSwap(FUNC&& func, ARG&&... arg)
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        bool ret = RenderWithoutSwap(typename std::is_member_pointer<FUNC>::type(), func, std::forward<ARG>(arg)...);
        return ret;
    }

#undef STRINGIFY
#ifdef _KHRFIX
#undef _KHRFIX
#endif
#undef KHRFIX
#undef _EGL_SYNC_FENCE
#undef _EGL_NO_SYNC
#undef _EGL_FOREVER
#undef _EGL_NO_IMAGE
#undef _EGL_NATIVE_PIXMAP
#undef _EGL_CONDITION_SATISFIED
#undef _EGL_SYNC_STATUS
#undef _EGL_SIGNALED
#undef _EGL_SYNC_FLUSH_COMMANDS_BIT

    CompositorImplementation::Config::Config()
        : Core::JSON::Container()
        , _connector(_T ( "/tmp/compositor" ))
        , _port(_T ( "HDMI0" ))
    {

        Add(_T ( "connector" ), &_connector);
        Add(_T ( "port" ), &_port);
    }

    CompositorImplementation::Config::~Config()
    {
    }

    void CompositorImplementation::PlatformReady()
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        PluginHost::ISubSystem* subSystems(_service->SubSystems());

        ASSERT(subSystems != nullptr);

        if (subSystems != nullptr) {
            subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
            subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);

            subSystems->Release();
        }
    }

    CompositorImplementation::CompositorImplementation()
        : _adminLock()
        , _service(nullptr)
        , _engine()
        , _externalAccess(nullptr)
        , _observers()
        , _clients()
        , _modeSet()
        , _natives(_modeSet)
        , _egl(_natives)
        , _dmaFdServer(*this)
        , _clientLock()
        , _textureRenderer(_egl, _gles, _clients)
        , _sceneRenderer(_egl, _gles, *this)
    {
    }

    CompositorImplementation::~CompositorImplementation()
    {
        _textureRenderer.Stop();
        _textureRenderer.Wait(Core::Thread::STOPPED, Core::infinite);

        _sceneRenderer.Stop();
        _sceneRenderer.Wait(Core::Thread::STOPPED, Core::infinite);

        _egl.Invalidate();
        _natives.Invalidate();

        _dmaFdServer.Close();

        _clients.Clear();

        if (_externalAccess != nullptr) {
            delete _externalAccess;
            _engine.Release();
        }

        _service->Release();

        for (auto& observer : _observers) {
            observer->Release();
        }
    }

    // TODO:  run the _textureRenderer as a job on the ThreadPool
    bool CompositorImplementation::CompositeFor(const std::string& name)
    {
        // One client at a time
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_clientLock);

        _textureRenderer.SetClientName(name);

        // Skip the request to create a frame texture if the rate is too high to be processed
        if (_textureRenderer.IsRunning() != true) {
            _textureRenderer.Run();
        }

        return true;
    }

    bool CompositorImplementation::FrameFlip()
    {
        TRACE_GLOBAL(CompositorTrace::Render, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        using milli_t = int64_t;

        auto RefreshRateFromResolution = [](Exchange::IComposition::ScreenResolution const resolution) -> milli_t {
            // Assume 'unknown' rate equals 60 Hz
            milli_t rate = 60;

            switch (resolution) {
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz: // 1920x1080 progressive @ 24 Hz
                rate = 24;
                break;
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i25Hz: // 1920x1080 interlaced @ 25 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p25Hz: // 1920x1080 progressive @ 25 Hz
                rate = 25;
                break;
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p30Hz: // 1920x1080 progressive @ 30 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p30Hz: // 4K, 3840x2160 progressive @ 30 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p30Hz: // 8K, 7680x4320 progressive @ 30 Hz
                rate = 30;
                break;
            case Exchange::IComposition::ScreenResolution::ScreenResolution_576p50Hz: // 1024x576 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz: // 1280x720 progressive @ 50 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz: // 1920x1080 interlaced @ 50 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz: // 1920x1080 progressive @ 50 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz: // 4K, 3840x2160 progressive @ 50 Hz
                rate = 50;
                break;
            case Exchange::IComposition::ScreenResolution::ScreenResolution_480i: // 720x480
            case Exchange::IComposition::ScreenResolution::ScreenResolution_480p: // 720x480 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_576p: // 1024x576 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_576i: // 1024x576 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_720p: // 1280x720 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p: // 1280x720 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i: // 1280x720 progressive
            case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz: // 1920x1080 progressive @ 60 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz: // 4K, 3840x2160 progressive @ 60 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p60Hz: // 8K, 7680x4320 progressive @ 60 Hz
            case Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown:
                rate = 60;
            }

            return rate;
        };

        constexpr milli_t milli = 1000;

        // No resize supported
        static decltype(milli) rate = RefreshRateFromResolution(Resolution());

        static std::chrono::milliseconds delay(milli / rate);

        static std::chrono::milliseconds duration(delay);

        static auto start = std::chrono::steady_clock::now();

        ModeSet::BufferInfo bufferInfo(_natives.Surface(), ModeSet::GBM::InvalidBuf(), ModeSet::DRM::InvalidFb());
        _modeSet.Swap(bufferInfo);

        auto finish = std::chrono::steady_clock::now();

        duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);

        // Do not overstress the system
        // TODO: allow delta
        if (duration.count() < delay.count()) {
            SleepMs(delay.count() - duration.count());
        }

        start = finish;

        bool ret = true;

        return ret;
    }

    uint32_t CompositorImplementation::WidthFromResolution(const ScreenResolution resolution)
    {
        // Assume an invalid width equals 0
        uint32_t width = 0;

        switch (resolution) {
        case Exchange::IComposition::ScreenResolution::ScreenResolution_480p: // 720x480
            width = 720;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_576p50Hz: // 1024x576 progressive
        case Exchange::IComposition::ScreenResolution::ScreenResolution_576i: // 1024x576
        case Exchange::IComposition::ScreenResolution::ScreenResolution_576p: // 1024x576 progressive
            width = 1024;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_720p: // 1280x720 progressive
        case Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz: // 1280x720 @ 50 Hz
            width = 1280;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p: // 1280x720 progressive
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i: // 1280x720 progressive
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz: // 1920x1080 progressive @ 24 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i25Hz: // 1920x1080 interlaced @ 25 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p25Hz: // 1920x1080 progressive @ 25 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz: // 1920x1080 interlaced  @ 50 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz: // 1920x1080 progressive @ 50 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz: // 1920x1080 progressive @ 60 Hz
            width = 1920;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz: // 4K, 3840x2160 progressive @ 50 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz: // 4K, 3840x2160 progressive @ 60 Hz
            width = 3840;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p30Hz: // 8K, 7680x4320 progressive @ 30 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p60Hz: // 8K, 7680x4320 progressive @ 60 Hz
            width = 7680;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_480i: // Unknown according to the standards (?)
        case Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown:
        default:
            width = 0;
        }

        return width;
    }

    uint32_t CompositorImplementation::HeightFromResolution(const ScreenResolution resolution)
    {
        // Assume an invalid height equals 0
        uint32_t height = 0;

        switch (resolution) {
        case Exchange::IComposition::ScreenResolution::ScreenResolution_480i: // 720x480
        case Exchange::IComposition::ScreenResolution::ScreenResolution_480p: // 720x480 progressive
            height = 480;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_576p50Hz: // 1024x576 progressive
        case Exchange::IComposition::ScreenResolution::ScreenResolution_576i: // 1024x576
        case Exchange::IComposition::ScreenResolution::ScreenResolution_576p: // 1024x576 progressive
            height = 576;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_720p: // 1280x720 progressive
        case Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz: // 1280x720 progressive @ 50 Hz
            height = 720;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p: // 1280x720 progressive
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i: // 1280x720 progressive
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz: // 1920x1080 progressive @ 24 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i25Hz: // 1920x1080 interlaced @ 25 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p25Hz: // 1920x1080 progressive @ 25 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz: // 1920x1080 interlaced @ 50 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz: // 1920x1080 progressive @ 50 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz: // 1920x1080 progressive @ 60 Hz
            height = 1080;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz: // 4K, 3840x2160 progressive @ 50 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz: // 4K, 3840x2160 progressive @ 60 Hz
            height = 2160;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p30Hz: // 8K, 7680x4320 progressive @ 30 Hz
        case Exchange::IComposition::ScreenResolution::ScreenResolution_4320p60Hz: // 8K, 7680x4320 progressive @ 60 Hz
            height = 4320;
            break;
        case Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown:
        default:
            height = 0;
        }

        return height;
    }

    Exchange::IComposition::ScreenResolution CompositorImplementation::ResolutionFromHeightWidth(uint32_t const /*height*/, uint32_t const /*width*/)
    {
        // Given the options, the refresh rate is also important so the only sensible value is 'unknown'
        return Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;
    }

    uint32_t CompositorImplementation::Configure(PluginHost::IShell* service)
    {
        uint32_t result = Core::ERROR_NONE;

        _service = service;

        _service->AddRef();

        string configuration(service->ConfigLine());

        Config config;
        config.FromString(service->ConfigLine());

        _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());

        _externalAccess = new ExternalAccess(*this, Core::NodeId(config._connector.Value().c_str()), service->ProxyStubPath(), _engine);

        _dmaFdServer.Open(DmaFdConnector);

        if ((_externalAccess->IsListening() == true)) {
            _port = config._port.Value();
            PlatformReady();
        } else {
            delete _externalAccess;
            _externalAccess = nullptr;

            _engine.Release();

            TRACE(Trace::Error, (_T ( "Could not report PlatformReady as there was a problem starting the Compositor RPC %s" ), _T ( "server" )));

            result = Core::ERROR_OPENING_FAILED;
        }

        return result;
    }

    void CompositorImplementation::Register(Exchange::IComposition::INotification* notification)
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        _adminLock.Lock();

        ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());

        notification->AddRef();

        _observers.push_back(notification);

        _clients.Visit(
            [=](const string& name, const Core::ProxyType<Client>& element) { notification->Attached(name, &(*element)); });

        _adminLock.Unlock();
    }

    void CompositorImplementation::Unregister(Exchange::IComposition::INotification* notification)
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        _adminLock.Lock();

        std::list<Exchange::IComposition::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

        ASSERT(index != _observers.end());

        if (index != _observers.end()) {

            _clients.Visit(
                [=](const string& name, const Core::ProxyType<Client>& /*element*/) {
                    notification->Detached(name);
                });

            _observers.erase(index);

            notification->Release();
        }

        _adminLock.Unlock();
    }

    void CompositorImplementation::Attached(const string& name, IClient* client)
    {
        _adminLock.Lock();

        for (auto& observer : _observers) {
            observer->Attached(name, client);
            TRACE(Trace::Information, (_T("Client %s attached"), name.c_str()));
        }

        _adminLock.Unlock();
    }

    void CompositorImplementation::Detached(const string& name)
    {
        _adminLock.Lock();

        for (auto& observer : _observers) {
            observer->Detached(name);
            TRACE(Trace::Information, (_T("Client %s detached"), name.c_str()));
        }

        _adminLock.Unlock();
    }

    Core::instance_id CompositorImplementation::Native() const
    {
        EGLNativeDisplayType result(EGL_DEFAULT_DISPLAY);

        if (_natives.Valid() != false) {
            // Just remove the unexpected const if it exist
            result = reinterpret_cast<EGLNativeDisplayType>(_natives.Display());
        } else {
            TRACE(Trace::Error, (_T ( "The native display (id) might be invalid / unsupported. Using the EGL default display instead!" )));
        }

        return reinterpret_cast<Core::instance_id>(result);
    }

    string CompositorImplementation::Port() const
    {
        return (_port);
    }

    Exchange::IComposition::IClient* CompositorImplementation::CreateClient(const string& name, const uint32_t width, const uint32_t height)
    {
        IClient* client = nullptr;

        Core::ProxyType<Client> object = _clients.Instance<Client>(name, _modeSet, *this, name, width, height);

        ASSERT(object.IsValid() == true);

        if (object.IsValid() == true) {
            client = &(*object);
            Attached(name, client);
            client->AddRef();
        }

        if (client == nullptr) {
            TRACE(Trace::Error, (_T ( "Unable to create the Client with name %s" ), name.c_str()));
        } else {

            if (_sceneRenderer.IsRunning() != true) {
                _sceneRenderer.Run();
            }
        }

        return client;
    }

    void CompositorImplementation::InvalidateClient(const string& name)
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        Core::ProxyType<Client> object = _clients.Find(name);

        if (object.IsValid() != false) {

            Detached(object->Name());

            const Client::SurfaceType& surf = object->Surface();

            _gles.SkipEGLImageFromScene(surf._image);

            // TODO: Const object contains invalid member
            /*  surf . _image = */ EGL::DestroyImage(_egl, surf);
        }
    }

    Exchange::IComposition::ScreenResolution CompositorImplementation::Resolution() const
    {
        Exchange::IComposition::ScreenResolution resolution = Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;

        // decltype(std::declval<ModeSet>().Width()) width = _modeSet.Width();
        decltype(std::declval<ModeSet>().Height()) height = _modeSet.Height();

        // TODO: This might not be the whole story to determine progressive versus interlaced

        decltype(std::declval<ModeSet>().RefreshRate()) vrefresh = _modeSet.RefreshRate();
        decltype(std::declval<ModeSet>().Interlaced()) interlaced = _modeSet.Interlaced();

        if (interlaced != true) {
            switch (height) {
            case 480: {
                resolution = ScreenResolution::ScreenResolution_480p;
                break;
            }
            case 720: {
                resolution = vrefresh != 50 ? ScreenResolution::ScreenResolution_720p : ScreenResolution::ScreenResolution_720p50Hz;
                break;
            }
            case 1080: {
                switch (vrefresh) {
                case 24:
                    resolution = ScreenResolution::ScreenResolution_1080p24Hz;
                    break;
                case 50:
                    resolution = ScreenResolution::ScreenResolution_1080p50Hz;
                    break;
                case 60:
                    resolution = ScreenResolution::ScreenResolution_1080p60Hz;
                    break;
                default:
                    resolution = ScreenResolution::ScreenResolution_Unknown;
                }
                break;
            }
            case 2160: {
                switch (vrefresh) {
                case 50:
                    resolution = ScreenResolution::ScreenResolution_2160p50Hz;
                    break;
                case 60:
                    resolution = ScreenResolution::ScreenResolution_2160p60Hz;
                    break;
                default:
                    resolution = ScreenResolution::ScreenResolution_Unknown;
                }
                break;
            }
            default: {
                resolution = ScreenResolution::ScreenResolution_Unknown;
            }
            }
        } else {
            switch (height) {
            case 480: {
                resolution = ScreenResolution::ScreenResolution_480i;
                break;
            }
            case 1080: {
                resolution = vrefresh != 50 ? ScreenResolution::ScreenResolution_Unknown : ScreenResolution::ScreenResolution_1080i50Hz;
                break;
            }
            default: {
                resolution = ScreenResolution::ScreenResolution_Unknown;
            }
            }
        }

        return resolution;
    }

    uint32_t CompositorImplementation::Resolution(Exchange::IComposition::ScreenResolution const format)
    {
        TRACE(Trace::Error, (_T ( "Could not set screen resolution to %s. Not supported for Mesa compositor" ), Core::EnumerateType<Exchange::IComposition::ScreenResolution>(format).Data()));
        return Core::ERROR_UNAVAILABLE;
    }

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
