/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological
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

#include <messaging/messaging.h>

#include <interfaces/IComposition.h>
#include <interfaces/ICompositionBuffer.h>

#include <IBuffer.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <compositorbuffer/CompositorBufferType.h>

#include <drm_fourcc.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {

    const Compositor::Color black = { 0.f, 0.f, 0.f, 1.0f };

    class CompositorImplementation : public Exchange::IComposition, public Exchange::IComposition::IDisplay {
    private:
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , RPCConnector(_T("/tmp/compositor"))
                , ClientBridge(_T("/tmp/compositorclient-bridge"))
                , Port("HDMI-A-1")
                , Resolution(Exchange::IComposition::ScreenResolution::ScreenResolution_720p)
                , Format(DRM_FORMAT_ARGB8888)
                , Modifier(DRM_FORMAT_MOD_LINEAR)
            {
                Add(_T("rpc_connector"), &RPCConnector);
                Add(_T("client_bridge"), &ClientBridge);
                Add(_T("port"), &Port);
                Add(_T("resolution"), &Resolution);
                Add(_T("format"), &Format);
                Add(_T("modifier"), &Modifier);
            }

            ~Config() override = default;

            Core::JSON::String RPCConnector;
            Core::JSON::String ClientBridge;
            Core::JSON::String Port;
            Core::JSON::EnumType<Exchange::IComposition::ScreenResolution> Resolution;
            Core::JSON::HexUInt32 Format;
            Core::JSON::HexUInt64 Modifier;
        };

    private:
        // The communication channel with the (remote) other site
        class ExternalAccess : public RPC::Communicator {
        public:
            ExternalAccess() = delete;
            ExternalAccess(const ExternalAccess&) = delete;
            ExternalAccess& operator=(const ExternalAccess&) = delete;

            ExternalAccess(
                CompositorImplementation& parent,
                const Core::NodeId& source,
                const string& proxyStubPath,
                const Core::ProxyType<RPC::InvokeServer>& handler)
                : RPC::Communicator(source, proxyStubPath.empty() == false ? Core::Directory::Normalize(proxyStubPath) : proxyStubPath, Core::ProxyType<Core::IIPCServer>(handler))
                , _parent(parent)
            {
                uint32_t result = RPC::Communicator::Open(RPC::CommunicationTimeOut);

                handler->Announcements(Announcement());

                if (result != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Could not open Compositor RPC link server. Error: %s"), Core::NumberType<uint32_t>(result).Text()));
                } else {
                    // We need to pass the communication channel NodeId via an environment variable, for process,
                    // not being started by the rpcprocess...
                    Core::SystemInfo::SetEnvironment(_T("COMPOSITOR"), RPC::Communicator::Connector(), true);
                }
            }

            ~ExternalAccess() override = default;

        private:
            virtual void* Acquire(const string& /*className*/, const uint32_t interfaceId, const uint32_t /*versionId*/) override
            {
                return (_parent.QueryInterface(interfaceId));
            }

            CompositorImplementation& _parent;
        };

    public:
        CompositorImplementation()
            : _adminLock()
            , _engine()
            , _externalAccess(nullptr)
            , _format(DRM_FORMAT_INVALID)
            , _modifier(DRM_FORMAT_MOD_INVALID)
            , _gpuConnector()
            , _renderer()
            , _clients()
            , _clientBridge(*this)
            , _observers()
            , _lastFrame(0)
            , _background(black)
        {
        }

        ~CompositorImplementation() override
        {
            if (_externalAccess != nullptr) {
                delete _externalAccess;
                _engine.Release();
            }

            _clientBridge.Close();
            _clients.Clear();
            _renderer.Release();
            _gpuConnector.Release();
        }

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        INTERFACE_ENTRY(Exchange::IComposition::IDisplay)
        END_INTERFACE_MAP

    private:
        class Client : public Exchange::IComposition::IClient, public Exchange::ICompositionBuffer {
        private:
            class SharedBuffer : public Compositor::CompositorBufferType<4> {
            private:
                using BaseClass = WPEFramework::Compositor::CompositorBufferType<4>;

            public:
                SharedBuffer() = delete;
                SharedBuffer(const SharedBuffer&) = delete;
                SharedBuffer& operator=(const SharedBuffer&) = delete;

            public:
                SharedBuffer(Client& parent)
                    : BaseClass(parent.Name(), parent.Identifier(), parent.Width(), parent.Height(), parent.Format(), parent.Modifier(), parent.Type())
                    , _parent(parent)
                {

                    Exchange::ICompositionBuffer::IIterator* planes = _parent.Planes(10);
                    ASSERT(planes != nullptr);

                    uint16_t nPlanes(0);

                    while ((planes->Next() == true) && (planes->IsValid() == true)) {
                        ASSERT(planes->IsValid() == true);

                        Exchange::ICompositionBuffer::IPlane* plane = planes->Plane();
                        ASSERT(plane != nullptr);

                        BaseClass::Add(plane->Accessor(), plane->Stride(), plane->Offset());

                        TRACE(Trace::Information, ("Added plane accessor=%" PRIx64 " stride=%d offset=%d", plane->Accessor(), plane->Stride(), plane->Offset()));

                        ++nPlanes;
                    }

                    _parent.Completed(false);
                }
                virtual ~SharedBuffer()
                {
                }

                void AddRef() const override
                {
                    _parent.AddRef();
                }

                uint32_t Release() const override
                {
                    return _parent.Release();
                }

                void Render() override
                {
                    _parent.Render();
                }

            private:
                Client& _parent;
            }; // class SharedBuffer

        public:
            using Container = Core::ProxyMapType<string, Client>;

            class Bridge : public Core::PrivilegedRequest {
            private:
                Bridge(const Bridge&) = delete;
                Bridge& operator=(const Bridge&) = delete;

            public:
                Bridge(CompositorImplementation& parent)
                    : Core::PrivilegedRequest()
                /*, _parent(parent)*/
                {
                }

                virtual ~Bridge()
                {
                    Close();
                }

                uint8_t Service(const uint32_t id, const uint8_t maxSize, int container[]) override
                {
                    uint8_t result(0);

                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

                    for (const SharedBuffer* buffer : _shareable) {

                        ASSERT(buffer != nullptr);

                        if (buffer->Identifier() == id) {
                            result = buffer->Descriptors(maxSize, container);
                            break;
                        }
                    }

                    return result;
                }

                void Announce(const SharedBuffer* buffer)
                {
                    ASSERT(buffer != nullptr);

                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

                    std::list<const SharedBuffer*>::iterator index(std::find(_shareable.begin(), _shareable.end(), buffer));

                    if (index == _shareable.end()) {
                        _shareable.push_back(buffer);
                    }
                }

                void Revoke(const SharedBuffer* buffer)
                {
                    ASSERT(buffer != nullptr);

                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

                    std::list<const SharedBuffer*>::iterator index(std::find(_shareable.begin(), _shareable.end(), buffer));

                    if (index != _shareable.end()) {
                        _shareable.erase(index);
                    }
                }

            private:
                mutable Core::CriticalSection _adminLock;
                std::list<const SharedBuffer*> _shareable;
            }; // class Bridge

        public:
            Client() = delete;
            Client(const Client&) = delete;
            Client& operator=(const Client&) = delete;

            Client(CompositorImplementation& parent, const string& callsign, const uint32_t width, const uint32_t height)
                : _parent(parent)
                , _callsign(callsign)
                , _opacity(0)
                , _zIndex(0)
                , _geometry({ .x = 0, .y = 0, .width = width, .height = height })
                , _buffer(Compositor::CreateBuffer(_parent.Native(), width, height, Compositor::PixelFormat(_parent.Format(), { _parent.Modifier() })))
                , _sharedBuffer(*this)
                , _texture(nullptr)
            {
                ASSERT(_buffer.IsValid());
                _parent.Announce(*this);
            }

            ~Client() override
            {
                _parent.Revoke(*this);
                _buffer.Release();

                if (_texture != nullptr) {
                    _texture->Release();
                }
            }

            /**
             * Exchange::IComposition::IClient methods
             */
            WPEFramework::Core::instance_id Native() const
            {
                return _buffer->Identifier();
            }

            string Name() const
            {
                return _callsign;
            }
            void Opacity(const uint32_t value)
            {
                _opacity = value;
            }
            uint32_t Opacity() const
            {
                return _opacity;
            }
            uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle)
            {
                _geometry = rectangle;
                return Core::ERROR_NONE;
            }
            Exchange::IComposition::Rectangle Geometry() const
            {
                return _geometry;
            }
            uint32_t ZOrder(const uint16_t index)
            {
                _zIndex = index;
                return Core::ERROR_NONE;
            }
            uint32_t ZOrder() const
            {
                return _zIndex;
            }

            BEGIN_INTERFACE_MAP(RemoteAccess)
            INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP

            /**
             * Exchange::ICompositionBuffer methods
             */

            void AddRef() const override
            {
                // We are not a reference counted object, so do nothing.
            }
            uint32_t Release() const override
            {
                // We are not a reference counted object, so do nothing.
                return (0);
            }

            uint32_t Identifier() const override
            {
                return (_buffer->Identifier());
            }
            IIterator* Planes(const uint32_t timeoutMs) override
            {
                return (_buffer->Planes(timeoutMs));
            }
            uint32_t Completed(const bool dirty) override
            {
                return (_buffer->Completed(dirty));
            }
            void Render() override
            {
                _buffer->Render();
            }
            uint32_t Width() const override
            {
                return (_buffer->Width());
            }
            uint32_t Height() const override
            {
                return (_buffer->Height());
            }
            uint32_t Format() const override
            {
                return (_buffer->Format());
            }
            uint64_t Modifier() const override
            {
                return (_buffer->Modifier());
            }
            DataType Type() const override
            {
                return (_buffer->Type());
            }

            /**
             *
             */
            Compositor::IRenderer::ITexture* Texture()
            {
                return _texture;
            }
            void Texture(Compositor::IRenderer::ITexture* texture)
            {
                ASSERT((_texture != nullptr) ^ (texture != nullptr));
                _texture = texture;
            }

            const SharedBuffer* Buffer() const
            {
                return &_sharedBuffer;
            }

        private:
            CompositorImplementation& _parent;
            const string _callsign; // the callsign of the surface
            uint32_t _opacity; // the opacity of the surface on the composition
            uint16_t _zIndex; // the z-index of the surface on the composition
            Exchange::IComposition::Rectangle _geometry; // the actual geometry of the surface on the composition
            Core::ProxyType<Exchange::ICompositionBuffer> _buffer; // the actual buffer we are rendering on.
            const SharedBuffer _sharedBuffer; //
            Compositor::IRenderer::ITexture* _texture;
        }; // class Client

        void PlatformReady(PluginHost::IShell* service)
        {
            PluginHost::ISubSystem* subSystems(service->SubSystems());
            ASSERT(subSystems != nullptr);
            if (subSystems != nullptr) {
                subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
                subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                subSystems->Release();
            }
        }

    public:
        /**
         * Exchange::IComposition methods
         */
        uint32_t Configure(PluginHost::IShell* service) override
        {
            uint32_t result = Core::ERROR_NONE;

            string configuration(service->ConfigLine());
            Config config;

            config.FromString(service->ConfigLine());

            _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());
            _externalAccess = new ExternalAccess(*this, Core::NodeId(config.RPCConnector.Value().c_str()), service->ProxyStubPath(), _engine);

            if (_externalAccess->IsListening() == true) {
                _format = config.Format.Value();
                _modifier = config.Modifier.Value();
                _port = config.Port.Value();
                _resolution = config.Resolution.Value();

                _gpuConnector = Compositor::Connector(_port, _resolution, _format, false);

                ASSERT(_gpuConnector.IsValid());

                _renderer = Compositor::IRenderer::Instance(_gpuConnector->Identifier());

                _clientBridge.Open(config.ClientBridge.Value());

                Core::SystemInfo::SetEnvironment(_T("COMPOSITORCLIENTBRIDGE"), config.ClientBridge.Value(), true);

                PlatformReady(service);

            } else {
                delete _externalAccess;
                _externalAccess = nullptr;
                _engine.Release();
                TRACE(Trace::Error, (_T("Could not report PlatformReady as there was a problem starting the Compositor RPC %s"), _T("server")));
                result = Core::ERROR_OPENING_FAILED;
            }

            return (result);
        }

        void Register(Exchange::IComposition::INotification* notification) override
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());

            notification->AddRef();

            _observers.push_back(notification);

            // _clients.Visit(
            //     [=](const string& name, const Core::ProxyType<Client>& element) { notification->Attached(name, &(*element)); });
        }

        void Unregister(Exchange::IComposition::INotification* notification) override
        {
            _adminLock.Lock();

            std::list<Exchange::IComposition::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

            ASSERT(index != _observers.end());

            if (index != _observers.end()) {

                // _clients.Visit(
                //     [=](const string& name, const Core::ProxyType<Client>& element) { silence ( element ); notification -> Detached ( name ) ; });

                _observers.erase(index);

                notification->Release();
            }

            _adminLock.Unlock();
        }

    private:
        void Announce(Client& client)
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            client.Texture(_renderer->Texture(&client));

            for (auto& observer : _observers) {
                observer->Attached(client.Name(), &client);
            }

            _clientBridge.Announce(client.Buffer());
        }

        void Revoke(Client& client)
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            _clientBridge.Revoke(client.Buffer());

            for (auto& observer : _observers) {
                observer->Detached(client.Name());
            }

            if (client.Texture() != nullptr) {
                client.Texture()->Release();
                client.Texture(nullptr);
            }
        }

        const uint32_t Format() const
        {
            return _format;
        }

        const uint64_t Modifier() const
        {
            return _modifier;
        }

    public:
        /**
         * Exchange::IComposition::IDisplay methods
         */

        WPEFramework::Core::instance_id Native() const override
        {
            return (_gpuConnector->Identifier());
        }

        string Port() const override
        {
            return _port;
        }

        IComposition::IClient* CreateClient(const string& name, const uint32_t width, const uint32_t height) override
        {
            IClient* client = nullptr;

            Core::ProxyType<Client> object = _clients.Instance<Client>(name, *this, name, width, height);

            ASSERT(object.IsValid() == true);

            if (object.IsValid() == true) {
                client = &(*object);
                client->AddRef();
            }

            return client;
        }

        uint64_t RenderScene()
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            const uint16_t width(_gpuConnector->Width());
            const uint16_t height(_gpuConnector->Height());

            _renderer->Bind(_gpuConnector);

            _renderer->Begin(width, height);
            _renderer->Clear(_background);

            _clients.Visit([&](const string& name, const Core::ProxyType<Client> client) {
                if (client->Texture() == nullptr) {
                    return; // no texture to render
                }

                Exchange::IComposition::Rectangle rectangle = client->Geometry();

                const Compositor::Box renderBox = { .x = int(rectangle.x), .y = int(rectangle.y), .width = int(rectangle.width), .height = int(rectangle.height) };

                Compositor::Matrix matrix;

                Compositor::Transformation::ProjectBox(matrix, renderBox, Compositor::Transformation::TRANSFORM_NORMAL, 0, _renderer->Projection());

                const Compositor::Box textureBox = { .x = 0, .y = 0, .width = int(client->Texture()->Width()), .height = int(client->Texture()->Height()) };

                _renderer->Render(client->Texture(), textureBox, matrix, float(client->Opacity() / Exchange::IComposition::maxOpacity));
            });

            _renderer->End(false);

            _gpuConnector->Render();

            _renderer->Unbind();

            return Core::Time::Now().Ticks();
        }

        uint32_t Resolution(const Exchange::IComposition::ScreenResolution format) override
        {
            TRACE(Trace::Information, (_T("FixMe: Could not set resolution to %s. This is still a to do"), Core::EnumerateType<Exchange::IComposition::ScreenResolution>(format).Data()));
            return (Core::ERROR_UNAVAILABLE);
        }

        Exchange::IComposition::ScreenResolution Resolution() const override
        {
            return _resolution;
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Core::ProxyType<RPC::InvokeServer> _engine;
        ExternalAccess* _externalAccess;
        string _port;
        Exchange::IComposition::ScreenResolution _resolution;
        uint32_t _format;
        uint64_t _modifier;
        Core::ProxyType<Exchange::ICompositionBuffer> _gpuConnector;
        Core::ProxyType<Compositor::IRenderer> _renderer;
        std::list<Exchange::IComposition::INotification*> _observers;
        Client::Bridge _clientBridge;
        Client::Container _clients;
        uint64_t _lastFrame;
        Compositor::Color _background;
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
