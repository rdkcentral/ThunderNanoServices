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
#include <IOutput.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <compositorbuffer/CompositorBufferType.h>

#include <drm_fourcc.h>

#include <condition_variable>
#include <mutex>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace Thunder {
namespace Plugin {
    const Compositor::Color pink = { 1.0f, 0.411f, 0.705f, 1.0f };
    const Compositor::Color black = { 0.f, 0.f, 0.f, 1.0f };

    class CompositorImplementation
        : public Exchange::IComposition,
          public Exchange::IComposition::IDisplay {
    private:
        static constexpr uint32_t DisplayId = 0;

        class Config : public Core::JSON::Container {
        public:
            class OutputConfig : public Core::JSON::Container {
            public:
                OutputConfig& operator=(OutputConfig&&) = delete;
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
                OutputConfig(OutputConfig&& other)
                    : Core::JSON::Container()
                    , Connector(std::move(other.Connector))
                    , X(std::move(other.X))
                    , Y(std::move(other.Y))
                    , Height(std::move(other.Height))
                    , Width(std::move(other.Width))
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

            Config(Config&&) = delete;
            Config(const Config&) = delete;
            Config& operator=(Config&&) = delete;
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
            DisplayDispatcher(DisplayDispatcher&&) = delete;
            DisplayDispatcher(const DisplayDispatcher&) = delete;
            DisplayDispatcher& operator=(DisplayDispatcher&&) = delete;
            DisplayDispatcher& operator=(const DisplayDispatcher&) = delete;

            DisplayDispatcher(
                const Core::NodeId& source,
                const std::string& proxyStubPath,
                Exchange::IComposition::IDisplay* parentInterface,
                const Core::ProxyType<RPC::InvokeServer>& engine)
                : RPC::Communicator(source, proxyStubPath, Core::ProxyType<Core::IIPCServer>(engine), _T("@Compositor"))
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

        class Client : public Exchange::IComposition::IClient, public Compositor::CompositorBuffer {
        private:
            class Remote : public Exchange::IComposition::IClient {
            public:
                Remote() = delete;
                Remote(Remote&&) = delete;
                Remote(const Remote&) = delete;
                Remote& operator=(Remote&&) = delete;
                Remote& operator=(const Remote&) = delete;

                Remote(Client& client)
                    : _refCount(0)
                    , _client(client)
                {
                }
                ~Remote() override = default;

            public:
                // Core::IUnknown
                BEGIN_INTERFACE_MAP(Remote)
                INTERFACE_ENTRY(Exchange::IComposition::IClient)
                END_INTERFACE_MAP

                // Core::IReferenceCounted
                uint32_t AddRef() const override
                {
                    if (Core::InterlockedIncrement(_refCount) == 1) {
                        const_cast<Client&>(_client).Announce();
                    }
                    return Core::ERROR_NONE;
                }

                uint32_t Release() const override
                {
                    if (Core::InterlockedDecrement(_refCount) == 0) {
                        // Time to say goodby, all remote clients died..
                        const_cast<Client&>(_client).Revoke();
                        return (Core::ERROR_DESTRUCTION_SUCCEEDED);
                    }
                    return Core::ERROR_NONE;
                }

                // Exchange::IComposition::IClient methods
                Thunder::Core::instance_id Native() const override
                {
                    return (_client.Native());
                }
                string Name() const override
                {
                    return _client.Name();
                }
                void Opacity(const uint32_t value) override
                {
                    _client.Opacity(value);
                }
                uint32_t Opacity() const override
                {
                    return _client.Opacity();
                }
                uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override
                {
                    return _client.Geometry(rectangle);
                }
                Exchange::IComposition::Rectangle Geometry() const override
                {
                    return _client.Geometry();
                }
                uint32_t ZOrder(const uint16_t index) override
                {
                    return _client.ZOrder(index);
                }
                uint32_t ZOrder() const override
                {
                    return _client.ZOrder();
                }

            private:
                mutable uint32_t _refCount;
                Client& _client;
            };

        public:
            Client() = delete;
            Client(Client&&) = delete;
            Client(const Client&) = delete;
            Client& operator=(Client&&) = delete;
            Client& operator=(const Client&) = delete;

            Client(CompositorImplementation& parent, const string& callsign, const uint32_t width, const uint32_t height)
                : Compositor::CompositorBuffer(width, height, parent.Format(), parent.Modifier(), Exchange::ICompositionBuffer::TYPE_DMA)
                , _parent(parent)
                , _id(Core::InterlockedIncrement(_sequence))
                , _callsign(callsign)
                , _opacity(255)
                , _zIndex(0)
                , _geometry({ 0, 0, width, height })
                , _texture()
                , _pendingOutputs(0)
                , _remoteClient(*this)
            {
                Core::ResourceMonitor::Instance().Register(*this);
            }
            ~Client() override
            {
                Core::ResourceMonitor::Instance().Unregister(*this);

                _parent.Render(*this); // request a render to remove this surface from the composition.

                TRACE(Trace::Information, (_T("Client %s[%p] destroyed"), _callsign.c_str(), this));
            }

        public:
            Exchange::IComposition::IClient* External()
            {
                _remoteClient.AddRef();
                return (&_remoteClient);
            }
            uint8_t Descriptors(const uint8_t maxSize, int container[])
            {
                return (Compositor::CompositorBuffer::Descriptors(maxSize, container));
            }

            void AttachPlanes(Core::PrivilegedRequest::Container& descriptors)
            {
                Compositor::CompositorBuffer::Planes(descriptors.data(), descriptors.size());
            }

            Core::ProxyType<Compositor::IRenderer::ITexture> Texture()
            {
                return _texture;
            }
            void Texture(Core::ProxyType<Compositor::IRenderer::ITexture> texture)
            {
                ASSERT(_texture.IsValid() ^ texture.IsValid());
                _texture = texture;
            }

            /**
             * Compositor::CompositorBuffer methods
             */
            void Request() override
            {
                _parent.Render(*this);
            }

            /**
             * Exchange::IComposition::IClient methods
             */
            Thunder::Core::instance_id Native() const override
            {
                return (_id);
            }

            string Name() const override
            {
                return _callsign;
            }
            void Opacity(const uint32_t value) override
            {
                _opacity = value;
            }
            uint32_t Opacity() const override
            {
                return _opacity;
            }
            uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override
            {
                _geometry = rectangle;
                return Core::ERROR_NONE;
            }
            Exchange::IComposition::Rectangle Geometry() const override
            {
                return _geometry;
            }
            uint32_t ZOrder(const uint16_t index) override
            {
                _zIndex = index;
                return Core::ERROR_NONE;
            }
            uint32_t ZOrder() const override
            {
                return _zIndex;
            }
            void Pending(uint8_t outputIndex)
            {
                _pendingOutputs |= (1 << outputIndex);
            }
            void Completed(uint8_t outputIndex)
            {
                _pendingOutputs &= ~(1 << outputIndex);

                if (_pendingOutputs == 0) {
                    Published();
                }
            }

            BEGIN_INTERFACE_MAP(Client)
            INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP

        private:
            void Announce()
            {
                _parent.Announce(this);
            }
            void Revoke()
            {
                if (_texture.IsValid()) {
                    _texture.Release(); // make sure to delete the texture so we are skipped by the renderer.
                }

                _parent.Revoke(this);
            }

        private:
            CompositorImplementation& _parent;
            uint32_t _id;
            const string _callsign; // the callsign of the surface
            uint32_t _opacity; // the opacity of the surface on the composition
            uint16_t _zIndex; // the z-index of the surface on the composition
            Exchange::IComposition::Rectangle _geometry; // the actual geometry of the surface on the composition
            Core::ProxyType<Compositor::IRenderer::ITexture> _texture; // the texture handle that is known in the GPU/Renderer.
            uint32_t _pendingOutputs;
            Remote _remoteClient;

            static uint32_t _sequence;
        }; // class Client

        class Bridge : public Core::PrivilegedRequest {
        private:
            class Callback : public Core::PrivilegedRequest::ICallback {
            public:
                Callback() = delete;
                Callback(Callback&&) = delete;
                Callback(const Callback&) = delete;
                Callback& operator=(Callback&&) = delete;
                Callback& operator=(const Callback&) = delete;

                Callback(Bridge& parent)
                    : _parent(parent)
                {
                }
                ~Callback() override = default;

            public:
                void Request(const uint32_t id, Container& descriptors) override
                {
                    _parent.Request(id, descriptors);
                }
                void Offer(const uint32_t id, Container&& descriptors) override
                {
                    _parent.Offer(id, std::move(descriptors));
                }

            private:
                Bridge& _parent;
            };

        public:
            Bridge() = delete;
            Bridge(Bridge&&) = delete;
            Bridge(const Bridge&) = delete;
            Bridge& operator=(Bridge&&) = delete;
            Bridge& operator=(const Bridge&) = delete;

            Bridge(CompositorImplementation& parent)
                : Core::PrivilegedRequest(&_callback)
                , _parent(parent)
                , _callback(*this)
            {
            }
            ~Bridge() override
            {
                Close();
            }

        private:
            void Request(const uint32_t id, Container& descriptors)
            {
                if (id == DisplayId) {
                    descriptors.emplace_back(static_cast<int>(_parent.Native()));
                } else {
                    Core::ProxyType<Client> client = _parent.ClientById(id);

                    if (client.IsValid() == false) {
                        TRACE(Trace::Information, (_T("Bridge for Id [%d] not found"), id));
                    } else {
                        int container[Core::PrivilegedRequest::MaxDescriptorsPerRequest];
                        uint8_t result = client->Descriptors(sizeof(container), container);

                        for (uint8_t index = 0; index < result; index++) {
                            descriptors.emplace_back(container[index]);
                        }
                    }
                }
            }
            void Offer(const uint32_t id, Core::PrivilegedRequest::Container&& descriptors)
            {
                Core::ProxyType<Client> client = _parent.ClientById(id);

                if (client.IsValid() == true) {
                    client->AttachPlanes(descriptors);

                    Core::ProxyType<Exchange::ICompositionBuffer> buffer(client);

                    client->Texture(_parent.Texture(buffer));
                } else {
                    TRACE(Trace::Information, (_T("Bridge for Id [%d] not found"), id));
                }

                descriptors.clear();
            }

            CompositorImplementation& _parent;
            Callback _callback;
        }; // class Bridge

        class Output {
        public:
            Output() = delete;
            Output(Output&&) = delete;
            Output(const Output&) = delete;
            Output& operator=(Output&&) = delete;
            Output& operator=(const Output&) = delete;

            Output(const string& name, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format)
                : _index(Core::InterlockedIncrement(_indexes))
                , _sink(*this)
                , _connector()
                , _gpuNode()
                , _pts(Core::Time::Now().Ticks())
                , _rendering()
                , _commit()
                , _pendingClients()
                , _renderingClients()

            {
                _connector = Compositor::CreateBuffer(name, rectangle, format, &_sink);
                TRACE(Trace::Information, (_T("Output %s created."), name.c_str()));
            }

            virtual ~Output()
            {
                _connector.Release();
            }

        private:
            class Sink : public Compositor::IOutput::ICallback {
            public:
                Sink(Sink&&) = delete;
                Sink(const Sink&) = delete;
                Sink& operator=(Sink&&) = delete;
                Sink& operator=(const Sink&) = delete;
                Sink() = delete;

                Sink(Output& parent)
                    : _parent(parent)
                {
                }
                ~Sink() override = default;

                virtual void Presented(const Compositor::IOutput* output, const uint32_t sequence, const uint64_t time) override
                {
                    _parent.HandleVSync(output, sequence, time);
                }

                // virtual void Display(const int fd, const std::string& node) override
                // {
                //     TRACE(Trace::Information, (_T("Connector fd %d opened on %s"), fd, node.c_str()));
                //     _parent.HandleGPUNode(node);
                // }

            private:
                Output& _parent;
            };

            void HandleVSync(const Compositor::IOutput* output VARIABLE_IS_NOT_USED, const uint32_t sequence VARIABLE_IS_NOT_USED, const uint64_t pts /*usec since epoch*/)
            {
                // float fps = 1 / ((pts - _pts) / 1000000.0f);
                _pts = pts;

                for (Core::ProxyType<Client> client : _renderingClients) {
                    client->Completed(_index);
                }

                _renderingClients.clear();

                _commit.unlock();

                // TRACE(Trace::Information, ("Connector running at %.2f fps", fps));
            }

            void HandleGPUNode(const std::string node)
            {
                if (_gpuNode.empty() == true) {
                    _gpuNode = node;
                }
            }

        public:
            const std::string& GpuNode() const
            {
                return _gpuNode;
            }

            Exchange::IComposition::Rectangle Geometry() const
            {
                return { _connector->X(), _connector->Y(), _connector->Width(), _connector->Height() };
            }

            bool IsIntersecting(const Exchange::IComposition::Rectangle& rectangle)
            {
                return (
                    std::min(_connector->X() + static_cast<int32_t>(_connector->Width()), rectangle.x + static_cast<int32_t>(rectangle.width)) > std::max(_connector->X(), rectangle.x) && // width > 0
                    std::min(_connector->Y() + static_cast<int32_t>(_connector->Height()), rectangle.y + static_cast<int32_t>(rectangle.height)) > std::max(_connector->Y(), rectangle.y)); // height > 0
            }

            bool IsValid()
            {
                return _connector.IsValid();
            }

            Core::ProxyType<Compositor::IOutput> Buffer()
            {
                return _connector;
            }

            void Pending(Core::ProxyType<Client> client)
            {
                _rendering.Lock();
                _pendingClients.emplace_back(client);
                _rendering.Unlock();
            }

            void Commit()
            {
                _commit.lock();

                _rendering.Lock();
                _renderingClients = std::move(_pendingClients);
                _rendering.Unlock();

                for (Core::ProxyType<Client> client : _renderingClients) {
                    client->Pending(_index);
                }

                _connector->Commit();
            }

        private:
            const uint8_t _index;
            Sink _sink;
            Core::ProxyType<Compositor::IOutput> _connector;
            std::string _gpuNode;
            uint64_t _pts;

            Core::CriticalSection _rendering;
            std::mutex _commit;
            std::vector<Core::ProxyType<Client>> _pendingClients;
            std::vector<Core::ProxyType<Client>> _renderingClients;

            static uint32_t _indexes;
        };

        using Clients = Core::ProxyMapType<string, Client>;
        using Observers = std::vector<Exchange::IComposition::INotification*>;
        using Outputs = std::list<Output>;

    public:
        CompositorImplementation(CompositorImplementation&&) = delete;
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(CompositorImplementation&&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        CompositorImplementation()
            : _adminLock()
            , _format(DRM_FORMAT_INVALID)
            , _modifier(DRM_FORMAT_MOD_INVALID)
            , _outputs()
            , _renderer()
            , _observers()
            , _clientBridge(*this)
            , _clients()
            , _lastFrame(0)
            , _background(pink)
            , _engine()
            , _dispatcher(nullptr)
            , _canvasBuffer()
            , _canvasTexture()
            , _gpuIdentifier(0)
            , _gpuNode()
            , _renderNode()
            , _present(*this)
        {
        }
        ~CompositorImplementation() override
        {
            _dispatcher.reset(nullptr);

            if (_engine.IsValid()) {
                _engine.Release();
            }

            _clientBridge.Close();
            _clients.Clear();
            _renderer.Release();

            if (_gpuIdentifier > 0) {
                ::close(_gpuIdentifier);
                _gpuIdentifier = -1;
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

                    _outputs.emplace_back(index.Current().Connector.Value(), rectangle, _format);

                    ASSERT(_outputs.back().IsValid());

                    TRACE(Trace::Information, ("Initialzed connector %s", index.Current().Connector.Value().c_str()));
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

            _canvasTexture = _renderer->Texture(_canvasBuffer);
            ASSERT(_canvasTexture.IsValid());

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

                    _canvasTexture.Release();
                    _canvasBuffer.Release();

                    result = Core::ERROR_UNAVAILABLE;

                    TRACE(Trace::Error, (_T("Failed to open display dispatcher %s"), connectorPath.c_str()));
                }
            } else {
                TRACE(Trace::Error, (_T("Failed to open client bridge %s error %d"), bridgePath.c_str(), result));
            }
            return (result);
        }

        void Register(Exchange::IComposition::INotification* notification) override
        {
            ASSERT(notification != nullptr);

            _adminLock.Lock();

            Observers::iterator index(std::find(_observers.begin(), _observers.end(), notification));
            ASSERT(index == _observers.end());

            if (index == _observers.end()) {
                notification->AddRef();

                _observers.push_back(notification);
                _clients.Visit(
                    [=](const string& name, const Core::ProxyType<Client>& element) {
                        notification->Attached(name, &(*element));
                    });
            }

            _adminLock.Unlock();
        }

        void Unregister(Exchange::IComposition::INotification* notification) override
        {
            ASSERT(notification != nullptr);

            _adminLock.Lock();

            Observers::iterator index(std::find(_observers.begin(), _observers.end(), notification));

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

        Thunder::Core::ProxyType<Compositor::IRenderer::ITexture> Texture(Core::ProxyType<Exchange::ICompositionBuffer> buffer)
        {

            ASSERT(buffer.IsValid());
            return _renderer->Texture(buffer);
        }

        void Announce(Exchange::IComposition::IClient* client)
        {
            _adminLock.Lock();

            for (auto& observer : _observers) {
                observer->Attached(client->Name(), client);
            }
            _adminLock.Unlock();
        }

        void Revoke(Exchange::IComposition::IClient* client)
        {
            _adminLock.Lock();

            for (auto& observer : _observers) {
                observer->Detached(client->Name());
            }

            _adminLock.Unlock();
        }

        uint32_t Format() const
        {
            return _format;
        }

        uint64_t Modifier() const
        {
            return _modifier;
        }

        /**
         * Exchange::IComposition::IDisplay methods
         */
        Thunder::Core::instance_id Native() const override
        {
            return _gpuIdentifier;
        }
        string Port() const override
        {
            return string("Mesa3d");
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

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        INTERFACE_ENTRY(Exchange::IComposition::IDisplay)
        END_INTERFACE_MAP

    private:
        void Render(Client& client)
        {
            uint8_t index = 1;
            for (auto& output : _outputs) {
                if (output.IsIntersecting(client.Geometry())) {
                    _present.Trigger(); // Only trigger a render loop if the client is actual visible on one of the outputs
                    break;
                } else {
                    TRACE(Trace::Information, (_T("%s no intersection with output %d"), client.Name().c_str(), index));
                }
                index++;
            }
        }

        IComposition::IClient* CreateClient(const string& name, const uint32_t width, const uint32_t height) override
        {
            IClient* client = nullptr;

            TRACE(Trace::Information, (_T("Create a %dx%d surface for client %s"), width, height, name.c_str()));

            Core::ProxyType<Client> object = _clients.Instance<Client>(name, *this, name, width, height);

            ASSERT(object.IsValid() == true);

            if (object.IsValid() == true) {
                client = object->External();
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

            _clients.Visit([&](const string& name, const Core::ProxyType<Client> client) {
                ASSERT(client.IsValid() == true);

                if (client->Texture() != nullptr) {
                    client->Acquire(Compositor::DefaultTimeoutMs);

                    Exchange::IComposition::Rectangle rectangle = client->Geometry();

                    const Exchange::IComposition::Rectangle renderBox = { rectangle.x, rectangle.y, rectangle.width, rectangle.height };

                    Compositor::Matrix clientProjection;
                    Compositor::Transformation::ProjectBox(clientProjection, renderBox, Compositor::Transformation::TRANSFORM_NORMAL, 0, _renderer->Projection());

                    const Exchange::IComposition::Rectangle clientArea = { 0, 0, client->Texture()->Width(), client->Texture()->Height() };

                    const float alpha = float(client->Opacity()) / float(Exchange::IComposition::maxOpacity);

                    _renderer->Render(client->Texture(), clientArea, clientProjection, alpha);

                    client->Relinquish();

                    client->Rendered();
                } else {
                    TRACE(Trace::Error, (_T("Skipping %s, no texture to render"), name.c_str()));
                }
            });

            _renderer->End(false);
            _renderer->Unbind();

            RenderOutputs();

            return Core::Time::Now().Ticks();
        }

        uint64_t RenderOutputs()
        {
            for (Output& output : _outputs) {
                ASSERT(_renderer.IsValid() == true);

                Core::ProxyType<Compositor::IOutput> buffer = output.Buffer();

                ASSERT(buffer.IsValid() == true);

                _renderer->Bind(static_cast<Core::ProxyType<Exchange::ICompositionBuffer>>(buffer));
                _renderer->Begin(buffer->Width(), buffer->Height()); // set viewport for render

                const Exchange::IComposition::Rectangle renderBox = { 0, 0, buffer->Width(), buffer->Height() };

                Compositor::Matrix canvasProjection;
                Compositor::Transformation::ProjectBox(canvasProjection, renderBox, Compositor::Transformation::TRANSFORM_NORMAL, 0, _renderer->Projection());

                // what part of the canvas do we want to render on this output
                const Exchange::IComposition::Rectangle canvasArea = {
                    0, // X
                    0, // Y
                    _canvasTexture->Width(),
                    _canvasTexture->Height()
                };

                _renderer->Render(_canvasTexture, canvasArea, canvasProjection, 1.0);

                _renderer->End(false);

                _clients.Visit([&](const string& /*name*/, const Core::ProxyType<Client> client) {
                    ASSERT(client.IsValid() == true);

                    if (output.IsIntersecting(client->Geometry())) {
                        output.Pending(client);
                    }
                });

                output.Commit(); // Blit to screen

                _renderer->Unbind();
            }

            return Core::Time::Now().Ticks();
        }

        class Presenter : public Core::Thread {
        public:
            Presenter(Presenter&&) = delete;
            Presenter(const Presenter&) = delete;
            Presenter& operator=(Presenter&&) = delete;
            Presenter& operator=(const Presenter&) = delete;

            Presenter(CompositorImplementation& parent)
                : Core::Thread(Thread::DefaultStackSize(), "CompositorPresenter")
                , _parent(parent)
            {
            }

            ~Presenter() = default;

            uint32_t Trigger()
            {
                uint32_t result(Core::ERROR_NONE);

                if (Thread::IsRunning() == true) {
                    _continue = true;
                    result = Core::ERROR_INPROGRESS;
                } else {
                    Thread::Run();
                }

                return result;
            }

        private:
            uint32_t Worker() override
            {
                uint32_t delay(Core::infinite);

                _continue = false;

                _parent.RenderCanvas();

                if (_continue == false) {
                    Core::Thread::Block();
                }

                return delay;
            }

        private:
            CompositorImplementation& _parent;
            std::atomic<bool> _continue;
        };

    private:
        Core::ProxyType<Client> ClientById(const uint32_t id)
        {
            Core::ProxyType<Client> result;

            _clients.Visit([&](const string& /* name */, const Core::ProxyType<Client>& element) {
                if (id == element->Native()) {
                    ASSERT(result.IsValid() == false);
                    result = element;
                }
            });

            return (result);
        }

    private:
        mutable Core::CriticalSection _adminLock;
        uint32_t _format;
        uint64_t _modifier;
        Outputs _outputs;
        Core::ProxyType<Compositor::IRenderer> _renderer;
        Observers _observers;
        Bridge _clientBridge;
        Clients _clients;
        uint64_t _lastFrame;
        Compositor::Color _background;
        Core::ProxyType<RPC::InvokeServer> _engine;
        std::unique_ptr<DisplayDispatcher> _dispatcher;
        Core::ProxyType<Exchange::ICompositionBuffer> _canvasBuffer;
        Core::ProxyType<Compositor::IRenderer::ITexture> _canvasTexture;
        int _gpuIdentifier;
        std::string _gpuNode;
        std::string _renderNode;
        Presenter _present;
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0)

    uint32_t CompositorImplementation::Output::_indexes = 0;
    uint32_t CompositorImplementation::Client::_sequence = 1;

} // namespace Plugin
} // namespace Thunder
