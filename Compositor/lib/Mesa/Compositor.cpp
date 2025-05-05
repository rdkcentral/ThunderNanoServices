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
#include <interfaces/IGraphicsBuffer.h>

#include <IBuffer.h>
#include <IOutput.h>
#include <IRenderer.h>
#include <Transformation.h>

#include <graphicsbuffer/GraphicsBufferType.h>

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
            Config(Config&&) = delete;
            Config(const Config&) = delete;
            Config& operator=(Config&&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , Render()
                , Height(0)
                , Width(0)
                , Format(DRM_FORMAT_ARGB8888)
                , Modifier(DRM_FORMAT_MOD_LINEAR)
                , Output()
                , AutoScale(true)
            {
                Add(_T("render"), &Render);
                Add(_T("height"), &Height);
                Add(_T("width"), &Width);
                Add(_T("format"), &Format);
                Add(_T("modifier"), &Modifier);
                Add(_T("output"), &Output);
                Add(_T("autoscale"), &AutoScale);
            }

            ~Config() override = default;

            Core::JSON::String Render;
            Core::JSON::DecUInt16 Height;
            Core::JSON::DecUInt16 Width;
            Core::JSON::HexUInt32 Format;
            Core::JSON::HexUInt64 Modifier;
            Core::JSON::String Output;
            Core::JSON::Boolean AutoScale;
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

        class Client : public Exchange::IComposition::IClient, public Graphics::ServerBufferType<1> {
        private:
            using BaseClass = Graphics::ServerBufferType<1>;

            enum class State : uint8_t {
                IDLE = 0x00,
                RENDERING = 0x01,
                PRESENTING = 0x02,
            };

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
                : BaseClass(width, height, parent.Format(), parent.Modifier(), Exchange::IGraphicsBuffer::TYPE_DMA)
                , _parent(parent)
                , _id(Core::InterlockedIncrement(_sequence))
                , _callsign(callsign)
                , _opacity(255)
                , _zIndex(0)
                , _geometry({ 0, 0, width, height })
                , _texture()
                , _remoteClient(*this)
                , _state(State::IDLE)
                , _geometryChanged(false)
            {
                Core::ResourceMonitor::Instance().Register(*this);
            }
            ~Client() override
            {
                Core::ResourceMonitor::Instance().Unregister(*this);

                _parent.Render(); // request a render to remove this surface from the composition.

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
                return (BaseClass::Descriptors(maxSize, container));
            }

            void AttachPlanes(Core::PrivilegedRequest::Container& descriptors)
            {
                BaseClass::Planes(descriptors.data(), descriptors.size());
            }

            Core::ProxyType<Compositor::IRenderer::ITexture> Texture()
            {
                return _texture;
            }
            void Texture(const Core::ProxyType<Compositor::IRenderer::ITexture>& texture)
            {
                _texture = texture;
            }

            /**
             * Compositor::CompositorBuffer methods
             */
            void Request() override
            {
                State expected(State::IDLE);

                if (_state.compare_exchange_strong(expected, State::RENDERING) == true) {
                    _parent.Render(); // Trigger rendering operations
                } else {
                    TRACE(Trace::Error, (_T("Surface %s[%d] is not idle, but received a Request event  while in %d"), _callsign.c_str(), _id, _state.load()));
                    ASSERT(false);
                }
            }

            void Pending()
            {
                State expected(State::RENDERING);

                if (_state.compare_exchange_strong(expected, State::PRESENTING) == true) {
                    Rendered();
                } else {
                    TRACE(Trace::Error, (_T("Surface %s[%d] is not rendering, but received a Pending event while in %d"), _callsign.c_str(), _id, _state.load()));
                }
            }

            void Completed()
            {
                State expected(State::PRESENTING);

                if (_state.compare_exchange_strong(expected, State::IDLE) == true) {
                    Published();
                }
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
                _parent.Render();
            }
            uint32_t Opacity() const override
            {
                return _opacity;
            }
            uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override
            {
                _geometry = rectangle;
                _geometryChanged = true;
                _parent.Render();
                return Core::ERROR_NONE;
            }

            bool GeometryChanged() const
            {
                return _geometryChanged;
            }

            Exchange::IComposition::Rectangle Geometry() const override
            {
                return _geometry;
            }
            uint32_t ZOrder(const uint16_t index) override
            {
                _zIndex = index;
                _parent.Render();
                return Core::ERROR_NONE;
            }
            uint32_t ZOrder() const override
            {
                return _zIndex;
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
            Remote _remoteClient;
            std::atomic<State> _state;
            bool _geometryChanged = false;

            static uint32_t _sequence;
        }; // class Client

        class DescriptorExchange : public Core::PrivilegedRequest {
        private:
            class Callback : public Core::PrivilegedRequest::ICallback {
            public:
                Callback() = delete;
                Callback(Callback&&) = delete;
                Callback(const Callback&) = delete;
                Callback& operator=(Callback&&) = delete;
                Callback& operator=(const Callback&) = delete;

                Callback(DescriptorExchange& parent)
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
                DescriptorExchange& _parent;
            };

        public:
            DescriptorExchange() = delete;
            DescriptorExchange(DescriptorExchange&&) = delete;
            DescriptorExchange(const DescriptorExchange&) = delete;
            DescriptorExchange& operator=(DescriptorExchange&&) = delete;
            DescriptorExchange& operator=(const DescriptorExchange&) = delete;

            DescriptorExchange(CompositorImplementation& parent)
                : Core::PrivilegedRequest(&_callback)
                , _parent(parent)
                , _callback(*this)
            {
            }
            ~DescriptorExchange() override
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
                        TRACE(Trace::Information, (_T("DescriptorExchange for Id [%d] not found"), id));
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

                    Core::ProxyType<Exchange::IGraphicsBuffer> buffer(client);

                    client->Texture(_parent.Texture(buffer));
                } else {
                    TRACE(Trace::Information, (_T("DescriptorExchange Id [%d] not found"), id));
                }

                descriptors.clear();
            }

            CompositorImplementation& _parent;
            Callback _callback;
        }; // class DescriptorExchange

        class Output {
        public:
            Output() = delete;
            Output(Output&&) = delete;
            Output(const Output&) = delete;
            Output& operator=(Output&&) = delete;
            Output& operator=(const Output&) = delete;

            Output(CompositorImplementation& parent, const string& name, const uint32_t width, const uint32_t height, const Compositor::PixelFormat& format, const Core::ProxyType<Compositor::IRenderer>& renderer)
                : _sink(parent)
                , _connector()

            {
                _connector = Compositor::CreateBuffer(name, width, height, format, renderer, &_sink);
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

                Sink(CompositorImplementation& parent)
                    : _parent(parent)
                {
                }
                ~Sink() override = default;

                virtual void Presented(const Compositor::IOutput* output, const uint32_t sequence, const uint64_t time) override
                {
                    _parent.VSync(output, sequence, time);
                }

            private:
                CompositorImplementation& _parent;
            };

        public:
            uint32_t Width() const
            {
                return (_connector->Width());
            }
            uint32_t Height() const
            {
                return (_connector->Height());
            }
            bool IsValid()
            {
                return _connector.IsValid();
            }

            Core::ProxyType<Compositor::IOutput> Buffer()
            {
                return _connector;
            }

            void Commit()
            {
                _connector->Commit();
            }

        private:
            Sink _sink;
            Core::ProxyType<Compositor::IOutput> _connector;
        };

        using Clients = Core::ProxyMapType<string, Client>;
        using Observers = std::vector<Exchange::IComposition::INotification*>;

    public:
        CompositorImplementation(CompositorImplementation&&) = delete;
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(CompositorImplementation&&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        CompositorImplementation()
            : _adminLock()
            , _format(DRM_FORMAT_INVALID)
            , _modifier(DRM_FORMAT_MOD_INVALID)
            , _output(nullptr)
            , _renderer()
            , _observers()
            , _descriptorExchange(*this)
            , _clientLock()
            , _clients()
            , _lastFrame(0)
            , _background(pink)
            , _engine()
            , _dispatcher(nullptr)
            , _renderDescriptor(Compositor::InvalidFileDescriptor)
            , _renderNode()
            , _present(*this)
            , _state(State::IDLE)
            , _autoScale(true)
        {
        }
        ~CompositorImplementation() override
        {
            if (_dispatcher != nullptr) {
                delete _dispatcher;
                _dispatcher = nullptr;
            }

            if (_engine.IsValid()) {
                _engine.Release();
            }

            _descriptorExchange.Close();
            _clients.Clear();
            _renderer.Release();

            if (_output != nullptr) {
                delete _output;
                _output = nullptr;
            }

            if (_renderDescriptor != Compositor::InvalidFileDescriptor) {
                ::close(_renderDescriptor);
                _renderDescriptor = Compositor::InvalidFileDescriptor;
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

            if ((config.Render.IsSet() == true) && (config.Render.Value().empty() == false)) {
                _renderDescriptor = ::open(config.Render.Value().c_str(), O_RDWR | O_CLOEXEC);
            } else {
                return Core::ERROR_INCOMPLETE_CONFIG;
            }

            ASSERT(_renderDescriptor != Compositor::InvalidFileDescriptor);

            _renderer = Compositor::IRenderer::Instance(_renderDescriptor);
            ASSERT(_renderer.IsValid());

            if(config.AutoScale.IsSet() == true) {
                _autoScale = config.AutoScale.Value();
            } 
            
            if (config.Output.IsSet() == false) {
                return Core::ERROR_INCOMPLETE_CONFIG;
            } else {
                _output = new Output(*this, config.Output.Value(), config.Width.Value(), config.Height.Value(), _format, _renderer);
                ASSERT((_output != nullptr) && (_output->IsValid()));

                TRACE(Trace::Information, ("Initialzed connector %s", config.Output.Value().c_str()));
            }

            RenderOutput(); // guarantee that the output is rendered once.

            string basePath;
            Core::SystemInfo::GetEnvironment(_T("XDG_RUNTIME_DIR"), basePath);
            basePath = Core::Directory::Normalize(basePath);

            const string bridgePath = basePath + _T("descriptors");
            result = _descriptorExchange.Open(bridgePath);

            if (result == Core::ERROR_NONE) {
                const string comrpcPath = basePath + _T("comrpc");

                ASSERT(_dispatcher == nullptr);

                _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());

                _dispatcher = new DisplayDispatcher(Core::NodeId(comrpcPath.c_str()), service->ProxyStubPath(), this, _engine);

                if (_dispatcher->IsListening() == true) {
                    PluginHost::ISubSystem* subSystems = service->SubSystems();

                    ASSERT(subSystems != nullptr);

                    if (subSystems != nullptr) {
                        subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                        subSystems->Release();
                    }

                    TRACE(Trace::Information, (_T("PID %d Compositor configured, communicator: %s, bridge: %s"), getpid(), _dispatcher->Connector().c_str(), bridgePath.c_str()));
                } else {
                    delete _dispatcher;
                    _dispatcher = nullptr;

                    _engine.Release();
                    _descriptorExchange.Close();

                    result = Core::ERROR_UNAVAILABLE;

                    TRACE(Trace::Error, (_T("Failed to open display dispatcher %s"), comrpcPath.c_str()));
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

        Thunder::Core::ProxyType<Compositor::IRenderer::ITexture> Texture(Core::ProxyType<Exchange::IGraphicsBuffer> buffer)
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
            return _renderDescriptor;
        }
        string Port() const override
        {
            return string("Mesa3d");
        }
        // Useless Resolution functions, this should be controlled by DisplayControl
        uint32_t Resolution(const Exchange::IComposition::ScreenResolution format VARIABLE_IS_NOT_USED) override
        {
            return (Core::ERROR_NONE);
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
        enum class State : uint8_t {
            IDLE = 0x00,
            PRESENTING = 0x01,
            PENDING = 0x02,
        };

        void VSync(const Compositor::IOutput* output VARIABLE_IS_NOT_USED, const uint32_t sequence VARIABLE_IS_NOT_USED, const uint64_t pts VARIABLE_IS_NOT_USED /*usec since epoch*/)
        {
            {
                std::lock_guard<std::mutex> lock(_clientLock);
                _clients.Visit([&](const string& /*name*/, const Core::ProxyType<Client> client) {
                    client->Completed();
                });
            }

            State expected = State::PRESENTING;

            if (_state.compare_exchange_strong(expected, State::IDLE) == false) {
                expected = State::PENDING;

                if (_state.compare_exchange_strong(expected, State::PRESENTING) == true) {
                    _present.Trigger();
                }
            }
        }

        void Render()
        {
            State expected = State::PRESENTING;

            if (_state.compare_exchange_strong(expected, State::PENDING) == false) {
                expected = State::IDLE;

                if (_state.compare_exchange_strong(expected, State::PRESENTING) == true) {
                    _present.Trigger();
                }
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

        uint64_t RenderOutput() /* 3000uS rpi4*/
        {
            ASSERT(_output != nullptr);
            ASSERT(_output->IsValid() == true);
            ASSERT(_renderer.IsValid() == true);

            Core::ProxyType<Compositor::IOutput> buffer = _output->Buffer();

            ASSERT(buffer.IsValid() == true);

            Core::ProxyType<Compositor::IRenderer::IFrameBuffer> frameBuffer = buffer->FrameBuffer();

            _renderer->Bind(frameBuffer);

            _renderer->Begin(buffer->Width(), buffer->Height()); // set viewport for render

            _renderer->Clear(_background);

            {
                std::lock_guard<std::mutex> lock(_clientLock);
                _clients.Visit([&](const string& /*name*/, const Core::ProxyType<Client> client) {
                    RenderClient(client); // ~500-900 uS rpi4
                });
            }

            _renderer->End(false);

            _renderer->Unbind(frameBuffer);

            _output->Commit(); // Blit to screen

            return Core::Time::Now().Ticks();
        }

        void RenderClient(const Core::ProxyType<Client> client)
        {
            ASSERT(client.IsValid() == true);

            if (client->Texture() != nullptr) {
                client->Acquire(Compositor::DefaultTimeoutMs);

                Exchange::IComposition::Rectangle renderBox;

                if ((_autoScale == true) && (client->GeometryChanged() == false)) {
                    renderBox.x = 0;
                    renderBox.y = 0;
                    renderBox.width = _output->Width();
                    renderBox.height = _output->Height();
                } else {
                    renderBox = client->Geometry();
                }

                Compositor::Matrix clientProjection;
                Compositor::Transformation::ProjectBox(clientProjection, renderBox, Compositor::Transformation::TRANSFORM_FLIPPED_180, 0, _renderer->Projection());

                const Exchange::IComposition::Rectangle clientArea = { 0, 0, client->Texture()->Width(), client->Texture()->Height() };

                const float alpha = float(client->Opacity()) / float(Exchange::IComposition::maxOpacity);

                _renderer->Render(client->Texture(), clientArea, clientProjection, alpha);

                client->Relinquish();

                client->Pending();
            } else {
                TRACE(Trace::Error, (_T("Skipping %s, no texture to render"), client->Name().c_str()));
            }
        }

        class Presenter : public Core::Thread {
        public:
            Presenter(Presenter&&) = delete;
            Presenter(const Presenter&) = delete;
            Presenter& operator=(Presenter&&) = delete;
            Presenter& operator=(const Presenter&) = delete;

            static constexpr const char* PresenterThreadName = "CompositorPresenter";

            Presenter(CompositorImplementation& parent)
                : Core::Thread(Thread::DefaultStackSize(), PresenterThreadName)
                , _parent(parent)
                , _continue(false)
            {
            }

            ~Presenter() override
            {
                Core::Thread::Stop();

                Core::Thread::Wait(Core::Thread::STOPPED, 100);
            }

            void Trigger()
            {
                Thread::Run();
            }

        private:
            uint32_t Worker() override
            {
                Core::Thread::Block();

                _parent.RenderOutput(); // 3000us

                return Core::infinite;
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
        Output* _output;
        Core::ProxyType<Compositor::IRenderer> _renderer;
        Observers _observers;
        DescriptorExchange _descriptorExchange;
        std::mutex _clientLock;
        Clients _clients;
        uint64_t _lastFrame;
        Compositor::Color _background;
        Core::ProxyType<RPC::InvokeServer> _engine;
        DisplayDispatcher* _dispatcher;
        int _renderDescriptor;
        std::string _renderNode;
        Presenter _present;
        std::atomic<State> _state;
        bool _autoScale;
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0)

    uint32_t CompositorImplementation::Client::_sequence = 1;
} // namespace Plugin
} // namespace Thunder
