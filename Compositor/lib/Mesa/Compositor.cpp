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

#include <IBackend.h>
#include <IBuffer.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <compositorbuffer/CompositorBufferType.h>

#include <drm_fourcc.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {
    const Compositor::Color pink = { 1.0f, 0.411f, 0.705f, 1.0f };
    const Compositor::Color black = { 0.f, 0.f, 0.f, 1.0f };

    const std::chrono::microseconds renderThrottle = std::chrono::microseconds(std::chrono::seconds(1)) / 60;

    class CompositorImplementation : public Exchange::IComposition, public Exchange::IComposition::IDisplay {
    private:
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        class Config : public Core::JSON::Container {
        public:
            class OutputConfig : public Core::JSON::Container {
            public:
                OutputConfig& operator=(const OutputConfig&) = delete;
                OutputConfig()
                    : Core::JSON::Container()
                    , Connector("HDMI-A-1")
                    , X(0)
                    , Y(0)
                    , Height(0)
                    , Width(0)
                {
                    Init();
                }

                OutputConfig(const OutputConfig& other)
                    : Core::JSON::Container()
                    , Connector(other.Connector)
                    , X(other.X)
                    , Y(other.Y)
                    , Height(other.Height)
                    , Width(other.Width)
                {
                    Init();
                }

                ~OutputConfig() override = default;

            private:
                void Init()
                {
                    Add(_T("connector"), &Connector);
                    Add(_T("x"), &X);
                    Add(_T("y"), &Y);
                    Add(_T("height"), &Height);
                    Add(_T("width"), &Width);
                }

            public:
                Core::JSON::String Connector;
                Core::JSON::DecUInt16 X;
                Core::JSON::DecUInt16 Y;
                Core::JSON::DecUInt16 Height;
                Core::JSON::DecUInt16 Width;
            };

            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , BufferConnector(_T("bufferconnector"))
                , DisplayConnector("displayconnector")
                , GPU()
                , Render()
                , Height(0)
                , Width(0)
                , Format(DRM_FORMAT_ARGB8888)
                , Modifier(DRM_FORMAT_MOD_LINEAR)
                , Outputs()
            {
                Add(_T("bufferconnector"), &BufferConnector);
                Add(_T("displayconnector"), &DisplayConnector);
                Add(_T("gpu"), &GPU);
                Add(_T("render"), &Render);
                Add(_T("height"), &Height);
                Add(_T("width"), &Width);
                Add(_T("format"), &Format);
                Add(_T("modifier"), &Modifier);
                Add(_T("outputs"), &Outputs);
            }

            ~Config() override = default;

            Core::JSON::String BufferConnector;
            Core::JSON::String DisplayConnector;
            Core::JSON::String GPU;
            Core::JSON::String Render;
            Core::JSON::DecUInt16 Height;
            Core::JSON::DecUInt16 Width;
            Core::JSON::HexUInt32 Format;
            Core::JSON::HexUInt64 Modifier;
            Core::JSON::ArrayType<OutputConfig> Outputs;
        };

        class DisplayDispatcher : public RPC::Communicator {
        public:
            DisplayDispatcher() = delete;
            DisplayDispatcher(const DisplayDispatcher&) = delete;
            DisplayDispatcher& operator=(const DisplayDispatcher&) = delete;

            DisplayDispatcher(
                const Core::NodeId& source,
                const std::string& proxyStubPath,
                Exchange::IComposition::IDisplay* parentInterface,
                const Core::ProxyType<RPC::InvokeServer>& engine)
                : RPC::Communicator(source, proxyStubPath, Core::ProxyType<Core::IIPCServer>(engine))
                , _parentInterface(parentInterface)
            {
                if (_parentInterface != nullptr) {
                    _parentInterface->AddRef();
                }
                Open(Core::infinite);
            }
            ~DisplayDispatcher() override
            {
                if (_parentInterface != nullptr) {
                    _parentInterface->Release();
                }

                Close(Core::infinite);
            }

        private:
            void* Acquire(const string&, const uint32_t interfaceId, const uint32_t versionId) override
            {
                void* result = nullptr;

                if (((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) && ((interfaceId == Exchange::IComposition::IDisplay::ID) || (interfaceId == Core::IUnknown::ID))) {
                    _parentInterface->AddRef();
                    result = _parentInterface;
                    TRACE(Trace::Information, ("IDisplay[%p] interface acquired", result));
                }

                return result;
            }

        private:
            Exchange::IComposition::IDisplay* _parentInterface;
        };

        using OutputRegister = std::list<Core::ProxyType<Exchange::ICompositionBuffer>>;

    public:
        CompositorImplementation()
            : _adminLock()
            , _format(DRM_FORMAT_INVALID)
            , _modifier(DRM_FORMAT_MOD_INVALID)
            , _outputs()
            , _sink(*this)
            , _renderer()
            , _observers()
            , _clientBridge(*this)
            , _clients()
            , _lastFrame(0)
            , _background(pink)
            , _engine()
            , _dispatcher(nullptr)
            , _canvasBuffer()
            , _canvasTexture(nullptr)
            , _canvasShare(nullptr)
            , _gpuIdentifier(0)
            , _gpuNode()
            , _renderNode()
        {
        }

        ~CompositorImplementation() override
        {
            _dispatcher.reset(nullptr);

            if (_engine.IsValid()) {
                _engine.Release();
            }

            _clientBridge.Revoke(_canvasShare.get());
            _canvasShare.reset(nullptr);

            _clientBridge.Close();
            _clients.Clear();
            _renderer.Release();

            if (_gpuIdentifier > 0) {
                ::close(_gpuIdentifier);
                _gpuIdentifier = -1;
            }

            for (auto output : _outputs) {
                output.Release();
            }
        }

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        INTERFACE_ENTRY(Exchange::IComposition::IDisplay)
        END_INTERFACE_MAP

    private:
        class SharedBuffer : public Compositor::CompositorBufferType<4> {
        private:
            using BaseClass = WPEFramework::Compositor::CompositorBufferType<4>;

        public:
            SharedBuffer() = delete;
            SharedBuffer(const SharedBuffer&) = delete;
            SharedBuffer& operator=(const SharedBuffer&) = delete;

        public:
            SharedBuffer(Exchange::ICompositionBuffer* parent)
                : BaseClass(std::to_string(parent->Identifier()), parent->Identifier(), parent->Width(), parent->Height(), parent->Format(), parent->Modifier(), parent->Type())
                , _parent(parent)
            {

                Exchange::ICompositionBuffer::IIterator* planes = _parent->Planes(10);
                ASSERT(planes != nullptr);

                while ((planes->Next() == true) && (planes->IsValid() == true)) {
                    ASSERT(planes->IsValid() == true);
                    BaseClass::Add(planes->Plane()->Accessor(), planes->Plane()->Stride(), planes->Plane()->Offset());
                    TRACE(Trace::Information, (_T("Client[%d] Added shared plane %" PRIuPTR " with stride: %d, offset=%d"), parent->Identifier(), planes->Plane()->Accessor(), planes->Plane()->Stride(), planes->Plane()->Offset()));
                }

                _parent->Completed(false);

                Core::ResourceMonitor::Instance().Register(*this);
            }
            virtual ~SharedBuffer()
            {
                Core::ResourceMonitor::Instance().Unregister(*this);
            }

            uint32_t AddRef() const override
            {
                return _parent->AddRef();
            }

            uint32_t Release() const override
            {
                return _parent->Release();
            }

            void Render() override
            {
                _parent->Render();
            }

        private:
            Exchange::ICompositionBuffer* _parent;
        }; // class SharedBuffer

        class Client : public Exchange::IComposition::IClient, public Exchange::ICompositionBuffer {
        public:
            using Container = Core::ProxyMapType<string, Client>;

            class Bridge : public Core::PrivilegedRequest {
                static constexpr uint32_t DisplayId = 0;

            public:
                Bridge(const Bridge&) = delete;
                Bridge& operator=(const Bridge&) = delete;
                Bridge() = delete;

                Bridge(CompositorImplementation& parent)
                    : Core::PrivilegedRequest()
                    , _parent(parent)
                    , _shareable()
                {
                }

                virtual ~Bridge()
                {
                    Close();
                }

                uint8_t Service(const uint32_t id, const uint8_t maxSize, int container[]) override
                {
                    uint8_t result(0);

                    if (id == DisplayId) {
                        ASSERT(maxSize > 0);
                        container[0] = _parent.Native();
                        result = 1;
                    } else {
                        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

                        for (const SharedBuffer* buffer : _shareable) {

                            ASSERT(buffer != nullptr);

                            if (buffer->Identifier() == id) {
                                result = buffer->Descriptors(maxSize, container);
                                break;
                            }
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
                        TRACE(Trace::Information, (_T("SharedBuffer %d Announced"), buffer->Identifier()));
                    }
                }

                void Revoke(const SharedBuffer* buffer)
                {
                    ASSERT(buffer != nullptr);

                    Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

                    std::list<const SharedBuffer*>::iterator index(std::find(_shareable.begin(), _shareable.end(), buffer));

                    if (index != _shareable.end()) {
                        TRACE(Trace::Information, (_T("SharedBuffer %d Revoked"), (*index)->Identifier()));
                        _shareable.erase(index);
                    }
                }

            private:
                mutable Core::CriticalSection _adminLock;
                CompositorImplementation& _parent;
                std::list<const SharedBuffer*> _shareable;
            }; // class Bridge

        public:
            Client() = delete;
            Client(const Client&) = delete;
            Client& operator=(const Client&) = delete;

            Client(CompositorImplementation& parent, const string& callsign, const uint32_t width, const uint32_t height)
                : _parent(parent)
                , _callsign(callsign)
                , _opacity(255)
                , _zIndex(0)
                , _geometry({ 0, 0, width, height })
                , _buffer(Compositor::CreateBuffer(_parent.Native(), width, height, Compositor::PixelFormat(_parent.Format(), { _parent.Modifier() })))
                , _sharedBuffer(this)
                , _texture(nullptr)
            {
                ASSERT(_buffer.IsValid());
                _parent.Announce(*this);

                TRACE(Trace::Information, (_T("Client %s[%p] created"), _callsign.c_str(), this));
            }

            virtual ~Client() override
            {
                _parent.Revoke(*this);

                if (_texture != nullptr) {
                    _texture->Release();
                    _texture = nullptr;
                }

                _buffer.Release();

                _parent.RequestRender();

                TRACE(Trace::Information, (_T("Client %s[%p] destroyed"), _callsign.c_str(), this));
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

            uint32_t AddRef() const override
            {
                // We are not a reference counted object, so do nothing.
                return (Core::ERROR_COMPOSIT_OBJECT);
            }
            uint32_t Release() const override
            {
                // We are not a reference counted object, so do nothing.
                return (Core::ERROR_COMPOSIT_OBJECT);
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
                const auto start = std::chrono::high_resolution_clock::now();

                _parent.RequestRender(*this);

                const auto next = renderThrottle - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
                std::this_thread::sleep_for((next.count() > 0) ? next : std::chrono::microseconds(0));
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
            Core::ProxyType<Exchange::ICompositionBuffer> _buffer; // the actual GMB/DRMDumb buffer for the client to draw on.
            const SharedBuffer _sharedBuffer; // the exported file descriptors pointing to _buffer planes.
            Compositor::IRenderer::ITexture* _texture; // the texture handle that is known in the GPU/Renderer.
        }; // class Client

        class OutputSink : public Compositor::ICallback {
        public:
            OutputSink(const OutputSink&) = delete;
            OutputSink& operator=(const OutputSink&) = delete;
            OutputSink() = delete;

            OutputSink(CompositorImplementation& parent)
                : _parent(parent)
            {
            }

            virtual ~OutputSink() = default;

            virtual void LastFrameTimestamp(const Exchange::ICompositionBuffer::buffer_id id VARIABLE_IS_NOT_USED, const uint64_t time VARIABLE_IS_NOT_USED) override
            {
            }

            virtual void Display(const Exchange::ICompositionBuffer::buffer_id id, const std::string& node) override
            {
                TRACE(Trace::Information, (_T("Connector id %d opened on %s"), id, node.c_str()));
                _parent.HandleGPUNode(node);
            }

        private:
            CompositorImplementation& _parent;
        };

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

            _format = config.Format.Value();
            _modifier = config.Modifier.Value();

            // FIXME: this needs to be moved to a plugin and the the buffer should be shared via
            //        a CompositorBuffer. For now embed the outputs in the compositor.
            if (config.Outputs.IsSet() == true) {
                auto index = config.Outputs.Elements();

                while (index.Next() == true) {
                    Exchange::IComposition::Rectangle rectangle;

                    rectangle.x = index.Current().X.Value();
                    rectangle.y = index.Current().Y.Value();
                    rectangle.width = index.Current().Width.Value();
                    rectangle.height = index.Current().Height.Value();

                    _outputs.emplace_back<Core::ProxyType<Exchange::ICompositionBuffer>>(Compositor::Connector(index.Current().Connector.Value(), rectangle, _format, &_sink));

                    TRACE(Trace::Information, ("Initialzed connector %s", index.Current().Connector.Value().c_str()));

                    ASSERT(_outputs.back().IsValid());
                }
            }

            if ((config.Render.IsSet() == true) && (config.Render.Value().empty() == false)) {
                _gpuIdentifier = ::open(config.Render.Value().c_str(), O_RDWR | O_CLOEXEC);
            } else {
                ASSERT(_gpuNode.empty() == false);
                _gpuIdentifier = ::open(_gpuNode.c_str(), O_RDWR | O_CLOEXEC);
            }

            ASSERT(_gpuIdentifier > 0);

            _renderer = Compositor::IRenderer::Instance(_gpuIdentifier);
            ASSERT(_renderer.IsValid());

            _canvasBuffer = Compositor::CreateBuffer(_gpuIdentifier, config.Width.Value(), config.Height.Value(), _format);
            ASSERT(_canvasBuffer.IsValid());

            _canvasTexture = _renderer->Texture(_canvasBuffer.operator->());
            ASSERT(_canvasTexture != nullptr);

            _canvasShare.reset(new SharedBuffer(_canvasBuffer.operator->()));
            _clientBridge.Announce(_canvasShare.get());

            RenderCanvas();

            std::string bridgePath = service->VolatilePath() + config.BufferConnector.Value();
            result = _clientBridge.Open(bridgePath);

            if (result == Core::ERROR_NONE) {
                std::string connectorPath = service->VolatilePath() + config.DisplayConnector.Value();

                ASSERT(_dispatcher == nullptr);

                _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());

                _dispatcher.reset(new DisplayDispatcher(Core::NodeId(connectorPath.c_str()), service->ProxyStubPath(), this, _engine));

                if (_dispatcher->IsListening()) {
                    PluginHost::ISubSystem* subSystems = service->SubSystems();

                    ASSERT(subSystems != nullptr);

                    if (subSystems != nullptr) {
                        subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                        subSystems->Release();
                    }

                    TRACE(Trace::Information, (_T("PID %d Compositor configured, communicator: %s, bridge: %s"), getpid(), _dispatcher->Connector().c_str(), bridgePath.c_str()));
                } else {
                    _dispatcher.reset(nullptr);
                    _engine.Release();
                    _clientBridge.Close();

                    _canvasTexture->Release();
                    _canvasBuffer.Release();

                    result = Core::ERROR_UNAVAILABLE;
                }
            }
            return (result);
        }

        void Register(Exchange::IComposition::INotification* notification) override
        {
            ASSERT(notification != nullptr);

            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            std::list<Exchange::IComposition::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));
            ASSERT(index == _observers.end());

            if (index == _observers.end()) {
                notification->AddRef();

                _observers.push_back(notification);
                _clients.Visit(
                    [=](const string& name, const Core::ProxyType<Client>& element) {
                        notification->Attached(name, &(*element));
                    });
            }
        }

        void Unregister(Exchange::IComposition::INotification* notification) override
        {
            ASSERT(notification != nullptr);

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

    private:
        void Announce(Client& client)
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            client.Texture(_renderer->Texture(&client));
            _clientBridge.Announce(client.Buffer());

            for (auto& observer : _observers) {
                observer->Attached(client.Name(), &client);
            }
        }

        void Revoke(Client& client)
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            for (auto& observer : _observers) {
                observer->Detached(client.Name());
            }

            _clientBridge.Revoke(client.Buffer());

            if (client.Texture() != nullptr) {
                client.Texture()->Release();
                client.Texture(nullptr);
            }
        }

        uint32_t Format() const
        {
            return _format;
        }

        uint64_t Modifier() const
        {
            return _modifier;
        }

        void RequestRender(Client& client VARIABLE_IS_NOT_USED)
        {
            RenderCanvas();
        }

        void RequestRender()
        {
            RenderCanvas();
        }

        void HandleGPUNode(const std::string node)
        {
            if (_gpuNode.empty() == true) {
                _gpuNode = node;
            }

            ASSERT(_gpuNode.compare(node) == 0); // we assume only 1 GPU is used.
        }

    public:
        /**
         * Exchange::IComposition::IDisplay methods
         */

        WPEFramework::Core::instance_id Native() const override
        {
            return _gpuIdentifier;
        }

        string Port() const override
        {
            // Who cares on what port we are running...
            return string();
        }

        IComposition::IClient* CreateClient(const string& name, const uint32_t width, const uint32_t height) override
        {
            IClient* client = nullptr;

            TRACE(Trace::Information, (_T("Create a %dx%d surface for client %s, "), width, height, name.c_str()));

            Core::ProxyType<Client> object = _clients.Instance<Client>(name, *this, name, width, height);

            ASSERT(object.IsValid() == true);

            if (object.IsValid() == true) {
                client = &(*object);
                client->AddRef();
                TRACE(Trace::Information, (_T("Created client[%p] %s, "), client, name.c_str()));
            }

            return client;
        }

        uint64_t RenderCanvas()
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            ASSERT((_canvasBuffer.IsValid() == true) && (_renderer.IsValid() == true));

            _renderer->Bind(_canvasBuffer);
            _renderer->Begin(_canvasBuffer->Width(), _canvasBuffer->Height());
            _renderer->Clear(_background);

            _clients.Visit([&](const string& /*name*/, const Core::ProxyType<Client> client) {
                if (client->Texture() == nullptr) {
                    return; // no texture to render
                }

                Exchange::IComposition::Rectangle rectangle = client->Geometry();

                const Compositor::Box renderBox = { int(rectangle.x), int(rectangle.y), int(rectangle.width), int(rectangle.height) };

                Compositor::Matrix clientProjection;
                Compositor::Transformation::ProjectBox(clientProjection, renderBox, Compositor::Transformation::TRANSFORM_FLIPPED_180, 0, _renderer->Projection());

                const Compositor::Box clientArea = { 0, 0, int(client->Texture()->Width()), int(client->Texture()->Height()) };

                // FIXME!!!
                // Opacity is not yet working....
                _renderer->Render(client->Texture(), clientArea, clientProjection, float(client->Opacity() / Exchange::IComposition::maxOpacity));
            });

            _renderer->End(false);
            _renderer->Unbind();

            RenderOutputs();

            return Core::Time::Now().Ticks();
        }

        uint64_t RenderOutputs()
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            for (auto output : _outputs) {
                ASSERT((output.IsValid() == true) && (_renderer.IsValid() == true));

                _renderer->Bind(output);
                _renderer->Begin(output->Width(), output->Height()); // set viewport for render

                const Compositor::Box renderBox = { 0, 0, int(output->Width()), int(output->Height()) };

                Compositor::Matrix canvasProjection;
                Compositor::Transformation::ProjectBox(canvasProjection, renderBox, Compositor::Transformation::TRANSFORM_NORMAL, 0, _renderer->Projection());

                // what part of the canvas do we want to render on this output
                const Compositor::Box canvasArea = {
                    0, // X
                    0, // Y
                    int(_canvasTexture->Width()),
                    int(_canvasTexture->Height())
                };

                _renderer->Render(_canvasTexture, canvasArea, canvasProjection, 1.0);

                _renderer->End(false);

                output->Render(); // Blit to screen

                _renderer->Unbind();
            }

            return Core::Time::Now().Ticks();
        }

        // Useless Resolution functions, this should be controlled by DisplayControl
        uint32_t Resolution(const Exchange::IComposition::ScreenResolution format VARIABLE_IS_NOT_USED) override
        {
            return (Core::ERROR_UNAVAILABLE);
        }
        Exchange::IComposition::ScreenResolution Resolution() const override
        {
            return Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;
        }

    private:
        mutable Core::CriticalSection _adminLock;
        uint32_t _format;
        uint64_t _modifier;
        OutputRegister _outputs;
        OutputSink _sink;
        Core::ProxyType<Compositor::IRenderer> _renderer;
        std::list<Exchange::IComposition::INotification*> _observers;
        Client::Bridge _clientBridge;
        Client::Container _clients;
        uint64_t _lastFrame;
        Compositor::Color _background;
        Core::ProxyType<RPC::InvokeServer> _engine;
        std::unique_ptr<DisplayDispatcher> _dispatcher;
        Core::ProxyType<Exchange::ICompositionBuffer> _canvasBuffer;
        Compositor::IRenderer::ITexture* _canvasTexture;
        std::unique_ptr<SharedBuffer> _canvasShare;
        uint32_t _gpuIdentifier;
        std::string _gpuNode;
        std::string _renderNode;
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0)

} // namespace Plugin
} // namespace WPEFramework
