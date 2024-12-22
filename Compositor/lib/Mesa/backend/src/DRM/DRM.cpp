/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
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

#include "../Module.h"
#include "IGpu.h"

#include <IBuffer.h>
#include <IOutput.h>
#include <interfaces/IComposition.h>

// #define __GBM__

#include <DRM.h>
#include <DRMTypes.h>
#include <PropertyAdministrator.h>

#include <drm.h>
#include <drm_fourcc.h>
#include <gbm.h>
// #include <libudev.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <mutex>

MODULE_NAME_ARCHIVE_DECLARATION

// clang-format on
// good reads
// https://docs.nvidia.com/jetson/l4t-multimedia/group__direct__rendering__manager.html
// http://github.com/dvdhrm/docs
// https://drmdb.emersion.fr
//
namespace Thunder {

namespace Compositor {

    namespace Backend {

        constexpr TCHAR drmDevicesRootPath[] = _T("/dev/dri/");

        DRM::Identifier FindConnectorId(const int fd, const string& connectorName)
        {
            Identifier connectorId(InvalidIdentifier);

            if (fd > 0) {
                drmModeResPtr resources = drmModeGetResources(fd);

                if (resources != nullptr) {
                    for (uint8_t i = 0; i < resources->count_connectors; i++) {
                        drmModeConnectorPtr connector = drmModeGetConnector(fd, resources->connectors[i]);

                        if (nullptr != connector) {
                            char name[59];
                            int nameLength;
                            nameLength = snprintf(name, sizeof(name), "%s-%u", drmModeGetConnectorTypeName(connector->connector_type), connector->connector_type_id);
                            name[nameLength] = '\0';

                            if (connectorName.compare(name) == 0) {
                                connectorId = connector->connector_id;
                                break;
                            }

                            drmModeFreeConnector(connector);
                        }
                    }

                    drmModeFreeResources(resources);
                }
            }

            return connectorId;
        }

        std::string GetGPUNode(const string& connectorName)
        {
            std::string file;
            std::vector<std::string> nodes;

            Compositor::DRM::GetNodes(DRM_NODE_PRIMARY, nodes);

            for (const auto& node : nodes) {
                int fd = ::open(node.c_str(), O_RDWR);

                if (FindConnectorId(fd, connectorName) != InvalidIdentifier) {
                    file = node;
                }

                ::close(fd);

                if (file.empty() == false) {
                    break;
                }
            }

            return file;
        }

        static Core::ResourceMonitorType<Core::IResource, Core::Void, 0, 1> _resourceMonitor;

        class DRM {
        public:
            struct FrameBuffer {
                Core::ProxyType<Exchange::ICompositionBuffer> data;
                Compositor::DRM::Identifier identifier;
            };

        private:
            class Monitor : public Core::IResource {
            public:
                Monitor() = delete;
                Monitor(const DRM& parent)
                    : _parent(parent)
                {
                }
                ~Monitor() = default;

                handle Descriptor() const override
                {
                    return (_parent.Descriptor());
                }
                uint16_t Events() override
                {
                    return (POLLIN | POLLPRI);
                }
                void Handle(const uint16_t events) override
                {
                    if (((events & POLLIN) != 0) || ((events & POLLPRI) != 0)) {
                        _parent.DrmEventHandler(Descriptor());
                    }
                }

            private:
                const DRM& _parent;
            };

            // using CommitRegister = std::unordered_map<uint32_t, ConnectorImplementation*>;

        public:
            DRM(DRM&&) = delete;
            DRM(const DRM&) = delete;
            DRM& operator=(DRM&&) = delete;
            DRM& operator=(const DRM&) = delete;

            DRM() = delete;

