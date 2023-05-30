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

#include <interfaces/IComposition.h>
#include <core/core.h>
#include <com/com.h>

#include "ModeSet.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <sys/socket.h>
#include <fcntl.h>

#ifndef _GNU_SOURCE
#error mkotemp requires feature macro _GNU_SOURCE to be defined
#endif

#ifdef __cplusplus
}
#endif

#include <stdlib.h>
#include <cstring>
#include <type_traits>
#include <functional>
#include <chrono>
#include <set>
#include <queue>
#include <stack>
#include<unordered_map>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

// Platform specific types
static_assert(std::is_convertible<struct gbm_device*, EGLNativeDisplayType>::value);
static_assert(std::is_convertible<struct gbm_surface*, EGLNativeWindowType>::value);

// Additional assumptions
static_assert(std::is_same<GLfloat, float>::value);
static_assert(std::is_same<GLclampf, float>::value);

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
#define _KHRFIX(left, right) left ## right
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

#define GL_ERROR_WITH_RETURN() (glGetError () == GL_NO_ERROR)
#ifdef NDEBUG
#define GL_ERROR() do{} while (0)
#else
#define GL_ERROR() ASSERT(GL_ERROR_WITH_RETURN())
#endif

namespace WPEFramework {
namespace GLResourceMediator {

    class ModeSetOpenOnce : public ModeSet {
    public:

        ModeSetOpenOnce(const ModeSetOpenOnce&) = delete;
        ModeSetOpenOnce& operator=(const ModeSetOpenOnce&) = delete;

        ModeSetOpenOnce(ModeSetOpenOnce&&) = delete;
        ModeSetOpenOnce& operator=(ModeSetOpenOnce&&) = delete;

        virtual ~ModeSetOpenOnce();

        static ModeSetOpenOnce& Instance();

        bool Open();
        bool Close();

    private:

        ModeSetOpenOnce();

        Core::CriticalSection _token;

        bool _open;
    };

    class Native final {
    public :

        class Prime final {
        public :

            Prime() = delete;
            Prime(const Prime&) = delete;
            Prime& operator=(const Prime&) = delete;

            explicit Prime(struct gbm_bo*);
            ~Prime();

            explicit Prime(Prime&&);
            Prime& operator=(Prime&&);

            int Fd() const { return _fd; }
            int SyncFd() const { return _sync_fd; }

            uint64_t Modifier() const { return _buf != ModeSet::GBM::InvalidBuffer() ? gbm_bo_get_modifier(_buf) : ModeSet::GBM::InvalidModifier(); }
            uint32_t Format() const  { return _buf != ModeSet::GBM::InvalidBuffer() ? gbm_bo_get_format(_buf) : ModeSet::GBM::InvalidFormat(); }
            uint32_t Stride() const { return _buf != ModeSet::GBM::InvalidBuffer() ? gbm_bo_get_stride(_buf) : 0; }

            uint32_t Height() const { return _buf != ModeSet::GBM::InvalidBuffer() ? gbm_bo_get_height(_buf) : ModeSet::GBM::InvalidHeight(); }
            uint32_t Width() const { return _buf != ModeSet::GBM::InvalidBuffer() ? gbm_bo_get_width(_buf) : ModeSet::GBM::InvalidWidth(); }

            bool Lock() const;
            bool Unlock() const;

            static constexpr int InvalidFd() { return ModeSet::GBM::InvalidFileDescriptor(); }
            static constexpr int InvalidSyncFd() { return ModeSet::GBM::InvalidFileDescriptor(); }
            static constexpr uint64_t InvalidModifier() { return ModeSet::GBM::InvalidModifier(); }
            static constexpr uint32_t InvalidFormat() { return ModeSet::GBM::InvalidFormat(); }
            static constexpr uint32_t InvalidStride() { return ModeSet::GBM::InvalidStride(); }
            static constexpr uint32_t InvalidWidth() { return  ModeSet::GBM::InvalidWidth(); }
            static constexpr uint32_t InvalidHeight() { return ModeSet::GBM::InvalidHeight(); }

            bool IsValid() const;

        private :

            bool InitializeLocks();

            int _fd;
            int _sync_fd;

            struct gbm_bo* _buf;

            mutable struct flock _acquire;
            mutable struct flock _release;

            bool _valid;
        };

    public :

        Native() = delete;

        Native(const Native&) = delete;
        Native& operator=(const Native&) = delete;

        Native(Native&&) = delete;
        Native& operator=(Native&&) = delete;

        explicit Native(ModeSet&, uint32_t, uint32_t);
        ~Native();

        const Prime& NativePrime() const { return _prime; }
        struct gbm_device* NativeDisplay() const { return _set.UnderlyingHandle(); }
        struct gbm_surface* NativeSurface() const { return _surf ; }

        uint32_t Width() const { return _width; }
        uint32_t Height() const { return _height; }

        bool Invalidate();

        static constexpr struct gbm_device* InvalidDisplay() { return ModeSet::GBM::InvalidDevice(); }
        static constexpr struct gbm_surface* InvalidSurface() { return ModeSet::GBM::InvalidSurface(); }
        static constexpr uint32_t InvalidWidth() { return ModeSet::GBM::InvalidWidth(); }
        static constexpr uint32_t InvalidHeight() { return ModeSet::GBM::InvalidHeight(); }

        bool IsValid() const { return _valid ; }

    private :

        explicit Native(ModeSet&);

        bool Initialize();
        bool Deinitialize();

        ModeSet& _set;

        struct gbm_device* _device;
        struct gbm_surface* _surf;
        struct gbm_bo* _buf;

        uint32_t _width;
        uint32_t _height;

        Prime _prime;

        bool _valid;
    };

    class EGL;

    class GLES final {
    public :

        class Opacity final {
        public :

            explicit Opacity(Opacity&&) = delete;
            Opacity& operator=(Opacity&&) = delete;

            Opacity();
            ~Opacity() = default;

            explicit Opacity(const Opacity&);
            Opacity& operator=(const Opacity&);

            GLfloat Alpha() const;
            bool Alpha(GLfloat);

            static constexpr GLfloat Min() { return 0.0f; }
            static constexpr GLfloat Max() { return 1.0f; }
            static constexpr GLfloat InitialOpacity() { return Opacity::Max(); }

        private :

            explicit Opacity(GLfloat);

            GLfloat _alpha;
        };

        class Offset final {
        public :

            Offset(Offset&&) = delete;
            Offset& operator=(Offset&&) = delete;

            Offset();
            ~Offset() = default;

            explicit Offset(const Offset&);
            Offset& operator=(const Offset&);

            GLfloat X() const { return _x; }
            bool X(GLfloat);

            GLfloat Y() const { return _y; }
            bool Y(GLfloat);

            GLfloat Z() const { return _z; }
            bool Z(GLfloat);

            static const Offset& InitialOffset();

            // Each coordinate in the range [-1.0f, 1.0f]
            static constexpr GLfloat Left() { return -1.0f; }
            static constexpr GLfloat Right() { return 1.0f; }
            static constexpr GLfloat Bottom() { return -1.0f; }
            static constexpr GLfloat Top() { return 1.0f; }
            static constexpr GLfloat Near() { return -1.0f; }
            static constexpr GLfloat Far() { return 1.0f; }

        private :

            explicit Offset(GLfloat, GLfloat, GLfloat);

            GLfloat _x;
            GLfloat _y;
            GLfloat _z;
        };

        class Scale final {
        public:

            Scale(Scale&&) = delete;
            Scale& operator=(Scale&&) = delete;

            explicit Scale();
            ~Scale() = default;

            explicit Scale(const Scale&);
            Scale& operator=(const Scale&);

            GLclampf Horizontal() const { return _horiz; }
            bool Horizontal(GLclampf);

            GLclampf Vertical() const { return _vert; }
            bool Vertical(GLclampf);

            static const Scale& InitialScale();

            static constexpr GLclampf Min() { return 0.0f; }
            static constexpr GLclampf Max() { return 1.0f; }

        private :

            explicit Scale(GLclampf, GLclampf);

            GLclampf _horiz;
            GLclampf _vert;
        };

        class Texture final {
        public :

            Texture() = delete;

            explicit Texture(const Texture&) = delete;
            Texture& operator=(const Texture&) = delete;

            explicit Texture(GLuint, GLuint, GLuint);
            ~Texture();

            explicit Texture(Texture&&);
            Texture& operator=(Texture&&);

            GLuint Target() const { return _target; }
            GLuint Handle() const { return _tex; }

            // Dimensions in pixels

            GLuint Width() const { return _width; }
            GLuint Height() const { return _height; }

            // Virtual offset in pixels, only relevant in rendering

            GLint X() const { return _x; }
            bool X(GLint);

            GLint Y() const { return _y; }
            bool Y(GLint);

            GLint Z() const { return _z; }
            bool Z(GLint);

            GLfloat SX() const { return _sx; }
            GLfloat SY() const { return _sx; }

            const GLES::Opacity& Opacity() const { return _opacity; }
            bool Opacity(GLfloat);

            bool Properties(GLuint, GLuint, GLuint, GLuint, GLuint, GLfloat);

            bool Bind();
            bool Unbind();

            bool Data(const GLvoid*, bool);

            template<typename FUNC, typename...ARG>
            bool Data(FUNC&& func, ARG&&... arg);

            static constexpr GLuint InvalidTexture() { return 0; }

            bool IsValid() { return _valid; }

        private :

            bool Initialize();

            template<typename FUNC, typename... ARG>
            bool Data(std::false_type, FUNC&&, ARG&&...);
            template<typename FUNC, typename ARG0, typename... ARG>
            bool Data(std::true_type, FUNC&&, ARG0&&, ARG&&...);

            static constexpr GLint MIN_X = -4096;
            static constexpr GLint MAX_X = 4096;
            static constexpr GLint MIN_Y = -4096;
            static constexpr GLint MAX_Y = 4096;
            static constexpr GLint MIN_Z = -128;
            static constexpr GLint MAX_Z = 128;

            GLuint _tex;

            GLuint _target;

            GLES::Opacity _opacity;

            GLint _x;
            GLint _y;
            GLint _z;

            GLuint _width;
            GLuint _height;

            GLfloat _sx;
            GLfloat _sy;

            bool _valid;
        };

        GLES(const GLES&) = delete;
        GLES& operator=(const GLES&) = delete;

        GLES(GLES&&) = delete;
        GLES& operator=(GLES&&) = delete;

        GLES() = delete;
        explicit GLES(EGL& egl);
        ~GLES();

        bool Render() { return IsValid(); }
        bool RenderColor(bool, bool, bool, bool) const;
        bool RenderSharedBuffer(const Native::Prime&, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
        // With user defined predicate for z-ordering
        bool RenderScene(GLuint, GLuint, std::function<bool (const Texture& , const Texture&)>);

        bool SkipSharedBufferFromScene(const Native::Prime&);

        static constexpr GLuint MajorVersion() { return 2; }
        static constexpr GLuint MinorVersion() { return 0; }

        bool IsValid() const { return _valid; }

    private :

        static constexpr GLuint DefaultFramebuffer() { return 0; }

        class Program final
        {
        public :

            Program() = delete;

            Program(const Program&) = delete;
            Program& operator=(const Program&) = delete;

            Program(Program&&) = delete;
            Program& operator=(Program&&) = delete;

            explicit Program(const std::string&, const std::string&);
            ~Program();

            bool LoadAndUse();

            static constexpr GLuint InvalidProgram() { return 0; }

            bool IsValid() const;

        private :

            static constexpr GLuint InvalidShader() { return 0; }

            bool CreateShader(GLenum, GLuint&) const;
            bool DestroyShader(GLuint) const;
            bool CompileAndLink(GLuint, GLuint);
            bool Destroy();

            const std::string _vtx_src;
            const std::string _frag_src;

            GLuint _id;
        };

        // x, y, z
        static constexpr uint8_t VerticeDimensions = 3;

        bool Initialize();
        bool Deinitialize();

        bool Supported(const std::string&) const;

        bool CreateTextureFromEGLImage(const KHRFIX(EGLImage)&, EGLint, EGLint, EGLint, EGLint, EGLint, EGLint);

        bool RenderSharedTextureToFBOTexture(Texture&, Texture&);
        bool RenderTileOES(const Opacity&) const;
        bool RenderTile(const Opacity&) const;
        template<size_t N>
        bool RenderPolygon(const std::array<GLfloat, N>&, const Opacity&) const;

        bool SetupViewport(GLuint, GLuint, GLuint, GLuint, const Offset&, const Scale&);

        std::unordered_map<KHRFIX(EGLImage), Texture> _scene;
        std::stack<Texture> _preallocated;

        Core::CriticalSection _token;

        EGL& _egl;

        std::function<void (GLenum, GLeglImageOES)> _pglEGLImageTargetTexture2DOES;

        bool _valid;
    };

    class EGL final {
    public :

        class Sync final {
        public :

            Sync() = delete;

            Sync(const Sync&) = delete;
            Sync& operator=(const Sync&) = delete;

            Sync(Sync&&) = delete;
            Sync& operator=(Sync&&) = delete;

            void* operator new(size_t) = delete;
            void* operator new[](size_t) = delete;
            void operator delete(void*) = delete;
            void operator delete[](void*) = delete;

            explicit Sync(const EGL&);
            ~Sync();

        private :

            static constexpr EGLDisplay InvalidDisplay() { return EGL::InvalidDisplay(); }

            static_assert(std::is_convertible<decltype(_EGL_NO_SYNC), KHRFIX(EGLSync)>::value);
            static constexpr KHRFIX(EGLSync) InvalidSync() { return _EGL_NO_SYNC; }

            // EGL may have changed at destruction time, but sync destruction is tied to the display that was used to create it
            EGLDisplay _dpy;

            const bool _supported;

            KHRFIX(EGLSync) _sync;
        };

        static constexpr EGLDisplay InvalidDisplay() { return EGL_NO_DISPLAY; }
        static constexpr EGLContext InvalidContext() { return EGL_NO_CONTEXT; }
        static constexpr EGLSurface InvalidSurface() { return EGL_NO_SURFACE; }

        static constexpr EGLNativeDisplayType InvalidDisplayType() { return Native::InvalidDisplay(); }
        static constexpr EGLNativeWindowType InvalidWindowType() { return Native::InvalidSurface(); }

        static_assert(std::is_convertible<decltype(_EGL_NO_IMAGE), KHRFIX(EGLImage)>::value);
        static constexpr KHRFIX(EGLImage) InvalidImage() { return _EGL_NO_IMAGE; }

        static constexpr EGLint RedBufferSize() { return 8; }
        static constexpr EGLint GreenBufferSize() { return 8; }
        static constexpr EGLint BlueBufferSize() { return 8; }
        static constexpr EGLint AlphaBufferSize() { return 8; }
        // For OpenGL ES 2.0 the only possible value is 16
        static constexpr EGLint DepthBufferSize() { return 16; }

        static constexpr EGLint InvalidWidth() { return  0; }
        static constexpr EGLint InvalidHeight() { return 0; }

        EGL() = delete;

        EGL(const EGL&) = delete;
        EGL& operator=(const EGL&) = delete;

        EGL& operator=(EGL&&) = delete;
        EGL(EGL&&) = delete;

        EGL(Native& native);
        ~EGL();

        bool CreateImage(const Native::Prime&);
        bool DestroyImage(const Native::Prime&);

        EGLDisplay Display() const { return _dpy; }
        EGLContext Context() const { return _ctx; }
        EGLSurface DrawSurface() const { return _draw; }
        EGLSurface ReadSurface() const { return _read; }

        KHRFIX(EGLImage) Image(const Native::Prime& prime) const;

        bool Bind() const;
        bool Unbind() const;

        bool Invalidate();

        bool Render() const;

        template<typename FUNC, typename... ARG>
        bool Render(FUNC&&, bool, ARG&&...) const;

        template<typename FUNC, typename... ARG>
        bool Render(FUNC&&, FUNC&&, ARG&&...) const;

        template<typename FUNC, typename... ARG>
        bool RenderWithoutSwap(FUNC&&, ARG&&...) const;

        bool Supported(const std::string&) const;

        bool IsValid() const { return _valid; }

    private :

        template<typename FUNC, typename... ARG>
        bool Render(std::false_type, FUNC&&, bool, ARG&&...) const;

