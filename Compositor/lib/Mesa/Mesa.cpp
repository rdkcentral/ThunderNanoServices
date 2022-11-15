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

#ifndef _GNU_SOURCE
#error mkotemp requires feature macro _GNU_SOURCE to be defined
#endif

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

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {

    // Forward declaration
    class CompositorImplementation;

    // Different namespace and scope than CompositorImplementation::EGL
    namespace EGL {
#ifdef EGL_VERSION_1_5
        using img_t = EGLImage;
#else
        using img_t = EGLImageKHR;
#endif
        using width_t = EGLint;
        using height_t = EGLint;

#ifdef EGL_VERSION_1_5
        static constexpr img_t InvalidImage()
        {
            return static_cast<img_t>(EGL_NO_IMAGE);
        }
#else
        static constexpr img_t InvalidImage()
        {
            return static_cast<img_t>(EGL_NO_IMAGE_KHR);
        }
#endif
    }

    constexpr char DmaFdConnector[] = "/tmp/Compositor/DmaFdConnector";

    class ClientSurface : public Exchange::IComposition::IClient,
                          public Exchange::IComposition::IRender,
                          public Exchange::IComposition::IClient::IProperties {
    private:
        // The buffer acts as a surface for the remote site
        struct {
            struct gbm_bo* _buf;
            int _fd;
            int _sync_fd;
            EGL::img_t _khr;

            bool Valid() const { return _buf != nullptr; }
            bool DMAComplete() const { return Valid() && _fd != ModeSet::GBM::InvalidFd() && _sync_fd != ModeSet::GBM::InvalidFd(); };
            bool RenderComplete() const
            {
                TRACE(Trace::Information,
                    (_T("ClientSurface _fd=%d _sync_fd=%d _khr=%p." ), _fd, _sync_fd, _khr));

                return Valid()
                    && _fd != ModeSet::GBM::InvalidFd()
                    && _sync_fd != ModeSet::GBM::InvalidFd()
                    && _khr != WPEFramework::Plugin::EGL::InvalidImage();
            }
        } _nativeSurface;

    private:
        ModeSet& _modeSet;
        CompositorImplementation& _compositor;
        std::string const _name;

        uint32_t _opacity;
        uint32_t _layer;

        Exchange::IComposition::Rectangle _destination;

    public:
        using surf_t = decltype(_nativeSurface);

        ClientSurface() = delete;
        ClientSurface(ClientSurface const&) = delete;
        ClientSurface& operator=(ClientSurface const&) = delete;

        ClientSurface(ModeSet&, CompositorImplementation&, const string&, uint32_t const, uint32_t const);
        ~ClientSurface() override;

        // Underlying native surface handle
        Core::instance_id Native() const override
        {
            TRACE(Trace::Information, (_T("%s:%d %s ClientSurface Native id=%d"), __FILE__, __LINE__, __FUNCTION__, static_cast<Core::instance_id>(_nativeSurface._fd)));
            return static_cast<Core::instance_id>(_nativeSurface._fd);
        }

        // Inter-process synchronization
        Core::instance_id SyncPrimitive() const;
        bool SyncPrimitiveStart() const;
        bool SyncPrimitiveEnd() const;

        // Just me
        string Name() const override
        {
            TRACE(Trace::Information, (_T("%s:%d %s ClientSurface Name %s"), __FILE__, __LINE__, __FUNCTION__, _name.c_str()));
            return _name;
        }

        // Opacity
        void Opacity(uint32_t const value) override { _opacity = value; }
        uint32_t Opacity() const override { return _opacity; }

        // Geometry
        uint32_t Geometry(Exchange::IComposition::Rectangle const& rectangle) override
        {
            _destination = rectangle;
            return static_cast<uint32_t>(Core::ERROR_NONE);
        }
        Exchange::IComposition::Rectangle Geometry() const override
        {
            TRACE(Trace::Information, (_T("%s:%d %s ClientSurface Geometry %dx%d"), __FILE__, __LINE__, __FUNCTION__, _destination.width, _destination.height));
            return _destination;
        }

        // Z-ordering within multiple surfaces
        uint32_t ZOrder(uint16_t const zorder) override
        {
            _layer = zorder;
            return static_cast<uint32_t>(Core::ERROR_NONE);
        }
        uint32_t ZOrder() const override { return _layer; }

        // Buffer to display flipping
        void ScanOut() override;

        surf_t const& Surface(EGL::img_t const& khr = EGL::InvalidImage());

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

        BEGIN_INTERFACE_MAP(ClientSurface)
        INTERFACE_ENTRY(Exchange::IComposition::IClient)
        INTERFACE_ENTRY(Exchange::IComposition::IRender)
        INTERFACE_ENTRY(Exchange::IComposition::IClient::IProperties)
        END_INTERFACE_MAP
    };

    class CompositorImplementation : public Exchange::IComposition, public Exchange::IComposition::IDisplay {
    public:
        // The (preferred) type of lock
        using lock_t = std::mutex;

        using ClientContainer = Core::ProxyMapType<string, ClientSurface>;

        // Underlying display 'true' dimensions
        using platform_w_t = decltype(std::declval<ModeSet>().Width());
        using platform_h_t = decltype(std::declval<ModeSet>().Height());

        CompositorImplementation(CompositorImplementation const&) = delete;
        CompositorImplementation& operator=(CompositorImplementation const&) = delete;

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
            ExternalAccess(ExternalAccess const&) = delete;
            ExternalAccess& operator=(ExternalAccess const&) = delete;

            ExternalAccess(CompositorImplementation&, Core::NodeId const&, string const&, Core::ProxyType<RPC::InvokeServer> const&);
            ~ExternalAccess() override;

        private:
            void* Acquire(string const&, uint32_t const, uint32_t const) override;

            CompositorImplementation& _parent;
        };

        class Natives final {
        private:
            ModeSet& _set;

            using dpy_return_t = decltype(std::declval<ModeSet>().UnderlyingHandle());
            using surf_return_t = decltype(std::declval<ModeSet>().CreateRenderTarget(0, 0));

            surf_return_t _surf = nullptr;

            bool _valid = false;

        public:
            Natives() = delete;
            Natives(Natives const&) = delete;
            Natives& operator=(Natives const&) = delete;

            using dpy_t = dpy_return_t;
            using surf_t = decltype(_surf);
            using valid_t = decltype(_valid);

            explicit Natives(ModeSet&);
            ~Natives();

            dpy_t Display() const { return _set.UnderlyingHandle(); }
            surf_t Surface() const { return _surf; }

            valid_t Invalidate();

            valid_t Valid() const { return _valid; }

        private:
            valid_t Initialize();
            void DeInitialize();
            static constexpr dpy_t InvalidDisplay() { return nullptr; }
            static constexpr surf_t InvalidSurface() { return nullptr; }
        };

        class GLES final {
        private:
            // x, y, z
            static constexpr uint8_t const VerticeDimensions = 3;

            struct offset {
                using coordinate_t = GLfloat;

                // Each coordinate in the range [-1.0f, 1.0f]
                static constexpr coordinate_t const _left = static_cast<coordinate_t>(-1.0f);
                static constexpr coordinate_t const _right = static_cast<coordinate_t>(1.0f);
                static constexpr coordinate_t const _bottom = static_cast<coordinate_t>(-1.0f);
                static constexpr coordinate_t const _top = static_cast<coordinate_t>(1.0f);
                static constexpr coordinate_t const _near = static_cast<coordinate_t>(-1.0f);
                static constexpr coordinate_t const _far = static_cast<coordinate_t>(1.0f);

                coordinate_t _x;
                coordinate_t _y;
                coordinate_t _z;

                offset();
                explicit offset(coordinate_t const&, coordinate_t const&, coordinate_t const&);
            } _offset;

            struct scale {
                using fraction_t = GLclampf;

                static constexpr fraction_t const _identity = static_cast<fraction_t>(1.0f);
                static constexpr fraction_t const _min = static_cast<fraction_t>(0.0f);
                static constexpr fraction_t const _max = static_cast<fraction_t>(1.0f);

                fraction_t _horiz;
                fraction_t _vert;

                scale();
                explicit scale(fraction_t const&, fraction_t const&);
            } _scale;

            struct opacity {
                using alpha_t = GLfloat;

                static constexpr alpha_t const _min = static_cast<alpha_t>(0.0f);
                static constexpr alpha_t const _max = static_cast<alpha_t>(1.0f);

                alpha_t _alpha;

                opacity();
                explicit opacity(alpha_t const&);
                explicit opacity(opacity const&);

                opacity& operator=(alpha_t const& alpha)
                {
                    _alpha = alpha;
                    return *this;
                }

                static opacity const& InitialOpacity()
                {
                    static opacity const op(_max);
                    return op;
                }
            } _opacity;

            bool _valid;

            using prog_t = GLuint;

            static constexpr prog_t InvalidProg() { return static_cast<prog_t>(0); }

        public:
            using tgt_t = GLuint;
            using tex_t = GLuint;
            using offset_t = decltype(_offset);
            using scale_t = decltype(_scale);
            using opacity_t = decltype(_opacity);

            using x_t = GLuint;
            using y_t = GLuint;
            using z_t = GLuint;
            using width_t = GLuint;
            using height_t = GLuint;

            using version_t = GLuint;

            using valid_t = decltype(_valid);

        public:
            struct texture {
                tex_t _tex;
                tgt_t _target;

                opacity_t _opacity;

                x_t _x;
                y_t _y;
                z_t _z;

                width_t _width;
                height_t _height;

                texture();
                explicit texture(tgt_t const, opacity_t const&);
                explicit texture(texture const&);
            };

        public:
            using texture_t = texture;

        private:
            std::map<EGL::img_t, texture_t> _scene;

            std::stack<texture_t> _preallocated;

            std::mutex _token;

        public:
            GLES(GLES const&) = delete;
            GLES& operator=(GLES const&) = delete;

            GLES();
            ~GLES();

            static offset_t const InitialOffset() { return offset(); }
            valid_t UpdateOffset(offset_t const&);

            static scale_t const InitialScale() { return scale(); }
            valid_t UpdateScale(scale_t const&);

            static opacity_t const& InitialOpacity() { return CompositorImplementation::GLES::opacity::InitialOpacity(); }
            valid_t UpdateOpacity(opacity_t const&);

            static constexpr tex_t InvalidTex() { return static_cast<tex_t>(0); }

            static constexpr version_t MajorVersion() { return static_cast<version_t>(2); }
            static constexpr version_t MinorVersion() { return static_cast<version_t>(0); }

            valid_t Valid() const { return _valid; }

            valid_t Render() { return Valid(); }
            valid_t RenderColor(bool const, bool const, bool const, bool const alpha = true);
            valid_t RenderEGLImage(EGL::img_t const&, EGLint const, EGLint const, EGL::width_t const, EGL::height_t const, EGLint, EGLint const);
            valid_t RenderScene(width_t const, height_t const, std::function<valid_t(texture_t const&, texture_t const&)>);

            valid_t SkipEGLImageFromScene(EGL::img_t const&);

        private:
            valid_t Initialize();
            valid_t Deinitialize();

            // TODO: precompile programs at initialization stage

            valid_t SetupProgram(char const[], char const[], prog_t&);

            valid_t RenderTileOES();
            valid_t RenderTile();

            template <size_t N>
            valid_t RenderPolygon(std::array<GLfloat const, N> const&);

            valid_t Supported(std::string const&);

            valid_t SetupViewport(EGLint const, EGLint const, EGL::width_t const, EGL::height_t const);
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

            using valid_t = bool;

        public:
            using ctx_t = decltype(_ctx);

            class Sync final {
            public:
                using dpy_t = EGLDisplay;

            public:
                Sync() = delete;
                Sync(Sync const&) = delete;
                Sync(Sync&&) = delete;

                Sync& operator=(Sync const&) = delete;
                Sync& operator=(Sync&&) = delete;

                void* operator new(size_t) = delete;
                void* operator new[](size_t) = delete;
                void operator delete(void*) = delete;
                void operator delete[](void*) = delete;

                explicit Sync(dpy_t&);
                ~Sync();

            private:
                using sync_t = KHRFIX(EGLSync);

                static constexpr dpy_t InvalidDpy() { return static_cast<dpy_t>(EGL::InvalidDpy()); }
                static constexpr sync_t InvalidSync() { return static_cast<sync_t>(_EGL_NO_SYNC); }

                sync_t _sync;
                dpy_t& _dpy;
            };

            class RenderThread : public Core::Thread {
            protected:
                using timeout_t = uint32_t;
                using lock_t = std::mutex;

                // Shared texture access
                lock_t& _sharing;

                EGL& _egl;
                GLES& _gles;

                explicit RenderThread(EGL&, GLES&);

            public:
                RenderThread(RenderThread const&) = delete;
                RenderThread& operator=(RenderThread const&) = delete;

                virtual ~RenderThread();

                uint32_t Worker() override;

            private:
                lock_t& Instance();
            };

            class SceneRenderer final : public RenderThread {
            private:
                using surf_t = ClientSurface;

                CompositorImplementation& _compositor;

                // Local
                std::mutex _access;

            public:
                SceneRenderer() = delete;
                SceneRenderer(SceneRenderer const&) = delete;
                SceneRenderer& operator=(SceneRenderer const&) = delete;

                explicit SceneRenderer(EGL&, GLES&, CompositorImplementation&);
                ~SceneRenderer() override;

                timeout_t Worker() override;

            private:
                EGL::valid_t Render();
            };

            class TextureRenderer final : public RenderThread {
            private:
                using surf_t = ClientSurface;

                // Local
                std::mutex _access;

                CompositorImplementation::ClientContainer& _clients;

                struct element {
                public:
                    std::string const& _name;

                    element() = delete;

                    explicit element(std::string const& name) : _name { name }
                    {
                    }
                    ~element() = default;
                };

                using element_t = struct element;

                using function_t = std::function<bool(element_t const&, element_t const&)>;

                // Unique elements, to prevent queue from growing beyond N
                std::set<element_t, function_t> set;
                // FIFO
                std::queue<element_t> queue;

            public:
                TextureRenderer() = delete;
                TextureRenderer(TextureRenderer const&) = delete;
                TextureRenderer& operator=(TextureRenderer const&) = delete;

                explicit TextureRenderer(EGL&, GLES&, CompositorImplementation::ClientContainer&);
                ~TextureRenderer();

                void SetClientName(std::string const&);

                timeout_t Worker() override;

            private:
                EGL::valid_t Render();
            };

        public:
            using dpy_t = Sync::dpy_t;
            using surf_t = EGLSurface;

            using height_t = WPEFramework::Plugin::EGL::height_t;
            using width_t = WPEFramework::Plugin::EGL::width_t;

            using size_t = EGLint;

            using img_t = WPEFramework::Plugin::EGL::img_t;

            EGL() = delete;
            EGL(EGL const&) = delete;
            EGL& operator=(EGL const&) = delete;

            EGL(Natives const& natives);
            ~EGL();

            static constexpr img_t InvalidImage() { return WPEFramework::Plugin::EGL::InvalidImage(); }

            static constexpr dpy_t InvalidDpy() { return EGL_NO_DISPLAY; }
            static constexpr ctx_t InvalidCtx() { return EGL_NO_CONTEXT; }
            static constexpr surf_t InvalidSurf() { return EGL_NO_SURFACE; }

            static constexpr EGL::size_t RedBufferSize() { return static_cast<EGL::size_t>(8); }
            static constexpr EGL::size_t GreenBufferSize() { return static_cast<EGL::size_t>(8); }
            static constexpr EGL::size_t BlueBufferSize() { return static_cast<EGL::size_t>(8); }
            static constexpr EGL::size_t AlphaBufferSize() { return static_cast<EGL::size_t>(8); }
            // For OpenGL ES 2.0 the only possible value is 16
            static constexpr EGL::size_t DepthBufferSize() { return static_cast<EGL::size_t>(GLES::MajorVersion() == static_cast<GLES::version_t>(2) ? 16 : 0); }

            static constexpr width_t InvalidWidth() { return static_cast<width_t>(0); }
            static constexpr height_t InvalidHeight() { return static_cast<height_t>(0); }

            dpy_t Display() const { return _dpy; }
            surf_t Surface() const { return _surf; }
            ctx_t Context() const { return _ctx; }

            height_t Height() const { return _height; }
            width_t Width() const { return _width; }

            static img_t CreateImage(EGL const&, ClientSurface::surf_t const&);
            static img_t DestroyImage(EGL const&, ClientSurface::surf_t const&);

            valid_t Invalidate();

            valid_t Valid() const { return _valid; }

        private:
            static constexpr void* const EGL_NO_CONFIG = nullptr;

            dpy_t _dpy = InvalidDpy();
            EGLConfig _conf = EGL_NO_CONFIG;
            surf_t _surf = InvalidSurf();

            width_t _width = InvalidWidth();
            height_t _height = InvalidHeight();

            Natives const& _natives;

            valid_t _valid = false;

            valid_t Initialize();
            void DeInitialize();

            template <typename FUNC, typename... ARG>
            valid_t Render(std::false_type const, FUNC&&, bool const, ARG&&...);

            template <typename FUNC, typename ARG0, typename... ARG>
            valid_t Render(std::true_type const, FUNC&&, bool const, ARG0&&, ARG&&...);

            // TODO: two different signatures for callables
            template <typename FUNC, typename... ARG>
            valid_t Render(std::false_type const, FUNC&&, FUNC&&, ARG&&...);

            template <typename FUNC, typename ARG0, typename... ARG>
            valid_t Render(std::true_type const, FUNC&&, FUNC&&, ARG0&&, ARG&&...);

            template <typename FUNC, typename... ARG>
            valid_t RenderWithoutSwap(std::false_type const, FUNC&&, ARG&&...);

            template <typename FUNC, typename ARG0, typename... ARG>
            valid_t RenderWithoutSwap(std::true_type const, FUNC&&, ARG0&&, ARG&&...);

        public:
            // Although compile / build time may succeed, runtime checks are also mandatory
            static valid_t Supported(dpy_t const, std::string const&);

            valid_t Render(GLES&);

            template <typename FUNC, typename... ARG>
            valid_t Render(FUNC&&, bool const, ARG&&...);

            template <typename FUNC, typename... ARG>
            valid_t Render(FUNC&&, FUNC&&, ARG&&...);

            template <typename FUNC, typename... ARG>
            valid_t RenderWithoutSwap(FUNC&&, ARG&&...);

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
            Config(Config const&) = delete;
            Config& operator=(Config const&) = delete;

            Config();
            ~Config();

            Core::JSON::String _connector;
            Core::JSON::String _port;
        };

        void PlatformReady();

        int SurfaceDmaFd(const uint32_t id)
        {
            static int res = -1;

            class Find : public ClientContainer::IFind {
            public:
                Find(const uint32_t id, const EGL& egl)
                    : _id(id)
                    , _result(-1)
                    , _egl(egl)
                {
                }

                bool Check(const string& key, const Core::ProxyType<ClientSurface>& client) const override
                {
                    bool isValid(false);

                    if (client->Native() == _id) {
                        ClientSurface::surf_t surf = client->Surface(EGL::CreateImage(_egl, client->Surface()));

                        _result = surf.Valid() ? client->Native() : -1;

                        isValid = surf.Valid();
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
        ModeSet _platform;

        Natives _natives;
        EGL _egl;
        GLES _gles;

        DmaFdServer _dmaFdServer;

        lock_t _clientLock;

        EGL::TextureRenderer _textureRenderer;
        EGL::SceneRenderer _sceneRenderer;

    public:
        CompositorImplementation();
        ~CompositorImplementation();

        bool CompositeFor(std::string const&);

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

        void Attached(string const&, IClient*);
        void Detached(string const&);

        //
        // Exchange::IComposition::IDisplay
        // ==================================================================================================================
        Core::instance_id Native() const override;

        string Port() const override;

        IClient* CreateClient(string const&, uint32_t const, uint32_t const) override;
        void InvalidateClient(string const&) /*override*/;

        Exchange::IComposition::ScreenResolution Resolution() const override;
        uint32_t Resolution(Exchange::IComposition::ScreenResolution const) override;

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        INTERFACE_ENTRY(Exchange::IComposition::IDisplay)
        END_INTERFACE_MAP
    };

    ClientSurface::ClientSurface(ModeSet& modeSet, CompositorImplementation& compositor, string const& name, uint32_t const width, uint32_t const height)
        : _nativeSurface { ModeSet::GBM::InvalidBuf(), ModeSet::GBM::InvalidFd(), ModeSet::GBM::InvalidFd(), WPEFramework::Plugin::EGL::InvalidImage() }
        , _modeSet { modeSet }
        , _compositor { compositor }
        , _name { name }
        , _opacity { Exchange::IComposition::maxOpacity }
        , _layer { 0 }
        , _destination({ 0, 0, width, height })
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        // The compositor should not be destroyed before any existing client surface
        _compositor.AddRef();

        // using surface_t = decltype(_nativeSurface._buf);
        // using class_t = std::remove_reference<decltype(_modeSet)>::type;
        // using return_t = decltype(std::declval<class_t>().CreateBufferObject(width, height));

        // TODO: The internal scan out flag might not be appropriate
        _nativeSurface._buf = _modeSet.CreateBufferObject(width, height);

        if (_nativeSurface.Valid() != true) {
            TRACE(Trace::Error, (_T ( "A ClientSurface cannot be created for %s" ), name.c_str()));
        } else {
            TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG gbm_bo_get_fd"), __FILE__, __LINE__, __FUNCTION__));
            _nativeSurface._fd = gbm_bo_get_fd(_nativeSurface._buf);

            constexpr char const SYNC_FD_TEMPLATE[] = "/tmp/Compositor/sync_fd_XXXXXX";

            _nativeSurface._sync_fd = mkostemp(const_cast<char*>(SYNC_FD_TEMPLATE), O_CLOEXEC);

            TRACE(Trace::Information, (_T("%s:%d %s gbm_bo_fd=%d, _sync_fd=%d, width=%d, height=%d, stride=%d, format=0x%04X, modifier=0x%" PRIX64), __FILE__, __LINE__, __FUNCTION__, _nativeSurface._fd, _nativeSurface._sync_fd, gbm_bo_get_width(_nativeSurface._buf), gbm_bo_get_height(_nativeSurface._buf), gbm_bo_get_stride(_nativeSurface._buf), gbm_bo_get_format(_nativeSurface._buf), gbm_bo_get_modifier(_nativeSurface._buf)));

            if (_nativeSurface.DMAComplete() != true) {
                TRACE(Trace::Error, (_T ( "The created ClientSurface for %s is not suitable for DMA." ), name.c_str()));
            }
        }
    }

    ClientSurface::~ClientSurface()
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        // Part of the client is cleaned up via the detached (hook)
        if (_nativeSurface._buf != ModeSet::GBM::InvalidBuf()) {
            _modeSet.DestroyBufferObject(_nativeSurface._buf);
        }

        if (_nativeSurface._fd != ModeSet::GBM::InvalidFd()) {
            int ret = close(_nativeSurface._fd);
            ASSERT(ret != -1);
        }

        if (_nativeSurface._sync_fd != ModeSet::GBM::InvalidFd()) {
            int ret = close(_nativeSurface._sync_fd);
            ASSERT(ret != -1);
        }

        _nativeSurface = { ModeSet::GBM::InvalidBuf(), ModeSet::GBM::InvalidFd(), ModeSet::GBM::InvalidFd(), WPEFramework::Plugin::EGL::InvalidImage() };

        _compositor.Release();
    }

    Core::instance_id ClientSurface::SyncPrimitive() const
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        return static_cast<Core::instance_id>(_nativeSurface._sync_fd);
    }

    bool ClientSurface::SyncPrimitiveStart() const
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        auto init = []() -> struct flock {
            struct flock fl;
            /* void * */ memset(&fl, 0, sizeof(fl));

            fl.l_type = F_WRLCK;
            fl.l_whence = SEEK_SET;
            fl.l_start = 0;
            fl.l_len = 0;

            return fl;
        };

        static struct flock fl = init();

        // Operates on i-node
        bool ret = _nativeSurface._sync_fd != ModeSet::GBM::InvalidFd()
            && fcntl(_nativeSurface._sync_fd, F_SETLKW, &fl) != -1;

        ASSERT(ret != false);

        return ret;
    }

    bool ClientSurface::SyncPrimitiveEnd() const
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        auto init = []() -> struct flock {
            struct flock fl;
            /* void * */ memset(&fl, 0, sizeof(fl));

            fl.l_type = F_UNLCK;
            fl.l_whence = SEEK_SET;
            fl.l_start = 0;
            fl.l_len = 0;

            return fl;
        };

        static struct flock fl = init();

        bool ret = _nativeSurface._sync_fd != ModeSet::GBM::InvalidFd()
            && fcntl(_nativeSurface._sync_fd, F_SETLK, &fl) != -1;

        ASSERT(ret != false);

        return ret;
    }

    void ClientSurface::ScanOut()
    {
        bool composed = _compositor.CompositeFor(_name);
        TRACE(Trace::Information, (_T("%s:%d %s for %s %s"), __FILE__, __LINE__, __FUNCTION__, _name.c_str(), (composed) ? "OK" : "Failed"));
    }

    ClientSurface::surf_t const& ClientSurface::Surface(EGL::img_t const& khr)
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        if (khr != EGL::InvalidImage()) {
            _nativeSurface._khr = khr;
        }

        return _nativeSurface;
    }

    CompositorImplementation::ExternalAccess::ExternalAccess(CompositorImplementation& parent, Core::NodeId const& source, string const& proxyStubPath, Core::ProxyType<RPC::InvokeServer> const& handler)
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

    CompositorImplementation::ExternalAccess::~ExternalAccess()
    {
    }

    void* CompositorImplementation::ExternalAccess::Acquire(string const& /*className*/, uint32_t const interfaceId, uint32_t const /*version*/)
    {
        // Use the className to check for multiple HDMI's.
        return (_parent.QueryInterface(interfaceId));
    }

    CompositorImplementation::Natives::Natives(ModeSet& set)
        : _set { set }
        , _valid { Initialize() }
    {
    }

    CompositorImplementation::Natives::~Natives()
    {
        DeInitialize();
        _valid = false;
    }

    CompositorImplementation::Natives::valid_t CompositorImplementation::Natives::Invalidate()
    {
        valid_t ret = true;

        DeInitialize();

        return ret;
    }

    CompositorImplementation::Natives::valid_t CompositorImplementation::Natives::Initialize()
    {
        valid_t ret = false;

        // TODO: return type Open
        ret = _set.Open("") == Core::ERROR_NONE
            && Display() != InvalidDisplay();

        using width_t = decltype(std::declval<ModeSet>().Width());
        using height_t = decltype(std::declval<ModeSet>().Height());

        width_t width = _set.Width();
        height_t height = _set.Height();

        if (ret != false) {
            _surf = _set.CreateRenderTarget(width, height);
            ret = _surf != InvalidSurface();
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Unable to create a compositor surface of dimensions: %u x %u [width, height]))." ), width, height));
        }

        return ret;
    }

    void CompositorImplementation::Natives::DeInitialize()
    {
        _valid = false;

        if (_surf != InvalidSurface()) {
            _set.DestroyRenderTarget(_surf);
        }

        _set.Close();
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
        : offset((_right - _left) / static_cast<coordinate_t>(2.0f) + _left, (_top - _bottom) / static_cast<coordinate_t>(2.0f) + _bottom, (_far - _near) / static_cast<coordinate_t>(2.0f) + _near)
    {
    }

    CompositorImplementation::GLES::offset::offset(coordinate_t const& x, coordinate_t const& y, coordinate_t const& z)
        : _x { x }
        , _y { y }
        , _z { z }
    {
    }

    // ODR use

    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_left;
    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_right;
    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_bottom;
    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_top;
    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_near;
    /* static */ constexpr CompositorImplementation::GLES::offset::coordinate_t const CompositorImplementation::GLES::offset::_far;

    /* static */ constexpr CompositorImplementation::GLES::scale::fraction_t const CompositorImplementation::GLES::scale::_identity;
    /* static */ constexpr CompositorImplementation::GLES::scale::fraction_t const CompositorImplementation::GLES::scale::_min;
    /* static */ constexpr CompositorImplementation::GLES::scale::fraction_t const CompositorImplementation::GLES::scale::_max;

    CompositorImplementation::GLES::scale::scale()
        : scale(CompositorImplementation::GLES::scale::_identity, CompositorImplementation::GLES::scale::_identity)
    {
    }

    CompositorImplementation::GLES::scale::scale(CompositorImplementation::GLES::scale::fraction_t const& horiz, CompositorImplementation::GLES::scale::fraction_t const& vert)
        : _horiz { horiz }
        , _vert { vert }
    {
    }

    /* static */ constexpr CompositorImplementation::GLES::opacity::alpha_t const CompositorImplementation::GLES::opacity::_min;
    /* static */ constexpr CompositorImplementation::GLES::opacity::alpha_t const CompositorImplementation::GLES::opacity::_max;

    CompositorImplementation::GLES::opacity::opacity()
        : opacity { _max }
    {
    }

    CompositorImplementation::GLES::opacity::opacity(CompositorImplementation::GLES::opacity::alpha_t const& alpha)
        : _alpha { alpha }
    {
    }

    CompositorImplementation::GLES::opacity::opacity(CompositorImplementation::GLES::opacity const& other)
        : _alpha { other._alpha }
    {
    }

    CompositorImplementation::GLES::texture::texture()
        : texture { static_cast<tgt_t>(GL_INVALID_ENUM), CompositorImplementation::GLES::opacity::InitialOpacity() }
    {
    }

    CompositorImplementation::GLES::texture::texture(CompositorImplementation::GLES::tgt_t const target, CompositorImplementation::GLES::opacity_t const& opacity)
        : _tex { 0 }
        , _target { target }
        , _opacity { opacity }
        , _x { 0 }
        , _y { 0 }
        , _z { 0 }
        , _width { 0 }
        , _height { 0 }
    {
    }

    CompositorImplementation::GLES::texture::texture(CompositorImplementation::GLES::texture const& other)
        : _tex { other._tex }
        , _target { other._target }
        , _opacity { other._opacity }
        , _x { other._x }
        , _y { other._y }
        , _z { other._z }
        , _width { other._width }
        , _height { other._height }
    {
    }

    CompositorImplementation::GLES::GLES()
        : _offset { InitialOffset() }
        , _scale { InitialScale() }
        , _opacity { InitialOpacity() }
        , _valid { Initialize() }
    {
    }

    CompositorImplementation::GLES::~GLES()
    {
        _valid = false;

        /* valid_t */ Deinitialize();
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::UpdateOffset(CompositorImplementation::GLES::offset_t const& off)
    {
        valid_t ret = false;

        // Range check without taking into account rounding errors
        if (off._x >= offset_t::_left
            && off._x <= offset_t::_right
            && off._y >= offset_t::_bottom
            && off._y <= offset_t::_top
            && off._z >= offset_t::_near
            && off._z <= offset_t::_far) {
            _offset = off;
            ret = true;
        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::UpdateScale(CompositorImplementation::GLES::scale_t const& scale)
    {
        valid_t ret = false;

        // Range check without taking into account rounding errors
        if (scale._horiz >= scale_t::_min
            && scale._horiz <= scale_t::_max
            && scale._vert >= scale_t::_min
            && scale._vert <= scale_t::_max) {
            _scale = scale;
            ret = true;
        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::UpdateOpacity(CompositorImplementation::GLES::opacity_t const& opacity)
    {
        valid_t ret = false;

        if (opacity._alpha >= opacity_t::_min
            && _opacity._alpha <= opacity_t::_max) {
            _opacity = opacity;
            ret = true;
        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderColor(bool const red, bool const green, bool const blue, bool const alpha)
    {
        static uint16_t degree = 0;

        constexpr decltype(degree) const ROTATION = 360;

        constexpr float const OMEGA = 3.14159265 / 180;

        valid_t ret = Valid();

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

        ret = ret
            && GL_ERROR_WITH_RETURN();

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderEGLImage(
        EGL::img_t const& img, EGLint const x, EGLint const y, EGL::width_t const width,
        EGL::height_t const height, EGLint const zorder, EGLint const opacity)
    {
        EGLDisplay dpy = EGL::InvalidDpy();
        EGLDisplay ctx = EGL::InvalidCtx();

        auto DestroyTexture = [this](texture_t& tex) -> valid_t {
            valid_t ret = tex._tex != InvalidTex();

            // Delete the previously created texture
            glDeleteTextures(1, &(tex._tex));

            tex._tex = InvalidTex();

            ret = ret
                && GL_ERROR_WITH_RETURN();

            return ret;
        };

        auto SetupTexture = [this, &dpy, &ctx](texture_t& tex, EGL::img_t const& img, EGL::width_t const width, EGL::height_t const height, bool const quick) -> valid_t {
            valid_t ret = GL_ERROR_WITH_RETURN();

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

                ret = dpy != EGL::InvalidDpy()
                    && ctx != EGL::InvalidCtx();

                static valid_t supported = (GLES::Supported("GL_OES_EGL_image")
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

            ret = ret
                && GL_ERROR_WITH_RETURN();

            return ret;
        };

        valid_t ret = GL_ERROR_WITH_RETURN()
            && img != EGL::InvalidImage()
            && width > 0
            && height > 0;

        // A valid GL context should exist for GLES::Supported ()
        /* EGL::ctx_t */ ctx = eglGetCurrentContext();
        /* EGL::dpy_t */ dpy = ctx != EGL::InvalidCtx() ? eglGetCurrentDisplay() : EGL::InvalidDpy();

        // OES dimensions should match the underlying buffer dimensions
        // Estimate using the EGL surface

        EGLSurface surf = EGL::InvalidSurf();

        if (ret != false) {
            surf = eglGetCurrentSurface(EGL_DRAW);

            ret = eglGetError() == EGL_SUCCESS
                && surf != EGL::InvalidSurf();
        }

        EGLint e_width = 0, e_height = 0;

        if (ret != false) {
            ret = eglQuerySurface(dpy, surf, EGL_WIDTH, &e_width) != EGL_FALSE
                && eglQuerySurface(dpy, surf, EGL_HEIGHT, &e_height) != EGL_FALSE
                && eglGetError() == EGL_SUCCESS;
        }

        ret = ret
            && eglGetError() == EGL_SUCCESS
            && ctx != EGL::InvalidCtx();

        // Set up the required textures

        // The  'shared' texture
        static texture_t tex_oes(GL_TEXTURE_EXTERNAL_OES, CompositorImplementation::GLES::opacity::InitialOpacity());

        static bool reuse = false;

        // The 'scene' texture
        texture_t tex_fbo(GL_TEXTURE_2D, CompositorImplementation::GLES::opacity::InitialOpacity());

        if (ret != false) {
            // Just an arbitrarily selected texture unit
            glActiveTexture(GL_TEXTURE0);
            GL_ERROR();

            // Using the EGL surface dimensions

#ifdef _RESIZE_TEXTURE
            // Did dimensions change ?

            using common_w_t = std::common_type<decltype(tex_oes._width), decltype(e_width)>::type;
            using common_h_t = std::common_type<decltype(tex_oes._height), decltype(e_height)>::type;

            if (static_cast<common_w_t>(tex_oes._width) != static_cast<common_w_t>(e_width)
                || static_cast<common_h_t>(tex_oes._height) != static_cast<common_h_t>(e_height)) {

                reuse = false;

                if (tex_oes._width > 0
                    && tex_oes._height > 0) {
                    reuse = DestroyTexture(tex_oes) != true;
                }
            }
#endif

            ret = SetupTexture(tex_oes, img, e_width, e_height, reuse) != false;
            reuse = ret;

            {
                std::lock_guard<decltype(_token)> const lock(_token);

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
                        auto it = _scene.insert(std::pair<EGL::img_t, texture_t>(img, tex_fbo));
                        ret = it.second != false;
                    }
                }

                // Update
                if (ret != false) {
                    using opacity_a_t = GLES::opacity_t::alpha_t;

                    using common_opacity_t = std::common_type<decltype(opacity), opacity_a_t, decltype(WPEFramework::Exchange::IComposition::minOpacity), decltype(WPEFramework::Exchange::IComposition::maxOpacity)>::type;

                    tex_fbo._opacity = (static_cast<opacity_a_t>(static_cast<common_opacity_t>(opacity) / (static_cast<common_opacity_t>(WPEFramework::Exchange::IComposition::maxOpacity) - static_cast<common_opacity_t>(WPEFramework::Exchange::IComposition::minOpacity))));
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
            ret = ret
                && GL_ERROR_WITH_RETURN()
                && status == GL_FRAMEBUFFER_COMPLETE;
            ;

            glDisable(GL_DEPTH_TEST);
            GL_ERROR();

            glDisable(GL_BLEND);
            GL_ERROR();

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

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderScene(CompositorImplementation::GLES::width_t const width, CompositorImplementation::GLES::height_t const height, std::function<CompositorImplementation::GLES::valid_t(CompositorImplementation::GLES::texture_t const& left, CompositorImplementation::GLES::texture_t const& right)> sortfunc)
    {
        valid_t ret = GL_ERROR_WITH_RETURN();

        if (ret != false) {

            // TODO: very inefficient way to get z-order sorted textures
            std::list<texture_t> sorted;

            {
                std::lock_guard<decltype(_token)> const lock(_token);

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
            glDepthRangef(GLES::offset_t::_near, GLES::offset_t::_far);
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
                offset_t off(_offset);
                scale_t scl(_scale);
                opacity_t op(_opacity);

                for (auto begin = sorted.begin(), it = begin, end = sorted.end(); it != end; it++) {
                    texture_t& tex = *it;

                    ASSERT(tex._target == static_cast<tgt_t>(GL_TEXTURE_2D));

                    glBindTexture(tex._target, tex._tex);
                    GL_ERROR();

                    using offset_x_t = decltype(GLES::offset_t::_x);
                    using offset_y_t = decltype(GLES::offset_t::_y);
                    using offset_z_t = decltype(GLES::offset_t::_z);

                    using scale_h_t = decltype(GLES::scale_t::_horiz);
                    using scale_v_t = decltype(GLES::scale_t::_vert);

                    using common_scale_h_t = std::common_type<scale_h_t, GLES::x_t, decltype(width)>::type;
                    using common_scale_v_t = std::common_type<scale_v_t, GLES::y_t, decltype(height)>::type;

                    GLES::scale_t g_scale(static_cast<GLES::scale::fraction_t>(static_cast<common_scale_h_t>(tex._width) / static_cast<common_scale_h_t>(width)), static_cast<GLES::scale::fraction_t>(static_cast<common_scale_v_t>(tex._height) / static_cast<common_scale_v_t>(height)));

                    using offset_x_t = decltype(GLES::offset_t::_x);
                    using offset_y_t = decltype(GLES::offset_t::_y);
                    using offset_z_t = decltype(GLES::offset_t::_z);

                    using zorder_t = GLES::z_t;

                    using common_offset_x_t = std::common_type<scale_h_t, GLES::x_t, offset_x_t>::type;
                    using common_offset_y_t = std::common_type<scale_v_t, GLES::y_t, offset_y_t>::type;
                    using common_offset_z_t = std::common_type<zorder_t, decltype(WPEFramework::Exchange::IComposition::minZOrder), decltype(WPEFramework::Exchange::IComposition::maxZOrder), offset_z_t>::type;

                    GLES::offset_t g_offset(static_cast<offset_x_t>(static_cast<common_offset_x_t>(g_scale._horiz) * static_cast<common_offset_x_t>(tex._x) / static_cast<common_offset_x_t>(tex._width)), static_cast<offset_y_t>(static_cast<common_offset_y_t>(g_scale._vert) * static_cast<common_offset_y_t>(tex._y) / static_cast<common_offset_y_t>(tex._height)), static_cast<offset_z_t>(static_cast<common_offset_z_t>(tex._z) / (static_cast<common_offset_z_t>(WPEFramework::Exchange::IComposition::maxZOrder) - static_cast<common_offset_z_t>(WPEFramework::Exchange::IComposition::minZOrder))));

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

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::SkipEGLImageFromScene(EGL::img_t const& img)
    {
        valid_t ret = false;

        std::lock_guard<decltype(_token)> const lock(_token);

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

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::Initialize()
    {
        valid_t ret = GL_ERROR_WITH_RETURN();
        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::Deinitialize()
    {
        valid_t ret = GL_ERROR_WITH_RETURN();

        glBindTexture(GL_TEXTURE_2D, InvalidTex());
        GL_ERROR();

        glBindTexture(GL_TEXTURE_EXTERNAL_OES, InvalidTex());
        GL_ERROR();

        std::lock_guard<decltype(_token)> const lock(_token);

        for (auto begin = _scene.begin(), it = begin, end = _scene.end(); it != end; it++) {
            tex_t& tex = it->second._tex;
            glDeleteTextures(1, &tex);
            GL_ERROR();
        }

        ret = ret && GL_ERROR_WITH_RETURN();

        return ret;
    }

    // TODO: precompile programs at initialization stage

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::SetupProgram(char const vtx_src[], char const frag_src[], prog_t& prog)
    {
        auto LoadShader = [](GLuint const type, GLchar const code[]) -> GLuint {
            GLuint shader = glCreateShader(type);
            GL_ERROR();

            valid_t ret = (GL_ERROR_WITH_RETURN() && shader != 0) != false;

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

        auto ShadersToProgram = [](GLuint const vertex, GLuint const fragment, prog_t& prog) -> valid_t {
            prog = glCreateProgram();
            GL_ERROR();

            valid_t ret = (GL_ERROR_WITH_RETURN() && prog != InvalidProg()) != false;

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

                ret = ret
                    && GL_ERROR_WITH_RETURN();
            }

            return ret;
        };

        auto DeleteCurrentProgram = []() -> valid_t {
            GLuint prog = InvalidProg();

            glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&prog));
            GL_ERROR();

            valid_t ret = (GL_ERROR_WITH_RETURN()
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

                ret = ret
                    && GL_ERROR_WITH_RETURN();
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
                /* valid_t */ DeleteCurrentProgram();
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

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderTileOES()
    {
        valid_t ret = GL_ERROR_WITH_RETURN();

        constexpr char const vtx_src[] = "#version 100                                  \n"
                                         "attribute vec3 position;                      \n"
                                         "varying vec2 coordinates;                     \n"
                                         "void main () {                                \n"
                                         "gl_Position = vec4 ( position . xyz , 1); \n"
                                         "coordinates = position . xy;              \n"
                                         "}                                             \n";

        constexpr char const frag_src[] = "#version 100                                                                     \n"
                                          "#extension GL_OES_EGL_image_external : require                                   \n"
                                          "precision mediump float;                                                         \n"
                                          "uniform samplerExternalOES sampler;                                              \n"
                                          "uniform float opacity;                                                           \n"
                                          "varying vec2 coordinates;                                                        \n"
                                          "void main () {                                                                   \n"
                                          "gl_FragColor = vec4 ( texture2D ( sampler , coordinates ) . rgb , opacity ); \n"
                                          "}                                                                                \n";

        std::array<GLfloat const, 4 * VerticeDimensions> const vert = {
            0.0f, 0.0f, 0.0f /* v0 */,
            1.0f, 0.0f, 0.0f /* v1 */,
            0.0f, 1.0f, 0.0f /* v2 */,
            1.0f, 1.0f, 0.0f /* v3 */
        };

        if (ret != false) {
            glDisable(GL_BLEND);
            GL_ERROR();

            static prog_t prog = InvalidProg();

            static valid_t supported = Supported("GL_OES_EGL_image_external") != false;

            ret = (ret
                      && RenderColor(false, false, false)
                      && supported
                      && SetupProgram(vtx_src, frag_src, prog)
                      && RenderPolygon(vert))
                != false;
        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderTile()
    {
        valid_t ret = GL_ERROR_WITH_RETURN();

        constexpr char const vtx_src[] = "#version 100                                   \n"
                                         "attribute vec3 position;                       \n"
                                         "varying vec2 coordinates;                      \n"
                                         "void main () {                                 \n"
                                         "gl_Position = vec4 ( position . xyz , 1 ); \n"
                                         "coordinates = position . xy;               \n"
                                         "}                                              \n";

        constexpr char const frag_src[] = "#version 100                                                            \n"
                                          "precision mediump float;                                                \n"
                                          "uniform sampler2D sampler;                                              \n"
                                          // Required by RenderPolygon
                                          "uniform float opacity;                                                  \n"
                                          "varying vec2 coordinates;                                               \n"
                                          "void main () {                                                          \n"
                                          "gl_FragColor = vec4 ( texture2D ( sampler , coordinates ) . rgba ); \n"
                                          "}                                                                       \n";

        std::array<GLfloat const, 4 * VerticeDimensions> const vert = {
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
            string_t const* ext = reinterpret_cast<string_t const*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

            valid_t ret = GL_ERROR_WITH_RETURN();
#endif

            static prog_t prog = InvalidProg();

            ret = (ret && SetupProgram(vtx_src, frag_src, prog) && RenderPolygon(vert)) != false;
        }

        return ret;
    }

    template <size_t N>
    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::RenderPolygon(std::array<GLfloat const, N> const& vert)
    {
        GLuint prog = InvalidProg();

        glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&prog));
        GL_ERROR();

        valid_t ret = (GL_ERROR_WITH_RETURN()
                          && prog != InvalidProg())
            != false;

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

            ret = ret
                && GL_ERROR_WITH_RETURN();
        }

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::Supported(std::string const& name)
    {
        valid_t ret = false;

        using string_t = std::string::value_type;
        using ustring_t = std::make_unsigned<string_t>::type;

        string_t const* ext = reinterpret_cast<string_t const*>(glGetString(GL_EXTENSIONS));
        GL_ERROR();

        ret = (GL_ERROR_WITH_RETURN() != false
                  && ext != nullptr
                  && name.size() > 0
                  && (std::string(ext).find(name) != std::string::npos))
            != false;

        return ret;
    }

    CompositorImplementation::GLES::valid_t CompositorImplementation::GLES::SetupViewport(EGLint const /*x*/, EGLint const /*y*/, EGL::width_t const width, EGL::height_t const height)
    {
        valid_t ret = GL_ERROR_WITH_RETURN();

        GLint dims[2] = { 0, 0 };

        glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &dims[0]);
        GL_ERROR();

#define _QUIRKS
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

        constexpr uint8_t const mult = 2;

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

        glViewport(static_cast<GLint>(quirk_x), static_cast<GLint>(quirk_y), static_cast<GLsizei>(quirk_width), static_cast<GLsizei>(quirk_height));
#else
        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
#endif
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

    CompositorImplementation::EGL::Sync::Sync(dpy_t& dpy)
        : _dpy { dpy }
    {
        static bool supported = EGL::Supported(dpy, "EGL_KHR_fence_sync") != false;

        ASSERT(supported != false);
        ASSERT(_dpy != InvalidDpy());

        _sync = (supported && _dpy != InvalidDpy()) != false ? KHRFIX(eglCreateSync)(_dpy, _EGL_SYNC_FENCE, nullptr) : InvalidSync();
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
            /* EGLint */ glGetError();
            /* EGLint */ eglGetError();
        }
    }

    CompositorImplementation::EGL::RenderThread::RenderThread(EGL& egl, GLES& gles)
        : Core::Thread()
        , _sharing { Instance() }
        , _egl { egl }
        , _gles { gles }
    {
    }

    CompositorImplementation::EGL::RenderThread::~RenderThread()
    {
        Stop();

        /* bool */ Wait(WPEFramework::Core::Thread::STOPPED, WPEFramework::Core::infinite);
    }

    uint32_t CompositorImplementation::EGL::RenderThread::Worker()
    {
        return static_cast<uint32_t>(Core::infinite);
    }

    CompositorImplementation::EGL::RenderThread::lock_t& CompositorImplementation::EGL::RenderThread::Instance()
    {
        static lock_t lock;
        return lock;
    }

    CompositorImplementation::EGL::SceneRenderer::SceneRenderer(EGL& egl, GLES& gles, CompositorImplementation& compositor)
        : RenderThread(egl, gles)
        , _compositor { compositor }
    {
    }

    CompositorImplementation::EGL::SceneRenderer::~SceneRenderer()
    {
        Stop();

        /* bool */ Wait(WPEFramework::Core::Thread::STOPPED, WPEFramework::Core::infinite);
    }

    CompositorImplementation::EGL::SceneRenderer::timeout_t CompositorImplementation::EGL::SceneRenderer::Worker()
    {
        // 'Lightning speed' frame flipping
        constexpr timeout_t const ret = 0;
        // TODO: frame flipping should limit the rate

        EGL::valid_t status = (Render() && _compositor.FrameFlip()) != false;

        if (status != false) {
            Block();
        } else {
            Stop();
        }

        return ret;
    }

    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::SceneRenderer::Render()
    {
        // No resize supported
        static Exchange::IComposition::ScreenResolution resolution = _compositor.Resolution();

        std::lock_guard<decltype(_sharing)> const sharing(_sharing);

        EGL::valid_t ret = _egl.Render(std::bind(&GLES::RenderScene, &_gles, static_cast<GLES::width_t>(WidthFromResolution(resolution)), static_cast<GLES::height_t>(HeightFromResolution(resolution)), [](GLES::texture_t const& left, GLES::texture_t const& right) -> GLES::valid_t { GLES::valid_t ret = left . _z > right . _z ; return ret ; }), true) != false;

        return ret;
    }

    CompositorImplementation::EGL::TextureRenderer::TextureRenderer(EGL& egl, GLES& gles, CompositorImplementation::ClientContainer& clients)
        : RenderThread(egl, gles)
        , _clients { clients }
        , set { [](element_t const& lhs, element_t const& rhs) -> bool {
            bool ret = !(!(lhs._name < rhs._name) && !(lhs._name > rhs._name));
            return ret;
        } }
    {
    }

    CompositorImplementation::EGL::TextureRenderer::~TextureRenderer()
    {
        Stop();

        /* bool */ Wait(WPEFramework::Core::Thread::STOPPED, WPEFramework::Core::infinite);
    }

    void CompositorImplementation::EGL::TextureRenderer::SetClientName(std::string const& name)
    {
        std::lock_guard<decltype(_access)> const lock(_access);

        auto result = set.insert(element_t(name));

        if (result.second != true) {
            // Probably the element exist
        } else {
            // Add element to the queue
            queue.push(element_t(name));
            TRACE(Trace::Information, (_T("%s:%d %s Queued: %s"), __FILE__, __LINE__, __FUNCTION__, name.c_str()));
        }
    }

    CompositorImplementation::EGL::TextureRenderer::timeout_t CompositorImplementation::EGL::TextureRenderer::Worker()
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        timeout_t ret = WPEFramework::Core::infinite;

        EGL::valid_t status = Render() != false;

        Block();

        if (status != false) {
            // TODO: do not exceed a single frame time for multiple

            std::lock_guard<decltype(_access)> const lock(_access);
            if (queue.size() > 0) {
                ret = 0;
            }

        } else {
            // TODO: Stop () implies no state change possible anymore.
            //            Stop ();
        }
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        return ret;
    }

    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::TextureRenderer::Render()
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        EGL::valid_t ret = false;

        Core::ProxyType<ClientSurface> client;

        {
            std::lock_guard<decltype(_access)> const lock(_access);

            if (queue.size() > 0) {
                client = _clients.Find(queue.front()._name);
            }
        }

        if (client.IsValid() != true) {
            TRACE(Trace::Error, (_T ( "%s does not appear to be a valid client." ), client->Name()));
        } else {
            ClientSurface::surf_t const& surf = client->Surface();

            ret = (surf.RenderComplete()
                      && _egl.Valid()
                      && _gles.Valid()
                      && client->SyncPrimitiveStart())
                != false;

            TRACE(Trace::Information,
                (_T ( "Client RenderComplete=%s _egl=%s _gles=%s SyncPrimitiveStart=%s ." ), surf.RenderComplete() ? "True" : "False", _egl.Valid() ? "True" : "False", _gles.Valid() ? "True" : "False", client->SyncPrimitiveStart() ? "True" : "False"));

            if (ret != false) {
                TRACE(Trace::Information, (_T ( "Client has an associated EGL image." )));

                using geom_t = decltype(std::declval<WPEFramework::Exchange::IComposition::IClient>().Geometry());
                geom_t geom = client->Geometry();

                //  Geom values should not exceed buf dimensions
                auto width = gbm_bo_get_width(surf._buf);
                auto height = gbm_bo_get_height(surf._buf);

                using buf_w_t = decltype(width);
                using buf_h_t = decltype(height);
                using geom_w_t = decltype(geom.width);
                using geom_h_t = decltype(geom.height);

                // TODO:
                constexpr bool const enable = true;

                // Narrowing, but not necessarily a problem
                // TODO minimum value
                using common_w_t = std::common_type<buf_w_t, geom_w_t>::type;
                ASSERT(static_cast<common_w_t>(geom.width) <= static_cast<common_w_t>(width));

                using common_h_t = std::common_type<buf_h_t, geom_h_t>::type;
                ASSERT(static_cast<common_h_t>(geom.height) <= static_cast<common_h_t>(height));

                // Opacity value should be within expected ranges
                auto opa = client->Opacity();

                using opacity_t = decltype(opa);

                using common_opacity_t = std::common_type<opacity_t, EGLint, decltype(WPEFramework::Exchange::IComposition::minOpacity), decltype(WPEFramework::Exchange::IComposition::maxOpacity)>::type;

                ASSERT(static_cast<common_opacity_t>(opa) >= static_cast<common_opacity_t>(std::numeric_limits<EGLint>::min())
                    || static_cast<common_opacity_t>(opa) <= static_cast<common_opacity_t>(std::numeric_limits<EGLint>::max()));

                auto zorder = client->ZOrder();

                using zorder_t = decltype(zorder);
                using common_zorder_t = std::common_type<zorder_t, EGLint>::type;

                ASSERT(static_cast<common_zorder_t>(zorder) >= static_cast<common_zorder_t>(std::numeric_limits<EGLint>::min())
                    || static_cast<common_zorder_t>(zorder) <= static_cast<common_zorder_t>(std::numeric_limits<EGLint>::max()));
                std::lock_guard<decltype(_sharing)> const sharing(_sharing);

                ret = (_egl.RenderWithoutSwap(std::bind(&GLES::RenderEGLImage, &_gles, std::cref(surf._khr), geom.x, geom.y, geom.width, geom.height, static_cast<EGLint>(zorder), static_cast<EGLint>(opa)))) != false;

                ret = client->SyncPrimitiveEnd() && ret;
            }

            client.Release();
        }

        {
            std::lock_guard<decltype(_access)> const lock(_access);

            /**/ set.erase(queue.front());
            /* void */ queue.pop();
        }

        return ret;
    }

    CompositorImplementation::EGL::EGL(Natives const& natives)
        : _natives { natives }
        , _valid { Initialize() }
    {
    }

    CompositorImplementation::EGL::~EGL()
    {
        DeInitialize();
        _valid = false;
    }

    EGL::img_t CompositorImplementation::EGL::CreateImage(EGL const& egl, ClientSurface::surf_t const& surf)
    {
        TRACE_GLOBAL(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        img_t ret = InvalidImage();

        static valid_t supported = (Supported(egl.Display(), "EGL_KHR_image")
                                       && Supported(egl.Display(), "EGL_KHR_image_base")
                                       && Supported(egl.Display(), "EGL_EXT_image_dma_buf_import")
                                       && Supported(egl.Display(), "EGL_EXT_image_dma_buf_import_modifiers"))
            != false;

        if ((egl.Valid() && supported) != false) {
            constexpr char methodName[] = XSTRINGIFY(KHRFIX(eglCreateImage));

            static KHRFIX(EGLImage) (*peglCreateImage)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, EGLAttrib const*) = reinterpret_cast<KHRFIX(EGLImage) (*)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, EGLAttrib const*)>(eglGetProcAddress(methodName));

            if (peglCreateImage != nullptr) {
                TRACE_GLOBAL(Trace::Information, (_T("%s:%d %s BRAM DEBUG gbm_bo_get_* props"), __FILE__, __LINE__, __FUNCTION__));

                auto width = gbm_bo_get_width(surf._buf);
                auto height = gbm_bo_get_height(surf._buf);
                auto stride = gbm_bo_get_stride(surf._buf);
                auto format = gbm_bo_get_format(surf._buf);
                auto modifier = gbm_bo_get_modifier(surf._buf);

                using width_t = decltype(width);
                using height_t = decltype(height);
                using stride_t = decltype(stride);
                using format_t = decltype(format);
                using fd_t = decltype(surf._fd);
                using modifier_t = decltype(modifier);

                // Does it already exist ?
                ASSERT(surf._fd != ModeSet::GBM::InvalidFd());

                // Test our initial assumption
                ASSERT(format == ModeSet::SupportedBufferType());
                ASSERT(modifier == ModeSet::FormatModifier());

                // TODO:
                constexpr bool const enable = false;

                TRACE_GLOBAL(Trace::Information, (_T("%s:%d %s BRAM DEBUG gbm_bo_get_device gbm_bo_get_fd"), __FILE__, __LINE__, __FUNCTION__));
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
                    EGLAttrib const _attrs[] = {
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

    EGL::img_t CompositorImplementation::EGL::DestroyImage(EGL const& egl, ClientSurface::surf_t const& surf)
    {
        TRACE_GLOBAL(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        img_t ret = surf._khr;

        static valid_t supported = (Supported(egl.Display(), "EGL_KHR_image") && Supported(egl.Display(), "EGL_KHR_image_base")) != false;

        if ((egl.Valid() && supported) != false) {
            constexpr char const methodName[] = XSTRINGIFY(KHRFIX(eglDestroyImage));

            static EGLBoolean (*peglDestroyImage)(EGLDisplay, KHRFIX(EGLImage)) = reinterpret_cast<EGLBoolean (*)(EGLDisplay, KHRFIX(EGLImage))>(eglGetProcAddress(KHRFIX("eglDestroyImage")));

            if (peglDestroyImage != nullptr && surf.RenderComplete() != false) {
                ret = peglDestroyImage(egl.Display(), surf._khr) != EGL_FALSE ? EGL::InvalidImage() : ret;
            } else {
                TRACE_GLOBAL(Trace::Error, (_T ( "%s is unavailable or invalid parameters are provided." ), methodName));
            }
        } else {
            TRACE_GLOBAL(Trace::Error, (_T ( "EGL is not properly initialized." )));
        }

        return ret;
    }

    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Invalidate()
    {
        valid_t ret = true;

        DeInitialize();

        return ret;
    }

    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Initialize()
    {
        valid_t ret = _natives.Valid();

        if (ret != false) {
            if (_dpy != InvalidDpy()) {
                ret = false;

                if (eglTerminate(_dpy) != EGL_FALSE) {
                    ret = true;
                }
            }
        }

        if (ret != false) {
            _dpy = eglGetDisplay(_natives.Display());
            ret = _dpy != InvalidDpy();
        }

        if (ret != false) {
            EGLint major = 0, minor = 0;
            ret = eglInitialize(_dpy, &major, &minor) != EGL_FALSE;

            TRACE(Trace::Information, (_T ( "EGL version : %d.%d" ), static_cast<int>(major), static_cast<int>(minor)));
        }

        if (ret != false) {
            constexpr EGLint const attr[] = {
                EGL_SURFACE_TYPE, static_cast<EGLint>(EGL_WINDOW_BIT),
                EGL_RED_SIZE, RedBufferSize(),
                EGL_GREEN_SIZE, GreenBufferSize(),
                EGL_BLUE_SIZE, BlueBufferSize(),
                EGL_ALPHA_SIZE, AlphaBufferSize(),
                EGL_BUFFER_SIZE, RedBufferSize() + GreenBufferSize() + BlueBufferSize() + AlphaBufferSize(),
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
                ret = eglGetError() == EGL_SUCCESS;
            }
        }

        if (ret != false) {
            constexpr EGLint const attr[] = {
                EGL_CONTEXT_CLIENT_VERSION, static_cast<EGLint>(static_cast<GLES::version_t>(GLES::MajorVersion())),
                EGL_NONE
            };

            _ctx = eglCreateContext(_dpy, _conf, EGL_NO_CONTEXT, attr);
            ret = _ctx != EGL_NO_CONTEXT;
        }

        if (ret != false) {
            constexpr EGLint const attr[] = {
                EGL_NONE
            };

            _surf = eglCreateWindowSurface(_dpy, _conf, _natives.Surface(), &attr[0]);
            ret = _surf != EGL::InvalidSurf();
        }

        if (ret != true) {
            DeInitialize();
        }

        return ret;
    }

    void CompositorImplementation::EGL::DeInitialize()
    {
        valid_t ret = _valid != false
            && eglMakeCurrent(_dpy, EGL::InvalidSurf(), EGL::InvalidSurf(), EGL::InvalidCtx()) != EGL_FALSE
            && eglGetError() == EGL_SUCCESS
            && eglDestroySurface(_dpy, _surf) != FALSE
            && eglGetError() == EGL_SUCCESS
            && eglDestroyContext(_dpy, _ctx) != EGL_FALSE
            && eglGetError() == EGL_SUCCESS
            && eglTerminate(_dpy) != EGL_FALSE
            && eglGetError() == EGL_SUCCESS;

        _valid = false;
    }

    // Although compile / build time may succeed, runtime checks are also mandatory
    bool CompositorImplementation::EGL::Supported(dpy_t const dpy, std::string const& name)
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
            char const* ext = eglQueryString(dpy, EGL_EXTENSIONS);

            ret = ext != nullptr
                && name.size() > 0
                && (std::string(ext).find(name)
                    != std::string::npos);
        }

        return ret;
    }

    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render(CompositorImplementation::GLES& /*gles*/)
    {
        // Ensure the client API is set per thread basis
        valid_t ret = Valid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            ret = eglSwapBuffers(_dpy, _surf) != EGL_FALSE;

            // Guarantee all (previous) effects of client API and frame buffer state are realized
            {
                WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Avoid any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL::InvalidSurf(), EGL::InvalidSurf(), EGL::InvalidCtx()) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename... ARG>
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render(std::false_type const, FUNC&& func, bool const post, ARG&&... arg)
    {
        // Ensure the client API is set per thread basis
        valid_t ret = Valid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            if (post != false) {
                ret = func(std::forward<ARG>(arg)...) != false;

                {
                    WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
                }

                ret = ret
                    && eglSwapBuffers(_dpy, _surf) != EGL_FALSE;
            } else {
                ret = eglSwapBuffers(_dpy, _surf) != EGL_FALSE
                    && func(std::forward<ARG>(arg)...) != false;
            }

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but it avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL::InvalidSurf(), EGL::InvalidSurf(), EGL::InvalidCtx()) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename ARG0, typename... ARG>
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render(std::true_type const, FUNC&& func, bool const post, ARG0&& arg0, ARG&&... arg)
    {
        // Ensure the client API is set per thread basis
        valid_t ret = Valid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            if (post != false) {
                ret = (std::forward<ARG0>(arg0).*func)(std::forward<ARG>(arg)...) != false;

                {
                    WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
                }

                ret = ret
                    && eglSwapBuffers(_dpy, _surf) != EGL_FALSE;
            } else {
                ret = eglSwapBuffers(_dpy, _surf) != EGL_FALSE
                    && (std::forward<ARG0>(arg0).*func)(std::forward<ARG>(arg)...) != false;
            }

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but it avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL::InvalidSurf(), EGL::InvalidSurf(), EGL::InvalidCtx()) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename... ARG>
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render(FUNC&& func, bool const post, ARG&&... arg)
    {
        valid_t ret = Render(typename std::is_member_pointer<FUNC>::type(), func, post, std::forward<ARG>(arg)...);
        return ret;
    }

    template <typename FUNC, typename... ARG>
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render(std::false_type const, FUNC&& prefunc, FUNC&& postfunc, ARG&&... arg)
    {
        // Ensure the client API is set per thread basis
        valid_t ret = Valid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            ret = prefunc(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            ret = ret
                && eglSwapBuffers(_dpy, _surf) != EGL_FALSE
                && postfunc(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL::InvalidSurf(), EGL::InvalidSurf(), EGL::InvalidCtx()) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename ARG0, typename... ARG>
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render(std::true_type const, FUNC&& prefunc, FUNC&& postfunc, ARG0&& arg0, ARG&&... arg)
    {
        // Ensure the client API is set per thread basis
        valid_t ret = Valid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            ret = (std::forward<ARG0>(arg0).*prefunc)(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            ret = ret
                && eglSwapBuffers(_dpy, _surf) != EGL_FALSE
                && (std::forward<ARG0>(arg0).*postfunc)(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL::InvalidSurf(), EGL::InvalidSurf(), EGL::InvalidCtx()) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename... ARG>
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::Render(FUNC&& prefunc, FUNC&& postfunc, ARG&&... arg)
    {
        valid_t ret = Render(typename std::is_member_pointer<FUNC>::type(), prefunc, postfunc, std::forward<ARG>(arg)...);
        return ret;
    }

    template <typename FUNC, typename... ARG>
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::RenderWithoutSwap(std::false_type const, FUNC&& func, ARG&&... arg)
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        // Ensure the client API is set per thread basis
        CompositorImplementation::EGL::valid_t ret = Valid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            ret = func(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL::InvalidSurf(), EGL::InvalidSurf(), EGL::InvalidCtx()) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename ARG0, typename... ARG>
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::RenderWithoutSwap(std::true_type const, FUNC&& func, ARG0&& arg0, ARG&&... arg)
    {
        // Ensure the client API is set per thread basis
        CompositorImplementation::EGL::valid_t ret = Valid() != false
            && eglMakeCurrent(_dpy, _surf, _surf, _ctx) != EGL_FALSE
            && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE;

        if (ret != false) {
            ret = (std::forward<ARG0>(arg0).*func)(std::forward<ARG>(arg)...) != false;

            // Guarantuee all (previous) effects of client API and frame buffer state are realized
            {
                WPEFramework::Plugin::CompositorImplementation::EGL::Sync sync(_dpy);
            }

            // Expensive, but avoids any memory leak if the local thread is stopped (by another thread)
            ret = eglMakeCurrent(_dpy, EGL::InvalidSurf(), EGL::InvalidSurf(), EGL::InvalidCtx()) != EGL_FALSE
                && ret;
        }

        if (ret != true) {
            TRACE(Trace::Error, (_T ( "Failed to complete rendering content." )));
        }

        return ret;
    }

    template <typename FUNC, typename... ARG>
    CompositorImplementation::EGL::valid_t CompositorImplementation::EGL::RenderWithoutSwap(FUNC&& func, ARG&&... arg)
    {
        valid_t ret = RenderWithoutSwap(typename std::is_member_pointer<FUNC>::type(), func, std::forward<ARG>(arg)...);
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
        , _platform()
        , _natives(_platform)
        , _egl(_natives)
        , _dmaFdServer(*this)
        , _clientLock()
        , _textureRenderer(_egl, _gles, _clients)
        , _sceneRenderer(_egl, _gles, *this)
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
    }

    CompositorImplementation::~CompositorImplementation()
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        _textureRenderer.Stop();

        /* bool */ _textureRenderer.Wait(WPEFramework::Core::Thread::STOPPED, WPEFramework::Core::infinite);

        _sceneRenderer.Stop();

        /* bool */ _sceneRenderer.Wait(WPEFramework::Core::Thread::STOPPED, WPEFramework::Core::infinite);

        /* valid_t ret = */ _egl.Invalidate();

        /* valid_t ret = */ _natives.Invalidate();

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
    bool CompositorImplementation::CompositeFor(std::string const& name)
    {
        TRACE(Trace::Information, (_T("%s:%d %s %s"), __FILE__, __LINE__, __FUNCTION__, name.c_str()));

        // One client at a time
        std::lock_guard<decltype(_clientLock)> const lock(_clientLock);

        /* void */ _textureRenderer.SetClientName(name);

        // Skip the request to create a frame texture if the rate is too high to be processed
        if (_textureRenderer.IsRunning() != true) {
            /* void */ _textureRenderer.Run();
        }

        return true;
    }

    bool CompositorImplementation::FrameFlip()
    {
        // TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
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
        /* void */ _platform.Swap(bufferInfo);

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

    uint32_t CompositorImplementation::WidthFromResolution(ScreenResolution const resolution)
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

    uint32_t CompositorImplementation::HeightFromResolution(ScreenResolution const resolution)
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
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

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
            [=](string const& name, Core::ProxyType<ClientSurface> const& element) { notification->Attached(name, &(*element)); });

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
                [=](string const& name, Core::ProxyType<ClientSurface> const& /*element*/) {
                    notification->Detached(name);
                });

            _observers.erase(index);

            notification->Release();
        }

        _adminLock.Unlock();
    }

    void CompositorImplementation::Attached(string const& name, IClient* client)
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        _adminLock.Lock();

        for (auto& observer : _observers) {
            observer->Attached(name, client);
        }

        _adminLock.Unlock();
    }

    void CompositorImplementation::Detached(string const& name)
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        _adminLock.Lock();

        for (auto& observer : _observers) {
            observer->Detached(name);
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

        TRACE(Trace::Information, (_T("%s:%d %s Display Native id=%d"), __FILE__, __LINE__, __FUNCTION__, reinterpret_cast<Core::instance_id>(result)));

        return reinterpret_cast<Core::instance_id>(result);
    }

    string CompositorImplementation::Port() const
    {
        return (_port);
    }

    WPEFramework::Exchange::IComposition::IClient* CompositorImplementation::CreateClient(string const& name, uint32_t const width, uint32_t const height)
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));
        IClient* client = nullptr;

        Core::ProxyType<ClientSurface> object = _clients.Instance<ClientSurface>(name, _platform, *this, name, width, height);

        ASSERT(object.IsValid() == true);

        if (object.IsValid() == true) {
            client = &(*object);

            Attached(name, client);

            client->AddRef();
        }

        if (client == nullptr) {
            TRACE(Trace::Error, (_T ( "Unable to create the ClientSurface with name %s" ), name.c_str()));
        } else {

            if (_sceneRenderer.IsRunning() != true) {
                /* void */ _sceneRenderer.Run();
            }
        }

        return client;
    }

    void CompositorImplementation::InvalidateClient(string const& name)
    {
        TRACE(Trace::Information, (_T("%s:%d %s BRAM DEBUG"), __FILE__, __LINE__, __FUNCTION__));

        Core::ProxyType<ClientSurface> object = _clients.Find(name);

        if (object.IsValid() != false) {

            Detached(object->Name());

            ClientSurface::surf_t const& surf = object->Surface();

            _gles.SkipEGLImageFromScene(surf._khr);

            // TODO: Const object contains invalid member
            /*  surf . _khr = */ EGL::DestroyImage(_egl, surf);
        }
    }

    Exchange::IComposition::ScreenResolution CompositorImplementation::Resolution() const
    {
        Exchange::IComposition::ScreenResolution resolution = WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;

        // decltype(std::declval<ModeSet>().Width()) width = _platform.Width();
        decltype(std::declval<ModeSet>().Height()) height = _platform.Height();

        // TODO: This might not be the whole story to determine progressive versus interlaced

        decltype(std::declval<ModeSet>().RefreshRate()) vrefresh = _platform.RefreshRate();
        decltype(std::declval<ModeSet>().Interlaced()) interlaced = _platform.Interlaced();

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