            explicit DRM(const string& gpuNode)
                : _flip()
                , _gpuFd(Compositor::DRM::OpenGPU(gpuNode))
                , _gpu(IGpu::Instance())
                , _monitor(*this)
                , _connectors()
                , _pendingFlips(0)
            {
                ASSERT(_gpuFd != Compositor::InvalidFileDescriptor);

                if (_gpuFd != Compositor::InvalidFileDescriptor) {
                    int drmResult(0);
                    uint64_t cap;

                    if ((drmResult = drmSetMaster(_gpuFd)) != 0) {
                        TRACE(Trace::Error, ("Could not become DRM master. Error: [%s]", strerror(errno)));
                    }

                    if ((drmResult == 0) && ((drmResult = drmSetClientCap(_gpuFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1)) != 0)) {
                        TRACE(Trace::Error, ("Could not set basic information. Error: [%s]", strerror(errno)));
                    }

#ifdef USE_ATOMIC
                    if ((drmResult == 0) && ((drmResult = drmSetClientCap(_gpuFd, DRM_CLIENT_CAP_ATOMIC, 1)) != 0)) {
                        TRACE(Trace::Error, ("Could not enable atomic API. Error: [%s]", strerror(errno)));
                    }
#endif

                    if ((drmResult == 0) && ((drmResult = drmGetCap(_gpuFd, DRM_CAP_CRTC_IN_VBLANK_EVENT, &cap) < 0 || !cap))) {
                        TRACE(Trace::Error, ("Device does not support vblank per CRTC"));
                    }

                    ASSERT(drmResult == 0);

                    if (drmResult == 0) {
                        if (drmSetClientCap(_gpuFd, DRM_CLIENT_CAP_ASPECT_RATIO, 1) != 0) {
                            TRACE(Trace::Information, ("Picture aspect ratio not supported."));
                        }

                        _resourceMonitor.Register(_monitor);
                    } else {
                        close(_gpuFd);
                        _gpuFd = Compositor::InvalidFileDescriptor;
                    }
                }
            }

            virtual ~DRM()
            {
                if (_gpuFd > 0) {
                    _resourceMonitor.Unregister(_monitor);

                    if ((drmIsMaster(_gpuFd) != 0) && (drmDropMaster(_gpuFd) != 0)) {
                        TRACE(Trace::Error, ("Could not drop DRM master. Error: [%s]", strerror(errno)));
                    }

                    if (TRACE_ENABLED(Trace::Information)) {
                        drmVersion* version = drmGetVersion(_gpuFd);
                        TRACE(Trace::Information, ("Destructing DRM backend for %s (%s)", drmGetDeviceNameFromFd2(_gpuFd), version->name));
                        drmFreeVersion(version);
                    }

                    close(_gpuFd);
                }
            }

            bool IsValid() const
            {
                return (_gpuFd > 0);
            }

        private:
            int Descriptor() const
            {
                return _gpuFd;
            }

            class ConnectorImplementation : public Exchange::ICompositionBuffer, public IGpu::IConnector {
            public:
                ConnectorImplementation() = delete;
                ConnectorImplementation(ConnectorImplementation&&) = delete;
                ConnectorImplementation(const ConnectorImplementation&) = delete;
                ConnectorImplementation& operator=(ConnectorImplementation&&) = delete;
                ConnectorImplementation& operator=(const ConnectorImplementation&) = delete;

                ConnectorImplementation(Core::ProxyType<DRM> backend, const string& connector, VARIABLE_IS_NOT_USED const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat format, Compositor::IOutputCallback* feedback)
                    : _backend(backend)
                    , _connector(_backend->Descriptor(), Compositor::DRM::object_type::Connector, FindConnectorId(_backend->Descriptor(), connector))
                    , _crtc()
                    , _primaryPlane()
                    , _swap()
                    , _frontBuffer(0)
                    , _frameBuffer()
                    , _format(format)
                    , _feedback(feedback)
                {
                    // ASSERT(_feedback != nullptr);
                    if (Scan() == false) {
                        TRACE(Trace::Backend, ("Connector %s is not in a valid state", connector.c_str()));
                    } else {
                        ASSERT(_frameBuffer[BackBuffer()].data.IsValid());
                        ASSERT(_frameBuffer[FrontBuffer()].data.IsValid());

                        ASSERT(_frameBuffer[BackBuffer()].identifier != InvalidIdentifier);
                        ASSERT(_frameBuffer[FrontBuffer()].identifier != InvalidIdentifier);

                        TRACE(Trace::Backend, ("Connector %p for Id=%u Crtc=%u, PrimaryPlane=%u", this, _connector.Id(), _crtc.Id(), _primaryPlane.Id()));
                    }
                }

