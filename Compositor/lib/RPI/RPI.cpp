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
#include <com/com.h>

#include "ModeSet.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

// Suppress compiler warnings of unused (parameters)
// Omitting the name is sufficient but a search on this keyword provides easy access to the location
template <typename T>
void silence (T &&) {}

namespace WPEFramework {
namespace Plugin {

class CompositorImplementation;

    class ClientSurface : public Exchange::IComposition::IClient {
    public:
        ClientSurface() = delete;
        ClientSurface(const ClientSurface&) = delete;
        ClientSurface& operator= (const ClientSurface&) = delete;

        ClientSurface(ModeSet& modeSet, CompositorImplementation& compositor, const string& name, const EGLSurface& surface, const uint32_t width, const uint32_t height);
        ~ClientSurface() override;

    public:
        RPC::instance_id Native() const override {
                return reinterpret_cast<RPC::instance_id>(_nativeSurface);
        }
        string Name() const override
        {
                return _name;
        }
        void Opacity(const uint32_t value) override {
            TRACE(Trace::Error, (_T("Opacity is currently not supported for Surface %s"), _name.c_str()));
        }
        uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override {
            TRACE(Trace::Error, (_T("Geometry is currently not supported for Surface %s"), _name.c_str()));
            return Core::ERROR_UNAVAILABLE;
        }
        Exchange::IComposition::Rectangle Geometry() const override {
            return _destination;
        }
        uint32_t ZOrder(const uint16_t zorder) override {
            TRACE(Trace::Error, (_T("ZOrder is currently not supported for Surface %s"), _name.c_str()));
            return Core::ERROR_UNAVAILABLE;
        }
        uint32_t ZOrder() const override {
            return _layer;
        }

        BEGIN_INTERFACE_MAP(ClientSurface)
            INTERFACE_ENTRY(Exchange::IComposition::IClient)
        END_INTERFACE_MAP

    private:
        ModeSet& _modeSet;
        CompositorImplementation& _compositor; 
        const std::string _name;
        EGLSurface _nativeSurface;

        uint32_t _opacity;
        uint32_t _layer;

        Exchange::IComposition::Rectangle _destination;
    };

    class CompositorImplementation : public Exchange::IComposition, public Exchange::IComposition::IDisplay {
    private:
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        class ExternalAccess : public RPC::Communicator {
        private:
            ExternalAccess() = delete;
            ExternalAccess(const ExternalAccess&) = delete;
            ExternalAccess& operator=(const ExternalAccess&) = delete;

        public:
            ExternalAccess(
                CompositorImplementation& parent, 
                const Core::NodeId& source, 
                const string& proxyStubPath, 
                const Core::ProxyType<RPC::InvokeServer>& handler)
                : RPC::Communicator(source,  proxyStubPath.empty() == false ? Core::Directory::Normalize(proxyStubPath) : proxyStubPath, Core::ProxyType<Core::IIPCServer>(handler))
                , _parent(parent)
            {
                uint32_t result = RPC::Communicator::Open(RPC::CommunicationTimeOut);

                handler->Announcements(Announcement());

                if (result != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Could not open RPI Compositor RPCLink server. Error: %s"), Core::NumberType<uint32_t>(result).Text()));
                } else {
                    // We need to pass the communication channel NodeId via an environment variable, for process,
                    // not being started by the rpcprocess...
                    Core::SystemInfo::SetEnvironment(_T("COMPOSITOR"), RPC::Communicator::Connector(), true);
                }
            }

            ~ExternalAccess() override = default;

        private:
            void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t version) override
            {
                // Use the className to check for multiple HDMI's. 
                return (_parent.QueryInterface(interfaceId));
            }

        private:
            CompositorImplementation& _parent;
        };

    public:
        CompositorImplementation()
            : _adminLock()
            , _service(nullptr)
            , _engine()
            , _externalAccess(nullptr)
            , _observers()
            , _clients()
            , _platform()
        {
        }
        ~CompositorImplementation()
        {
            _clients.Clear();
            if (_externalAccess != nullptr) {
                delete _externalAccess;
                _engine.Release();
            }
        }

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        INTERFACE_ENTRY(Exchange::IComposition::IDisplay)
        END_INTERFACE_MAP

    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connector(_T("/tmp/compositor"))
                , Port(_T("HDMI0"))
            {
                Add(_T("connector"), &Connector);
                Add(_T("port"), &Port);
            }

            ~Config()
            {
            }

        public:
            Core::JSON::String Connector;
            Core::JSON::String Port;
        };

    public:
        //
        // Echange::IComposition
        // ==================================================================================================================

