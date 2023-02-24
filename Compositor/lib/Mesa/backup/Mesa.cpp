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

#include <IBackend.h>

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
    constexpr int InvalidFd = -1;

    class CompositorImplementation : public Exchange::IComposition, public Exchange::IComposition::IDisplay {
    public:
        class Client : public Exchange::IComposition::IClient,
                       public Exchange::IComposition::IRender,
                       public Exchange::IComposition::IClient::IProperties {
        public:
            // The buffer acts as a surface for the remote site
            struct RemoteSurface {
                struct gbm_bo* _fbo;
                int _fd;
                EGLImage _image;

                RemoteSurface()
                    : _fbo(nullptr)
                    , _fd(InvalidFd)
                    , _image(EGL_NO_IMAGE)
                {
                }

                bool IsValid() const
                {
                    return (_fbo != nullptr) && (_fd != InvalidFd) && (_image != EGL_NO_IMAGE);
                }
            };

            using RemoteSurfaceType = RemoteSurface;

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
                _nativeSurface._fbo = _modeSet.CreateBufferObject(width, height);

                if (_nativeSurface._fbo != nullptr) {
                    _nativeSurface._fd = gbm_bo_get_fd(_nativeSurface._fbo); // acts

                    TRACE(CompositorTrace::Client, (_T("Constructed Client gbm_bo_fd=%d, width=%d, height=%d, stride=%d, format=0x%04X, modifier=0x%" PRIX64), _nativeSurface._fd, gbm_bo_get_width(_nativeSurface._fbo), gbm_bo_get_height(_nativeSurface._fbo), gbm_bo_get_stride(_nativeSurface._fbo), gbm_bo_get_format(_nativeSurface._fbo), gbm_bo_get_modifier(_nativeSurface._fbo)));
                } else {
                    TRACE(Trace::Error, (_T ( "A Client cannot be created for %s" ), name.c_str()));
                }
            }

            ~Client() override
            {
                // Part of the client is cleaned up via the detached (hook)
                if (_nativeSurface._fbo != ModeSet::GBM::InvalidBuf()) {
                    _modeSet.DestroyBufferObject(_nativeSurface._fbo);
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
                bool composed = _compositor.Compose(_name);
                TRACE(CompositorTrace::Client, (_T("%s:%d %s for %s %s"), __FILE__, __LINE__, __FUNCTION__, _name.c_str(), (composed) ? "OK" : "Failed"));
            }

            const RemoteSurface& Surface(const EGLImage& khr = EGL_NO_IMAGE)
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
                return gbm_bo_get_stride(_nativeSurface._fbo);
            }
            uint32_t Format()
            {
                return gbm_bo_get_format(_nativeSurface._fbo);
            }
            uint32_t Offset()
            {
                return gbm_bo_get_offset(_nativeSurface._fbo, 0);
            }
            uint64_t Modifier()
            {
                return gbm_bo_get_modifier(_nativeSurface._fbo);
            }

            BEGIN_INTERFACE_MAP(Client)
            INTERFACE_ENTRY(Exchange::IComposition::IClient)
            INTERFACE_ENTRY(Exchange::IComposition::IRender)
            INTERFACE_ENTRY(Exchange::IComposition::IClient::IProperties)
            END_INTERFACE_MAP

        private:
            RemoteSurface _nativeSurface;
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

            uint8_t Service(const uint32_t id, Core::PrivilegedRequest::Container& container) override
            {
                return _parent.SurfaceDmaFd(id, container);
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

            ExternalAccess(CompositorImplementation& parent, const Core::NodeId& source, const string& proxyStubPath, const Core::ProxyType<RPC::InvokeServer>& handler)
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
            ~ExternalAccess() override = default;

        private:
            void* Acquire(const string& /*className*/, const uint32_t interfaceId, uint32_t const /*version*/) override
            {

                // Use the className to check for multiple HDMI's.
                return (_parent.QueryInterface(interfaceId));
            }

            CompositorImplementation& _parent;
        };

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , _connector(_T ( "/tmp/compositor" ))
                , _port(_T ( "HDMI0" ))
            {
                Add(_T ( "connector" ), &_connector);
                Add(_T ( "port" ), &_port);
            }

            ~Config() override = default;

            Core::JSON::String _connector;
            Core::JSON::String _port;
        };

        class Natives final {
        public:
            static constexpr gbm_device* InvalidDisplay = nullptr;
            static constexpr gbm_surface* InvalidSurface = nullptr;

            Natives() = delete;
            Natives(const Natives&) = delete;
            Natives& operator=(const Natives&) = delete;

            explicit Natives(ModeSet& modeSet)
                : _modeSet(modeSet)
                , _gbmSurface(InvalidSurface)
            {
                Initialize();
            }

            ~Natives()
            {
                Deinitialize();
            }

            gbm_device* Display() const { return _modeSet.UnderlyingHandle(); }
            gbm_surface* Surface() const { return _gbmSurface; }

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

                ret = _modeSet.Open("") == Core::ERROR_NONE && Display() != InvalidDisplay;

                if (ret == true) {
                    _gbmSurface = _modeSet.CreateRenderTarget(_modeSet.Width(), _modeSet.Height());
                    ret = _gbmSurface != InvalidSurface;
                }

                if (ret != true) {
                    TRACE(Trace::Error, (_T ( "Unable to create a compositor surface of dimensions: %u x %u [width, height]))." ), _modeSet.Width(), _modeSet.Height()));
                }

                return ret;
            }
            void Deinitialize()
            {
                if (_gbmSurface != InvalidSurface) {
                    _modeSet.DestroyRenderTarget(_gbmSurface);
                }

                _modeSet.Close();
            }

        private:
            ModeSet& _modeSet;
            gbm_surface* _gbmSurface;
        };

        void PlatformReady();

        int SurfaceDmaFd(const uint32_t id, Core::PrivilegedRequest::Container& container)
        {

            class Find : public ClientContainer::IFind {
            public:
                Find(const uint32_t id /*,  const EGL& egl*/)
                    : _id(id)
                    , _result(-1)
                /*, _egl(egl)*/
                {
                }

                bool Check(const string& /*key*/, const Core::ProxyType<Client>& client) const override
                {

                    bool isValid(false);

                    if (client->Native() == _id) {
                        // Client::RemoteSurface surf = client->Surface(EGL::CreateImage(_egl, client->Surface()));
                        // _result = surf.IsValid() ? client->Native() : -1;
                        // isValid = surf.IsValid();
                        isValid = true;
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
                // const EGL& _egl;
            } callback(id /*, _egl*/);

            if (_clients.Find(callback) == true) {
                container.push_back(callback.Result());
            }

            return container.size();
        }

    public:
        CompositorImplementation();
        ~CompositorImplementation();

        bool Compose(const std::string& client)
        {

            // One client at a time
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_clientLock);

            return true;
        }

        bool FrameFlip()
        {

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
            _platform.Swap(bufferInfo);

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

    private:
        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;

        Core::ProxyType<RPC::InvokeServer> _engine;
        ExternalAccess* _externalAccess;

        std::list<Exchange::IComposition::INotification*> _observers;

        DmaFdServer _dmaFdServer;

        Core::CriticalSection _clientLock;
        ClientContainer _clients;

        Compositor::Interfaces::IBackend* _backend;

        string _port;
        ModeSet _platform;
        Natives _natives;
    };

    void CompositorImplementation::PlatformReady()
    {

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
        , _dmaFdServer(*this)
        , _clientLock()
        , _clients()
        , _backend(nullptr)
        , _platform()
        , _natives(_platform)
    {
    }

    CompositorImplementation::~CompositorImplementation()
    {

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
            _backend = Compositor::Interfaces::IBackend::Instance(_port);

            ASSERT(_backend != nullptr);

            _backend->Initialize(service->ConfigLine());

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

        Core::ProxyType<Client> object = _clients.Instance<Client>(name, _platform, *this, name, width, height);

        ASSERT(object.IsValid() == true);

        if (object.IsValid() == true) {
            client = &(*object);
            Attached(name, client);
            client->AddRef();
        }

        if (client == nullptr) {
            TRACE(Trace::Error, (_T ( "Unable to create the Client with name %s" ), name.c_str()));
        } else {
            TRACE(Trace::Error, (_T ( "Client Created with name %s" ), name.c_str()));
        }

        return client;
    }

    void CompositorImplementation::InvalidateClient(const string& name)
    {

        Core::ProxyType<Client> object = _clients.Find(name);

        if (object.IsValid() != false) {

            Detached(object->Name());

            const Client::RemoteSurface& surf = object->Surface();
        }
    }

    Exchange::IComposition::ScreenResolution CompositorImplementation::Resolution() const
    {

        Exchange::IComposition::ScreenResolution resolution = Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;

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
