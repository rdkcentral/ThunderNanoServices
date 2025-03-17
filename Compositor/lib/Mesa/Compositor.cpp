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
            class Output : public Core::JSON::Container {
            public:
                Output& operator=(Output&&) = delete;
                Output& operator=(const Output&) = delete;

                Output()
                    : Core::JSON::Container()
                    , Connector("HDMI-A-1")
                    , X(0)
                    , Y(0)
                    , Height(0)
                    , Width(0)
                {
                    Init();
                }
                Output(Output&& other)
                    : Core::JSON::Container()
                    , Connector(std::move(other.Connector))
                    , X(std::move(other.X))
                    , Y(std::move(other.Y))
                    , Height(std::move(other.Height))
                    , Width(std::move(other.Width))
                {
                    Init();
                }
                Output(const Output& other)
                    : Core::JSON::Container()
                    , Connector(other.Connector)
                    , X(other.X)
                    , Y(other.Y)
                    , Height(other.Height)
                    , Width(other.Width)
                {
                    Init();
                }
                ~Output() override = default;

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
                , Render()
                , Height(0)
                , Width(0)
                , Format(DRM_FORMAT_ARGB8888)
                , Modifier(DRM_FORMAT_MOD_LINEAR)
                , Out()
            {
                Add(_T("render"), &Render);
                Add(_T("height"), &Height);
                Add(_T("width"), &Width);
                Add(_T("format"), &Format);
                Add(_T("modifier"), &Modifier);
                Add(_T("output"), &Out);
            }

            ~Config() override = default;

            Core::JSON::String Render;
            Core::JSON::DecUInt16 Height;
            Core::JSON::DecUInt16 Width;
            Core::JSON::HexUInt32 Format;
            Core::JSON::HexUInt64 Modifier;
            Output Out;
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

            class Remote : public Exchange::IComposition::IClient {
            public:
                Remote() = delete;
                Remote(Remote&&) = delete;
                Remote(const Remote&) = delete;
                Remote& operator=(Remote&&) = delete;
                Remote& operator=(const Remote&) = delete;

                Remote(Client& client)
                    : _client(client)
                    , _refCount(0) {
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
                Client& _client;
                mutable uint32_t _refCount;
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
                , _pending(false)
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
            Exchange::IComposition::IClient* External() {
                _remoteClient.AddRef();
                return (&_remoteClient);
            }
            uint8_t Descriptors(const uint8_t maxSize, int container[]) {
                return (BaseClass::Descriptors(maxSize, container));
            }
            void AttachPlanes(Core::PrivilegedRequest::Container& descriptors) {
                BaseClass::Planes(descriptors.data(), descriptors.size());
            }
            Core::ProxyType<Compositor::IRenderer::ITexture> Texture() {
                return _texture;
            }

            /**
             * Compositor::CompositorBuffer methods
             */
            void Request() override {
                _parent.Render(*this);
                _pending.store(true);
            }

            /**
             * Exchange::IComposition::IClient methods
             */
            Thunder::Core::instance_id Native() const override {
                return (_id);
            }
            string Name() const override {
                return _callsign;
            }
            void Opacity(const uint32_t value) override {
                _opacity = value;
            }
            uint32_t Opacity() const override {
                return _opacity;
            }
            uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override {
                _geometry = rectangle;
                return Core::ERROR_NONE;
            }
            Exchange::IComposition::Rectangle Geometry() const override {
                return _geometry;
            }
            uint32_t ZOrder(const uint16_t index) override {
                _zIndex = index;
                return Core::ERROR_NONE;
            }
            uint32_t ZOrder() const override {
                return _zIndex;
            }
            void Completed() {
                bool expected = true;
                if (_pending.compare_exchange_strong(expected, false) == true) {
                    Published();
                }
            }

            BEGIN_INTERFACE_MAP(Client)
            INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP

        private:
            void Announce() {
                _parent.Announce(this);
            }
            void Revoke() {

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
            std::atomic<bool> _pending;

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

                    // client->Texture(_parent.Texture(buffer));
                    _parent.Texture(buffer);
                } else {
                    TRACE(Trace::Information, (_T("Bridge for Id [%d] not found"), id));
                }

                descriptors.clear();
            }

            CompositorImplementation& _parent;
            Callback _callback;
        }; // class Bridge

        class Output {
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
                    , _pts(Core::Time::Now().Ticks()) {
                }
                ~Sink() override = default;

            public:
                virtual void Presented(const Compositor::IOutput* /* output */, const uint32_t sequence, const uint64_t time) override {
                    _parent.VSync(sequence, time);
                }

            private:
                CompositorImplementation& _parent;
                uint64_t _pts;
            };

        public:
            Output() = delete;
            Output(Output&&) = delete;
            Output(const Output&) = delete;
            Output& operator=(Output&&) = delete;
            Output& operator=(const Output&) = delete;

            Output(CompositorImplementation& parent, const string& name, const uint32_t width, const uint32_t height, const Compositor::PixelFormat& format, Core::ProxyType<Compositor::IRenderer> renderer)
                : _sink(parent)
                , _connector(Compositor::CreateBuffer(name, width, height, format, renderer, &_sink)) {
                TRACE(Trace::Information, (_T("Output %s created."), name.c_str()));
            }
            ~Output() {
                _connector.Release();
            }

        public:
           bool IsValid() {
                return _connector.IsValid();
            }
            uint32_t Width() const {
                return(_connector->Width());
            }
            uint32_t Height() const {
                return(_connector->Height());
            }
            Core::ProxyType<Exchange::ICompositionBuffer> Buffer() {
                return Core::ProxyType<Exchange::ICompositionBuffer>(_connector);
            }
            int Descriptor() {
                return (_connector->Backend()->Descriptor());
            }
            void Commit() {
                _connector->Commit();
            }
            Core::ProxyType<Compositor::IRenderer::ITexture> Texture() {
                return (_connector->Texture());
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
            , _X(0)
            , _Y(0)
            , _format(DRM_FORMAT_INVALID)
            , _modifier(DRM_FORMAT_MOD_INVALID)
            , _output(nullptr)
            , _renderer()
            , _observers()
            , _clientBridge(*this)
            , _clients()
            , _lastFrame(0)
            , _background(pink)
            , _engine()
            , _dispatcher(nullptr)
            , _renderNode()
            , _renderDescriptor(-1)
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

            _clientBridge.Close();
            _clients.Clear();
            _renderer.Release();

            if (_output != nullptr) {
                delete _output;
                _output = nullptr;
            }
        }

    public:
        /**
         * Exchange::IComposition methods
         */
        uint32_t Configure(PluginHost::IShell* service) override {
            uint32_t result = Core::ERROR_NONE;

            string configuration(service->ConfigLine());
            Config config;

            config.FromString(service->ConfigLine());

            _format = config.Format.Value();
            _modifier = config.Modifier.Value();

            // FIXME: this needs to be moved to a plugin and the the buffer should be shared via
            //        a CompositorBuffer. For now embed the outputs in the compositor.
            if (config.Out.IsSet() == false) {
                TRACE(Trace::Error, (_T("No connector defined. Please define an output.")));
                result = Core::ERROR_INCOMPLETE_CONFIG;
            }
            else {
                Exchange::IComposition::Rectangle rectangle;

                rectangle.x = config.Out.X.Value();
                rectangle.y = config.Out.Y.Value();
                rectangle.width = config.Out.Width.Value();
                rectangle.height = config.Out.Height.Value();

                _renderDescriptor = ::open(config.Render.Value().c_str(), O_RDWR | O_CLOEXEC);
                TRACE(Trace::Information, ("Opening Render node %s", config.Render.Value().c_str()));

                if (_renderDescriptor < 0) {
                    result = Core::ERROR_INVALID_DESIGNATOR;
                }
                else {
                    string comPath; 

                    Core::SystemInfo::GetEnvironment(_T("XDG_RUNTIME_DIR"), comPath);
                    comPath = Core::Directory::Normalize(comPath);

                    _renderer = Compositor::IRenderer::Instance(_renderDescriptor);
                    ASSERT(_renderer.IsValid());

                    _renderer->ViewPort(rectangle.width, rectangle.height);

                    _output = new Output(*this, config.Out.Connector.Value(), rectangle.width, rectangle.height, _format, _renderer);
                    ASSERT((_output != nullptr) && (_output->IsValid()));

                    TRACE(Trace::Information, ("Initialzed connector %s", config.Out.Connector.Value().c_str()));

                    result = _clientBridge.Open(comPath + _T("comrpc"));

                    if (result != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("Failed to open client bridge %s error %d"), (comPath + _T("comrpc")).c_str(), result));
                    }
                    else {
                        ASSERT(_dispatcher == nullptr);

                        _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());

                        _dispatcher = new DisplayDispatcher(Core::NodeId((comPath+_T("descriptor")).c_str()), service->ProxyStubPath(), this, _engine);

                        if (_dispatcher->IsListening() == false) {
                            delete _dispatcher;
                            _dispatcher = nullptr;

                            _engine.Release();

                            result = Core::ERROR_UNAVAILABLE;
    
                            TRACE(Trace::Error, (_T("Failed to open display dispatcher %s"), (comPath+_T("descriptor")).c_str()));    
                        }
                        else {
                            PluginHost::ISubSystem* subSystems = service->SubSystems();

                            ASSERT(subSystems != nullptr);

                            if (subSystems != nullptr) {
                                subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                                subSystems->Release();
                            }

                            TRACE(Trace::Information, (_T("PID %d Compositor configured, compath: %s"), getpid(), comPath.c_str()));
                        }
                    }
                }
            } 
            return (result);
        }
        void Register(Exchange::IComposition::INotification* notification) override {
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
        void Unregister(Exchange::IComposition::INotification* notification) override {
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
        Thunder::Core::ProxyType<Compositor::IRenderer::ITexture> Texture(Core::ProxyType<Exchange::ICompositionBuffer> buffer) {
            ASSERT(buffer.IsValid());
            return _renderer->Texture(buffer);
        }
        void Announce(Exchange::IComposition::IClient* client) {
            _adminLock.Lock();

            for (auto& observer : _observers) {
                observer->Attached(client->Name(), client);
            }
            _adminLock.Unlock();
        }
        void Revoke(Exchange::IComposition::IClient* client) {
            _adminLock.Lock();

            for (auto& observer : _observers) {
                observer->Detached(client->Name());
            }

            _adminLock.Unlock();
        }
        uint32_t Format() const {
            return _format;
        }
        uint64_t Modifier() const {
            return _modifier;
        }

        /**
         * Exchange::IComposition::IDisplay methods
         */
        Thunder::Core::instance_id Native() const override {
            // Hier wordt de render node descriptor terug gegeven..
            return _renderDescriptor;
        }
        string Port() const override {
            return string("Mesa3d");
        }
        // Useless Resolution functions, this should be controlled by DisplayControl
        uint32_t Resolution(const Exchange::IComposition::ScreenResolution format VARIABLE_IS_NOT_USED) override {
            return (Core::ERROR_NONE);
        }
        Exchange::IComposition::ScreenResolution Resolution() const override {

  fprintf(stdout, "-------------------- %s ------------------------ %d -------------------\n", __FUNCTION__, __LINE__); fflush(stdout);
            _clients.Visit([&](const string& name, const Core::ProxyType<Client>& element) {
  fprintf(stdout, "Rendering: %s\n", name.c_str()); fflush(stdout);
                const_cast<CompositorImplementation*>(this)->Render(*element);
            });

            return Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;
        }

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        INTERFACE_ENTRY(Exchange::IComposition::IDisplay)
        END_INTERFACE_MAP

    private:
        bool IsIntersecting(const Exchange::IComposition::Rectangle& rectangle) {
            return (
                std::min(_X + static_cast<int32_t>(_output->Width()), rectangle.x + static_cast<int32_t>(rectangle.width)) > std::max(_X, rectangle.x) && // width > 0
                std::min(_Y + static_cast<int32_t>(_output->Height()), rectangle.y + static_cast<int32_t>(rectangle.height)) > std::max(_Y, rectangle.y)); // height > 0
        }
 
        void Render(Client& client)
        {
            uint8_t index = 1;
            if (IsIntersecting(client.Geometry())) {
                if (client.Texture() == nullptr) {
                    TRACE(Trace::Error, (_T("Skipping %s, no texture to render"), client.Name().c_str()));
                }
                else {
                    client.Acquire(Compositor::DefaultTimeoutMs);

                    Compositor::Matrix clientProjection;
                    Compositor::Transformation::ProjectBox(clientProjection, client.Geometry(), Compositor::Transformation::TRANSFORM_NORMAL, 0, _renderer->Projection());

                    const Exchange::IComposition::Rectangle clientArea = { 0, 0, client.Texture()->Width(), client.Texture()->Height() };

                    const float alpha = float(client.Opacity()) / float(Exchange::IComposition::maxOpacity);

                    _renderer->Render(_output->Texture(), client.Texture(), clientArea, clientProjection, alpha);

                    client.Relinquish();
                    client.Rendered();
                    _output->Commit();
                }
            } else {
                TRACE(Trace::Information, (_T("%s no intersection with output %d"), client.Name().c_str(), index));
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

    private:
        void VSync(const uint32_t /* sequence */, const uint64_t /* pts */) {
            _clients.Visit([&](const string& /* name */, const Core::ProxyType<Client>& element) {
                element->Completed();
            });
        }
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
        int32_t _X;
        int32_t _Y;
        uint32_t _format;
        uint64_t _modifier;
        Output* _output;
        Core::ProxyType<Compositor::IRenderer> _renderer;
        Observers _observers;
        Bridge _clientBridge;
        Clients _clients;
        uint64_t _lastFrame;
        Compositor::Color _background;
        Core::ProxyType<RPC::InvokeServer> _engine;
        DisplayDispatcher* _dispatcher;
        std::string _renderNode;
        int _renderDescriptor;
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0)

    uint32_t CompositorImplementation::Client::_sequence = 1;

} // namespace Plugin
} // namespace Thunder