                virtual ~ConnectorImplementation() override
                {
                    _backend.Release();

                    TRACE(Trace::Backend, ("Connector %p Destroyed", this));
                }

            private:
                Compositor::DRM::Identifier BackBuffer() const
                {
                    return _frontBuffer ^ 1;
                }

                Compositor::DRM::Identifier FrontBuffer() const
                {
                    return _frontBuffer;
                }
                void Render()
                {
                    if (IsConnected() == true) {
                        _backend->SchedulePageFlip(*this);
                    }
                }

            public:
                /**
                 * Exchange::ICompositionBuffer implementation
                 *
                 * Returning the info of the back buffer because its used to
                 * draw a new frame.
                 */
                IIterator* Acquire(const uint32_t timeoutMs) override
                {
                    _swap.Lock();
                    return _frameBuffer[BackBuffer()].data->Acquire(timeoutMs);
                }
                void Relinquish() override
                {
                    _swap.Unlock();
                    _frameBuffer[BackBuffer()].data->Relinquish();
                }
                uint32_t Width() const override
                {
                    return _frameBuffer[BackBuffer()].data->Width();
                }
                uint32_t Height() const override
                {
                    return _frameBuffer[BackBuffer()].data->Height();
                }
                uint32_t Format() const override
                {
                    return _frameBuffer[BackBuffer()].data->Format();
                }
                uint64_t Modifier() const override
                {
                    return _frameBuffer[BackBuffer()].data->Modifier();
                }
                Exchange::ICompositionBuffer::DataType Type() const
                {
                    return _frameBuffer[BackBuffer()].data->Type();
                }

                /**
                 * IGpu::IConnector implementation
                 */
                bool IsEnabled() const override
                {
                    // Todo: this will control switching on or of the display by controlling the Display Power Management Signaling (DPMS)
                    return true;
                }

                const Compositor::DRM::Properties& Properties() const override {
                    return _connector;
                }

                const Compositor::DRM::Properties& Plane() const override {
                    return _primaryPlane;
                }

                const Compositor::DRM::CRTCProperties& CrtController() const override {
                    return _crtc;
                }

                // current framebuffer id to scan out
                Compositor::DRM::Identifier FrameBufferId() const override
                {
                    return _frameBuffer[FrontBuffer()].identifier;
                }

                const Core::ProxyType<Thunder::Exchange::ICompositionBuffer> FrameBuffer() const override
                {
                    return _frameBuffer[FrontBuffer()].data;
                }

                void SwapBuffer()
                {
                    _swap.Lock();
                    _frontBuffer ^= 1;
                    _swap.Unlock();
                }

                void Presented(const uint32_t sequence, const uint64_t pts) override
                {                  
                    if (_feedback != nullptr) {
                        _feedback->Presented(_connector.Id(), sequence, pts);
                    }
                }

            private:
                bool IsConnected() const
                {
                    return (_crtc.Id() != Compositor::InvalidIdentifier);
                }