        template<typename FUNC, typename ARG0, typename... ARG>
        bool Render(std::true_type, FUNC&&, bool, ARG0&&, ARG&&...) const;

        template<typename FUNC, typename...ARG>
        bool Render(std::false_type, FUNC&&, FUNC&&, ARG&&...) const;

        template<typename FUNC, typename ARG0, typename... ARG>
        bool Render(std::true_type, FUNC&&, FUNC&&, ARG0&&, ARG&&...) const;

        template<typename FUNC, typename... ARG>
        bool RenderWithoutSwap(std::false_type, FUNC&&, ARG&&...) const;

        template<typename FUNC, typename ARG0, typename... ARG>
        bool RenderWithoutSwap(std::true_type, FUNC&&, ARG0&&, ARG&&...) const;

        bool Configure();

        bool Initialize();
        bool DeInitialize();

        // Use with care
        bool UpdateCurrents();

#ifdef EGL_VERSION_1_5
#else
        using EGLAttrib = EGLint;
#endif
        using EGLuint64KHR = khronos_uint64_t;

        static_assert(std::is_pointer<EGLConfig>::value);
        static constexpr const EGLConfig InvalidConfig() { return nullptr; }

        Native& _native;

        EGLDisplay _dpy;
        EGLConfig _cfg;
        EGLContext _ctx;
        EGLSurface _draw;
        EGLSurface _read;

// FIXME:: unordered
        std::map<std::tuple<int, int>, KHRFIX(EGLImage)> _images;

        std::function<KHRFIX(EGLImage) (EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLAttrib*)> _peglCreateImage;
        std::function<EGLBoolean (EGLDisplay, KHRFIX(EGLImage))> _peglDestroyImage;
        std::function<EGLBoolean (EGLDisplay, EGLint, EGLint*, EGLint*)> _peglQueryDmaBufFormatsEXT;
        std::function<EGLBoolean (EGLDisplay, EGLint, EGLint, EGLuint64KHR*, EGLBoolean*, EGLint*)> _peglQueryDmaBufModifiersEXT;

        bool _valid;
    };

}
}

namespace WPEFramework {
namespace Plugin {

    class CompositorImplementation;

    class ClientSurface : public Exchange::IComposition::IClient, Exchange::IComposition::IRender {
    public :

        ClientSurface() = delete;

        ClientSurface(const ClientSurface&) = delete;
        ClientSurface& operator=(const ClientSurface&) = delete;

        ClientSurface(ClientSurface&&) = delete;
        ClientSurface& operator=(ClientSurface&&) = delete;

        ClientSurface(ModeSet&, CompositorImplementation&, const string&, uint32_t, uint32_t);
        ~ClientSurface() override;

        // Underlying native surface handle
        Core::instance_id Native() const override;

        const GLResourceMediator::Native& NativeAccessor() const;

        // Inter-process synchronization
        int SyncPrimitive() const;
        bool SyncPrimitiveStart() const;
        bool SyncPrimitiveEnd() const;

        std::string Name() const override { return _name; }

        void Opacity(uint32_t value) override { _opacity = value; }
        uint32_t Opacity() const override { return _opacity; }

        uint32_t Geometry(const Exchange::IComposition::Rectangle&) override;
        Exchange::IComposition::Rectangle Geometry() const override { return _destination; }

        // Z-ordering within multiple surfaces
        uint32_t ZOrder(uint16_t zorder) override;
        uint32_t ZOrder() const override { return _layer; }

        // Show content
        void ScanOut() override;

        BEGIN_INTERFACE_MAP(ClientSurface)
            INTERFACE_ENTRY(IClient)
            INTERFACE_ENTRY(IRender)
        END_INTERFACE_MAP

    private :

        GLResourceMediator::Native _native;

        ModeSet& _modeSet;

        CompositorImplementation& _compositor;

        const std::string _name;

        uint32_t _opacity;
        uint32_t _layer;

        Exchange::IComposition::Rectangle _destination;
    };

    class CompositorImplementation : public Exchange::IComposition, public Exchange::IComposition::IDisplay {
    public :

        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        CompositorImplementation(CompositorImplementation&&) = delete;
        CompositorImplementation& operator=(CompositorImplementation&&) = delete;

        CompositorImplementation();
        virtual ~CompositorImplementation() override;

        template<size_t N> bool FDFor(const std::string&, std::array<int, N>&, std::string&);

        bool CompositeFor(const std::string&);

        static uint8_t RefreshRateFromResolution(const ScreenResolution);

        bool FrameFlip();

        // Exchange::IComposition

        static uint32_t WidthFromResolution(const ScreenResolution);
        static uint32_t HeightFromResolution(const ScreenResolution);
        static ScreenResolution ResolutionFromHeightWidth(uint32_t, uint32_t);

        uint32_t Configure(PluginHost::IShell*) override;

        void Register(INotification*) override;
        void Unregister(INotification*) override;

        void Attached(const string&, IClient*);
        void Detached(const string&);

        // Exchange::IComposition::IDisplay

        Core::instance_id Native() const override;

        string Port() const override;

        IClient* CreateClient(const string&, uint32_t, uint32_t) override;
        void InvalidateClient(const string&) override;

        ScreenResolution Resolution() const override;
        uint32_t Resolution(const ScreenResolution) override;

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(IComposition)
        INTERFACE_ENTRY(IComposition::IDisplay)
        END_INTERFACE_MAP

    private :

        using ClientContainer = Core::ProxyMapType<string, ClientSurface>;

        // The communication channel with the (remote) other site
        class ExternalAccess : public RPC::Communicator {
        public :

            ExternalAccess() = delete;

            ExternalAccess(const ExternalAccess&) = delete;
            ExternalAccess& operator=(const ExternalAccess&) = delete;

            ExternalAccess(ExternalAccess&&) = delete;
            ExternalAccess& operator=(ExternalAccess&&) = delete;

            ExternalAccess(CompositorImplementation&, const Core::NodeId&, const string&, const Core::ProxyType<RPC::InvokeServer>&);
            ~ExternalAccess() override;

        private :

            void* Aquire(const string&, uint32_t, uint32_t) override;

            CompositorImplementation& _parent;
        };

        class DMATransfer final : public Core::Thread {
        public :

            DMATransfer() = delete;

            DMATransfer(const DMATransfer&) = delete;
            DMATransfer& operator=(const DMATransfer&) = delete;

            DMATransfer(DMATransfer&&) = delete;
            DMATransfer& operator=(DMATransfer&&) = delete;

            explicit DMATransfer(CompositorImplementation&);
            ~DMATransfer();

            bool IsValid() const;

            template<size_t N> bool Receive(std::string&, std::array<int, N>&);
            template<size_t N> bool Send(const std::string&, const std::array<int, N>&);

            static constexpr int InvalidFiledescriptor() { return -1; };

        private :

            static constexpr int InvalidSocket() { return -1; };

            uint32_t Worker() override;

            bool Init();
            bool Deinit();

            bool Connect(uint32_t) const;
            bool Disconnect(uint32_t) const;

            bool Send(const std::string&, const int* , uint8_t);
            bool Receive(std::string&, int*, uint8_t);

            CompositorImplementation& _compositor;

            int _listen;
            int _transfer;

            const struct sockaddr_un _addr;

            bool _valid;
        };

        class RenderThread : public Core::Thread {
        public :

            RenderThread(const RenderThread&) = delete;
            RenderThread& operator=(const RenderThread&) = delete;

            RenderThread(RenderThread&&) = delete;
            RenderThread& operator=(RenderThread&&) = delete;

            virtual ~RenderThread();

            uint32_t Worker() override;

        protected :

            explicit RenderThread(GLResourceMediator::EGL&, GLResourceMediator::GLES&);

            // Shared texture access
            Core::CriticalSection& _sharing;

            GLResourceMediator::EGL& _egl;
            GLResourceMediator::GLES& _gles;

        private :

            Core::CriticalSection& Instance();
        };

        class SceneRenderer final : public RenderThread {
        public :

            SceneRenderer() = delete;

            SceneRenderer(const SceneRenderer&) = delete;
            SceneRenderer& operator=(const SceneRenderer&) = delete;

            SceneRenderer(SceneRenderer&&) = delete;
            SceneRenderer& operator=(SceneRenderer&&) = delete;

            explicit SceneRenderer(GLResourceMediator::EGL&, GLResourceMediator::GLES&, CompositorImplementation&);
            virtual ~SceneRenderer();

            uint32_t Worker() override;

        private :

            bool Render();

            CompositorImplementation& _compositor;

            // Local
            Core::CriticalSection _access;
        };

        class TextureRenderer final : public RenderThread {
        public :

            TextureRenderer() = delete;

            TextureRenderer(const TextureRenderer&) = delete;
            TextureRenderer& operator=(const TextureRenderer&) = delete;

            TextureRenderer(TextureRenderer&&) = delete;
            TextureRenderer& operator=(TextureRenderer&&) = delete;

            explicit TextureRenderer(GLResourceMediator::EGL&, GLResourceMediator::GLES&, CompositorImplementation::ClientContainer&);
            virtual ~TextureRenderer();

            void SetClientName(const std::string&);

            uint32_t Worker() override;

        private :

            template <typename T>
            class Element final {
            public :

                Element() = delete;
                Element& operator=(const Element&) = delete;

                Element& operator=(Element&&) = delete;

                explicit Element(const T& element) : _element{element} {}
                ~Element() = default;

                Element(const Element& other) : Element(other._element) {}

                Element(Element&& other) : Element(other._element) {}

                const T& Value() const { return _element; }

            private :

                const T& _element;
            };

            using StringElement = Element<std::string>;

            using function_t = std::function<bool (const StringElement&, const StringElement&)>;

            bool Render();

            // Local
            Core::CriticalSection _access;

            ClientContainer& _clients;

            // Unique elements, to prevent queue from growing beyond N
            std::set<StringElement, function_t> _set;
            // FIFO
            std::queue<StringElement> _queue;
        };

        class Config final : public Core::JSON::Container {
        public :

            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config(Config&&) = delete;
            Config& operator=(Config&&) = delete;

            Config();
            ~Config();

            const Core::JSON::String& Connector() const { return _connector; }
            const Core::JSON::String& Port() const { return _port; }

        private :

            Core::JSON::String _connector;
            Core::JSON::String _port;
        };

        void PlatformReady();

        mutable Core::CriticalSection _adminLock;

        PluginHost::IShell* _service;

        Core::ProxyType<RPC::InvokeServer> _engine;

        ExternalAccess* _externalAccess;

        std::list<INotification*> _observers;

        ClientContainer _clients;

        Config _config;

        GLResourceMediator::ModeSetOpenOnce& _platform;

        DMATransfer* _dma;
        GLResourceMediator::Native _native;
        GLResourceMediator::EGL _egl;
        GLResourceMediator::GLES _gles;

        Core::CriticalSection _clientLock;

        TextureRenderer _textureRenderer;
        SceneRenderer _sceneRenderer;
    };
}
}

    WPEFramework::GLResourceMediator::ModeSetOpenOnce::ModeSetOpenOnce()
        : _open{false}
    {}

    WPEFramework::GLResourceMediator::ModeSetOpenOnce::~ModeSetOpenOnce()
    {
        /* bool */ Close();
    }

    /*static*/ WPEFramework::GLResourceMediator::ModeSetOpenOnce& WPEFramework::GLResourceMediator::ModeSetOpenOnce::Instance()
    {
        static ModeSetOpenOnce set;

        return set;
    }

    bool WPEFramework::GLResourceMediator::ModeSetOpenOnce::Open()
    {
        _token.Lock();

        _open =    _open
                || ModeSet::Open() == Core::ERROR_NONE
                ;

        _token.Unlock();

        return _open;
    }

    bool WPEFramework::GLResourceMediator::ModeSetOpenOnce::Close()
    {
        _token.Lock();

        _open =    !_open
                || !(ModeSet::Close() == Core::ERROR_NONE)
                ;

        _token.Unlock();

        return _open;
    }


    WPEFramework::GLResourceMediator::Native::Prime::Prime(struct gbm_bo* buf)
        : _fd{buf != ModeSet::GBM::InvalidBuffer() ? gbm_bo_get_fd(buf) : InvalidFd()}
        , _sync_fd{InvalidSyncFd()}
        , _buf{buf}
        , _valid{_fd != InvalidFd() && InitializeLocks()}
    {}

    WPEFramework::GLResourceMediator::Native::Prime::~Prime()
    {
        if (_sync_fd != InvalidSyncFd()) {
            // Implicitly releases any locks
            /* int */ close(_sync_fd);
        }
    }

    WPEFramework::GLResourceMediator::Native::Prime::Prime(Prime&& other)
    {
         *this = std::move(other);
    }

    WPEFramework::GLResourceMediator::Native::Prime& WPEFramework::GLResourceMediator::Native::Prime::operator=(Prime&& other)
    {
        if (this != &other) {
            _fd = other._fd;
            _sync_fd = other._sync_fd;

            _buf = other._buf;

            memcpy(&_acquire, &(other._acquire), sizeof(_acquire));
            memcpy(&_release, &(other._release), sizeof(_release));

            _valid = other._valid;

            other._fd = InvalidFd();
            other._sync_fd = InvalidSyncFd();

            other._buf = ModeSet::GBM::InvalidBuffer();

            /* void * */ memset(&(other._acquire), 0, sizeof(other._acquire));
            /* void * */ memset(&(other._release), 0, sizeof(other._release));

            other._valid = false;
        }

        return *this;
    }

    bool WPEFramework::GLResourceMediator::Native::Prime::Lock() const
    {
// FIXME: not signal safe
        bool ret =   _sync_fd != InvalidSyncFd()
                   && fcntl(_sync_fd, F_SETLKW, &_acquire) != -1
                   ;

        return ret;
    }

    bool WPEFramework::GLResourceMediator::Native::Prime::Unlock() const
    {
// FIXME: not signal safe
        bool ret =    _sync_fd != InvalidSyncFd()
                   && fcntl(_sync_fd, F_SETLK, &_release) != -1
                   ;

        return ret;
    }

    bool WPEFramework::GLResourceMediator::Native::Prime::IsValid() const
    {
        return _valid;
    }

    bool WPEFramework::GLResourceMediator::Native::Prime::InitializeLocks()
    {
        constexpr const char SYNC_FD_TEMPLATE[] = "/tmp/Compositor/sync_fdXXXXXX";

        bool result =    _buf != ModeSet::GBM::InvalidBuffer()
                      && _sync_fd == InvalidSyncFd()
                      ;

        // Once is enough
        if (result) {
            static_assert(InvalidSyncFd() == -1);
            _sync_fd = mkostemp(const_cast<char*>(SYNC_FD_TEMPLATE), O_CLOEXEC);

            result = _sync_fd != InvalidSyncFd();

            // Clear all fields since not all may be set
            /* void * */ memset(&_acquire, 0, sizeof(_acquire));
            _acquire.l_type = F_WRLCK;
            _acquire.l_whence = SEEK_SET;
            _acquire.l_start = 0;
            _acquire.l_len = 0;

            // Clear all fields since not all may be set
            /* void * */ memset(&_release, 0, sizeof(_release));
            _release.l_type = F_UNLCK;
            _release.l_whence = SEEK_SET;
            _release.l_start = 0;
            _release.l_len = 0;
        }

        return result;
    }

    WPEFramework::GLResourceMediator::Native::Native(ModeSet& set, uint32_t width, uint32_t height)
        : Native(set)
    {
        _buf = set.CreateBufferObject(width, height);

        if (_buf != ModeSet::GBM::InvalidBuffer()) {
            WPEFramework::GLResourceMediator::Native::Prime prime{_buf};

            _prime = std::move(prime);
        }
    }

    WPEFramework::GLResourceMediator::Native::Native(ModeSet& set)
        : _set{set}
        , _device{ModeSet::GBM::InvalidDevice()}
        , _surf{ModeSet::GBM::InvalidSurface()}
        , _buf{ModeSet::GBM::InvalidBuffer()}
        , _width{0}
        , _height{0}
        , _prime{ModeSet::GBM::InvalidBuffer()}
        , _valid{Initialize()}
    {}

    WPEFramework::GLResourceMediator::Native::~Native()
    {
        /* bool */ Deinitialize();
        _valid = false;
    }

    bool WPEFramework::GLResourceMediator::Native::Invalidate()
    {
        return Deinitialize();
    }