        static uint32_t WidthFromResolution(const ScreenResolution resolution) {
            // Asumme an invalid width equals 0
            uint32_t _width = 0;

            switch (resolution) {
                case ScreenResolution_480p      : // 720x480
                                                    _width = 720; break;
                case ScreenResolution_720p      : // 1280x720 progressive
                case ScreenResolution_720p50Hz  : // 1280x720 @ 50 Hz
                                                    _width = 720; break;
                case ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
                case ScreenResolution_1080i50Hz : // 1920x1080 interlaced  @ 50 Hz
                case ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
                case ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
                                                    _width = 1080; break;
                case ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
                case ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
                                                    _width = 2160; break;
                case ScreenResolution_480i      : // Unknown according to the standards (?)
                case ScreenResolution_Unknown   :
                default                         : _width = 0;
            }

            return _width;
        }

        static uint32_t HeightFromResolution(const ScreenResolution resolution) {
            // Asumme an invalid height equals 0
            uint32_t _height = 0;

            switch (resolution) {
                case ScreenResolution_480i      :
                case ScreenResolution_480p      : _height = 480; break;
                case ScreenResolution_720p      :
                case ScreenResolution_720p50Hz  : _height =720; break;
                case ScreenResolution_1080p24Hz :
                case ScreenResolution_1080i50Hz :
                case ScreenResolution_1080p50Hz :
                case ScreenResolution_1080p60Hz : _height = 1080; break;
                case ScreenResolution_2160p50Hz :
                case ScreenResolution_2160p60Hz : _height = 2160; break;
                case ScreenResolution_Unknown   :
                default                         : _height = 0;
            }

            return _height;
        }

        static ScreenResolution ResolutionFromHeightWidth(const uint32_t height, const uint32_t width) {
            // Given the options, the refresh rate is also important so the only sensible value is 'unknown'
            return Exchange::IComposition::ScreenResolution_Unknown;
        }

        uint32_t Configure(PluginHost::IShell* service) override
        {
            uint32_t result = Core::ERROR_NONE;
            _service = service;

            string configuration(service->ConfigLine());
            Config config;
            config.FromString(service->ConfigLine());

            _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());
            _externalAccess = new ExternalAccess(*this, Core::NodeId(config.Connector.Value().c_str()), service->ProxyStubPath(), _engine);