                bool Scan()
                {
                    bool result(false);

                    drmModeResPtr drmModeResources = drmModeGetResources(_backend->Descriptor());
                    drmModePlaneResPtr drmModePlaneResources = drmModeGetPlaneResources(_backend->Descriptor());

                    ASSERT(drmModeResources != nullptr);
                    ASSERT(drmModePlaneResources != nullptr);

                    drmModeEncoderPtr encoder = NULL;

                    TRACE(Trace::Backend, ("Found %d connectors", drmModeResources->count_connectors));
                    TRACE(Trace::Backend, ("Found %d encoders", drmModeResources->count_encoders));
                    TRACE(Trace::Backend, ("Found %d CRTCs", drmModeResources->count_crtcs));
                    TRACE(Trace::Backend, ("Found %d planes", drmModePlaneResources->count_planes));

                    for (int c = 0; c < drmModeResources->count_connectors; c++) {
                        drmModeConnectorPtr drmModeConnector = drmModeGetConnector(_backend->Descriptor(), drmModeResources->connectors[c]);

                        ASSERT(drmModeConnector != nullptr);

                        /* Find our drmModeConnector if it's connected*/
                        if ((drmModeConnector->connector_id != _connector.Id()) && (drmModeConnector->connection != DRM_MODE_CONNECTED)) {
                            continue;
                        }

                        /* Find the encoder (a deprecated KMS object) for this drmModeConnector. */
                        ASSERT(drmModeConnector->encoder_id != Compositor::InvalidIdentifier);

                        const char* name = drmModeGetConnectorTypeName(drmModeConnector->connector_type);
                        TRACE(Trace::Backend, ("Connector %s-%u connected", name, drmModeConnector->connector_type_id));

                        /* Get a bitmask of CRTCs the connector is compatible with. */
                        uint32_t _possibleCrtcs = drmModeConnectorGetPossibleCrtcs(_backend->Descriptor(), drmModeConnector);

                        if (_possibleCrtcs == 0) {
                            TRACE(Trace::Error, ("No CRTC possible for id", drmModeConnector->connector_id));
                        }

                        ASSERT(_possibleCrtcs != 0);

                        for (uint8_t e = 0; e < drmModeResources->count_encoders; e++) {
                            if (drmModeResources->encoders[e] == drmModeConnector->encoder_id) {
                                encoder = drmModeGetEncoder(_backend->Descriptor(), drmModeResources->encoders[e]);
                                break;
                            }
                        }

                        if (encoder != nullptr) { /*
                                                   * Find the CRTC currently used by this drmModeConnector. It is possible to
                                                   * use a different CRTC if desired, however unlike the pre-atomic API,
                                                   * we have to explicitly change every object in the routing path.
                                                   */

                            drmModePlanePtr plane(nullptr);
                            drmModeCrtcPtr crtc(nullptr);

                            ASSERT(encoder->crtc_id != Compositor::InvalidIdentifier);
                            TRACE(Trace::Information, ("Start looking for crtc_id: %d", encoder->crtc_id));

                            for (uint8_t c = 0; c < drmModeResources->count_crtcs; c++) {

                                if (drmModeResources->crtcs[c] == encoder->crtc_id) {
                                    crtc = drmModeGetCrtc(_backend->Descriptor(), drmModeResources->crtcs[c]);
                                    break;
                                }
                            }

                            /* Ensure the CRTC is active. */
                            ASSERT(crtc != nullptr);
                            ASSERT(crtc->crtc_id != 0);
                            /*
                             * On the banana pi, the following ASSERT fired but, although the documentation
                             * suggests that buffer_id means disconnected, the screen was not disconnected
                             * Hence wht the ASSERT is, until further investigation, commented out !!!
                             * ASSERT(crtc->buffer_id != 0);
                             */

                            /*
                             * The kernel doesn't directly tell us what it considers to be the
                             * single primary plane for this CRTC (i.e. what would be updated
                             * by drmModeSetCrtc), but if it's already active then we can cheat
                             * by looking for something displaying the same framebuffer ID,
                             * since that information is duplicated.
                             */
                            for (uint8_t p = 0; p < drmModePlaneResources->count_planes; p++) {
                                plane = drmModeGetPlane(_backend->Descriptor(), drmModePlaneResources->planes[p]);

                                TRACE(Trace::Backend, ("[PLANE: %" PRIu32 "] CRTC ID %" PRIu32 ", FB ID %" PRIu32, plane->plane_id, plane->crtc_id, plane->fb_id));

                                if ((plane->crtc_id == crtc->crtc_id) && (plane->fb_id == crtc->buffer_id)) {
                                    break;
                                }
                            }

                            ASSERT(plane);

                            uint64_t refresh = ((crtc->mode.clock * 1000000LL / crtc->mode.htotal) + (crtc->mode.vtotal / 2)) / crtc->mode.vtotal;

                            TRACE(Trace::Backend, ("[CRTC:%" PRIu32 ", CONN %" PRIu32 ", PLANE %" PRIu32 "]: active at %ux%u, %" PRIu64 " mHz", crtc->crtc_id, drmModeConnector->connector_id, plane->plane_id, crtc->width, crtc->height, refresh));

                            _crtc.Load(_backend->Descriptor(), crtc);
                            _primaryPlane.Load(_backend->Descriptor(), Compositor::DRM::object_type::Plane, plane->plane_id);

                            // allocate a double buffered framebuffer.
                            for (auto& buffer : _frameBuffer) {

                                if (buffer.data.IsValid()) {
                                    buffer.data.Release();
                                }

                                if (buffer.identifier != Compositor::InvalidIdentifier) {
                                    Compositor::DRM::DestroyFrameBuffer(_backend->Descriptor(), buffer.identifier);
                                    buffer.identifier = Compositor::InvalidIdentifier;
                                }

                                buffer.data = Compositor::CreateBuffer(
                                    _backend->Descriptor(),
                                    crtc->width,
                                    crtc->height,
                                    _format,
                                    nullptr);

                                buffer.identifier = Compositor::DRM::CreateFrameBuffer(_backend->Descriptor(), buffer.data.operator->());
                            }

                            // _refreshRate = crtc->mode.vrefresh;

                            plane = drmModeGetPlane(_backend->Descriptor(), _primaryPlane.Id());

                            drmModeFreeCrtc(crtc);
                            drmModeFreePlane(plane);

                            drmModeFreeEncoder(encoder);
                        } else {
                            TRACE(Trace::Error, ("Failed to get encoder for id %u", drmModeConnector->connector_id));
                        }

                        result = ((_crtc.Id() != Compositor::InvalidIdentifier) && (_primaryPlane.Id() != Compositor::InvalidIdentifier));

                        break;
                    }

                    drmModeFreeResources(drmModeResources);
                    drmModeFreePlaneResources(drmModePlaneResources);

                    return result;
                }