    bool WPEFramework::GLResourceMediator::Native::Initialize()
    {
        constexpr bool const enable = true;

        // An empty string suffices
        bool result =    !_valid
                      && static_cast<ModeSetOpenOnce&>(_set).Open()
                      && NativeDisplay() != InvalidDisplay()
                      ;

        if (result) {
            uint32_t width = _set.Width();
            uint32_t height = _set.Height();

            _surf = _set.CreateRenderTarget(width, height);

            result = _surf != InvalidSurface();

            if (!result) {
                static_assert(!narrowing<uint32_t, unsigned int, enable>::value);
                TRACE(Trace::Error, (_T("Unable to create a compositor surface of dimensions: %u x %u [width, height]))."), width, height));
            }
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::Native::Deinitialize()
    {
        Prime prime(std::move(_prime));

        if (_buf != ModeSet::GBM::InvalidBuffer()) {
            _set.DestroyBufferObject(_buf);
        }

        if (_surf != InvalidSurface()) {
            _set.DestroyRenderTarget(_surf);
        }

        _valid =    _buf != ModeSet::GBM::InvalidBuffer()
                 || _surf != InvalidSurface()
                 ;

        return    _set.Close()
               && _valid
               ;
    }

    WPEFramework::GLResourceMediator::GLES::Opacity::Opacity()
        : Opacity{InitialOpacity()}
    {}

    WPEFramework::GLResourceMediator::GLES::Opacity::Opacity(const Opacity& other)
    {
        *this = other;
    }

    WPEFramework::GLResourceMediator::GLES::Opacity& WPEFramework::GLResourceMediator::GLES::Opacity::operator=(const Opacity& other)
    {
        _alpha = other._alpha;

        return *this;
    }

    GLfloat WPEFramework::GLResourceMediator::GLES::Opacity::Alpha() const
    {
        return _alpha;
    }

    bool WPEFramework::GLResourceMediator::GLES::Opacity::Alpha(GLfloat alpha)
    {
        bool result =    alpha >= Min()
                      && alpha <= Max()
                    ;

        if (result) {
            _alpha = alpha;
        }

        return result;
    }

    WPEFramework::GLResourceMediator::GLES::Opacity::Opacity(GLfloat alpha)
        : _alpha{alpha}
    {
    }

    WPEFramework::GLResourceMediator::GLES::Offset::Offset()
        : Offset(Offset::InitialOffset())
    {
    }

    WPEFramework::GLResourceMediator::GLES::Offset::Offset(const Offset& other)
    {
        *this = other;
    }


    WPEFramework::GLResourceMediator::GLES::Offset& WPEFramework::GLResourceMediator::GLES::Offset::operator=(const Offset& other)
    {
        if (this != &other) {
            _x = other._x;
            _y = other._y;
            _z = other._z;
        }

        return *this;
    }

    bool WPEFramework::GLResourceMediator::GLES::Offset::X(GLfloat x)
    {
        bool result =    x >=  Left()
                      && x <= Right()
                      ;

        if (result) {
            _x = x;
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Offset::Y(GLfloat y)
    {
        bool result =    y >= Bottom()
                      && y <= Top()
                      ;

        if (result) {
            _y = y;
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Offset::Z(GLfloat z)
    {
        bool result =    z >= Near()
                      && z <= Far()
                      ;

        if (result) {
            _z = z;
        }

        return result;
    }

    /*static*/ const WPEFramework::GLResourceMediator::GLES::Offset& WPEFramework::GLResourceMediator::GLES::Offset::InitialOffset()
    {
        static Offset offset(  (Right() + Left()) / 2.0f
                             , (Top() + Bottom()) / 2.0f
                             , (Far() + Near()) / 2.0f
                            );
        return offset;
    }

    WPEFramework::GLResourceMediator::GLES::Offset::Offset(GLfloat x, GLfloat y, GLfloat z)
        : _x{x}
        , _y{y}
        , _z{z}
    {}

    WPEFramework::GLResourceMediator::GLES::Scale::Scale()
        : Scale(InitialScale())
    {
    }

    WPEFramework::GLResourceMediator::GLES::Scale::Scale(const Scale& other)
    {
        *this = other;
    }

    WPEFramework::GLResourceMediator::GLES::Scale& WPEFramework::GLResourceMediator::GLES::Scale::operator=(const Scale& other)
    {
        _horiz = other._horiz;
        _vert = other._vert;

        return *this;
    }

    bool WPEFramework::GLResourceMediator::GLES::Scale::Horizontal(GLfloat value)
    {
        bool result =    value >= Min()
                      && value <= Max()
                      ;

        if (result) {
            _horiz = value;
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Scale::Vertical(GLclampf value)
    {
        bool result =    value >= Min()
                      && value <= Max()
                      ;

        if (result) {
            _vert = value;
        }

        return result;
    }

    /*static*/ const WPEFramework::GLResourceMediator::GLES::Scale& WPEFramework::GLResourceMediator::GLES::Scale::InitialScale()
    {   static Scale scale(Max(), Max());
        return scale;
    }

    WPEFramework::GLResourceMediator::GLES::Scale::Scale(GLclampf horiz, GLclampf vert)
        : _horiz{horiz}
        , _vert{vert}
    {}

    WPEFramework::GLResourceMediator::GLES::Texture::Texture(GLuint target, GLuint width, GLuint height)
        : _tex{InvalidTexture()}
        , _target{target}
        , _opacity{}
        , _x{0}
        , _y{0}
        , _z{0}
        , _width{width}
        , _height{height}
        , _sx{1.0f}
        , _sy{1.0f}
        , _valid{Initialize()}
    {
        ASSERT(   _target == GL_TEXTURE_2D
               || _target == GL_TEXTURE_EXTERNAL_OES
              );
    }

    WPEFramework::GLResourceMediator::GLES::Texture::~Texture()
    {
        if (   IsValid()
            && Unbind()
           ) {
            glDeleteTextures(1, &_tex);
        }
    }

    WPEFramework::GLResourceMediator::GLES::Texture::Texture(WPEFramework::GLResourceMediator::GLES::Texture&& other)
    {
        *this = std::move(other);
    }

    WPEFramework::GLResourceMediator::GLES::Texture& WPEFramework::GLResourceMediator::GLES::Texture::operator=(Texture&& other)
    {
        if (this != &other) {
            _tex = other._tex;
            _target = other._target;
            _opacity = other._opacity;
            _x = other._x;
            _y = other._y;
            _z = other._z;
            _width = other._width;
            _height = other._height;
            _valid = other._valid;

            other._tex = InvalidTexture();
// FIXME: sometimes other is still used in Bind() which results in an error
            other._target = 0;

            other._opacity = Opacity();

            other._x = 0;
            other._y = 0;
            other._z = 0;
            other._width = 0;
            other._height = 0;
            other._valid = false;
        }

        return *this;
    }

    bool WPEFramework::GLResourceMediator::GLES::Texture::X(GLint x)
    {
        bool result =    x >= MIN_X
                      && x <= MAX_X
                      ;

        if (result) {
            _x = x;
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Texture::Y(GLint y)
    {
        bool result =    y >= MIN_Y
                      && y <= MAX_Y
                      ;

        if (result) {
            _y = y;
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Texture::Z(GLint z)
    {
        bool result =    z >= MIN_Z
                      && z <= MAX_Z
                      ;

        if (result) {
            _z = z;
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Texture::Opacity(GLfloat alpha)
    {
        return _opacity.Alpha(alpha);
    }

    bool WPEFramework::GLResourceMediator::GLES::Texture::Properties(GLuint x, GLuint y, GLuint z, GLuint width, GLuint height, GLfloat alpha)
    {
        const GLuint w = Width();
        const GLuint h = Height();

        bool result =    width > 0
                      && height > 0
                      && width <= w
                      && height <= h
                      && X(x)
                      && Y(y)
                      && Z(z)
                      && Opacity(alpha)
                      ;

        if (result) {
            _sx = static_cast<GLdouble>(width) / static_cast<GLdouble>(w);
            _sy = static_cast<GLdouble>(height) / static_cast<GLdouble>(h);
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Texture::Bind()
    {
        bool result = GL_ERROR_WITH_RETURN();

        if (result) {
            // Just an arbitrarily selected texture unit
            glActiveTexture(GL_TEXTURE0);
            result = GL_ERROR_WITH_RETURN();
        }

        if (result) {
            glBindTexture(_target, _tex);
            result = GL_ERROR_WITH_RETURN();
        }

        if (result) {
            glTexParameteri(_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            result = GL_ERROR_WITH_RETURN();
        }

        if (result) {
            glTexParameteri(_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            result = GL_ERROR_WITH_RETURN();
        }

        if (result) {
            glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            result = GL_ERROR_WITH_RETURN();
        }

        if (result) {
            glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            result = GL_ERROR_WITH_RETURN();
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Texture::Unbind()
    {
        bool result = GL_ERROR_WITH_RETURN();

        if (result) {
            glBindTexture(_target, InvalidTexture());
            result = GL_ERROR_WITH_RETURN();
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Texture::Data(const GLvoid* data, bool reuse)
    {
        bool result =    IsValid()
                      && Target() == GL_TEXTURE_2D
                      && Bind()
                      ;

        if (result) {
            if (!reuse) {
                // Allocate full size
                glTexImage2D(Target(), 0, GL_RGBA, Width(), Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            } else {
                // Clear the content
                glTexSubImage2D(Target(), 0, 0, 0, Width(), Height(), GL_RGBA, GL_UNSIGNED_BYTE, data);
            }

            result = GL_ERROR_WITH_RETURN();

            result = Unbind() && result;
        }

        return result;
    }

    template<typename FUNC, typename...ARG>
    bool WPEFramework::GLResourceMediator::GLES::Texture::Data(FUNC&& func, ARG&&... arg)
    {
        return Data(typename std::is_member_pointer<FUNC>::type(), func, std::forward<ARG>(arg)...);
    }

    bool WPEFramework::GLResourceMediator::GLES::Texture::Initialize()
    {
        bool result =    _tex == InvalidTexture()
                      && GL_ERROR_WITH_RETURN()
                      ;

        if (result) {
            glGenTextures(1, &_tex);

            result =    _tex != InvalidTexture()
                     && GL_ERROR_WITH_RETURN()
                     ;
        }

        return result;
    }

    template<typename FUNC, typename... ARG>
    bool WPEFramework::GLResourceMediator::GLES::Texture::Data(std::false_type, FUNC&& func, ARG&&... arg)
    {
        bool result =    IsValid()
                      && Target() == GL_TEXTURE_EXTERNAL_OES
                      && Bind()
                      ;

        if (result) {
            // Assume it is a genuine GL API (member) function
            // -- no return
            // -- glGetError retrieves a set error value
            func(std::forward<ARG>(arg)...);

            result = GL_ERROR_WITH_RETURN();

            result = Unbind() && result;
        }

        return result;
    }

    template<typename FUNC, typename ARG0, typename... ARG>
    bool WPEFramework::GLResourceMediator::GLES::Texture::Data(std::true_type, FUNC&& func, ARG0&& arg0, ARG&&... arg)
    {
        bool result =    IsValid()
                      && Target() == GL_TEXTURE_EXTERNAL_OES
                      && Bind()
                      ;

        if (result) {
            // Assume it is not a genuine GL API (method) call
            // - unknown return value
            (std::forward<ARG0>(arg0).*func)(std::forward<ARG>(arg)...);

            result = Unbind();;
        }

        return result;
    }

    WPEFramework::GLResourceMediator::GLES::GLES(EGL& egl)
        : _scene{}
        , _preallocated{}
        , _token{}
        , _egl{egl}
        , _pglEGLImageTargetTexture2DOES{nullptr}
        , _valid{Initialize()}
    {}

    WPEFramework::GLResourceMediator::GLES::~GLES()
    {
        /* bool */ Deinitialize();
    }

    bool WPEFramework::GLResourceMediator::GLES::RenderColor(bool red, bool green, bool blue, bool alpha) const
    {
        bool result = IsValid();

        if (result) {
            constexpr const uint16_t ROTATION = 360;

            constexpr const GLfloat default_color = 0.0f;

            constexpr const GLfloat OMEGA = 3.14159265f / 180.0f;

            static uint16_t degree = 0;

            GLfloat rad = cos(degree * OMEGA);

            // The function clamps the input to [0.0f, 1.0f]
            /* void */ glClearColor(  red ? rad : default_color
                                    , green ? rad : default_color
                                    , blue ? rad : default_color
                                    , alpha ? default_color : 1.0f
                                   );
            GL_ERROR();

            /* void */ glClear(GL_COLOR_BUFFER_BIT);

            result = GL_ERROR_WITH_RETURN();

            degree = (degree + 1) % ROTATION;
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::RenderSharedBuffer(const Native::Prime& prime, GLuint x, GLuint y, GLuint z, GLuint width, GLuint height, GLuint alpha)
    {
        using common_t = std::common_type<GLuint, EGLint>::type;
        constexpr common_t threshold = static_cast<common_t>(std::numeric_limits<EGLint>::max());

        bool result =    static_cast<common_t>(x) <= threshold
                      && static_cast<common_t>(y) <= threshold
                      && static_cast<common_t>(z) <= threshold
                      && static_cast<common_t>(width) <= threshold
                      && static_cast<common_t>(height) <= threshold
                      && static_cast<common_t>(alpha) <= threshold
                      ;

        if (!result) {
            TRACE(Trace::Error, (_T("Shared buffer target dimenions are out of range.")));
        } else {
            const KHRFIX(EGLImage)& image = _egl.Image(prime);

            result =     prime.IsValid()
                     && _egl.RenderWithoutSwap(std::bind(&GLResourceMediator::GLES::CreateTextureFromEGLImage, &*this, std::cref(image), x, y, z, width, height, alpha))
                     ;
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::RenderScene(GLuint width, GLuint height, std::function<bool (const Texture&, const Texture&)> sortfunc)
    {
        bool result =    IsValid()
                      && width > 0
                      && height > 0
                      ;

        if (result) {
            _token.Lock();

            if (!(_scene.empty())) {
                std::vector<int> indices(_scene.size(), 0);

                for (auto begin = indices.begin(), end = indices.end(), it = begin; it != end; it++) {
                    *it = std::distance(begin, it);
                }

                if (indices.size() > 1) {
                    auto begin = _scene.begin();
                    auto end = _scene.end();

                    bool sorted = true;

                    do {
                        sorted = true;

                        for (std::vector<int>::size_type first = 0, last = (indices.size() - 1), index = first; index < last; index++) {
                            auto left = begin, right = begin;

                            std::advance(left, indices[index]);
                            std::advance(right, indices[index + 1]);

                            if (sortfunc(left->second, right->second)) {
                                sorted &= true;
                            } else {
                                std::swap(indices[index], indices[index + 1]);
                                sorted = false;
                            }
                        }
                    } while (!sorted);
                }

                glBindFramebuffer(GL_FRAMEBUFFER, DefaultFramebuffer());
                GL_ERROR();

                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                GL_ERROR();

                result = status == GL_FRAMEBUFFER_COMPLETE;

                if (result) {
                    glEnable(GL_BLEND);
                    GL_ERROR();

                    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                    GL_ERROR();

                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    GL_ERROR();

                    glEnable(GL_DEPTH_TEST);
                    GL_ERROR();

                    GLint bits = 0;

                    glGetIntegerv(GL_DEPTH_BITS, &bits);
                    GL_ERROR();

                    if (bits > 0) {
                        glDepthMask(GL_TRUE);
                        GL_ERROR();

                        glDepthFunc(GL_LESS);
                        GL_ERROR();

                        // Fully utilize the depth buffer range
                        glDepthRangef(Offset::Near(), Offset::Far());
                        GL_ERROR ();

// FIXME: magic number
                        glClearDepthf(1.0f);
                        GL_ERROR();

                        glClear(GL_DEPTH_BUFFER_BIT);
                        GL_ERROR();
                    }

                    // Start with an empty canvas
#ifdef NDEBUG
                    /* bool */ RenderColor(false, false, false, false);
#else
                    /* bool */ RenderColor(true, true, true, false);
#endif

                    // Draw all textures in sorted order
                    for (std::vector<int>::size_type first = 0, last = indices.size(), index = first; index < last; index++) {
                        auto it = _scene.begin();

                        std::advance(it, indices[index]);

                        Texture& tex = it->second;

                        Scale scale;
                        Offset offset;

                        static_assert(Exchange::IComposition::maxZOrder > Exchange::IComposition::minZOrder);

                        using common_t = std::common_type<GLuint, GLint, GLdouble, GLfloat, uint32_t>::type;
                        static_assert(std::is_floating_point<common_t>::value);

                        const common_t tx = static_cast<common_t>(tex.X()),
                                       ty = static_cast<common_t>(tex.Y()),
                                       tz = static_cast<common_t>(tex.Z()),
                                       // Buffer dimensions [Width, Height]
                                       tw = static_cast<common_t>(tex.Width()),
                                       th = static_cast<common_t>(tex.Height()),
                                       // Geometry drawing rectangle [tx, ty, dw, dh]
                                       dw = tw * static_cast<common_t>(tex.SX()),
                                       dh = th * static_cast<common_t>(tex.SY()),
                                       // Resolution width height defining a rectangle [0, 0, width, height]
                                       w = static_cast<common_t>(width),
                                       h = static_cast<common_t>(height),
                                       dz = static_cast<common_t>(WPEFramework::Exchange::IComposition::maxZOrder - WPEFramework::Exchange::IComposition::minZOrder)
                                       ;

                        result =    tw > 0
                                 && th > 0
                                 && w > 0
                                 && h > 0
                                 && w >= tx
                                 && h >= ty
                                 && dz >= tz
                                 && scale.Horizontal(static_cast<GLclampf>(dw / w))
                                 && scale.Vertical(static_cast<GLclampf>(dh / h))
                                 && offset.X(static_cast<GLfloat>(tx / w))
                                 && offset.Y(static_cast<GLfloat>(ty / h))
                                 && offset.Z(static_cast<GLfloat>(tz / dz))
                                 && tex.Bind()
                                 && SetupViewport(tx, ty, static_cast<GLuint>(dw), static_cast<GLuint>(dh), offset, scale)
                                 && RenderTile(tex.Opacity())
                                 && tex.Unbind()
                                 ;

                        if (!result) {
                            break;
                        }
                    }

                    // Unconditionally
                    glDisable(GL_DEPTH_TEST);
                    GL_ERROR();

                    glDisable(GL_BLEND);
                    GL_ERROR();
                }
            }

            _token.Unlock();
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::SkipSharedBufferFromScene(const Native::Prime& prime)
    {
        bool result = prime.IsValid();

        if (result) {
            const KHRFIX(EGLImage) image = _egl.Image(prime);

            _token.Lock();

            auto it = _scene.find(image);

            if (it != _scene.end()) {

                _preallocated.push(std::move(it->second));

                auto size = _scene.size();

                _scene.erase(it);

                result =    (size - _scene.size()) ==  1
                         && _egl.DestroyImage(prime)
                         ;
            }

            _token.Unlock();
        }

        return result;
    }

    WPEFramework::GLResourceMediator::GLES::Program::Program(const std::string& vtx_src, const std::string& frag_src)
        : _vtx_src{vtx_src}
        , _frag_src{frag_src}
        , _id {InvalidProgram()}
    {}

    WPEFramework::GLResourceMediator::GLES::Program::~Program()
    {
        /* bool */ Destroy();
    }

    bool WPEFramework::GLResourceMediator::GLES::Program::LoadAndUse()
    {
        GLuint vtx_shad = InvalidShader();
        GLuint frag_shad = InvalidShader();

        bool result =    IsValid()
                      || (   CreateShader(GL_VERTEX_SHADER, vtx_shad)
                          && CreateShader(GL_FRAGMENT_SHADER, frag_shad)
                          && CompileAndLink(vtx_shad, frag_shad)
                          // Cleanup shaders
                          && DestroyShader(vtx_shad)
                          && DestroyShader(frag_shad)
                         )
                      ;

        if (result) {
            glUseProgram(_id);

            result = GL_ERROR_WITH_RETURN();
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Program::IsValid() const
    {
        return _id != InvalidProgram();
    }

    bool WPEFramework::GLResourceMediator::GLES::Program::CreateShader(GLenum type, GLuint& shader) const
    {
        bool result =    (   type == GL_VERTEX_SHADER
                          || type == GL_FRAGMENT_SHADER
                         )
                      && (   (   type == GL_VERTEX_SHADER
                              && !_vtx_src.empty()
                             )
                          || (   type == GL_FRAGMENT_SHADER
                              && !_frag_src.empty()
                             )
                         )
                      && GL_ERROR_WITH_RETURN()
                      ;

        if (result) {
            const GLchar* code = type == GL_VERTEX_SHADER ? _vtx_src.data() : _frag_src.data();

            shader = glCreateShader(type);
            GL_ERROR();

            glShaderSource(shader, 1, &code, nullptr);
            GL_ERROR();

            glCompileShader (shader);
            GL_ERROR();

            GLint status = GL_FALSE;

            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

            result =    GL_ERROR_WITH_RETURN()
                     && status != GL_FALSE
                     ;

#ifndef NDEBUG
            if (result) {
                GLint size = 0;

                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
                GL_ERROR();

                GLchar info[size];
                GLsizei length = 0;

                glGetShaderInfoLog(shader, static_cast<GLsizei>(size), &length, &info[0]);
                GL_ERROR();

                info[size] = '\0';

                TRACE_GLOBAL(Trace::Error, (_T("Error: shader log: %s"), static_cast<char*>(&info[0])));
            }
#endif
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Program::DestroyShader(GLuint shader) const
    {
        bool result =    shader != InvalidShader()
                      && GL_ERROR_WITH_RETURN()
                      && IsValid()
                      ;

        if (result) {
            glDetachShader(_id, shader);
            GL_ERROR();

            glDeleteShader(shader);

            result = GL_ERROR_WITH_RETURN();
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Program::CompileAndLink(GLuint vtx_shad, GLuint frag_shad)
    {
        bool result =    GL_ERROR_WITH_RETURN()
                      && vtx_shad != InvalidShader()
                      && frag_shad != InvalidShader()
                      ;

        if (result) {
            _id = glCreateProgram();
            GL_ERROR();

            glAttachShader(_id, vtx_shad);
            GL_ERROR();

            glAttachShader(_id, frag_shad);
            GL_ERROR();

            glBindAttribLocation(_id, 0, "position");
            GL_ERROR();

            glLinkProgram(_id);
            GL_ERROR();

            GLint status = GL_FALSE;

            glGetProgramiv(_id, GL_LINK_STATUS, &status);
            GL_ERROR();

            result =    GL_ERROR_WITH_RETURN()
                     && status != GL_FALSE
                     ;

#ifndef NDEBUG
            if (result) {
                GLint size = 0;

                glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &size);
                GL_ERROR();

                GLchar info[size];
                GLsizei length = 0;

                glGetProgramInfoLog(_id, static_cast<GLsizei>(size), &length, &info[0]);
                GL_ERROR();

                info[size] = '\0';

                TRACE_GLOBAL(Trace::Error, (_T("Error: program log: %s"), static_cast<char*>(&info[0])));
            }
#endif
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Program::Destroy()
    {
        bool result =    GL_ERROR_WITH_RETURN()
                      && IsValid()
                      ;

        if (result) {
            GLint count = 0;

            glGetProgramiv(_id, GL_ATTACHED_SHADERS, &count);
            GL_ERROR();

            if (count > 0) {
                GLuint shaders[count];

                glGetAttachedShaders(_id, count, static_cast<GLsizei*>(&count), &shaders[0]);
                GL_ERROR();

                for (; count > 0 ; --count) {
                    glDetachShader(_id, shaders[count]);
                    GL_ERROR();

                    glDeleteShader(shaders[count]);
                    GL_ERROR();
                }
            }

            glDeleteProgram(_id);
            GL_ERROR();

            _id = InvalidProgram();

            result = !IsValid();
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Initialize()
    {
        _pglEGLImageTargetTexture2DOES = reinterpret_cast<void (*)(GLenum, GLeglImageOES)>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

        bool result =    _pglEGLImageTargetTexture2DOES
                      // This may fail if the client is not yet complete
                      && Supported("GL_OES_EGL_image")
                      && Supported("GL_OES_EGL_image_external")
                      ;

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Deinitialize()
    {
        bool result = GL_ERROR_WITH_RETURN();

        _token.Lock();

        _scene.clear();

        while (_preallocated.size() > 0) {
            _preallocated.pop();
        }

        _token.Unlock();

        _pglEGLImageTargetTexture2DOES = nullptr;

        _valid = false;

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::Supported(const std::string& name) const
    {
        static_assert(std::is_convertible<GLubyte, unsigned char>::value);
        static_assert(std::is_convertible<std::string::value_type, unsigned char>::value);

        const GLubyte* str = nullptr;

        // A valid GL context should exist
        bool result = _egl.Bind();

        std::string ext;

        if (result) {
            str = glGetString(GL_EXTENSIONS);

            if ((   GL_ERROR_WITH_RETURN()
                 && str != nullptr
                )
                && _egl.Unbind()
               ) {
                std::basic_string<unsigned char> uext = str;
                ext  = reinterpret_cast<const char*>(uext.data());
            }

            result =    !ext.empty()
                     && name.size() > 0
                     && ext.find(name) != std::string::npos
                     ;
        }

         return result;
     }

    bool WPEFramework::GLResourceMediator::GLES::CreateTextureFromEGLImage(const KHRFIX(EGLImage)& image, EGLint x, EGLint y, EGLint z, EGLint width, EGLint height, EGLint alpha)
    {
        bool result =    IsValid()
                      && image != EGL::InvalidImage()
                      && x >= 0
                      && y >= 0
                      && z >= 0
                      && width > 0
                      && height > 0
                      && alpha >= 0
                      && GL_ERROR_WITH_RETURN()
                     ;

        if (result) {
            // Set up the required textures
            static_assert(in_unsigned_range<GLuint, std::numeric_limits<EGLint>::max()>::value);
            using common_t = std::common_type<GLuint, EGLint>::type;

            using opacity_t = std::common_type<EGLint, uint32_t /*IComposition::maxOpacity*/ , GLfloat>::type;
            static_assert(std::is_floating_point<opacity_t>::value);

            opacity_t opacity = static_cast<opacity_t>(alpha) / (static_cast<opacity_t>(WPEFramework::Exchange::IComposition::maxOpacity) - static_cast<opacity_t>(WPEFramework::Exchange::IComposition::minOpacity));

            result =    opacity >= std::numeric_limits<GLfloat>::min()
                     && opacity <= std::numeric_limits<GLfloat>::max()
                     ;

            if (result) {
                // Try to reuse the existing scene texture

                _token.Lock();

                bool allocate = true;

                auto it = _scene.find(image);

                if (it != _scene.end()) {
                    Texture& tex_fbo = it->second;

                    // The 'shared' texture
                    Texture tex_oes(GL_TEXTURE_EXTERNAL_OES, tex_fbo.Width(), tex_fbo.Height());

                    result =    tex_oes.Data(std::bind(_pglEGLImageTargetTexture2DOES, tex_oes.Target(), reinterpret_cast<GLeglImageOES>(const_cast<KHRFIX(EGLImage)>(image))))
                             && tex_fbo.Data(static_cast<const GLvoid*>(nullptr), false)
                             && tex_fbo.Properties(x, y, z, width, height, static_cast<GLfloat>(opacity))
                             && RenderSharedTextureToFBOTexture(tex_oes, tex_fbo)
                             ;
                } else {
                    // Try to reuse a skipped scene texture

                    if (_preallocated.size() > 0) {
                        auto it = _scene.emplace(  std::piecewise_construct
                                                 , std::forward_as_tuple(image)
                                                 , std::forward_as_tuple(std::move(_preallocated.top()))
                                                );

                        if (it.second) {
                            Texture& tex_fbo = it.first->second;

                            _preallocated.pop();

                            // The 'shared' texture
                            Texture tex_oes(GL_TEXTURE_EXTERNAL_OES, tex_fbo.Width(), tex_fbo.Height());

                            result =    tex_oes.Data(std::bind(_pglEGLImageTargetTexture2DOES, tex_oes.Target(), reinterpret_cast<GLeglImageOES>(const_cast<KHRFIX(EGLImage)>(image))))
                                     && tex_fbo.Data(static_cast<const GLvoid*>(nullptr), false)
                                     && tex_fbo.Properties(x, y, z, width, height, static_cast<GLfloat>(opacity))
                                     && RenderSharedTextureToFBOTexture(tex_oes, tex_fbo)
                                     ;

                        }
                    } else {
                        // Just allocate a new texture

                        auto it = _scene.emplace(  std::piecewise_construct
                                                 , std::forward_as_tuple(image)
                                                 , std::forward_as_tuple(Texture(GL_TEXTURE_2D, width, height))
                                                );

                        if (it.second) {
                            Texture& tex_fbo = it.first->second;

                            // The 'shared' texture
                            Texture tex_oes(GL_TEXTURE_EXTERNAL_OES, width, height);

                            result =    tex_oes.Data(std::bind(_pglEGLImageTargetTexture2DOES, tex_oes.Target(), reinterpret_cast<GLeglImageOES>(const_cast<KHRFIX(EGLImage)>(image))))
                                     && tex_fbo.Data(static_cast<const GLvoid*>(nullptr), false)
                                     && tex_fbo.Properties(x, y, z, width, height, static_cast<GLfloat>(opacity))
                                     && RenderSharedTextureToFBOTexture(tex_oes, tex_fbo)
                                     ;
                        }
                    }
                }

                _token.Unlock();
            }
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::RenderSharedTextureToFBOTexture(Texture& tex_oes, Texture& tex_fbo)
    {
        bool result =    GL_ERROR_WITH_RETURN()
                      && tex_oes.Bind()
                      && tex_fbo.Bind()
                      ;

        if (result) {
            GLuint fbo; /* unknown proper initialization value */

            glGenFramebuffers(1, &fbo);
            GL_ERROR();

            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            GL_ERROR();

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_fbo.Handle(), 0);
            GL_ERROR();

            glDisable(GL_DEPTH_TEST);
            GL_ERROR();

            glDisable(GL_BLEND);
            GL_ERROR();

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

            result =    GL_ERROR_WITH_RETURN()
                     && status == GL_FRAMEBUFFER_COMPLETE
                     && SetupViewport(0, 0, tex_fbo.Width(), tex_fbo.Height(), Offset::InitialOffset(), Scale::InitialScale())
                     && RenderTileOES(tex_fbo.Opacity())
                     ;

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture::InvalidTexture(), 0);
            GL_ERROR();

            glDeleteFramebuffers(1, &fbo);
            GL_ERROR();

            /* bool */ tex_oes.Unbind();
            /* bool */ tex_fbo.Unbind();
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::RenderTileOES(const Opacity& opacity) const
    {
        constexpr const char vtx_src[] =
            "#version 100                             \n"
            "attribute vec3 position;                 \n"
            "varying vec2 coordinates;                \n"
            "void main() {                            \n"
                "gl_Position = vec4(position.xyz, 1); \n"
                "coordinates = position.xy;           \n"
            "}                                        \n"
        ;

        constexpr const char frag_src[] =
            "#version 100                                                           \n"
            "#extension GL_OES_EGL_image_external : require                         \n"
            "precision mediump float;                                               \n"
            "uniform samplerExternalOES sampler;                                    \n"
            "uniform float opacity;                                                 \n"
            "varying vec2 coordinates;                                              \n"
            "void main() {                                                          \n"
                "gl_FragColor = vec4(texture2D(sampler, coordinates).rgb, opacity); \n"
            "}                                                                      \n"
        ;

        const std::array<GLfloat, 4 * VerticeDimensions> vert = {
            0.0f, 0.0f, 0.0f /* v0 */,
            1.0f, 0.0f, 0.0f /* v1 */,
            0.0f, 1.0f, 0.0f /* v2 */,
            1.0f, 1.0f, 0.0f /* v3 */
        };

        // Re-use program */
        static Program prog{std::string(vtx_src), std::string(frag_src)};

        bool result = GL_ERROR_WITH_RETURN();

        if (result) {
            glDisable(GL_BLEND);
            GL_ERROR();

            result =    RenderColor(false, false, false, true)
                     && prog.LoadAndUse()
                     && RenderPolygon(vert, opacity)
                     ;
        }

        return result;
     }

    bool WPEFramework::GLResourceMediator::GLES::RenderTile(const Opacity& opacity) const
    {
        constexpr const char vtx_src[] =
            "#version 100                             \n"
            "attribute vec3 position;                 \n"
            "varying vec2 coordinates;                \n"
            "void main() {                            \n"
                "gl_Position = vec4(position.xyz, 1); \n"
                "coordinates = position.xy;           \n"
            "}                                        \n"
        ;

        constexpr const char frag_src[] =
            "#version 100                                                   \n"
            "precision mediump float;                                       \n"
            "uniform sampler2D sampler;                                     \n"
            // Required by RenderPolygon
            "uniform float opacity;                                         \n"
            "varying vec2 coordinates;                                      \n"
            "void main() {                                                  \n"
                "gl_FragColor = vec4(texture2D(sampler, coordinates).rgba); \n"
            "}                                                              \n"
        ;

        const std::array<GLfloat, 4 * VerticeDimensions> vert = {
            0.0f, 0.0f, 0.0f /* v0 */,
            1.0f, 0.0f, 0.0f /* v1 */,
            0.0f, 1.0f, 0.0f /* v2 */,
            1.0f, 1.0f, 0.0f /* v3 */
        };

        // Re-use program */
        static Program prog{std::string(vtx_src), std::string(frag_src)};

        bool result = GL_ERROR_WITH_RETURN();

        if (result) {
            glDisable(GL_BLEND);
            GL_ERROR();

            result =    prog.LoadAndUse()
                     && RenderPolygon(vert, opacity)
                     ;
        }

        return result;
    }

    template<size_t N>
    bool WPEFramework::GLResourceMediator::GLES::RenderPolygon(const std::array<GLfloat, N>& vert, const Opacity&opacity) const
    {
        GLuint prog = Program::InvalidProgram();

        bool result = GL_ERROR_WITH_RETURN();

        if (result) {
            glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&prog));
            GL_ERROR();

            result = prog != Program::InvalidProgram();
        }

        if (result) {
            GLint loc_vert = 0, loc_op = 0;

            loc_op = glGetUniformLocation(prog, "opacity");
            GL_ERROR();

            glUniform1f(loc_op, opacity.Alpha());
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
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::GLES::SetupViewport(GLuint x, GLuint y, GLuint width, GLuint height, const Offset& offset, const Scale& scale)
    {
        silence(offset);
        silence(scale);

        bool result = GL_ERROR_WITH_RETURN();

        if (result) {
            GLint dims [2] = {0, 0};

            glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &dims[0]);
            GL_ERROR();

            using common_t = std::common_type<uint8_t, GLuint, GLint, GLsizei, GLdouble, GLfloat>::type;
            static_assert(std::is_signed<common_t>::value);

            common_t quirk_x = static_cast<common_t>(x);
            common_t quirk_y = static_cast<common_t>(y);
            common_t quirk_width = static_cast<common_t>(width);
            common_t quirk_height = static_cast<common_t>(height);

#define _QUIRKS
#ifdef _QUIRKS
            constexpr uint8_t const mult = 2;

            quirk_width *= static_cast<common_t>(mult);
            quirk_height *= static_cast<common_t>(mult);

            quirk_x -= static_cast<common_t>(width);
            quirk_y -= static_cast<common_t>(height);
#endif

            result =    static_cast<common_t>(std::numeric_limits<GLint>::lowest()) <= quirk_x
                     && quirk_x <= static_cast<common_t>(std::numeric_limits<GLint>::max())
                     && static_cast<common_t>(std::numeric_limits<GLint>::lowest()) <= quirk_y
                     && quirk_y <= static_cast<common_t>(std::numeric_limits<GLint>::max())
                     && quirk_width <= static_cast<common_t>(std::numeric_limits<GLsizei>::max())
                     && quirk_height <= static_cast<common_t>(std::numeric_limits<GLsizei>::max())
                     && quirk_width <= static_cast<common_t>(dims[0])
                     && quirk_height <= static_cast<common_t>(dims[1])
                     ;

            if (result) {
                glViewport(static_cast<GLint>(quirk_x), static_cast<GLint>(quirk_y), static_cast<GLsizei>(quirk_width), static_cast<GLsizei>(quirk_height));
                GL_ERROR();
            }
        }

        return result;
    }

    WPEFramework::GLResourceMediator::EGL::Sync::Sync(const EGL& egl)
        : _dpy{egl.Display()}
        , _supported{egl.IsValid()}
        , _sync{  _supported
                ? KHRFIX(eglCreateSync)(_dpy, _EGL_SYNC_FENCE, nullptr)
                : InvalidSync()
               }
    {}

    WPEFramework::GLResourceMediator::EGL::Sync::~Sync()
    {
        if (_sync == InvalidSync()) {
            // Error : unable to create sync object
            glFinish();
        } else {
            glFlush();

            EGLint val = static_cast<EGLint>(KHRFIX(eglClientWaitSync)(_dpy, _sync, _EGL_SYNC_FLUSH_COMMANDS_BIT, _EGL_FOREVER));

            static_assert(   !narrowing<decltype(EGL_FALSE), EGLint, true>::value
                          || in_signed_range<EGLint, EGL_FALSE>::value
                         );
            static_assert(   !narrowing<decltype(_EGL_CONDITION_SATISFIED), EGLint, true>::value
                          || in_signed_range<EGLint, _EGL_CONDITION_SATISFIED>::value
                         );
            if (   val == static_cast<EGLint>(EGL_FALSE)
                || val != static_cast<EGLint>(_EGL_CONDITION_SATISFIED)
               ) {
                EGLAttrib status;

                bool result =    KHRFIX(eglGetSyncAttrib)(_dpy, _sync, _EGL_SYNC_STATUS, &status) != EGL_FALSE
                              && status == _EGL_SIGNALED
                              ;

                if (!result) {
                    TRACE(Trace::Error, (_T("EGL: synchronization primitive failed")));
                }
             }

            /* EGLBoolean */ val = static_cast<EGLint>(KHRFIX(eglDestroySync)(_dpy, _sync));

            if (val != EGL_TRUE) {
                 // Error
            }

            // Consume the (possible) errors
            /* ELGint */ glGetError();
            /* ELGint */ eglGetError();
        }
    }

    WPEFramework::GLResourceMediator::EGL::EGL(Native& native)
        : _native{native}
        , _dpy{InvalidDisplay()}
        , _cfg{InvalidConfig()}
        , _ctx{InvalidContext()}
        , _draw{InvalidSurface()}
        , _read{InvalidSurface()}
        , _images{}
        , _peglCreateImage{nullptr}
        , _peglDestroyImage{nullptr}
        , _peglQueryDmaBufFormatsEXT{nullptr}
        , _peglQueryDmaBufModifiersEXT{nullptr}
        , _valid{Initialize()}
    {}

    WPEFramework::GLResourceMediator::EGL::~EGL()
    {
        /* bool */ DeInitialize();
    }

    bool WPEFramework::GLResourceMediator::EGL::CreateImage(const Native::Prime& prime)
    {
        constexpr const bool enable = true;

        bool result =   Image(prime) == InvalidImage()
                      && IsValid()
                      ;

        if (result) {
            EGLint count = 0;

            // Function addresses are already been resolved and valid
            result = _peglQueryDmaBufFormatsEXT(_dpy, 0, nullptr, &count) != EGL_FALSE;

            std::vector<EGLint> formats(count, 0);

            result =     result
                     && _peglQueryDmaBufFormatsEXT(_dpy, count, formats.data(), &count) != EGL_FALSE;

            // Format should be listed as supported
            if (result) {
                auto it = std::find(formats.begin(), formats.end(), prime.Format());

                result = it != formats.end();
            }

            result =    result
                     && _peglQueryDmaBufModifiersEXT(_dpy, prime.Format(), 0, nullptr, nullptr, &count) != EGL_FALSE;

            std::vector<EGLuint64KHR> modifiers(count, 0);
            std::vector<EGLBoolean> external(count, EGL_FALSE);

            // External is required for GL_TEXTURE_EXTERNAL_OES
            result =    result
                     && _peglQueryDmaBufModifiersEXT(_dpy, prime.Format(), count, modifiers.data(), external.data(), &count) != EGL_FALSE;

            if (result) {
                static_assert(!narrowing<uint64_t, EGLuint64KHR, enable>::value);
                auto it = std::find(modifiers.begin(), modifiers.end(), static_cast<EGLuint64KHR>(prime.Modifier()));

                result =    it != modifiers.end ()
                         && !external[std::distance(modifiers.begin(), it)]
                         ;
            }

            if (result) {
                static_assert(in_signed_range<EGLAttrib, EGL_WIDTH>::value);
                static_assert(in_signed_range<EGLAttrib, EGL_HEIGHT>::value);
                static_assert(in_signed_range<EGLAttrib, EGL_LINUX_DRM_FOURCC_EXT>::value);
                static_assert(in_signed_range<EGLAttrib, EGL_DMA_BUF_PLANE0_FD_EXT>::value);
                static_assert(in_signed_range<EGLAttrib, EGL_DMA_BUF_PLANE0_OFFSET_EXT>::value);
                static_assert(in_signed_range<EGLAttrib, EGL_DMA_BUF_PLANE0_PITCH_EXT>::value);
                static_assert(in_signed_range<EGLAttrib, EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT>::value);
                static_assert(in_signed_range<EGLAttrib, EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT>::value);
                static_assert(in_signed_range<EGLAttrib, EGL_TRUE>::value);
                static_assert(in_signed_range<EGLAttrib, EGL_NONE>::value);

                // 64 bit hence the shift 32 and mask 0xFFFFFFF, each half equals 32 bits
                static_assert(sizeof(EGLuint64KHR) == static_cast<size_t>(8));

                const EGLAttrib attrs[] = {
                    EGL_WIDTH, static_cast<EGLAttrib>(prime.Width()),
                    EGL_HEIGHT, static_cast<EGLAttrib>(prime.Height()),
                    EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLAttrib>(prime.Format()),
                    EGL_DMA_BUF_PLANE0_FD_EXT, static_cast<EGLAttrib>(prime.Fd()),
// FIXME: magic constant
                    EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
                    EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLAttrib>(prime.Stride()),
                    EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, static_cast<EGLAttrib>(static_cast<EGLuint64KHR>(prime.Modifier()) & 0xFFFFFFFF),
                    EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, static_cast<EGLAttrib>(static_cast<EGLuint64KHR>(prime.Modifier()) >> 32),
//                    EGL_IMAGE_PRESERVED_KHR, static_cast<EGLAttrib>(EGL_TRUE),
                    EGL_NONE
                };

                static_assert(std::is_convertible<std::nullptr_t, EGLClientBuffer>::value);
                KHRFIX(EGLImage) image = _peglCreateImage(_dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attrs);

                result =    image != WPEFramework::GLResourceMediator::EGL::InvalidImage()
                         && std::get<1>(_images.insert(std::make_pair(std::make_tuple<int, int>(prime.Fd(), prime.SyncFd()), image)))
                         ;
            }
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::EGL::DestroyImage(const Native::Prime& prime)
    {
        bool result =    prime.IsValid()
                      && IsValid()
                      ;

        if (result) {
            const int fd = prime.Fd();
            const int sync_fd = prime.SyncFd();

            for (auto begin = _images.begin(), end = _images.end(), it = begin; it != end; it++) {
                std::pair<std::tuple<int, int>, KHRFIX(EGLImage)> pair = *it;

                std::tuple<int, int> tuple = std::get<0>(pair);

                result =    fd == std::get<0>(tuple)
                         && sync_fd == std::get<1>(tuple)
                         ;

                if (result) {
                    _images.erase(it);
                    break;
                }
            }
        }

        return result;
    }

    KHRFIX(EGLImage) WPEFramework::GLResourceMediator::EGL::Image(const Native::Prime& prime) const
    {
        KHRFIX(EGLImage) image = InvalidImage();

        bool result =    prime.IsValid()
                      && IsValid()
                      ;

        if (result) {
            const int fd = prime.Fd();
            const int sync_fd = prime.SyncFd();

            for (auto begin = _images.begin(), end = _images.end(), it = begin; it != end; it++) {
                std::pair<std::tuple<int,int>, KHRFIX(EGLImage)> pair = *it;

                std::tuple<int, int> tuple = std::get<0>(pair);

                result =    fd == std::get<0>(tuple)
                         && sync_fd == std::get<1>(tuple)
                         ;

                if (result) {
                    image = std::get<1>(pair);
                    break;
                }
            }
        }

        return image;
    }

    bool WPEFramework::GLResourceMediator::EGL::Bind() const
    {
        bool result = IsValid();

        if (result) {
            { Sync sync(*this); }

            result = eglMakeCurrent(_dpy, _draw, _read, _ctx) != EGL_FALSE;
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::EGL::Unbind() const
    {
        bool result = IsValid();

        if (result) {
            { Sync sync(*this); }

            result = eglMakeCurrent(_dpy, InvalidSurface(), InvalidSurface(), InvalidContext()); 
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::EGL::Invalidate()
    {
        return DeInitialize();
    }

    bool WPEFramework::GLResourceMediator::EGL::Render() const
    {
        bool result =    IsValid()
                      && Bind()
                      && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE
                     ;

        if (result) {
            { Sync sync(*this); }

            result =    eglSwapBuffers(Display(), ReadSurface()) != EGL_FALSE
                     && Unbind()
                     ;

            /* EGLBoolean */ eglMakeCurrent(Display(), InvalidSurface(), InvalidSurface(), InvalidContext());
        }

        return result;
    }

    template<typename FUNC, typename... ARG>
    bool WPEFramework::GLResourceMediator::EGL::Render(FUNC&& func, bool post, ARG&&... arg) const
    {
        return Render(typename std::is_member_pointer<FUNC>::type(), func, post, std::forward<ARG>(arg)...);
    }

    template<typename FUNC, typename... ARG>
    bool WPEFramework::GLResourceMediator::EGL::RenderWithoutSwap(FUNC&& func, ARG&&... arg) const
    {
        return RenderWithoutSwap(typename std::is_member_pointer<FUNC>::type(), func, std::forward<ARG>(arg)...);
    }

    bool WPEFramework::GLResourceMediator::EGL::Supported(const std::string& name) const
    {
        bool ret = false;

#ifdef EGL_VERSION_1_5
        // KHR extentions that have become part of the standard
        ret =    name.find("EGL_KHR_fence_sync") != std::string::npos
              || name.find("EGL_KHR_image") != std::string::npos
              || name.find("EGL_KHR_image_base") != std::string::npos
              ;
#endif

        if (!ret) {
            const char* str = eglQueryString(_dpy, EGL_EXTENSIONS);

            std::string ext;

            if (str != nullptr) {
                ext = str;
            }

            ret =    !ext.empty()
                  && name.size() > 0
                  && ext.find(name) != std::string::npos
                  ;
        }

        return ret;
    }

    template<typename FUNC, typename... ARG>
    bool WPEFramework::GLResourceMediator::EGL::Render(std::false_type, FUNC&& func, bool post, ARG&&... arg) const
    {
        bool result =    IsValid()
                      && Bind()
                      && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE
                      ;

        if (result) {
            if (post) {
                result = func(std::forward<ARG>(arg)...);

                { Sync sync(*this); }

                result =    result
                         && eglSwapBuffers(Display(), ReadSurface()) != EGL_FALSE
                         && Unbind()
                         ;
            } else {
                result =    eglSwapBuffers(Display(), ReadSurface()) != EGL_FALSE
                         && func(std::forward<ARG>(arg)...)
                         && Unbind()
                         ;
            }
        }

        return result;
    }

    template<typename FUNC, typename ARG0, typename... ARG>
    bool WPEFramework::GLResourceMediator::EGL::Render(std::true_type, FUNC&& func, bool post, ARG0&& arg0, ARG&&... arg) const
    {
        bool result =    IsValid()
                      && Bind()
                     && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE
                     ;

        if (result) {
            if (post) {
                result = (std::forward<ARG0>(arg0).*func)(std::forward<ARG>(arg)...);

                { Sync sync(*this); }

                result =    result
                         && eglSwapBuffers(Display(), ReadSurface()) != EGL_FALSE
                         && Unbind()
                         ;
            } else {
                result =     result
                         &&  eglSwapBuffers ( Display() , ReadSurface() ) != EGL_FALSE
                         && (std::forward<ARG0>(arg0).*func)(std::forward<ARG>(arg)...)
                         && Unbind()
                         ;
            }
        }

        return result;
    }

    template<typename FUNC, typename... ARG>
    bool WPEFramework::GLResourceMediator::EGL::Render(std::false_type, FUNC&& prefunc, FUNC&& postfunc, ARG&&... arg) const
    {
        bool result =    IsValid()
                      && Bind()
                      && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE
                      && prefunc(std::forward<ARG>(arg)...)
                      ;

        { Sync sync(*this); }

        result =    result
                 && eglSwapBuffers(Display(), ReadSurface()) != EGL_FALSE
                 && postfunc(std::forward<ARG>(arg)...)
                 && Unbind()
                 ;

        return result;
    }

    template<typename FUNC, typename ARG0, typename... ARG>
    bool WPEFramework::GLResourceMediator::EGL::Render(std::true_type, FUNC&& prefunc, FUNC&& postfunc, ARG0&& arg0, ARG&&... arg) const
    {
        bool result =    IsValid()
                      && Bind()
                      && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE
                      && (std::forward<ARG0>(arg0).*prefunc)(std::forward<ARG>(arg)...)
                      ;

        { Sync sync(*this); }

        result =    result
                 && eglSwapBuffers(Display(), ReadSurface()) != EGL_FALSE
                 && (std::forward<ARG0>(arg0).*postfunc)(std::forward<ARG>(arg)...)
                 && Unbind()
                 ;

        return result;
    }

    template<typename FUNC, typename... ARG>
    bool WPEFramework::GLResourceMediator::EGL::RenderWithoutSwap(std::false_type, FUNC&& func, ARG&&... arg) const
    {
        bool result =    IsValid()
                      && Bind()
                      && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE
                      && func(std::forward<ARG>(arg)...)
                      && Unbind()
                      ;

        return result;
    }

    template<typename FUNC, typename ARG0, typename... ARG>
    bool WPEFramework::GLResourceMediator::EGL::RenderWithoutSwap(std::true_type, FUNC&& func, ARG0&& arg0, ARG&&... arg) const
    {
        bool result =    IsValid()
                      && Bind()
                      && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE
                      && (std::forward<ARG0>(arg0).*func)(std::forward<ARG>(arg)...)
                      && Unbind()
                      ;

        return result;
    }

    bool WPEFramework::GLResourceMediator::EGL::Configure()
    {
        constexpr const bool enable = true;

        bool result =    _native.IsValid()
                      && !IsValid()
                      ;

        if (result) {
            const int step = 0;

            switch (step) {
            case 0  :   {
                            _dpy = eglGetDisplay(_native.NativeDisplay());

                            EGLint major = 0, minor = 0;

                            result =    _dpy != InvalidDisplay()
                                     && eglInitialize(_dpy, &major, &minor) != EGL_FALSE
                                     ;

                            if (!result) {
                                break;
                            }

                            static_assert(!narrowing<EGLint, int, enable>::value);
                            TRACE(Trace::Information, (_T("EGL version : %d.%d"), major, minor));
                        }
                        __attribute__((fallthrough));
            case 1  :   {
                            constexpr const EGLint attr[] = {
                                EGL_SURFACE_TYPE   , EGL_WINDOW_BIT,
                                EGL_RED_SIZE       , RedBufferSize(),
                                EGL_GREEN_SIZE     , GreenBufferSize(),
                                EGL_BLUE_SIZE      , BlueBufferSize(),
                                EGL_ALPHA_SIZE     , AlphaBufferSize(),
                                EGL_BUFFER_SIZE    , RedBufferSize() + GreenBufferSize() + BlueBufferSize() + AlphaBufferSize(),
                                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                                EGL_DEPTH_SIZE     , DepthBufferSize(),
                                EGL_NONE
                            };

                            EGLint count = 0;

                            if (eglGetConfigs(_dpy, nullptr, 0, &count) != EGL_TRUE) {
                                count = 0;
                            }

                            std::vector<EGLConfig> configs(count, InvalidContext());

                            result =   count >= 1
                                    && eglChooseConfig(_dpy, &attr[0], configs.data(), configs.size(), &count) != EGL_FALSE
                                    && configs[0] != InvalidConfig()
                                    ;

                            if (!result) {
                                break;
                            }

                            _cfg = configs[0];
                        }
                        __attribute__((fallthrough));
            case 2  :   {
                            result =   eglQueryAPI() == EGL_OPENGL_ES_API
                                    && eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE
                                    && eglGetError() == EGL_SUCCESS
                                    ;

                            if (!result) {
                                break;
                            }
                        }
                        __attribute__((fallthrough));
            case 3  :   {
                            static_assert(in_signed_range<EGLint, GLES::MajorVersion()>::value);

                            constexpr const EGLint attr[] = {
                                EGL_CONTEXT_CLIENT_VERSION, static_cast<EGLint>(GLES::MajorVersion()),
                                EGL_NONE
                            };

                            _ctx = eglCreateContext(_dpy, _cfg, EGL_NO_CONTEXT, attr);

                            result = _ctx != EGL_NO_CONTEXT;

                            if (!result) {
                                break;
                            }
                        }
                        __attribute__((fallthrough));
            case 4  :   {
                            constexpr const EGLint attr[] = {
                                EGL_NONE
                            };

                            _draw = eglCreateWindowSurface(_dpy, _cfg, _native.NativeSurface(), &attr[0]);

                            result = _draw != InvalidSurface();

                            if (!result) {
                                break;
                            }

                            _read = _draw;
                        }
                        __attribute__((fallthrough));
            default :   ;
            }
        }

        if (!result) {
            /* bool */ DeInitialize();
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::EGL::Initialize()
    {
        bool result =    Configure()
                      && eglMakeCurrent(_dpy, _draw, _read, _ctx) != EGL_FALSE
                      && UpdateCurrents()
                      && eglMakeCurrent(_dpy, InvalidSurface(), InvalidSurface(), InvalidContext()) != EGL_FALSE
                      && Supported("EGL_KHR_fence_sync")
                      && Supported("EGL_KHR_image")
                      && Supported("EGL_KHR_image_base")
                      && Supported("EGL_EXT_image_dma_buf_import")
                      && Supported("EGL_EXT_image_dma_buf_import_modifiers")
                      ;

        if (result) {
            _peglCreateImage = reinterpret_cast<KHRFIX(EGLImage)(*)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLAttrib*)>(eglGetProcAddress(XSTRINGIFY(KHRFIX(eglCreateImage))));
            _peglDestroyImage = reinterpret_cast<EGLBoolean (*)(EGLDisplay, KHRFIX(EGLImage))>(eglGetProcAddress(XSTRINGIFY(KHRFIX(eglDestroyImage))));
            _peglQueryDmaBufFormatsEXT = reinterpret_cast<EGLBoolean (*)(EGLDisplay, EGLint, EGLint*, EGLint*)>(eglGetProcAddress("eglQueryDmaBufFormatsEXT"));
            _peglQueryDmaBufModifiersEXT = reinterpret_cast<EGLBoolean (*)(EGLDisplay, EGLint, EGLint, EGLuint64KHR*, EGLBoolean*, EGLint*)>(eglGetProcAddress("eglQueryDmaBufModifiersEXT"));

            result =    _peglCreateImage
                     && _peglDestroyImage
                     && _peglQueryDmaBufFormatsEXT
                     && _peglQueryDmaBufModifiersEXT
                     ;
        }

        return result;
    }

    bool WPEFramework::GLResourceMediator::EGL::DeInitialize()
    {
        bool result = true;

// FIXME:: lock
        for (auto begin=_images.begin(), end = _images.end(), it = begin; it != end; it++) {
            _images.erase(it);

            result = _peglDestroyImage(_dpy, it->second) != EGL_FALSE
                     && result
                     ;
        }

        _dpy = WPEFramework::GLResourceMediator::EGL::InvalidDisplay();
        _ctx = WPEFramework::GLResourceMediator::EGL::InvalidContext();
        _draw = WPEFramework::GLResourceMediator::EGL::InvalidSurface();
        _read = WPEFramework::GLResourceMediator::EGL::InvalidSurface();

        _peglCreateImage = nullptr;
        _peglDestroyImage = nullptr;
        _peglQueryDmaBufFormatsEXT = nullptr;
        _peglQueryDmaBufModifiersEXT = nullptr;

        _valid = false;

        return result;
    }

    bool WPEFramework::GLResourceMediator::EGL::UpdateCurrents()
    {
        bool result = eglGetError() == EGL_SUCCESS;

        if (result) {
            EGLDisplay dpy = eglGetCurrentDisplay();
            _dpy = dpy;

            result =    dpy != InvalidDisplay()
                     && eglGetError() == EGL_SUCCESS
                     ;
        }

        if (result) {
            EGLContext ctx = eglGetCurrentContext();
            _ctx = ctx;

            result =    ctx != InvalidContext
                     && eglGetError() == EGL_SUCCESS
                    ;
        }

        if (result) {
            EGLSurface draw = eglGetCurrentSurface(EGL_DRAW);
            _draw = draw;

            result =    draw != InvalidSurface()
                     && eglGetError() == EGL_SUCCESS
                     ;
        }

        if (result) {
            EGLSurface read = eglGetCurrentSurface(EGL_READ);
            _read = read;

            result =    read != InvalidSurface()
                     && eglGetError() == EGL_SUCCESS
                     ;
        }

        return result;
    }

    WPEFramework::Plugin::ClientSurface::ClientSurface(ModeSet& set, CompositorImplementation& compositor, const string& name, uint32_t width, uint32_t height)
        : _native{set, width, height}
        , _modeSet{set}
        , _compositor{compositor}
        , _name{name}
        , _opacity{Exchange::IComposition::maxOpacity}
        , _layer{Exchange::IComposition::minZOrder}
        , _destination{0, 0, width, height}
    {
        _compositor.AddRef();

        if (!(_native.IsValid())) {
            TRACE(Trace::Error, (_T("A ClientSurface cannot be created for %s"), name.c_str()));
        }
    }

    WPEFramework::Plugin::ClientSurface::~ClientSurface()
    {
        _compositor.Release();
    }

    WPEFramework::Core::instance_id WPEFramework::Plugin::ClientSurface::Native() const
    {
        using common_t = std::common_type<int, Core::instance_id>::type;

        const int fd = _native.NativePrime().Fd();

        ASSERT(   fd != GLResourceMediator::Native::Prime::InvalidFd()
               && static_cast<common_t>(fd) > static_cast<common_t>(std::numeric_limits<Core::instance_id>::lowest())
              );

        return static_cast<Core::instance_id>(_native.NativePrime().Fd());
    }

    const WPEFramework::GLResourceMediator::Native& WPEFramework::Plugin::ClientSurface::NativeAccessor() const
    {
        return _native;
    }

    int WPEFramework::Plugin::ClientSurface::SyncPrimitive() const
    {
        return _native.NativePrime().SyncFd();
    }

    bool WPEFramework::Plugin::ClientSurface::SyncPrimitiveStart() const
    {
        return _native.NativePrime().Lock();
    }

    bool WPEFramework::Plugin::ClientSurface::SyncPrimitiveEnd() const
    {
        return _native.NativePrime().Unlock();
    }

    uint32_t WPEFramework::Plugin::ClientSurface::Geometry(const Exchange::IComposition::Rectangle& rectangle)
    {
        _destination = rectangle;
        return Core::ERROR_NONE;
    }

    uint32_t WPEFramework::Plugin::ClientSurface::ZOrder(uint16_t zorder)
    {
        _layer = zorder;
        return Core::ERROR_NONE;
    }

    void WPEFramework::Plugin::ClientSurface::ScanOut()
    {
        /* bool */ _compositor.CompositeFor(_name);
    }

    WPEFramework::Plugin::CompositorImplementation::CompositorImplementation()
        : _adminLock{}
        , _service{nullptr}
        , _engine{}
        , _externalAccess{nullptr}
        , _observers{}
        , _clients{}
        , _config{}
        , _platform{GLResourceMediator::ModeSetOpenOnce::Instance()}
        , _dma{nullptr}
        , _native{_platform, _platform.Width(), _platform.Height()}
        , _egl{_native}
        , _gles{_egl}
        , _clientLock{}
        , _textureRenderer{_egl, _gles, _clients}
        , _sceneRenderer{_egl, _gles, *this}
    {}

    WPEFramework::Plugin::CompositorImplementation::~CompositorImplementation()
    {
        _textureRenderer.Stop();

        /* bool */ _textureRenderer.Wait(Core::Thread::STOPPED, Core::infinite);

        _sceneRenderer.Stop();

        /* bool */ _sceneRenderer.Wait(Core::Thread::STOPPED, Core::infinite);

        /* bool */ _egl.Invalidate();

        /* bool */ _native.Invalidate();

        if (_dma != nullptr) {
            delete _dma;
            _dma = nullptr;
        }

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

    template<size_t N>
    bool WPEFramework::Plugin::CompositorImplementation::FDFor(const std::string& name, std::array<int, N>& fds, std::string& properties)
    {
        _clientLock.Lock();

        /* ProxyType<> */ const auto& client = _clients.Find(name);

        bool result =    client.IsValid()
                      && name.compare(client->Name()) == 0
                      && _dma != nullptr
                      ;

        if (result) {
            constexpr const char spacer[] = ":";

            auto& prime = client->NativeAccessor().NativePrime();

            properties.clear();

            properties =   std::string(spacer)
                         + std::to_string(prime.Width()).append(spacer)
                         + std::to_string(prime.Height()).append(spacer)
                         + std::to_string(prime.Stride()).append(spacer)
                         + std::to_string(prime.Format()).append(spacer)
                         + std::to_string(prime.Modifier())
                         ;

            using common_t = std::common_type<int, Core::instance_id>::type; 

            const Core::instance_id id = client->Native();

            fds[0] = static_cast<int>(id);
            fds[1] = client->SyncPrimitive();

            static_assert(std::is_unsigned<Core::instance_id>::value);

            result =    static_cast<common_t>(id) <= static_cast<common_t>(std::numeric_limits<int>::max())
                     && fds[0] != DMATransfer::InvalidFiledescriptor()
                     && fds[1] != DMATransfer::InvalidFiledescriptor()
                     && _egl.CreateImage(prime);
                     ;
        }

        client.Release();

        _clientLock.Unlock();

        return result;
    }

    bool WPEFramework::Plugin::CompositorImplementation::CompositeFor(const std::string& name)
    {
        _clientLock.Lock();

        /* void */ _textureRenderer.SetClientName(name);

        // Skip the request to create a frame texture if the rate is too high to be processed
        if (!(_textureRenderer.IsRunning())) {
            /* void */ _textureRenderer.Run();
        }

        _clientLock.Unlock();

        return true;
    }


    /* static */ uint8_t WPEFramework::Plugin::CompositorImplementation::RefreshRateFromResolution(const ScreenResolution resolution)
    {
        // Assume 'unknown' rate equals 60 Hz
        uint8_t rate = 60;

        switch (resolution) {
        case ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
                                          rate = 24; break;
        case ScreenResolution_720p50Hz  : // 1280x720 progressive @ 50 Hz
                                          __attribute__((fallthrough));
        case ScreenResolution_1080i50Hz : // 1920x1080 interlaced @ 50 Hz
                                          __attribute__((fallthrough));
        case ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
                                          __attribute__((fallthrough));
        case ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
                                          rate = 50; break;
        case ScreenResolution_2160p30Hz : // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
                                          rate = 30; break;
        case ScreenResolution_480i      : // 720x480
                                          __attribute__((fallthrough));
        case ScreenResolution_480p      : // 720x480 progressive
                                          __attribute__((fallthrough));
        case ScreenResolution_720p      : // 1280x720 progressive
                                          __attribute__((fallthrough));
        case ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
                                          __attribute__((fallthrough));
        case ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
        case ScreenResolution_Unknown   : rate = 60;
        }

        return rate;
    }

    bool WPEFramework::Plugin::CompositorImplementation::FrameFlip()
    {
        constexpr const uint16_t milli = 1000;

        // Some performance gain at the expense of no reize support
        static const auto frameticks = std::chrono::milliseconds(milli / WPEFramework::Plugin::CompositorImplementation::RefreshRateFromResolution(Resolution())).count();

        static auto start = std::chrono::steady_clock::now();

        /* void */ _platform.Swap(_native.NativeSurface());

        auto finish = std::chrono::steady_clock::now();

        auto ticks = std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(finish - start)).count() - frameticks;

        // Do not free loop
        if (ticks < 0) {
            SleepMs(-ticks);
        }

        start = finish;

        return true;
    }

    /* static */ uint32_t WPEFramework::Plugin::CompositorImplementation::WidthFromResolution(const ScreenResolution resolution)
    {
        // Asumme an invalid width equals 0
        uint32_t width = 0;

        switch (resolution) {
        case ScreenResolution_480p      : // 720x480
                                          width = 720; break;
        case ScreenResolution_720p      : // 1280x720 progressive
                                          __attribute__((fallthrough));
        case ScreenResolution_720p50Hz  : // 1280x720 @ 50 Hz
                                          width = 1280; break;
        case ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
                                          __attribute__((fallthrough));
        case ScreenResolution_1080i50Hz : // 1920x1080 interlaced  @ 50 Hz
                                          __attribute__((fallthrough));
        case ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
                                          __attribute__((fallthrough));
        case ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
                                          width = 1920; break;
        case ScreenResolution_2160p30Hz : // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
                                          __attribute__((fallthrough));
        case ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
                                          __attribute__((fallthrough));
        case ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
                                          width = 3840; break;
        case ScreenResolution_480i      : // Unknown according to the standards (?)
                                          __attribute__((fallthrough));
        case ScreenResolution_Unknown   : __attribute__((fallthrough));
        default                         : width = 0;
        }

        return width;
    }

    /* static */ uint32_t WPEFramework::Plugin::CompositorImplementation::HeightFromResolution(const ScreenResolution resolution)
    {
        // Asumme an invalid height equals 0
        uint32_t height = 0;

        switch (resolution) {
        case ScreenResolution_480i      : __attribute__((fallthrough));
        case ScreenResolution_480p      : height = 480; break;
        case ScreenResolution_720p      : __attribute__((fallthrough));
        case ScreenResolution_720p50Hz  : height = 720; break;
        case ScreenResolution_1080p24Hz : __attribute__((fallthrough));
        case ScreenResolution_1080i50Hz : __attribute__((fallthrough));
        case ScreenResolution_1080p50Hz : __attribute__((fallthrough));
        case ScreenResolution_1080p60Hz : height = 1080; break;
        case ScreenResolution_2160p30Hz : // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
                                          __attribute__((fallthrough));
        case ScreenResolution_2160p50Hz : __attribute__((fallthrough));
        case ScreenResolution_2160p60Hz : height = 2160; break;
        case ScreenResolution_Unknown   : __attribute__((fallthrough));
        default                         : height = 0;
        }

        return height;
    }

    /* static */ WPEFramework::Exchange::IComposition::ScreenResolution WPEFramework::Plugin::CompositorImplementation::ResolutionFromHeightWidth(uint32_t width, uint32_t height)
    {
        silence(height);
        silence(width);

        // Given the options, the refresh rate is also important so the only sensible value is 'unknown'
        return Exchange::IComposition::ScreenResolution_Unknown;
    }

    uint32_t WPEFramework::Plugin::CompositorImplementation::Configure(PluginHost::IShell* service)
    {
        uint32_t result = Core::ERROR_INVALID_RANGE;

        if (service != nullptr) {
            _service = service;

            _service->AddRef();

            _config.FromString(service->ConfigLine());

            _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());

            _externalAccess = new ExternalAccess(*this, Core::NodeId(_config.Connector().Value().c_str()), service->ProxyStubPath(), _engine);

            if (   _externalAccess != nullptr
                && _externalAccess->IsListening()
               ) {
                PlatformReady();

                result = Core::ERROR_NONE;
            } else {
                delete _externalAccess;

                _externalAccess = nullptr;

                _engine.Release();

                result = Core::ERROR_OPENING_FAILED;
            }
        }

        return result;
    }

    void WPEFramework::Plugin::CompositorImplementation::Register(INotification* notification)
    {
        _adminLock.Lock();

        if (notification != nullptr) {
            ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());

            notification->AddRef();

            _observers.push_back(notification);

            _clients.Visit(
                           [=](const string& name, const Core::ProxyType<ClientSurface>& element) { notification->Attached(name, &(*element)); }
                          );

        }

        _adminLock.Unlock();
    }

    void WPEFramework::Plugin::CompositorImplementation::Unregister(INotification* notification)
    {
        _adminLock.Lock();

        if (notification != nullptr) {
            std::list<INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

            if (index != _observers.end()) {
                _clients.Visit(
                               [=](const string& name, const Core::ProxyType<ClientSurface>& element)
                               {
                                 silence(element);
                                 notification->Detached(name);
                               }
                              );

                _observers.erase(index);

                notification->Release();
            }
        }

        _adminLock.Unlock();
    }

    void WPEFramework::Plugin::CompositorImplementation::Attached(const string& name, IClient* client)
    {
        _adminLock.Lock();

        if (client != nullptr
            && !(name.empty())
           ) {
            for (auto& observer : _observers) {
                observer->Attached(name, client);
            }
        }

        _adminLock.Unlock();
    }

    void WPEFramework::Plugin::CompositorImplementation::Detached(const string& name)
    {
        _adminLock.Lock();

        if (!(name.empty())) {
            for (auto& observer : _observers) {
                observer->Detached(name);
            }
        }

        _adminLock.Unlock();
    }

    WPEFramework::Core::instance_id WPEFramework::Plugin::CompositorImplementation::Native() const
    {
        EGLNativeDisplayType result = EGL_DEFAULT_DISPLAY;

        if (_native.IsValid()) {
            result = static_cast<EGLNativeDisplayType>(_native.NativeDisplay());
        } else {
            TRACE(Trace::Error, (_T("The native display (id) might be invalid / unsupported. Using the EGL default display instead!")));
        }

        static_assert(sizeof(EGLNativeDisplayType) <= sizeof(Core::instance_id));

        return reinterpret_cast<Core::instance_id>(result);
    }

    std::string WPEFramework::Plugin::CompositorImplementation::Port() const
    {
        return _config.Port().Value();
    }

    WPEFramework::Exchange::IComposition::IClient* WPEFramework::Plugin::CompositorImplementation::CreateClient(const string& name, uint32_t width, uint32_t height)
    {
        IClient* client = nullptr;

        Core::ProxyType<ClientSurface> object = _clients.Instance<ClientSurface>(name, _platform, *this, name, width, height);

        ASSERT(object.IsValid());

        if (object.IsValid()) {
            client = &(*object);

            Attached(name, client);

            client->AddRef();
        }

        if (client == nullptr) {
            TRACE(Trace::Error, (_T("Unable to create the ClientSurface with name %s"), name.c_str()));
        } else {
            if (_dma == nullptr) {
                _dma = new DMATransfer(*this);
            }

            if (   _dma == nullptr
                || !(_dma->IsValid())
               ) {
                delete _dma;
                _dma = nullptr;
            } else {
                if (!(_dma->IsRunning())) {
                    /* void */ _dma->Run();
                }

                if (!(_sceneRenderer.IsRunning())) {
                    /* void */ _sceneRenderer.Run();
                }
            }
        }

        return client;
    }

    void WPEFramework::Plugin::CompositorImplementation::InvalidateClient(const string& name)
    {
        _clientLock.Lock();

        Core::ProxyType<ClientSurface> object = _clients.Find(name);

        if (object.IsValid()) {
            Detached(object->Name());

            const GLResourceMediator::Native& surf = object->NativeAccessor();

           /* bool */  _gles.SkipSharedBufferFromScene(surf.NativePrime());
        }

        _clientLock.Unlock();
    }

    WPEFramework::Exchange::IComposition::ScreenResolution WPEFramework::Plugin::CompositorImplementation::Resolution() const
    {
        Exchange::IComposition::ScreenResolution resolution = WPEFramework::Exchange::IComposition::ScreenResolution_Unknown;

        //uint32_t width = _platform.Width();
        uint32_t height = _platform.Height();

        uint32_t vrefresh = _platform.RefreshRate();

        bool interlaced = _platform.Interlaced();

        if (!interlaced) {
            switch (height) {
            case 480    :   {
                                resolution = ScreenResolution_480p;
                                break;
                            }
            case 720    :   {
                                resolution = vrefresh != 50 ? ScreenResolution_720p : ScreenResolution_720p50Hz;
                                break;
                            }
            case 1080   :   {
                                switch (vrefresh) {
                                case 24 : resolution = ScreenResolution_1080p24Hz; break;
                                case 50 : resolution = ScreenResolution_1080p50Hz; break;
                                case 60 : resolution = ScreenResolution_1080p60Hz; break;
                                default : resolution = ScreenResolution_Unknown;
                                }
                                break;
                            }
            case 2160   :   {
                                switch (vrefresh) {
                                case 30 : resolution = ScreenResolution_2160p30Hz; break; // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
                                case 50 : resolution = ScreenResolution_2160p50Hz; break;
                                case 60 : resolution = ScreenResolution_2160p60Hz; break;
                                default : resolution = ScreenResolution_Unknown;
                                }
                                break;
                            }
            default     :   {
                                resolution = ScreenResolution_Unknown;
                            }
            }
        } else {
            switch (height) {
            case 480    :   {
                                resolution = ScreenResolution_480i;
                                break;
                            }
            case 1080   :   {
                                resolution = vrefresh != 50 ? ScreenResolution_Unknown : ScreenResolution_1080i50Hz;
                                break;
                            }
            default     :   {
                                resolution = ScreenResolution_Unknown;
                            }
            }
         }

        return resolution;
     }

    uint32_t WPEFramework::Plugin::CompositorImplementation::Resolution(const ScreenResolution format)
    {
        TRACE(Trace::Error, (_T("Unable to set screenresolution to %s. Not supported!"), Core::EnumerateType<ScreenResolution>(format).Data()));
        return Core::ERROR_UNAVAILABLE;
    }

    WPEFramework::Plugin::CompositorImplementation::ExternalAccess::ExternalAccess(CompositorImplementation& parent, const Core::NodeId& source, const string& proxyStubPath, const Core::ProxyType<RPC::InvokeServer>& handler)
        : RPC::Communicator(source, !( proxyStubPath.empty()) ? Core::Directory::Normalize(proxyStubPath) : proxyStubPath, Core::ProxyType<Core::IIPCServer>(handler))
        , _parent(parent)
    {

        uint32_t result = !(handler.IsValid()) ? Core::ERROR_UNAVAILABLE : RPC::Communicator::Open(RPC::CommunicationTimeOut);

        if (result != Core::ERROR_NONE) {
            TRACE(Trace::Error, (_T("Unable to set up RPCLink. Error: %s"), Core::NumberType<uint32_t>(result).Text()));
        } else {
            handler->Announcements(Announcement());

            Core::SystemInfo::SetEnvironment(_T("COMPOSITOR"), RPC::Communicator::Connector(), true);
        }
    }

    WPEFramework::Plugin::CompositorImplementation::ExternalAccess::~ExternalAccess()
    {
// FIXME:: What about close
    }

    void* WPEFramework::Plugin::CompositorImplementation::ExternalAccess::Aquire(const string& className, uint32_t interfaceId, uint32_t version)
    {
        silence(className);
        silence(version);

        // Use the className to check for multiple HDMI's.
        return _parent.QueryInterface(interfaceId);
    }

    WPEFramework::Plugin::CompositorImplementation::DMATransfer::DMATransfer(CompositorImplementation& compositor)
        : Core::Thread(/* 0 , _T ( "" ) */)
        , _compositor{compositor}
        , _listen{InvalidSocket()}
        , _transfer{InvalidSocket()}
        , _addr{AF_UNIX, "/tmp/Compositor/DMA"}
        , _valid{Init()}
    {}

    WPEFramework::Plugin::CompositorImplementation::DMATransfer::~DMATransfer()
    {
        Stop ();

        /* bool */ Wait(WPEFramework::Core::Thread::STOPPED, WPEFramework::Core::infinite);

        /* bool */ Deinit();
    }

    bool WPEFramework::Plugin::CompositorImplementation::DMATransfer::IsValid() const
    {
        return _valid;
    }

    template<size_t N>
    bool WPEFramework::Plugin::CompositorImplementation::DMATransfer::Receive(std::string& msg, std::array<int, N>& fds)
    {
        bool result =    IsValid()
                      && Connect(Core::infinite)
                      ;

        if (!result) {
            TRACE(Trace::Information, (_T("Unable to receive (DMA) data.")));
        } else {
            result = Receive(msg, fds.data(), fds.size());
            result =    Disconnect(Core::infinite)
                     && result
                     ;
        }

        return result;
    }

    template<size_t N>
    bool WPEFramework::Plugin::CompositorImplementation::DMATransfer::Send(const std::string& msg, const std::array<int, N>& fds)
    {
        bool result =    IsValid()
                      && Connect(Core::infinite)
                      ;

        if (!result) {
            TRACE(Trace::Information, (_T("Unable to send (DMA) data.")));
        } else {
            result = Send(msg, fds.data(), fds.size());
            result =    Disconnect(Core::infinite)
                     && result
                     ;
        }

        return result;
    }

    uint32_t WPEFramework::Plugin::CompositorImplementation::DMATransfer::Worker()
    {
        // Reschedule approximately @ 1 Hz
        constexpr const uint32_t result = 1000;

        // Default behavior
        Block();

        fd_set fds;

        FD_ZERO(& fds);
        FD_SET(_listen, &fds);

        // Just use 'polling'
        constexpr const struct timespec timeout = { .tv_sec = 0, .tv_nsec = 0 };

        int status = pselect(_listen + 1, &fds, nullptr, nullptr, &timeout, nullptr);

        if (status < 0) {
            // Error
            TRACE(Trace::Error, (_T("Unable to accept DMA connections")));

            Stop();
        } else {
            if (status == 0) {
                // Timeout, no (new) connection available
            } else {
                if (FD_ISSET(_listen, &fds) != 0) {
                    _transfer = accept(_listen, nullptr, nullptr);

                    std::string message;

                    // Shared buffer and synchronization primitive
                    std::array<int, 2> handles = { InvalidFiledescriptor(), InvalidFiledescriptor() };

                    std::string properties;

                    if (_transfer != InvalidSocket()) {
                        if (   Receive(message, handles)
                            && _compositor.FDFor(message, handles, properties)
                            && Send(message + properties, handles)
                           ) {
                            // Just wait for the remote peer to close the connection
                            const ssize_t size = read(_transfer, nullptr, 0);

                            const int status = errno;

                            switch (size) {
                            case -1 :   // Error
                                        TRACE(Trace::Error, (_T("Error after DMA transfer : %d."), status));
                                        break;
                            case 0  :   // Remote has closed the connection
                                        TRACE(Trace::Information, (_T("Remote has closed the DMA connection.")));
                                        break;
                            default :   // Unexpected data available
                                        TRACE(Trace::Error, (_T("Unexpected data read after DMA transfer.")));
                            }

                            /* int */ close(_transfer);

                            _transfer = InvalidSocket();
                        } else {
                            TRACE(Trace::Error, (_T("Failed to exchange DMA information for %s."), message.length() > 0 ? message.c_str() : "'<no name provided>'"));
                        }
                    } else {
                        // Not necessarily an error
                        TRACE(Trace::Information, (_T("Premature incoming connection loss.")));
                    }
                }
            }
        }

        return result;
    }

    bool WPEFramework::Plugin::CompositorImplementation::DMATransfer::Init()
    {
        // Just a precaution
        /* int */ unlink(_addr.sun_path);

        _listen = socket(_addr.sun_family /* domain */, SOCK_STREAM /* type */, 0 /* protocol */);

// FIXME: Derive it from maximum number of supported clients
        // Number of pending requests for accept to handle
        constexpr const int queue_size = 1;

        bool result =    _listen != InvalidSocket()
                      && bind(_listen, reinterpret_cast<const struct sockaddr*>(&_addr), sizeof(_addr)) == 0
                      && listen(_listen, queue_size) == 0
                      && fcntl(_listen, F_SETFL, O_NONBLOCK) == 0
                      ;

         return result;
    }

    bool WPEFramework::Plugin::CompositorImplementation::DMATransfer::Deinit()
    {
        bool result =    _listen != InvalidSocket()
                      && close(_listen) == 0
                      ;

        result =    _transfer != InvalidSocket()
                 && close(_transfer) == 0
                 && result;
                 ;

        // (Always) delete the (bind) socket in the file system if no reference exist (anymore)
        result =    unlink(_addr.sun_path) == 0
                 && result;

        return result;
    }

    bool WPEFramework::Plugin::CompositorImplementation::DMATransfer::Connect(uint32_t timeout) const
    {
        silence(timeout);

        int status = errno;

        bool result =    _transfer != InvalidSocket()
                      && status == 0
                      ;

        return result;
    }

    bool WPEFramework::Plugin::CompositorImplementation::DMATransfer::Disconnect(uint32_t timeout) const
    {
        silence(timeout);

        int status = errno;

        bool result =    _transfer != InvalidSocket()
                      && status == 0
                      ;

        return result;
    }

    bool WPEFramework::Plugin::CompositorImplementation::DMATransfer::Send(const std::string& msg, const int* fd, uint8_t count)
    {
        constexpr const bool enable = true;

        static_assert(!narrowing<std::string::size_type, size_t, enable>::value);
        const size_t bufsize = msg.size();

        bool result = bufsize > 0;

        if (bufsize > 0) {
            // Scatter array for vector I/O
            struct iovec iov;

            static_assert(std::is_same<char, std::string::value_type>::value);
            // Starting address for copying data
            char* address = const_cast<char*>(msg.data());
            iov.iov_base = reinterpret_cast<void*>(address);
            // Number of bytes to transfer
            iov.iov_len = bufsize;

            // Actual message
            struct msghdr msgh{};

            // Optional address
            msgh.msg_name = nullptr;
            // Size of address
            msgh.msg_namelen = 0;
            // Scatter array
            msgh.msg_iov = &iov;
            // Elements in msg_iov
            msgh.msg_iovlen = 1;

            // Ancillary data
            // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
            char control[CMSG_SPACE(sizeof(int) * count)];

            // Only valid file descriptor (s) can be sent via extra payload
            uint8_t bitmask = 0;

            for (uint8_t i = 0; i < count && count < 8 && fd != nullptr; i++) {
                bitmask |= fd[i] != InvalidFiledescriptor() ? count : 0;
            }

            result = bitmask != 0;

            if (result) {
                // Contruct ancillary data to be added to the transfer via the control message

                // Ancillary data, pointer
                msgh.msg_control = control;

                // Ancillery data buffer length
                msgh.msg_controllen = sizeof(control);

                // Ancillary data should be access via cmsg macros
                // https://linux.die.net/man/2/recvmsg
                // https://linux.die.net/man/3/cmsg
                // https://linux.die.net/man/2/setsockopt
                // https://www.man7.org/linux/man-pages/man7/unix.7.html

                // Control message

                // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                struct cmsghdr* cmsgh = CMSG_FIRSTHDR(&msgh);

                result = cmsgh != nullptr;

                if (result) {
                    // Originating protocol
                    // To manipulate options at the sockets API level
                    cmsgh->cmsg_level = SOL_SOCKET;

                    // Protocol specific type
                    // Option at the API level, send or receive a set of open file descriptors from another process
                    cmsgh->cmsg_type = SCM_RIGHTS;

                    // The value to store in the cmsg_len member of the cmsghdr structure, taking into account any necessary alignmen, eg byte count of control message including header
                    cmsgh->cmsg_len = CMSG_LEN(sizeof(int) * count);

                    // Initialize the payload
                    /* void */ memcpy(CMSG_DATA(cmsgh), fd, sizeof(int) * count);
                }
            } else {
                // No extra payload, ie file descriptor(s), to include
                msgh.msg_control = nullptr;
                msgh.msg_controllen = 0;

                result = !result;
            }

            if (result) {
                // Configuration succeeded

                // https://linux.die.net/man/2/sendmsg
                // https://linux.die.net/man/2/write
                // Zero flags is equivalent to write

                ssize_t size = -1;
                socklen_t len = sizeof(size);

                // Only send data if the buffer is large enough to contain all data
                if (getsockopt(_transfer, SOL_SOCKET, SO_SNDBUF, &size, &len) == 0) {
                    // Most options use type int, ssize_t was just a placeholder
                    static_assert(!narrowing<ssize_t, long, enable>::value);
                    TRACE(Trace::Information, (_T("The sending buffer capacity equals %ld bytes."), size));

// FIXME: do not send if the sending buffer is too small
                    size = sendmsg(_transfer, &msgh, 0);
                } else {
                    // Unable to determine buffer capacity
                }

                result = size > 0;

                if (result) {
                    static_assert(!narrowing <ssize_t, long, enable>::value);
                    static_assert(!narrowing<decltype(msgh.msg_iov->iov_len + msgh.msg_controllen), unsigned long, enable>::value);
                    TRACE(Trace::Information, (_T("Send %ld bytes out of %lu."), size, msgh.msg_iov->iov_len /* just a single element */ + msgh.msg_controllen));
                } else {
                    TRACE(Trace::Error, (_T("Failed to send data.")));
                }
            }

        } else {
            TRACE(Trace::Error, (_T("A data message to be send cannot be empty!")));
        }

        return result;
    }

    bool WPEFramework::Plugin::CompositorImplementation::DMATransfer::Receive(std::string& msg, int* fd, uint8_t count)
    {
        constexpr const bool enable = true;

        msg.clear();

        static_assert(!narrowing<socklen_t, size_t, enable>::value);
        socklen_t capacity = 0, len = sizeof(capacity);

        if (getsockopt(_transfer, SOL_SOCKET, SO_RCVBUF, &capacity, &len) == 0) {
            static_assert(!narrowing<socklen_t, long, enable>::value);
            TRACE(Trace::Information, (_T("The receiving buffer maximum capacity equals %ld [bytes]."), capacity));

            static_assert(!narrowing<socklen_t, std::string::size_type, enable>::value);
            msg.reserve(capacity);
        } else {
            // Unable to determine buffer capacity
            static_assert(!narrowing<std::string::size_type, unsigned long, enable>::value);
            TRACE(Trace::Information, (_T("Unable to determine buffer maximum cpacity. Using %lu [bytes] instead."), msg.capacity()));

            msg.reserve(0);
        }

        const size_t bufsize = msg.capacity();

        bool ret = false;

        if (   bufsize > 0
            && count > 0
            && fd != nullptr
           ) {
            for (uint8_t i = 0; i < count; i++) {
                fd [i] = InvalidFiledescriptor();
            }

            // Scatter array for vector I/O
            struct iovec iov;

            static_assert(std::is_same<char, std::string::value_type>::value);
            // Starting address for writing data
            char buf[bufsize];
            iov.iov_base = reinterpret_cast<void*>(&buf[0]);
            // Number of bytes to transfer
            iov.iov_len = bufsize;

            // Actual message
            struct msghdr msgh{};

            // Optional address
            msgh.msg_name = nullptr;
            // Size of address
            msgh.msg_namelen = 0;
            // Elements in msg_iov
            msgh.msg_iovlen = 1;
            // Scatter array
            msgh.msg_iov = &iov;

            // Ancillary data
            // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
            char control[CMSG_SPACE(sizeof(int) * count)];

            // Ancillary data, pointer
            msgh.msg_control = control;

            // Ancillary data buffer length
            msgh.msg_controllen = sizeof(control);

            // No flags set
            ssize_t size = recvmsg(_transfer, &msgh, 0);

            ret = size > 0;

            switch (size) {
            case -1 :   // Error
                        {
                            TRACE(Trace::Error, (_T("Error receiving remote (DMA) data.")));
                            break;
                        }
            case 0  :   // Peer has done an orderly shutdown, before any data was received
                        {
                            TRACE(Trace::Error, (_T("Error receiving remote (DMA) data. Client may have become unavailable.")));
                            break;
                        }
            default :   // Data
                        {
                            // Extract the file descriptor information
                            static_assert(!narrowing<ssize_t, long, enable>::value);
                            TRACE(Trace::Information, (_T("Received %ld bytes."), size));

                            // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                            struct cmsghdr* cmsgh = CMSG_FIRSTHDR(&msgh);

                            // Check for the expected properties the client should have set
                            if (   cmsgh != nullptr
                                && cmsgh->cmsg_level == SOL_SOCKET
                                && cmsgh->cmsg_type == SCM_RIGHTS
                            ) {
                                // The macro returns a pointer to the data portion of a cmsghdr.
                                /* void */ memcpy(fd, CMSG_DATA(cmsgh), sizeof(int) * count);
                            } else {
                                TRACE(Trace::Information, (_T("No (valid) ancillary data received.")));
                            }

                            using common_t = std::common_type<ssize_t, std::string::size_type>::type;

                            if (static_cast<common_t>(size) <= static_cast<common_t>(std::numeric_limits<std::string::size_type>::max())) {
                                msg.assign(buf, size);
                            } else {
                                ret = false;
                            }
                        }
            }
        } else {
            // Error
            TRACE(Trace::Error, (_T("A receiving data buffer (message) cannot be empty!")));
        }

        return ret;
    }

    WPEFramework::Plugin::CompositorImplementation::RenderThread::~RenderThread()
    {
        Stop();

        /* bool */ Wait(Core::Thread::STOPPED, Core::infinite);
    }

    uint32_t WPEFramework::Plugin::CompositorImplementation::RenderThread::Worker()
    {
        return Core::infinite;
    }

    WPEFramework::Plugin::CompositorImplementation::RenderThread::RenderThread(GLResourceMediator::EGL& egl, GLResourceMediator::GLES& gles)
        : Core::Thread()
        , _sharing{Instance()}
        , _egl{egl}
        , _gles{gles}
    {}

    WPEFramework::Core::CriticalSection& WPEFramework::Plugin::CompositorImplementation::RenderThread::Instance()
    {
        static Core::CriticalSection lock;
        return lock;
    }

    WPEFramework::Plugin::CompositorImplementation::SceneRenderer::SceneRenderer(GLResourceMediator::EGL& egl, GLResourceMediator::GLES& gles, CompositorImplementation& compositor)
        : RenderThread{egl, gles}
        , _compositor{compositor}
    {}

    WPEFramework::Plugin::CompositorImplementation::SceneRenderer::~SceneRenderer()
    {
        Stop();

        /* bool */ Wait(Core::Thread::STOPPED, Core::infinite);
    }

    uint32_t WPEFramework::Plugin::CompositorImplementation::SceneRenderer::Worker()
    {
        constexpr const uint32_t ret = 0;

        bool status =     Render()
                      && _compositor.FrameFlip()
                      ;

        if (status) {
            Block();
        } else {
            Stop();
        }

        return ret;
    }

    bool WPEFramework::Plugin::CompositorImplementation::SceneRenderer::Render()
    {
        Exchange::IComposition::ScreenResolution resolution = _compositor.Resolution();

        uint32_t w = WidthFromResolution(resolution);
        uint32_t h = HeightFromResolution(resolution);

        using common_t = std::common_type<uint32_t, GLuint>::type;

        _sharing.Lock();

        bool ret =    static_cast<common_t>(w) <= static_cast<common_t>(std::numeric_limits<GLuint>::max())
                   && static_cast<common_t>(h) <= static_cast<common_t>(std::numeric_limits<GLuint>::max())
                   && _egl.Render(std::bind(&GLResourceMediator::GLES::RenderScene,
                                         &_gles,
                                         w,
                                         h,
                                         [](const GLResourceMediator::GLES::Texture &left, const GLResourceMediator::GLES::Texture& right) -> bool
                                         { return left.Z() > right.Z(); } 
                                         ), true)
                   ;

        _sharing.Unlock();

        return ret;
    }

    WPEFramework::Plugin::CompositorImplementation::TextureRenderer::TextureRenderer(GLResourceMediator::EGL& egl, GLResourceMediator::GLES& gles, CompositorImplementation::ClientContainer& clients)
        : RenderThread{egl, gles}
        , _clients{clients}
        , _set{[](const StringElement& lhs, const StringElement& rhs) -> bool
               {
                    return !(   !(lhs.Value() < rhs.Value())
                             && !(lhs.Value() > rhs.Value())
                            )
                           ;
               }
              }
    {}

    WPEFramework::Plugin::CompositorImplementation::TextureRenderer::~TextureRenderer()
    {
        Stop();

        /* bool */ Wait(Core::Thread::STOPPED, Core::infinite);
    }

    void WPEFramework::Plugin::CompositorImplementation::TextureRenderer::SetClientName(const std::string& name)
    {
        _access.Lock();

        auto result = _set.emplace(name);

        if (result.second) {
            _queue.push(StringElement(name));
        }

        _access.Unlock();
    }

    uint32_t WPEFramework::Plugin::CompositorImplementation::TextureRenderer::Worker()
    {
        uint32_t ret = WPEFramework::Core::infinite;

        bool status = Render();

        Block();

        if (status) {
            _access.Lock();

            if (_queue.size() > 0) {
                ret = 0;
            }

            _access.Unlock();
        }

        return ret;
    }

    bool WPEFramework::Plugin::CompositorImplementation::TextureRenderer::Render()
    {
        Core::ProxyType<ClientSurface> client;

        _access.Lock();

        if (_queue.size() > 0) {
            // Implicit AddRef
            client = _clients.Find(_queue.front().Value());
        }

        _access.Unlock();

        bool result =     client.IsValid()
                      && _egl.IsValid()
                      && _gles.IsValid()
                      && client->SyncPrimitiveStart()
                      ;

        if (result) {
            const GLResourceMediator::Native& surf = client->NativeAccessor();
            const Exchange::IComposition::Rectangle geom = client->Geometry();

            //  Geom values should not exceed buffer dimensions
            uint32_t width = surf.Width();
            uint32_t height = surf.Height();

            silence(width);
            silence(height);

            // Z-order value should be within expected ranges
            uint32_t zorder = client->ZOrder();

            // Opacity value should be within expected ranges
            uint32_t opa = client->Opacity();

            const GLResourceMediator::Native::Prime& prime = surf.NativePrime();

            _sharing.Lock();

            using common_t = std::common_type<uint32_t, GLuint>::type;

            result =    static_cast<common_t>(geom.x) <= static_cast<common_t>(std::numeric_limits<GLuint>::max())
                     && static_cast<common_t>(geom.y) <= static_cast<common_t>(std::numeric_limits<GLuint>::max())
                     && static_cast<common_t>(geom.width) <= static_cast<common_t>(std::numeric_limits<GLuint>::max())
                     && static_cast<common_t>(geom.height) <= static_cast<common_t>(std::numeric_limits<GLuint>::max())
                     && static_cast<common_t>(zorder) <= static_cast<common_t>(std::numeric_limits<GLuint>::max())
                     && static_cast<common_t>(opa) <= static_cast<common_t>(std::numeric_limits<GLuint>::max())
                     && _egl.RenderWithoutSwap(std::bind(&GLResourceMediator::GLES::RenderSharedBuffer, &_gles, std::cref(prime), geom.x, geom.y, zorder, geom.width, geom.height, opa));

            _sharing.Unlock();

            result =    client->SyncPrimitiveEnd()
                     && result;

            client.Release();
        }

        _access.Lock();

        if (_queue.size() > 0) {
            result =    _set.erase(_queue.front()) == 1
                     && result;

            /* void */ _queue.pop();
        }

        _access.Unlock();

        return result;
    }

    WPEFramework::Plugin::CompositorImplementation::Config::Config()
        : Core::JSON::Container ()
        , _connector(_T ( "/tmp/compositor"))
        , _port(_T("HDMI0"))
    {
        Add(_T("connector"), &_connector);
        Add(_T("port"), &_port);
    }

    WPEFramework::Plugin::CompositorImplementation::Config::~Config()
    {
    }

    void WPEFramework::Plugin::CompositorImplementation::PlatformReady()
    {
        PluginHost::ISubSystem* subSystems(_service->SubSystems());

        ASSERT(subSystems!= nullptr);

        if (subSystems != nullptr) {
            subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
            subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);

            subSystems->Release();
        }
    }

namespace WPEFramework {
namespace Plugin {
    SERVICE_REGISTRATION(CompositorImplementation, 1, 0);
}
}

#undef GL_ERROR_WITH_RETURN
#undef GL_ERROR

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