            if ((_externalAccess->IsListening() == true) && (_platform.Open(config.Port.Value()) == Core::ERROR_NONE)) {
                _port = config.Port.Value();
                _platform.Open(_port);
                PlatformReady();
            } else {
                delete _externalAccess;
                _externalAccess = nullptr;
                _engine.Release();
                TRACE(Trace::Error, (_T("Could not report PlatformReady as there was a problem starting the Compositor RPC %s"), _T("server")));
                result = Core::ERROR_OPENING_FAILED;
            }
            return result;
        }
        void Register(Exchange::IComposition::INotification* notification) override
        {
            _adminLock.Lock();
            ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());
            notification->AddRef();
            _observers.push_back(notification);

            _clients.Visit([=](const string& name, const Core::ProxyType<ClientSurface>& element){
                notification->Attached(name, &(*element));
            });

            _adminLock.Unlock();
        }
        void Unregister(Exchange::IComposition::INotification* notification) override
        {
            _adminLock.Lock();
            std::list<Exchange::IComposition::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));
            ASSERT(index != _observers.end());
            if (index != _observers.end()) {
                _observers.erase(index);
                notification->Release();
            }
            _adminLock.Unlock();
        }
        void Attached(const string& name, IClient* client) {
            _adminLock.Lock();
            for(auto& observer : _observers) {
                observer->Attached(name, client);
            }
            _adminLock.Unlock();
        }
        void Detached(const string& name) {
            _adminLock.Lock();
            for(auto& observer : _observers) {
                observer->Detached(name);
            }
            _adminLock.Unlock();
        }

        //
        // Echange::IComposition::IDisplay
        // ==================================================================================================================
        RPC::instance_id Native() const override {
            EGLNativeDisplayType result (static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY));

            const struct gbm_device* pointer = _platform.UnderlyingHandle();

            if(pointer != nullptr) {
                result = reinterpret_cast<EGLNativeDisplayType>(const_cast<struct gbm_device*>(pointer));
            }
            else {
                TRACE(Trace::Error, (_T("The native display (id) might be invalid / unsupported. Using the EGL default display instead!")));
            }

            return reinterpret_cast<RPC::instance_id>(result);
        }

        string Port() const override {
            return (_port);
        }
        IClient* CreateClient(const string& name, const uint32_t width, const uint32_t height) override {
            IClient* client = nullptr;

            struct gbm_surface* surface = _platform.CreateRenderTarget(width, height);
            if (surface != nullptr) {

                Core::ProxyType<ClientSurface> object = _clients. template Instance(
                             name,
                             _platform,
                             *this,
                             name,
                             reinterpret_cast<EGLSurface>(surface),
                             width,
                             height);

                ASSERT(object.IsValid() == true);

                if( object.IsValid() == true ) {
                    client = &(*object);
                }
            } 

            if(client == nullptr) {
                TRACE(Trace::Error, (_T("Could not create the Surface with name %s"), name.c_str()));
            }

            return (client);
        }

        Exchange::IComposition::ScreenResolution Resolution() const override
        {
            Exchange::IComposition::ScreenResolution _resolution = WPEFramework::Exchange::IComposition::ScreenResolution_Unknown;

            decltype ( std::declval <ModeSet> ().Width () ) _width = _platform.Width (); silence (_width);
            decltype ( std::declval <ModeSet> ().Height () ) _height = _platform.Height ();

// TODO: This might not be the whole story to determine progressive versus interlaced

            decltype ( std::declval <ModeSet> ().RefreshRate () ) _vrefresh = _platform.RefreshRate ();
            decltype ( std::declval <ModeSet> ().Interlaced () ) _interlaced = _platform.Interlaced ();

            if (_interlaced != true) {
                switch (_height) {
                    case 480    :   {
                                        _resolution = ScreenResolution_480p;
                                        break;
                                    }
                    case 720    :   {
                                        _resolution = _vrefresh != 50 ? ScreenResolution_720p : ScreenResolution_720p50Hz;
                                        break;
                                    }
                    case 1080   :   {
                                        switch (_vrefresh) {
                                            case 24 : _resolution = ScreenResolution_1080p24Hz; break;
                                            case 50 : _resolution = ScreenResolution_1080p50Hz; break;
                                            case 60 : _resolution = ScreenResolution_1080p60Hz; break;
                                            default : _resolution = ScreenResolution_Unknown;
                                        }
                                        break;
                                    }
                    case 2160   :   {
                                        switch (_vrefresh) {
                                            case 50 : _resolution = ScreenResolution_2160p50Hz; break;
                                            case 60 : _resolution = ScreenResolution_2160p60Hz; break;
                                            default : _resolution = ScreenResolution_Unknown;
                                        }
                                        break;
                                    }
                    default     :   {
                                        _resolution = ScreenResolution_Unknown;
                                    }
                }
            }
            else {
                switch (_height) {
                    case 480    :   {
                                        _resolution = ScreenResolution_480i;
                                        break;
                                    }
                    case 1080   :   {
                                        _resolution = _vrefresh != 50 ? ScreenResolution_Unknown : ScreenResolution_1080i50Hz;
                                        break;
                                    }
                    default     :   {
                                        _resolution = ScreenResolution_Unknown;
                                    }
                }
            }

            return _resolution;

        }
        uint32_t Resolution(const Exchange::IComposition::ScreenResolution format) override
        {
            TRACE(Trace::Error, (_T("Could not set screenresolution to %s. Not supported for Rapberry Pi compositor"), Core::EnumerateType<Exchange::IComposition::ScreenResolution>(format).Data()));
            return (Core::ERROR_UNAVAILABLE);
        }

    private:
        void PlatformReady()
        {
            PluginHost::ISubSystem* subSystems(_service->SubSystems());
            ASSERT(subSystems != nullptr);
            if (subSystems != nullptr) {
                subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
                subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                subSystems->Release();
            }
        }

    private:
        using ClientContainer = Core::ProxyMapType<string, ClientSurface>;

        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Core::ProxyType<RPC::InvokeServer> _engine;
        ExternalAccess* _externalAccess;
        std::list<Exchange::IComposition::INotification*> _observers;
        ClientContainer _clients;
        string _port;
        ModeSet _platform;
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

    ClientSurface::ClientSurface(ModeSet& modeSet, CompositorImplementation& compositor, const string& name, const EGLSurface& surface, const uint32_t width, const uint32_t height)
        : _modeSet(modeSet)
        , _compositor(compositor)
        , _name(name)
        , _nativeSurface(surface)
        , _opacity(Exchange::IComposition::maxOpacity)
        , _layer(0)
        , _destination( { 0, 0, width, height } ) {

        _compositor.Attached(_name, this);

    }

    ClientSurface::~ClientSurface() {

        _compositor.Detached(_name);
        
    }

} // namespace Plugin
} // namespace WPEFramework
