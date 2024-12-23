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

        class Connector : public IConnector {
        private:
            class FrameBufferImplementation {
            public:
                FrameBufferImplementation(FrameBufferImplementation&&) = delete;
                FrameBufferImplementation(const FrameBufferImplementation&&) = delete;
                FrameBufferImplementation& operator=(FrameBufferImplementation&&) = delete;
                FrameBufferImplementation& operator=(const FrameBufferImplementation&&) = delete;

                FrameBufferImplementation() 
                    : _swap()
                    , _fd(-1)
                    , _activePlane(~0) {
                    _buffer[0] = Core::ProxyType<Exchange::ICompositionBuffer>();
                    _buffer[1] = Core::ProxyType<Exchange::ICompositionBuffer>();
                }
                ~FrameBufferImplementation() {
                    if (IsValid() == true) {
                        Compositor::DRM::DestroyFrameBuffer(_fd, _frameId[0]);
                        Compositor::DRM::DestroyFrameBuffer(_fd, _frameId[1]);
                        _buffer[0].Release();
                        _buffer[1].Release();
                        ::close(_fd);
                    }
                } 

            public:
                bool IsValid() const {
                    return (_activePlane != static_cast<uint8_t>(~0));
                }
                void Configure(const int fd, const uint32_t width, const uint32_t height, const Compositor::PixelFormat& format) {
                    // Configure should only be called once..
                    ASSERT ( IsValid() == false );

                    _buffer[0]  = Compositor::CreateBuffer(fd, width, height, format);
                    if (_buffer[0].IsValid() == false) {
                            TRACE(Trace::Error, ("Failed to create first DRM Buffer"));
                    }
                    else {
                        _buffer[1] = Compositor::CreateBuffer(fd, width, height, format);

                        if (_buffer[1].IsValid() == false) {
                            TRACE(Trace::Error, ("Failed to create second DRM Buffer"));
                            _buffer[0].Release();
                        }
                        else {
                            _frameId[0] = Compositor::DRM::CreateFrameBuffer(fd, _buffer[0].operator->());
                            _frameId[1] = Compositor::DRM::CreateFrameBuffer(fd, _buffer[1].operator->());
                            _activePlane = 0;
                            _fd = ::dup(fd);
                        }
                    }
                }
                IIterator* Acquire(const uint32_t timeoutMs) {
                    _swap.Lock();
                    return _buffer[_activePlane]->Acquire(timeoutMs);
                }
                void Relinquish() {
                    _swap.Unlock();
                    _buffer[_activePlane]->Relinquish();
                }
                uint32_t Width() const {
                    return (_buffer[0]->Width());
                }
                uint32_t Height() const {
                    return (_buffer[0]->Height());
                }
                uint32_t Format() const {
                    return (_buffer[0]->Format());
                }
                uint64_t Modifier() const {
                    return (_buffer[0]->Modifier());
                }
                Exchange::ICompositionBuffer::DataType Type() const {
                    return (_buffer[0]->Type());
                }
                Compositor::DRM::Identifier Id() const {
                    return _frameId[_activePlane];
                }
                Core::ProxyType<Thunder::Exchange::ICompositionBuffer> Buffer() const {
                    return _buffer[_activePlane];
                }
                void Swap() {
                    _swap.Lock();
                    _activePlane ^= 1;
                    _swap.Unlock();
                }

            private:
                Core::CriticalSection _swap;
                int _fd;
                uint8_t _activePlane;
                Core::ProxyType<Exchange::ICompositionBuffer> _buffer[2];
                Compositor::DRM::Identifier _frameId[2];
            };
    
        public:
            Connector() = delete;
            Connector(Connector&&) = delete;
            Connector(const Connector&) = delete;
            Connector& operator=(Connector&&) = delete;
            Connector& operator=(const Connector&) = delete;

            Connector(int fd, DRM::Identifier connectorId, VARIABLE_IS_NOT_USED const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat format, Compositor::IOutputCallback* feedback)
                : _backend(::dup(fd))
                , _format(format)
                , _connector(_backend, Compositor::DRM::object_type::Connector, connectorId)
                , _crtc()
                , _primaryPlane()
                , _frameBuffer()
                , _feedback(feedback)
            {
                ASSERT(_feedback != nullptr);

                if (Scan() == false) {
                    TRACE(Trace::Backend, ("Connector %d is not in a valid state", connectorId));
                } else {
                    ASSERT(_frameBuffer.IsValid() == true);
                    TRACE(Trace::Backend, ("Connector %p for Id=%u Crtc=%u, PrimaryPlane=%u", this, _connector.Id(), _crtc.Id(), _primaryPlane.Id()));
                }
            }

            ~Connector() override {
                ::close(_backend);

                TRACE(Trace::Backend, ("Connector %p Destroyed", this));
            }

        public:
            bool IsValid() const {
                return (_frameBuffer.IsValid());
            }

            /**
             * Exchange::ICompositionBuffer implementation
             *
             * Returning the info of the back buffer because its used to
             * draw a new frame.
             */
            IIterator* Acquire(const uint32_t timeoutMs) override {
                return (_frameBuffer.Acquire(timeoutMs));
            }
            void Relinquish() override {
                _frameBuffer.Relinquish();
            }
            uint32_t Width() const override {
                return (_frameBuffer.Width());
            }
            uint32_t Height() const override {
                return (_frameBuffer.Height());
            }
            uint32_t Format() const override {
                return (_frameBuffer.Format());
            }
            uint64_t Modifier() const override {
                return (_frameBuffer.Modifier());
            }
            Exchange::ICompositionBuffer::DataType Type() const {
                return (_frameBuffer.Type());
            }

            /**
             * IConnector implementation
             */
            bool IsEnabled() const override {
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
            Compositor::DRM::Identifier FrameBufferId() const override {
                return (_frameBuffer.Id());
            }
            void Presented(const uint32_t sequence, const uint64_t pts) override {                  
                if (_feedback != nullptr) {
                    _feedback->Presented(_connector.Id(), sequence, pts);
                }
            }
            void Swap() {
                _frameBuffer.Swap();
            }

        private:
            bool Scan()
            {
                bool result(false);

                drmModeResPtr drmModeResources = drmModeGetResources(_backend);
                drmModePlaneResPtr drmModePlaneResources = drmModeGetPlaneResources(_backend);

                ASSERT(drmModeResources != nullptr);
                ASSERT(drmModePlaneResources != nullptr);

                drmModeEncoderPtr encoder = NULL;

                TRACE(Trace::Backend, ("Found %d connectors", drmModeResources->count_connectors));
                TRACE(Trace::Backend, ("Found %d encoders", drmModeResources->count_encoders));
                TRACE(Trace::Backend, ("Found %d CRTCs", drmModeResources->count_crtcs));
                TRACE(Trace::Backend, ("Found %d planes", drmModePlaneResources->count_planes));

                for (int c = 0; c < drmModeResources->count_connectors; c++) {
                    drmModeConnectorPtr drmModeConnector = drmModeGetConnector(_backend, drmModeResources->connectors[c]);

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
                    uint32_t _possibleCrtcs = drmModeConnectorGetPossibleCrtcs(_backend, drmModeConnector);

                    if (_possibleCrtcs == 0) {
                        TRACE(Trace::Error, ("No CRTC possible for id", drmModeConnector->connector_id));
                    }

                    ASSERT(_possibleCrtcs != 0);

                    for (uint8_t e = 0; e < drmModeResources->count_encoders; e++) {
                        if (drmModeResources->encoders[e] == drmModeConnector->encoder_id) {
                            encoder = drmModeGetEncoder(_backend, drmModeResources->encoders[e]);
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
                                crtc = drmModeGetCrtc(_backend, drmModeResources->crtcs[c]);
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
                            plane = drmModeGetPlane(_backend, drmModePlaneResources->planes[p]);

                            TRACE(Trace::Backend, ("[PLANE: %" PRIu32 "] CRTC ID %" PRIu32 ", FB ID %" PRIu32, plane->plane_id, plane->crtc_id, plane->fb_id));

                            if ((plane->crtc_id == crtc->crtc_id) && (plane->fb_id == crtc->buffer_id)) {
                                break;
                            }
                        }

                        ASSERT(plane);

                        uint64_t refresh = ((crtc->mode.clock * 1000000LL / crtc->mode.htotal) + (crtc->mode.vtotal / 2)) / crtc->mode.vtotal;

                        TRACE(Trace::Backend, ("[CRTC:%" PRIu32 ", CONN %" PRIu32 ", PLANE %" PRIu32 "]: active at %ux%u, %" PRIu64 " mHz", crtc->crtc_id, drmModeConnector->connector_id, plane->plane_id, crtc->width, crtc->height, refresh));

                        _crtc.Load(_backend, crtc);
                        _primaryPlane.Load(_backend, Compositor::DRM::object_type::Plane, plane->plane_id);

                        _frameBuffer.Configure(_backend, crtc->width, crtc->height, _format);

                        // _refreshRate = crtc->mode.vrefresh;

                        plane = drmModeGetPlane(_backend, _primaryPlane.Id());

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

        private:
            int _backend;

            const Compositor::PixelFormat   _format;
            Compositor::DRM::Properties     _connector; 
            Compositor::DRM::CRTCProperties _crtc; 
            Compositor::DRM::Properties     _primaryPlane;

            FrameBufferImplementation _frameBuffer;

            Compositor::IOutputCallback* _feedback;
        };

        static Core::ResourceMonitorType<Core::IResource, Core::Void, 0, 1> _resourceMonitor;
        static Core::ProxyMapType<string, Connector> _connectors;

        class DRM : public Core::IResource {
        public:
            DRM() = delete;
            DRM(DRM&&) = delete;
            DRM(const DRM&) = delete;
            DRM& operator=(DRM&&) = delete;
            DRM& operator=(const DRM&) = delete;

            explicit DRM(const string& gpuNode)
                : _flip()
                , _gpuFd(Compositor::DRM::OpenGPU(gpuNode))
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

                        _resourceMonitor.Register(*this);
                    } else {
                        close(_gpuFd);
                        _gpuFd = Compositor::InvalidFileDescriptor;
                    }
                }
            }
            ~DRM() override {
                if (_gpuFd > 0) {
                    _resourceMonitor.Unregister(*this);

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

        public:
            bool IsValid() const {
                return (_gpuFd > 0);
            }

            //
            // Core::IResource memebers
            // -----------------------------------------------------------------
            handle Descriptor() const override {
                return (_gpuFd);
            }
            uint16_t Events() override {
                return (POLLIN | POLLPRI);
            }
            void Handle(const uint16_t events) override {
                if (((events & POLLIN) != 0) || ((events & POLLPRI) != 0)) {
                    drmEventContext eventContext;
                    memset(&eventContext, 0, sizeof(eventContext));

                    eventContext.version = 3;
                    eventContext.page_flip_handler2 = PageFlipHandler;

                    if (drmHandleEvent(_gpuFd, &eventContext) != 0) {
                        TRACE(Trace::Error, ("Failed to handle drm event"));
                    }
                }
            }

        private:
            uint32_t SchedulePageFlip(Connector& connector VARIABLE_IS_NOT_USED) {
                uint32_t result(Core::ERROR_GENERAL);

                if (_flip.try_lock()) {
                    std::vector<IConnector*> GpuConnectors;

                    _connectors.Visit([&](const string& /*name*/, const Core::ProxyType<Connector> connector) {
                            if (connector->IsEnabled() == true) {
                                connector.AddRef();
                                connector->Swap();
                                GpuConnectors.push_back(connector.operator->());
                            }
                        });

                    if (GpuConnectors.empty() == false) {
                        _pendingFlips = GpuConnectors.size();
                        result = Commit(_gpuFd, GpuConnectors, this);
                        TRACE_GLOBAL(Trace::Information, ("Committed %u connectors: %u", _pendingFlips, result));
                    } else {
                        TRACE_GLOBAL(Trace::Information, ("Nothing to commit..."));
                    }
                } 
                else {
                    TRACE_GLOBAL(Trace::Error, ("Page flip still in progress", _pendingFlips));
                    result = Core::ERROR_INPROGRESS;
                }

                return result;
            }
            void FinishPageFlip(const Compositor::DRM::Identifier crtc, const unsigned sequence, unsigned seconds, const unsigned useconds) {
                _connectors.Visit([&](const string& name, const Core::ProxyType<Connector> connector) {
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
            static void PageFlipHandler(int cardFd VARIABLE_IS_NOT_USED, unsigned seq, unsigned sec, unsigned usec, unsigned crtc_id VARIABLE_IS_NOT_USED, void* userData) {
                ASSERT(userData != nullptr);

                DRM* backend = reinterpret_cast<DRM*>(userData);

                backend->FinishPageFlip(crtc_id, seq, sec, usec);
            }

        private:
            std::mutex _flip;
            int _gpuFd; // GPU opened as master or lease...
            uint8_t _pendingFlips;
        }; // class DRM

        static Core::ProxyMapType<string, Backend::DRM> _backends;

    } // namespace Backend

    static DRM::Identifier FindConnectorId(const int fd, const string& connectorName)
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

    static std::string GetGPUNode(const string& connectorName)
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

    Core::ProxyType<Exchange::ICompositionBuffer> CreateBuffer(const string& connectorName, const Exchange::IComposition::Rectangle& rectangle, const Compositor::PixelFormat& format, Compositor::IOutputCallback* feedback) {
        ASSERT(drmAvailable() == 1);
        ASSERT(connectorName.empty() == false);

        TRACE_GLOBAL(Trace::Backend, ("Requesting connector '%s'", connectorName.c_str()));
        std::string gpuNodeName (GetGPUNode(connectorName));
        Core::ProxyType<Backend::DRM> backend = Backend::_backends.Instance<Backend::DRM>(gpuNodeName, gpuNodeName);

        Core::ProxyType<Exchange::ICompositionBuffer> connector;

        if (backend.IsValid()) {
            connector = Backend::_connectors.Instance<Backend::Connector>(connectorName, backend->Descriptor(), FindConnectorId(backend->Descriptor(), connectorName), rectangle, format, feedback);
        }

        return connector;
    }
} // namespace Compositor
} // Thunder
