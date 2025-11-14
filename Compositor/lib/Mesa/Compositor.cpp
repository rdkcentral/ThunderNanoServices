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

#include <CompositorUtils.h>

#include <drm_fourcc.h>

#include <condition_variable>
#include <mutex>
#include <sstream>
#include <iomanip>

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
                , Resolution(Exchange::IDeviceVideoCapabilities::ScreenResolution::ScreenResolution_Unknown) // Auto-detect
                , Format(DRM_FORMAT_INVALID) // auto select
                , Modifier(DRM_FORMAT_MOD_INVALID) // auto select
                , Output()
                , AutoScale(true)
            {
                Add(_T("render"), &Render);
                Add(_T("resolution"), &Resolution);
                Add(_T("format"), &Format);
                Add(_T("modifier"), &Modifier);
                Add(_T("output"), &Output);
                Add(_T("autoscale"), &AutoScale);
            }

            ~Config() override = default;

            Core::JSON::String Render;
            Core::JSON::EnumType<Exchange::IDeviceVideoCapabilities::ScreenResolution> Resolution;
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
                Open(Core::infinite);
            }
            ~DisplayDispatcher() override
            {
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

        class PrivilegedRequestCallback : public Core::PrivilegedRequest::ICallback {
        public:
            PrivilegedRequestCallback() = delete;
            PrivilegedRequestCallback(PrivilegedRequestCallback&&) = delete;
            PrivilegedRequestCallback(const PrivilegedRequestCallback&) = delete;
            PrivilegedRequestCallback& operator=(PrivilegedRequestCallback&&) = delete;
            PrivilegedRequestCallback& operator=(const PrivilegedRequestCallback&) = delete;

            PrivilegedRequestCallback(CompositorImplementation& parent)
                : _parent(parent)
            {
            }
            ~PrivilegedRequestCallback() override { }

        public:
            void Request(const uint32_t id, Core::PrivilegedRequest::Container& descriptors) override
            {
                if (id == DisplayId) {
                    descriptors.emplace_back(static_cast<int>(_parent.Native()));
                } else {
                    TRACE(Trace::Error, (_T("No sharable buffers for id %d"), id));
                }
            }
            void Offer(const uint32_t id, Core::PrivilegedRequest::Container&& descriptors) override
            {
                Core::ProxyType<Client> client = _parent.ClientById(id);

                if (client.IsValid() == true) {
                    uint32_t result = client->AttachBuffer(descriptors);
                    TRACE(Trace::Information, (_T("Client Id [%d] buffer attached, result:%d"), id, result));
                } else {
                    TRACE(Trace::Information, (_T("Client Id [%d] not found"), id));
                }

                descriptors.clear();
            }

        private:
            CompositorImplementation& _parent;
        };

        class Client : public Exchange::IComposition::IClient {
        public:
            constexpr static uint8_t InvalidBufferId = uint8_t(~0);

            constexpr static uint32_t MaxClientBuffers = 4;
            struct BufferStats {
                std::atomic<uint64_t> requestCount; // Times requested
                std::atomic<uint64_t> renderCount; // Times rendered
                std::atomic<uint64_t> skipCount; // Times skipped
                std::atomic<uint64_t> totalAge; // Sum of ages (µs)
                std::atomic<uint64_t> maxAge; // Max age seen (µs)
                std::atomic<uint64_t> lastUsed; // Timestamp last used

                BufferStats()
                    : requestCount(0)
                    , renderCount(0)
                    , skipCount(0)
                    , totalAge(0)
                    , maxAge(0)
                    , lastUsed(0)
                {
                }

                uint64_t AvgAge() const
                {
                    uint64_t count = renderCount.load();
                    return count > 0 ? totalAge.load() / count : 0;
                }
            };

            struct Stats {
                // Overall stats
                std::atomic<uint64_t> renderCount;
                std::atomic<uint64_t> totalRenderTime;
                std::atomic<uint64_t> skipCount;

                // Per-buffer detailed stats
                std::array<BufferStats, MaxClientBuffers> buffers;

                // Queue stats
                std::atomic<uint64_t> queueFullCount;
                std::atomic<uint64_t> queueMaxSize; // Max queue depth seen

                // Current state
                std::atomic<uint8_t> currentBufferId;
                std::atomic<uint64_t> lastRenderTime;

                Stats()
                    : renderCount(0)
                    , totalRenderTime(0)
                    , skipCount(0)
                    , buffers {} // Zero-init array
                    , queueFullCount(0)
                    , queueMaxSize(0)
                    , currentBufferId(0xFF) // InvalidBufferId
                    , lastRenderTime(0)
                {
                }

                string ToString() const
                {
                    std::ostringstream ss;

                    uint64_t renders = renderCount.load();
                    uint64_t skips = skipCount.load();
                    uint64_t total = renders + skips;
                    ss << "Overall:\n";
                    ss << "  Renders:     " << renders << "\n";
                    ss << "  Skips:       " << skips;
                    if (total > 0) {
                        ss << " (" << std::fixed << std::setprecision(1) << (100.0 * skips / total) << "%)\n";
                    } else {
                        ss << "\n";
                    }

                    if (renders > 0) {
                        uint64_t avgTime = totalRenderTime.load() / renders;
                        ss << "  Avg Time:    " << avgTime << " µs\n";
                    }

                    ss << "Queue:\n";
                    ss << "  Full Events: " << queueFullCount.load() << "\n";
                    ss << "  Max Depth:   " << queueMaxSize.load() << "\n";

                    ss << "Buffers:\n";
                    for (size_t i = 0; i < MaxClientBuffers; i++) {
                        const auto& buf = buffers[i];
                        uint64_t requests = buf.requestCount.load();
                        uint64_t bufRenders = buf.renderCount.load();
                        uint64_t bufSkips = buf.skipCount.load();

                        if (requests > 0 || bufRenders > 0) {
                            ss << "  [" << i << "] Req:" << requests
                               << " Rnd:" << bufRenders
                               << " Skip:" << bufSkips;

                            if (bufRenders > 0) {
                                ss << " AvgAge:" << buf.AvgAge()
                                   << " MaxAge:" << buf.maxAge.load() << " µs";
                            }
                            ss << "\n";
                        }
                    }

                    uint8_t current = currentBufferId.load();
                    if (current < MaxClientBuffers) {
                        uint64_t now = Core::Time::Now().Ticks();
                        uint64_t lastRender = lastRenderTime.load();
                        uint64_t idleTime = lastRender > 0 ? (now - lastRender) : 0;
                        ss << "Current: Buffer " << static_cast<uint32_t>(current)
                           << " (idle " << idleTime << " µs / "
                           << std::fixed << std::setprecision(1) << (idleTime / 1000.0) << " ms)\n";
                    }

                    return ss.str();
                }
            };

        private:
            class ExternalBuffer : public Graphics::ServerBufferType<1> {
                using BaseClass = Graphics::ServerBufferType<1>;

            public:
                ExternalBuffer() = delete;
                ExternalBuffer(ExternalBuffer&&) = delete;
                ExternalBuffer(const ExternalBuffer&) = delete;
                ExternalBuffer& operator=(ExternalBuffer&&) = delete;
                ExternalBuffer& operator=(const ExternalBuffer&) = delete;

                ExternalBuffer(Client& client, Core::PrivilegedRequest::Container& descriptors)
                    : BaseClass()
                    , _client(client)
                    , _id(InvalidBufferId)
                    , _texture()
                {
                    Load(descriptors);
                    TRACE(Trace::Information, (_T("ExternalBuffer for %s[%p] %dx%d, format=0x%08" PRIX32 ", modifier=0x%016" PRIX64), client.Name().c_str(), this, BaseClass::Height(), BaseClass::Width(), BaseClass::Format(), BaseClass::Modifier()));
                    Core::ResourceMonitor::Instance().Register(*this);
                }

                ~ExternalBuffer() override
                {
                    Core::ResourceMonitor::Instance().Unregister(*this);
                    _texture.Release();

                    Rendered();
                    Published();

                    TRACE(Trace::Information, (_T("ExternalBuffer for %s[%p] destructed"), _client.Name().c_str(), this));
                }

                void Request() override
                {
                    _client.Activate(_id);
                }

                Core::ProxyType<Compositor::IRenderer::ITexture> Texture() const
                {
                    return _texture;
                }

                uint8_t Id() const
                {
                    return _id;
                }

                void Configure(const uint8_t id, const Core::ProxyType<Compositor::IRenderer::ITexture>& texture)
                {
                    ASSERT(id < MaxClientBuffers);
                    ASSERT(_id == InvalidBufferId);
                    ASSERT(_texture.IsValid() == false);

                    _id = id;
                    _texture = texture;

                    TRACE(Trace::Information, (_T("ExternalBuffer for %s[%p] configured as id:%d"), _client.Name().c_str(), this, _id));
                }

            private:
                Client& _client;
                uint8_t _id;
                Core::ProxyType<Compositor::IRenderer::ITexture> _texture; // the texture handle that is known in the GPU/Renderer.
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
                ~Remote() override { }

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

            template <typename T, size_t SIZE>
            class AtomicFifo {
            public:
                AtomicFifo()
                    : _queue()
                    , _head(0)
                    , _tail(0)
                {
                    // Initialize array elements
                    for (auto& element : _queue) {
                        element.store(T {}, std::memory_order_relaxed);
                    }
                }

                ~AtomicFifo() = default;

                AtomicFifo(const AtomicFifo&) = delete;
                AtomicFifo& operator=(const AtomicFifo&) = delete;
                AtomicFifo(AtomicFifo&&) = delete;
                AtomicFifo& operator=(AtomicFifo&&) = delete;

                /**
                 * @brief Add an item to the queue (single producer, multi-threaded)
                 * @param item The item to add
                 * @param dropOldest If true and queue is full, drop the oldest item
                 * @return true if item was added, false if queue was full and dropOldest=false
                 */
                bool Push(const T& item, bool dropOldest = true)
                {
                    const size_t head = _head.load(std::memory_order_relaxed);
                    const size_t tail = _tail.load(std::memory_order_acquire); // Sync with Pop()
                    const size_t next_head = (head + 1) % SIZE;

                    if (next_head == tail) {
                        // Queue is full
                        if (dropOldest) {
                            // Can't safely advance tail from producer thread
                            // Fall back to original behavior - force a Pop
                            T dummy;
                            if (!Pop(dummy)) {
                                return false; // Shouldn't happen but be safe
                            }
                        } else {
                            return false;
                        }
                    }

                    _queue[head].store(item, std::memory_order_relaxed);
                    _head.store(next_head, std::memory_order_release); // Sync with Pop()
                    return true;
                }

                /**
                 * @brief Remove and return the oldest item from the queue (single consumer, multi-threaded)
                 * @param item Reference to store the popped item
                 * @return true if item was popped, false if queue was empty
                 */
                bool Pop(T& item)
                {
                    const size_t tail = _tail.load(std::memory_order_relaxed);
                    const size_t head = _head.load(std::memory_order_acquire);

                    if (tail == head) {
                        return false; // Queue empty
                    }

                    item = _queue[tail].load(std::memory_order_relaxed);
                    _tail.store((tail + 1) % SIZE, std::memory_order_release);
                    return true;
                }

                /**
                 * @brief Clear all items from the queue
                 */
                void Clear()
                {
                    T dummy;
                    while (Pop(dummy)) {
                        // Pop all items
                    }
                }

            private:
                std::array<std::atomic<T>, SIZE> _queue;
                std::atomic<size_t> _head;
                std::atomic<size_t> _tail;
            };

        public:
            Client() = delete;
            Client(Client&&) = delete;
            Client(const Client&) = delete;
            Client& operator=(Client&&) = delete;
            Client& operator=(const Client&) = delete;

            Client(CompositorImplementation& parent, const string& callsign, const uint32_t width, const uint32_t height)
                : _parent(parent)
                , _id(Core::InterlockedIncrement(_sequence))
                , _callsign(callsign)
                , _width(width) // surface width
                , _height(height) // surface height
                , _opacity(255)
                , _zIndex(0)
                , _geometry({ 0, 0, width, height }) // rendering geometry
                , _remoteClient(*this)
                , _geometryChanged(false)
                , _buffers(MaxClientBuffers)
                , _requestQueue()
                , _pendingQueue()
                , _currentId(InvalidBufferId)
                , _renderingId(InvalidBufferId)
                , _stats()
            {
                TRACE(Trace::Information, (_T("Client Constructed %s[%p] id:%d %dx%d"), _callsign.c_str(), this, _id, _width, _height));
            }
            ~Client() override
            {
                _buffers.Clear();

                _parent.Render(); // request a render to remove this surface from the composition.

                // while (_pendingQueue.Pop(bufferId)) {
                //     if (_buffers[bufferId].IsValid()) {
                //         _buffers[bufferId]->Published();
                //     }
                // }

                TRACE(Trace::Information, (_T("Client %s[%p] destroyed"), _callsign.c_str(), this));
            }

        public:
            Exchange::IComposition::IClient* External()
            {
                _remoteClient.AddRef();
                return (&_remoteClient);
            }

        private:
            void Activate(const uint8_t id)
            {
                if (id < _buffers.Count()) {
                    _stats.buffers[id].requestCount.fetch_add(1, std::memory_order_relaxed);
                    _stats.buffers[id].lastUsed.store(Core::Time::Now().Ticks(), std::memory_order_relaxed);

                    _currentId.store(id, std::memory_order_release);

                    // Signal new frame via queue
                    bool queued = _requestQueue.Push(id, false);

                    if (queued) {
                        _parent.Render();
                    } else {
                        TRACE(Trace::Error, (_T("Client %s request queue full, dropping buffer %d"), _callsign.c_str(), id));
                        // Note: _currentId is still updated, so next vsync will render this buffer
                        _stats.queueFullCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }

        public:
            uint8_t CurrentBufferId() const
            {
                return _stats.currentBufferId.load(std::memory_order_relaxed);
            }

            void IncrementBufferSkip(uint8_t id)
            {
                if (id < MaxClientBuffers) {
                    _stats.buffers[id].skipCount.fetch_add(1, std::memory_order_relaxed);
                }
            }

            const Stats& Statistics() const
            {
                return _stats;
            }

            Core::ProxyType<Compositor::IRenderer::ITexture> Acquire()
            {
                Core::ProxyType<Compositor::IRenderer::ITexture> texture;

                uint8_t id;
                uint64_t acquireTime = Core::Time::Now().Ticks();

                // Check for new frame in queue
                if (_requestQueue.Pop(id)) {
                    // New frame requested! Update current
                    _currentId.store(id, std::memory_order_release);
                } else {
                    // No new frame, use current buffer (static content)
                    id = _currentId.load(std::memory_order_acquire);
                }

                // Render the buffer (new or repeated)
                if (id != InvalidBufferId && id < _buffers.Count() && _buffers[id].IsValid()) {
                    _renderingId.store(id, std::memory_order_release);

                    // Record buffer render
                    _stats.buffers[id].renderCount.fetch_add(1, std::memory_order_relaxed);

                    // Calculate and record age
                    uint64_t requestTime = _stats.buffers[id].lastUsed.load(std::memory_order_relaxed);
                    if (requestTime > 0 && acquireTime >= requestTime) {
                        uint64_t age = acquireTime - requestTime;
                        _stats.buffers[id].totalAge.fetch_add(age, std::memory_order_relaxed);

                        // Update max age using CAS
                        uint64_t currentMax = _stats.buffers[id].maxAge.load(std::memory_order_relaxed);
                        while (age > currentMax && !_stats.buffers[id].maxAge.compare_exchange_weak(currentMax, age, std::memory_order_relaxed))
                            ;
                    }

                    // Update current buffer tracking
                    _stats.currentBufferId.store(id, std::memory_order_relaxed);
                    _stats.lastRenderTime.store(acquireTime, std::memory_order_relaxed);

                    if (_buffers[id]->Acquire(0) != nullptr) {
                        texture = _buffers[id]->Texture();
                    } else {
                        TRACE(Trace::Error, (_T("Client %s buffer %d failed to acquire for rendering"), _callsign.c_str(), id));
                    }
                } else {
                    _renderingId.store(InvalidBufferId, std::memory_order_release);

                    if (id != InvalidBufferId) {
                        TRACE(Trace::Error, (_T("Client %s invalid buffer %d"), _callsign.c_str(), id));
                    }
                }

                return texture;
            }

            void Relinquish()
            {
                uint8_t id = _renderingId.exchange(InvalidBufferId, std::memory_order_acq_rel);

                if (id != InvalidBufferId) {
                    _buffers[id]->Relinquish();

                    bool queued = _pendingQueue.Push(id, false);

                    if (!queued) {
                        TRACE(Trace::Error, (_T("Queue full! Buffer %d rejected"), id));
                    }

                    ASSERT(queued == true);

                    if (_buffers[id].IsValid()) {
                        _buffers[id]->Rendered();
                    }
                } else {
                    TRACE(Trace::Error, (_T("Can't release invalid buffers")));
                }
            }

            void Completed()
            {
                uint8_t bufferId;

                if (_pendingQueue.Pop(bufferId)) {
                    if (_buffers[bufferId].IsValid()) {
                        _buffers[bufferId]->Published();
                    }
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
                _pendingQueue.Clear();

                _parent.Revoke(this);
            }

            uint32_t AttachBuffer(Core::PrivilegedRequest::Container& descriptors)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                if (_buffers.Count() < MaxClientBuffers) {
                    Core::ProxyType<ExternalBuffer> buffer = Core::ProxyType<ExternalBuffer>::Create(*this, descriptors);

                    if (buffer.IsValid() == true) {
                        int index = _buffers.Add(buffer);
                        buffer->Configure(index, _parent.Texture(Core::ProxyType<Exchange::IGraphicsBuffer>(buffer)));
                        result = Core::ERROR_NONE;
                    } else {
                        result = Core::ERROR_GENERAL;
                    }
                }

                return result;
            }

            friend class PrivilegedRequestCallback; // to be able to call AttachBuffer

        private:
            CompositorImplementation& _parent;
            uint32_t _id;
            const string _callsign; // the callsign of the surface
            uint32_t _width; // the width of the surface
            uint32_t _height; // the height of the surface
            uint32_t _opacity; // the opacity of the surface on the composition
            uint16_t _zIndex; // the z-index of the surface on the composition
            Exchange::IComposition::Rectangle _geometry; // the actual geometry of the surface on the composition
            Remote _remoteClient;
            bool _geometryChanged;
            Core::ProxyList<ExternalBuffer> _buffers; // the actual pixel buffers that are used by this client but are owed by the compositorclient.
            AtomicFifo<uint8_t, MaxClientBuffers> _requestQueue;
            AtomicFifo<uint8_t, MaxClientBuffers> _pendingQueue;
            std::atomic<uint8_t> _currentId;
            std::atomic<uint8_t> _renderingId;
            Stats _stats;

            static uint32_t _sequence;
        }; // class Client

        class Output {
        public:
            Output() = delete;
            Output(Output&&) = delete;
            Output(const Output&) = delete;
            Output& operator=(Output&&) = delete;
            Output& operator=(const Output&) = delete;

            Output(CompositorImplementation& parent, const string& name, const uint32_t width, const uint32_t height, const uint32_t refreshRate, const Compositor::PixelFormat& format, const Core::ProxyType<Compositor::IRenderer>& renderer)
                : _sink(parent)
                , _connector()

            {
                _connector = Compositor::CreateBuffer(name, width, height, refreshRate, format, renderer, &_sink);

                if (_connector.IsValid() == false) {
                    TRACE(Trace::Error, (_T("Could not create output connector for %s"), name.c_str()));
                }
            }

            ~Output()
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
                ~Sink() override { }

                virtual void Presented(const Compositor::IOutput* output, const uint64_t sequence, const uint64_t time) override
                {
                    _parent.VSync(output, sequence, time);
                }

                virtual void Terminated(const Compositor::IOutput* output VARIABLE_IS_NOT_USED) override
                {
                    _parent.Deactivate(PluginHost::IShell::REQUESTED);
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
            bool IsValid() const
            {
                return _connector.IsValid();
            }

            Core::ProxyType<Compositor::IOutput> Buffer()
            {
                return _connector;
            }

            uint32_t Commit()
            {
                return _connector->Commit();
            }

            uint32_t Format() const
            {
                return _connector->Format();
            }

            uint64_t Modifier() const
            {
                return _connector->Modifier();
            }

        private:
            Sink _sink;
            Core::ProxyType<Compositor::IOutput> _connector;
        };

        using Clients = Core::ProxyMapType<string, Client>;
        using Observers = std::vector<Exchange::IComposition::INotification*>;

        void Stop()
        {
            _terminated.store(true, std::memory_order_release);

            _renderPending.store(false, std::memory_order_release);

            {
                std::lock_guard<std::mutex> lock(_commitMutex);
                _canCommit.store(true, std::memory_order_release);
            }

            _commitCV.notify_all(); // Wake up all waiting threads

            _present.Stop(); // This should stop the rendering loop

            if (_present.Wait(Core::Thread::STOPPED, 5000) == false) {
                TRACE(Trace::Error, ("Presenter thread did not stop in time!"));
            }

            _descriptorExchange.Close();

            {
                std::lock_guard<std::mutex> lock(_clientLock);

                _clients.Clear();
            }

            if (_renderer.IsValid() && _output != nullptr && _output->IsValid()) {
                Core::ProxyType<Compositor::IOutput> buffer = _output->Buffer();
                if (buffer.IsValid()) {
                    Core::ProxyType<Compositor::IRenderer::IFrameBuffer> frameBuffer = buffer->FrameBuffer();
                    if (frameBuffer.IsValid()) {
                        _renderer->Unbind(frameBuffer); // Ensure we're unbound
                    }
                }
            }
        }

    public:
        CompositorImplementation(CompositorImplementation&&) = delete;
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(CompositorImplementation&&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        CompositorImplementation()
            : _adminLock()
            , _service(nullptr)
            , _output(nullptr)
            , _renderer()
            , _observers()
            , _privilegedRequestCallback(*this)
            , _descriptorExchange(&_privilegedRequestCallback)
            , _clientLock()
            , _clients()
            , _lastFrame(0)
            , _background(pink)
            , _engine()
            , _dispatcher(nullptr)
            , _renderDescriptor(Compositor::InvalidFileDescriptor)
            , _renderNode()
            , _present(*this)
            , _autoScale(true)
            , _commitMutex()
            , _commitCV()
            , _canCommit(true)
            , _totalRenderTime(0)
            , _frameCount(0)
            , _frameTime(0)
            , _renderPending(false)
            , _terminated(false)
        {
        }
        ~CompositorImplementation() override
        {
            Stop();

            if (_dispatcher != nullptr) {
                delete _dispatcher;
                _dispatcher = nullptr;
            }

            if (_engine.IsValid()) {
                _engine.Release();
            }

            if (_renderer.IsValid()) {
                _renderer.Release();
            }

            if (_output != nullptr) {
                delete _output;
                _output = nullptr;
            }

            if (_renderDescriptor != Compositor::InvalidFileDescriptor) {
                ::close(_renderDescriptor);
                _renderDescriptor = Compositor::InvalidFileDescriptor;
            }

            if (_service != nullptr) {
                _service->Release();
                _service = nullptr;
            }
        }

    public:
        /**
         * Exchange::IComposition methods
         */
        uint32_t Configure(PluginHost::IShell* service) override
        {

            ASSERT(service != nullptr);

            _service = service;
            _service->AddRef();

            uint32_t result = Core::ERROR_NONE;
            Config config;

            Core::OptionalType<Core::JSON::Error> error;
            bool parseSuccess = config.FromString(_service->ConfigLine(), error);

            if (!parseSuccess) {
                if (error.IsSet()) {
                    TRACE(Trace::Error, (_T("JSON parsing failed: %s"), Core::JSON::ErrorDisplayMessage(error.Value()).c_str()));
                } else {
                    TRACE(Trace::Error, (_T("JSON parsing failed - unknown error")));
                }
                return Core::ERROR_PARSE_FAILURE;
            }

            if ((config.Render.IsSet() == true) && (config.Render.Value().empty() == false)) {
                _renderDescriptor = ::open(config.Render.Value().c_str(), O_RDWR | O_CLOEXEC);
            } else {
                TRACE(Trace::Error, (_T("Render node is not set in the configuration")));
                return Core::ERROR_INCOMPLETE_CONFIG;
            }

            if (_renderDescriptor < 0) {
                TRACE(Trace::Error, (_T("Failed to open render node %s, error %d"), config.Render.Value().c_str(), errno));
                return Core::ERROR_OPENING_FAILED;
            }

            _renderer = Compositor::IRenderer::Instance(_renderDescriptor);
            ASSERT(_renderer.IsValid());

            if (config.AutoScale.IsSet() == true) {
                _autoScale = config.AutoScale.Value();
            }

            if (config.Output.IsSet() == false) {
                TRACE(Trace::Error, (_T("Output is not set in the configuration")));
                return Core::ERROR_INCOMPLETE_CONFIG;
            } else {
                const Exchange::IComposition::ScreenResolution resolution = config.Resolution.Value();

                const uint32_t width(WidthFromResolution(resolution));
                const uint32_t height(HeightFromResolution(resolution));
                const uint32_t refreshRate(RefreshRateFromResolution(resolution));

                const Compositor::PixelFormat format(config.Format.Value(), { config.Modifier.Value() });

                TRACE(Trace::Information, ("Requested format: %s", Compositor::ToString(format).c_str()));

                _output = new Output(*this, config.Output.Value(), width, height, refreshRate, format, _renderer);
                ASSERT((_output != nullptr) && (_output->IsValid()));

                if (_output == nullptr) {
                    TRACE(Trace::Error, (_T("Failed to create output surface %s"), config.Output.Value().c_str()));
                    return Core::ERROR_SURFACE_UNAVAILABLE;
                }

                if (_output->IsValid() == false) {
                    TRACE(Trace::Error, (_T("Output surface %s is not valid"), config.Output.Value().c_str()));
                    delete _output;
                    _output = nullptr;
                    return Core::ERROR_ILLEGAL_STATE;
                }

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

                _dispatcher = new DisplayDispatcher(Core::NodeId(comrpcPath.c_str()), _service->ProxyStubPath(), this, _engine);

                if (_dispatcher->IsListening() == true) {
                    PluginHost::ISubSystem* subSystems = _service->SubSystems();

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

                _observers.erase(index);

                notification->Release();
            }

            _adminLock.Unlock();
        }

        Core::ProxyType<Compositor::IRenderer::ITexture> Texture(Core::ProxyType<Exchange::IGraphicsBuffer> buffer)
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
            return (_output != nullptr) ? _output->Format() : DRM_FORMAT_INVALID;
        }

        uint64_t Modifier() const
        {
            return (_output != nullptr) ? _output->Modifier() : DRM_FORMAT_MOD_INVALID;
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
        void VSync(const Compositor::IOutput* output VARIABLE_IS_NOT_USED, const uint64_t sequence VARIABLE_IS_NOT_USED, const uint64_t pts VARIABLE_IS_NOT_USED /*usec since epoch*/)
        {
            if (_terminated.load(std::memory_order_acquire)) {
                return;
            }

            {
                std::lock_guard<std::mutex> lock(_clientLock);
                _clients.Visit([&](const string& /*name*/, const Core::ProxyType<Client> client) {
                    client->Completed();
                });
            }

            {
                std::lock_guard<std::mutex> lock(_commitMutex);
                _canCommit.store(true, std::memory_order_release);
            }
            _commitCV.notify_one();
        }

        void Deactivate(const PluginHost::IShell::reason reason)
        {
            ASSERT(_service != nullptr);
            Stop();
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, reason));
        }

        void Render()
        {
            _renderPending.store(true, std::memory_order_release);
            _present.Run();
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

        void RenderOutput() /* 3000uS rpi4*/
        {
            const uint64_t start(Core::Time::Now().Ticks());

            if (_output != nullptr) {
                ASSERT(_output->IsValid() == true);
                ASSERT(_renderer.IsValid() == true);

                Core::ProxyType<Compositor::IOutput> buffer = _output->Buffer();
                ASSERT(buffer.IsValid() == true);

                Core::ProxyType<Compositor::IRenderer::IFrameBuffer> frameBuffer = buffer->FrameBuffer();

                if (frameBuffer.IsValid() == true) {
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

                    // Ensure all rendering commands are finished on the GPU
                    uint32_t syncResult = _renderer->Finish();
                    if (syncResult != Core::ERROR_NONE) {
                        TRACE(Trace::Error, (_T("GPU sync failed: %d"), syncResult));
                    }

                    _renderer->Unbind(frameBuffer);

                    _totalRenderTime.fetch_add((Core::Time::Now().Ticks() - start), std::memory_order_relaxed);

                    // Block until VSync allows commit
                    {
                        std::unique_lock<std::mutex> lock(_commitMutex);
                        if (_commitCV.wait_for(lock, std::chrono::milliseconds(100), [this] {
                                return _canCommit.load(std::memory_order_acquire); // Wait for permission
                            })) {
                            // Got permission, reset it for next frame
                            _canCommit.store(false, std::memory_order_release);
                        } else {
                            TRACE(Trace::Error, (_T("Timeout waiting for VSync, forcing commit")));
                        }
                    }

                    if (_output != nullptr) {
                        uint32_t commit = _output->Commit();

                        if (commit != Core::ERROR_NONE) {
                            TRACE(Trace::Error, (_T("Commit failed: %d"), commit));
                        }
                    }

                    _frameCount.fetch_add(1, std::memory_order_relaxed);
                    _frameTime.fetch_add((Core::Time::Now().Ticks() - start), std::memory_order_relaxed);
                }
            }
        }

        void RenderClient(const Core::ProxyType<Client> client)
        {
            ASSERT(client.IsValid() == true);

            const uint64_t start(Core::Time::Now().Ticks());

            Core::ProxyType<Compositor::IRenderer::ITexture> texture = client->Acquire();

            if ((texture.IsValid() == true)) {
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

                const Exchange::IComposition::Rectangle clientArea = { 0, 0, texture->Width(), texture->Height() };

                const float alpha = float(client->Opacity()) / float(Exchange::IComposition::maxOpacity);

                _renderer->Render(texture, clientArea, clientProjection, alpha);

                const uint64_t duration = Core::Time::Now().Ticks() - start;
                UpdateClientStats(client->Name(), duration, false);
            } else {
                TRACE(Trace::Error, (_T("Skipping %s, no texture to render for buffer"), client->Name().c_str()));
                UpdateClientStats(client->Name(), 0, true);

                uint8_t currentId = client->CurrentBufferId();
                if (currentId < Client::MaxClientBuffers) {
                    client->IncrementBufferSkip(currentId);
                }
            }

            client->Relinquish();
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
                , _lastStatsTime(std::chrono::steady_clock::now())
            {
            }

            ~Presenter() override
            {
                Core::Thread::Stop();
                Core::Thread::Wait(Core::Thread::STOPPED, 100);
            }

        private:
            uint32_t Worker() override
            {
                while (IsRunning()) {
                    // Wait for signal
                    Core::Thread::Block();

                    // Process one frame per wake-up
                    bool expected = true;
                    if (_parent._renderPending.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) {
                        _parent.RenderOutput();

                        // Print stats every 5 seconds
                        auto now = std::chrono::steady_clock::now();

                        if (std::chrono::duration_cast<std::chrono::seconds>(now - _lastStatsTime).count() >= 5) {
                            _parent.PrintStats();
                            _lastStatsTime = now;
                        }
                    }
                }

                return Core::infinite;
            }

        private:
            CompositorImplementation& _parent;
            std::chrono::steady_clock::time_point _lastStatsTime;
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

        void UpdateClientStats(const std::string& clientName, uint64_t renderTime, bool skipped)
        {
            std::lock_guard<std::mutex> lock(_statsMutex);
            Client::Stats& stats = _clientStats[clientName];

            if (skipped) {
                stats.skipCount.fetch_add(1, std::memory_order_relaxed);
            } else {
                stats.renderCount.fetch_add(1, std::memory_order_relaxed);
                stats.totalRenderTime.fetch_add(renderTime, std::memory_order_relaxed);
            }
        }

        void PrintStats()
        {
            uint64_t frames = _frameCount.exchange(0);
            uint64_t totalRender = _totalRenderTime.exchange(0);
            uint64_t frameTime = _frameTime.exchange(0);

            if (frames > 0) {
                uint64_t avgFrameTime = frameTime / frames; // microseconds per frame
                double fps = 1000000.0 / avgFrameTime; // convert to FPS (1 second = 1,000,000 microseconds)

                TRACE(Trace::Stats, (_T("Global: frames: %llu, avg: %llu µs , fps: %.2f"), frames, (totalRender / frames), fps));
            }

            std::lock_guard<std::mutex> lock(_clientLock);
            _clients.Visit([&](const string& name, const Core::ProxyType<Client>& client) {
                if (client.IsValid()) {
                    string stats = client->Statistics().ToString();
                    TRACE(Trace::Stats, (_T("=====%s======\n%s"), name.c_str(), stats.c_str()));
                }
            });
        }

    private:
        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Output* _output;
        Core::ProxyType<Compositor::IRenderer> _renderer;
        Observers _observers;
        PrivilegedRequestCallback _privilegedRequestCallback;
        Core::PrivilegedRequest _descriptorExchange;
        std::mutex _clientLock;
        Clients _clients;
        uint64_t _lastFrame;
        Compositor::Color _background;
        Core::ProxyType<RPC::InvokeServer> _engine;
        DisplayDispatcher* _dispatcher;
        int _renderDescriptor;
        std::string _renderNode;
        Presenter _present;
        bool _autoScale;
        std::mutex _commitMutex;
        std::condition_variable _commitCV;
        std::atomic<bool> _canCommit;
        std::atomic<uint64_t> _totalRenderTime;
        std::atomic<uint64_t> _frameCount;
        std::atomic<uint64_t> _frameTime;
        mutable std::mutex _statsMutex;
        std::unordered_map<std::string, Client::Stats> _clientStats;
        std::atomic<bool> _renderPending;
        std::atomic<bool> _terminated;
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0)

    uint32_t CompositorImplementation::Client::_sequence = 1;
} // namespace Plugin
} // namespace Thunder