                /* simple stringification operator to make defines and errorcodes human readable */
#define CASE_TO_STRING(value) \
    case value:               \
        return #value;

                static const char* ObjectSting(const uint32_t object)
                {
                    switch (object) {
                        CASE_TO_STRING(DRM_MODE_OBJECT_CRTC)
                        CASE_TO_STRING(DRM_MODE_OBJECT_CONNECTOR)
                        CASE_TO_STRING(DRM_MODE_OBJECT_PLANE)
                    default:
                        return "Invalid";
                    }
                }
#undef CASE_TO_STRING

            private:
                Core::ProxyType<DRM> _backend;

                Compositor::DRM::Properties     _connector; 
                Compositor::DRM::CRTCProperties _crtc; 
                Compositor::DRM::Properties     _primaryPlane;

                Core::CriticalSection _swap;
                uint8_t _frontBuffer;
                std::array<DRM::FrameBuffer, 2> _frameBuffer;

                const Compositor::PixelFormat _format;

                Compositor::IOutputCallback* _feedback;
            };

            using Connectors = Core::ProxyMapType<string, ConnectorImplementation>;

            uint32_t SchedulePageFlip(ConnectorImplementation& connector VARIABLE_IS_NOT_USED)
            {
                return PageFlip();
            }

            void FinishPageFlip(const Compositor::DRM::Identifier crtc, const unsigned sequence, unsigned seconds, const unsigned useconds)
            {
                _connectors.Visit([&](const string& name, const Core::ProxyType<ConnectorImplementation> connector) {
                    if (connector->CrtController().Id() == crtc) {
                        struct timespec presentationTimestamp;

                        presentationTimestamp.tv_sec = seconds;
                        presentationTimestamp.tv_nsec = useconds * 1000;

                        connector->Presented(sequence, Core::Time(presentationTimestamp).Ticks());
                        // TODO: Check with Bram the intention of the Release()
                        // connector->Release();

                        --_pendingFlips;

                        TRACE(Trace::Backend, ("Pageflip finished for %s, pending flips: %u", name.c_str(), _pendingFlips));
                    }
                });

                if (_pendingFlips == 0) {
                    // all connectors are flipped... ready for the next round.
                    TRACE(Trace::Backend, ("all connectors are flipped... ready for the next round."));
                    _flip.unlock();
                }
            }

            static void PageFlipHandler(int cardFd VARIABLE_IS_NOT_USED, unsigned seq, unsigned sec, unsigned usec, unsigned crtc_id VARIABLE_IS_NOT_USED, void* userData)
            {
                ASSERT(userData != nullptr);

                DRM* backend = reinterpret_cast<DRM*>(userData);

                backend->FinishPageFlip(crtc_id, seq, sec, usec);
            }

            int DrmEventHandler(int fd) const
            {
                drmEventContext eventContext;
                memset(&eventContext, 0, sizeof(eventContext));

                eventContext.version = 3;
                eventContext.page_flip_handler2 = PageFlipHandler;

                if (drmHandleEvent(fd, &eventContext) != 0) {
                    TRACE(Trace::Error, ("Failed to handle drm event"));
                }

                return 1;
            }

            uint32_t PageFlip()
            {
                uint32_t result(Core::ERROR_GENERAL);

                if (_flip.try_lock()) {
                std::vector<IGpu::IConnector*> GpuConnectors;

                _connectors.Visit([&](const string& /*name*/, const Core::ProxyType<ConnectorImplementation> connector) {
                    if (connector->IsEnabled() == true) {
                        connector.AddRef();
                        connector->SwapBuffer();
                        GpuConnectors.push_back(connector.operator->());
                    }
                });

                if (GpuConnectors.empty() == false) {
                    _pendingFlips = GpuConnectors.size();
                    result = _gpu.Commit(_gpuFd, GpuConnectors, this);
                        TRACE_GLOBAL(Trace::Information, ("Committed %u connectors: %u", _pendingFlips, result));
                    } else {
                        TRACE_GLOBAL(Trace::Information, ("Nothing to commit..."));
                    }
                } else {
                    TRACE_GLOBAL(Trace::Error, ("Page flip still in progress", _pendingFlips));
                    result = Core::ERROR_INPROGRESS;
                }

                return result;
            }

        public:
            Core::ProxyType<ConnectorImplementation> Connector(const string& connectorName, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format, Compositor::IOutputCallback* feedback)
            {
                return _connectors.Instance<ConnectorImplementation>(connectorName, Core::ProxyType<DRM>(*this), connectorName, rectangle, format, feedback);
            }

        private:
            std::mutex _flip;
            int _gpuFd; // GPU opened as master or lease...
            IGpu& _gpu; // The buffer commit interface
            Monitor _monitor;
            Connectors _connectors;
            uint8_t _pendingFlips;
        }; // class DRM
    } // namespace Backend

    static Core::ProxyMapType<string, Backend::DRM> backends;

    Core::ProxyType<Exchange::ICompositionBuffer> CreateBuffer(const string& connectorName, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format, Compositor::IOutputCallback* feedback)
    {
        ASSERT(drmAvailable() == 1);
        ASSERT(connectorName.empty() == false);

        TRACE_GLOBAL(Trace::Backend, ("Requesting connector '%s'", connectorName.c_str()));

        Core::ProxyType<Backend::DRM> backend = backends.Instance<Backend::DRM>(Backend::GetGPUNode(connectorName), Backend::GetGPUNode(connectorName));

        Core::ProxyType<Exchange::ICompositionBuffer> connector;

        if (backend.IsValid()) {
            connector = backend->Connector(connectorName, rectangle, format, feedback);
        }

        return connector;
    }
} // namespace Compositor
} // Thunder
