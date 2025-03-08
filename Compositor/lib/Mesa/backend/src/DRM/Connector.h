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
#pragma once

#include "../Module.h"

#include <IOutput.h>
#include <DRM.h>
#include <DRMTypes.h>
#include <mutex>

namespace Thunder {
namespace Compositor {
    namespace Backend {
        class Connector : public IOutput {
        private:
            class FrameBufferImplementation {
            public:
                FrameBufferImplementation(FrameBufferImplementation&&) = delete;
                FrameBufferImplementation(const FrameBufferImplementation&&) = delete;
                FrameBufferImplementation& operator=(FrameBufferImplementation&&) = delete;
                FrameBufferImplementation& operator=(const FrameBufferImplementation&&) = delete;

                FrameBufferImplementation(IRenderer* renderer)
                    : _swap()
                    , _fd(-1)
                    , _activePlane(~0)
                {
                    _buffer[0] = Core::ProxyType<Exchange::ICompositionBuffer>();
                    _buffer[1] = Core::ProxyType<Exchange::ICompositionBuffer>();
                    if (renderer != nullptr) {
                        _texture[0] = renderer->Texture(_buffer[0]);
                        _texture[1] = renderer->Texture(_buffer[1]);
                    }
                }
                ~FrameBufferImplementation()
                {
                    if (IsValid() == true) {
                        Compositor::DRM::DestroyFrameBuffer(_fd, _frameId[0]);
                        Compositor::DRM::DestroyFrameBuffer(_fd, _frameId[1]);
                        _buffer[0].Release();
                        _buffer[1].Release();
                        if (_texture[0] != nullptr) {
                            _texture[0].Release();
                            _texture[1].Release();
                        }
                        ::close(_fd);
                    }
                }

            public:
                bool IsValid() const
                {
                    return (_activePlane != static_cast<uint8_t>(~0));
                }
                void Configure(const int fd, const uint32_t width, const uint32_t height, const Compositor::PixelFormat& format)
                {
                    // Configure should only be called once..
                    ASSERT(IsValid() == false);

                    _buffer[0] = Compositor::CreateBuffer(fd, width, height, format);
                    if (_buffer[0].IsValid() == false) {
                        TRACE(Trace::Error, ("Failed to create first DRM Buffer"));
                    } else {
                        _buffer[1] = Compositor::CreateBuffer(fd, width, height, format);

                        if (_buffer[1].IsValid() == false) {
                            TRACE(Trace::Error, ("Failed to create second DRM Buffer"));
                            _buffer[0].Release();
                        } else {
                            _frameId[0] = Compositor::DRM::CreateFrameBuffer(fd, _buffer[0].operator->());
                            _frameId[1] = Compositor::DRM::CreateFrameBuffer(fd, _buffer[1].operator->());
                            _activePlane = 0;
                            _fd = ::dup(fd);
                        }
                    }
                }
                IIterator* Acquire(const uint32_t timeoutMs)
                {
                    _swap.Lock();
                    return _buffer[_activePlane ^ 1]->Acquire(timeoutMs);
                }
                void Relinquish()
                {
                    _swap.Unlock();
                    _buffer[_activePlane ^ 1]->Relinquish();
                }
                uint32_t Width() const
                {
                    return (_buffer[0]->Width());
                }
                uint32_t Height() const
                {
                    return (_buffer[0]->Height());
                }
                uint32_t Format() const
                {
                    return (_buffer[0]->Format());
                }
                uint64_t Modifier() const
                {
                    return (_buffer[0]->Modifier());
                }
                Exchange::ICompositionBuffer::DataType Type() const
                {
                    return (_buffer[0]->Type());
                }
                Compositor::DRM::Identifier Id() const
                {
                    return _frameId[_activePlane];
                }
                Core::ProxyType<Exchange::ICompositionBuffer> Buffer() const
                {
                    return _buffer[_activePlane];
                }
                Core::ProxyType<IRenderer::ITexture> Texture() const
                {
                    return _texture[_activePlane];
                }
                void Swap()
                {
                    _swap.Lock();
                    _activePlane ^= 1;
                    _swap.Unlock();
                }

            private:
                Core::CriticalSection _swap;
                int _fd;
                uint8_t _activePlane;
                Core::ProxyType<Exchange::ICompositionBuffer> _buffer[2];
                Core::ProxyType<IRenderer::ITexture> _texture[2];
                Compositor::DRM::Identifier _frameId[2];
            };
            class BackendImpl : public IOutput::IBackend {
            private:
                using Connectors = std::vector<Connector*>;

            protected:
                BackendImpl(const string& gpuNode)
                    : _adminLock()
                    , _gpuNode(gpuNode)
                    , _gpuFd(Compositor::DRM::OpenGPU(_gpuNode))
                    , _flip()
                    , _connectors()
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

                            memset(&_eventContext, 0, sizeof(_eventContext));

                            _eventContext.version = 3;
                            _eventContext.page_flip_handler2 = PageFlipHandler;

                            Core::ResourceMonitor::Instance().Register(*this);
                        } else {
                            ::close(_gpuFd);
                            _gpuFd = Compositor::InvalidFileDescriptor;
                        }
                    }
                }
 
            public:
                BackendImpl() = delete;
                BackendImpl(BackendImpl&&) = delete;
                BackendImpl(const BackendImpl&) = delete;
                BackendImpl& operator=(BackendImpl&&) = delete;
                BackendImpl& operator=(const BackendImpl&) = delete;

                ~BackendImpl()
                {
                    if (_gpuFd > 0) {
                        Core::ResourceMonitor::Instance().Unregister(*this);

                        if ((drmIsMaster(_gpuFd) != 0) && (drmDropMaster(_gpuFd) != 0)) {
                            TRACE(Trace::Error, ("Could not drop DRM master. Error: [%s]", strerror(errno)));
                        }

                        if (TRACE_ENABLED(Trace::Information)) {
                            drmVersion* version = drmGetVersion(_gpuFd);
                            TRACE(Trace::Information, ("Destructing DRM backend for %s (%s)", drmGetDeviceNameFromFd2(_gpuFd), version->name));
                            drmFreeVersion(version);
                        }

                        ::close(_gpuFd);
                    }
                }

                static Core::ProxyType<BackendImpl> Instance (const string& connectorName) {
                    static Core::ProxyMapType<string, BackendImpl> backends;
                    string gpuNode (DRM::GetGPUNode(connectorName));
                    return (backends.Instance<BackendImpl>(gpuNode, gpuNode));
                }

            public:
                void Announce(Connector& connector) {
                    _adminLock.Lock();
                    Connectors::iterator entry = std::find(_connectors.begin(), _connectors.end(), &connector);
                    ASSERT(entry == _connectors.end());
                    if (entry == _connectors.end()) {
                        _connectors.emplace_back(&connector);
                    }
                    _adminLock.Unlock();
                }
                void Revoke(Connector& connector) {
                    _adminLock.Lock();
                    Connectors::iterator entry = std::find(_connectors.begin(), _connectors.end(), &connector);
                    if (entry != _connectors.end()) {
                        _connectors.erase(entry);
                    }
                    _adminLock.Unlock();
                }
                bool IsValid() const {
                    return(_gpuFd != Compositor::InvalidFileDescriptor);
                }
                uint32_t Commit(Connector&);

                //
                // Core::IResource members
                // -----------------------------------------------------------------
                handle Descriptor() const override {
                    return (_gpuFd);
                }
                uint16_t Events() override {
                    return (POLLIN | POLLPRI);
                }
                void Handle(const uint16_t events) override {
                    if (((events & POLLIN) != 0) || ((events & POLLPRI) != 0)) {
                        if (drmHandleEvent(_gpuFd, &_eventContext) != 0) {
                            TRACE(Trace::Error, ("Failed to handle drm event"));
                        }
                    }
                }
                //
                // IBackend members
                // -----------------------------------------------------------------
                const string& Node() const override {
                    return(_gpuNode);
                }
 
            private:
                void FinishPageFlip(const Compositor::DRM::Identifier /* crtc */, const unsigned sequence, unsigned seconds, const unsigned useconds)
                {
                    struct timespec presentationTimestamp;

                    presentationTimestamp.tv_sec = seconds;
                    presentationTimestamp.tv_nsec = useconds * 1000;
                    uint64_t stamp = Core::Time(presentationTimestamp).Ticks();

                    _adminLock.Lock();

                    for (auto& entry : _connectors) {
                        entry->Presented(sequence, stamp);
                    }  
                    _adminLock.Unlock();

                    TRACE(Trace::Backend, ("Pageflip finished"));

                    _flip.unlock();
                }
                static void PageFlipHandler(int cardFd VARIABLE_IS_NOT_USED, unsigned seq, unsigned sec, unsigned usec, unsigned crtc_id VARIABLE_IS_NOT_USED, void* userData)
                {
                    ASSERT(userData != nullptr);

                    reinterpret_cast<BackendImpl*>(userData)->FinishPageFlip(crtc_id, seq, sec, usec);
                }

        private:
            Core::CriticalSection _adminLock;
            const string _gpuNode;
            int _gpuFd; // GPU opened as master or lease...
            std::mutex _flip;
            uint8_t _pendingFlips;
            drmEventContext _eventContext;
            Connectors _connectors;
        };

        public:
            Connector() = delete;
            Connector(Connector&&) = delete;
            Connector(const Connector&) = delete;
            Connector& operator=(Connector&&) = delete;
            Connector& operator=(const Connector&) = delete;

            Connector(const string& connectorName, const Exchange::IComposition::Rectangle& rectangle, const PixelFormat format, IRenderer* renderer, IOutput::ICallback* feedback)
                : _backend(BackendImpl::Instance(connectorName))
                , _x(rectangle.x)
                , _y(rectangle.y)
                , _format(format)
                , _connector(_backend->Descriptor(), Compositor::DRM::object_type::Connector, Compositor::DRM::FindConnectorId(_backend->Descriptor(), connectorName))
                , _crtc()
                , _frameBuffer(renderer)
                , _feedback(feedback) {
                ASSERT(_feedback != nullptr);

                if (Scan() == false) {
                    TRACE(Trace::Backend, (("Connector %s is not in a valid state"), connectorName.c_str()));
                } else {
                    ASSERT(_frameBuffer.IsValid() == true);
                    TRACE(Trace::Backend, ("Connector %s for Id=%u Crtc=%u,", connectorName.c_str(), _connector.Id(), _crtc.Id()));
                    
                }
                _backend->Announce(*this);
            }
            ~Connector() override {
                TRACE(Trace::Backend, ("Connector %p Destroyed", this));
                _backend->Revoke(*this);
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
             * IOutput implementation
             *
             * Returning the info of the back buffer because its used to
             * draw a new frame.
             */
            int32_t X() const override {
                return _x;
            }
            int32_t Y() const override {
                return _y;
            }
            uint32_t Commit() override {
                uint32_t result(Core::ERROR_NONE);

                if (IsEnabled() == true) {
                    _backend->Commit(*this);
                } else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                return result;
            }
            IOutput::IBackend* Backend() override {
                return &(*_backend);
            }
            Core::ProxyType<IRenderer::ITexture> Texture() override {
                return (_frameBuffer.Texture());
            }

            /**
             * Connector methods
             */
            bool IsEnabled() const {
                // Todo: this will control switching on or of the display by controlling the Display Power Management Signaling (DPMS)
                return true;
            }
            const Compositor::DRM::Properties& Properties() const {
                return _connector;
            }
            const Compositor::DRM::Properties& Plane() const {
                return _primaryPlane;
            }
            const Compositor::DRM::CRTCProperties& CrtController() const {
                return _crtc;
            }
            // current framebuffer id to scan out
            Compositor::DRM::Identifier FrameBufferId() const {
                return (_frameBuffer.Id());
            }
            void Presented(const uint32_t sequence, const uint64_t pts) {
                if (_feedback != nullptr) {
                    _feedback->Presented(this, sequence, pts);
                }
            }
            void Swap() {
                _frameBuffer.Swap();
            }

        private:
            bool Scan() {
                bool result(false);

                int backendFd = _backend->Descriptor();

                drmModeResPtr drmModeResources = drmModeGetResources(backendFd);
                drmModePlaneResPtr drmModePlaneResources = drmModeGetPlaneResources(backendFd);

                ASSERT(drmModeResources != nullptr);
                ASSERT(drmModePlaneResources != nullptr);

                drmModeEncoderPtr encoder = NULL;

                TRACE(Trace::Backend, ("Found %d connectors", drmModeResources->count_connectors));
                TRACE(Trace::Backend, ("Found %d encoders", drmModeResources->count_encoders));
                TRACE(Trace::Backend, ("Found %d CRTCs", drmModeResources->count_crtcs));
                TRACE(Trace::Backend, ("Found %d planes", drmModePlaneResources->count_planes));

                for (int c = 0; c < drmModeResources->count_connectors; c++) {
                    drmModeConnectorPtr drmModeConnector = drmModeGetConnector(backendFd, drmModeResources->connectors[c]);

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
                    uint32_t _possibleCrtcs = drmModeConnectorGetPossibleCrtcs(backendFd, drmModeConnector);

                    if (_possibleCrtcs == 0) {
                        TRACE(Trace::Error, ("No CRTC possible for id", drmModeConnector->connector_id));
                    }

                    ASSERT(_possibleCrtcs != 0);

                    for (uint8_t e = 0; e < drmModeResources->count_encoders; e++) {
                        if (drmModeResources->encoders[e] == drmModeConnector->encoder_id) {
                            encoder = drmModeGetEncoder(backendFd, drmModeResources->encoders[e]);
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
                                crtc = drmModeGetCrtc(backendFd, drmModeResources->crtcs[c]);
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
                            plane = drmModeGetPlane(backendFd, drmModePlaneResources->planes[p]);

                            TRACE(Trace::Backend, ("[PLANE: %" PRIu32 "] CRTC ID %" PRIu32 ", FB ID %" PRIu32, plane->plane_id, plane->crtc_id, plane->fb_id));

                            if ((plane->crtc_id == crtc->crtc_id) && (plane->fb_id == crtc->buffer_id)) {
                                break;
                            }
                        }

                        ASSERT(plane);

                        uint64_t refresh = ((crtc->mode.clock * 1000000LL / crtc->mode.htotal) + (crtc->mode.vtotal / 2)) / crtc->mode.vtotal;

                        TRACE(Trace::Backend, ("[CRTC:%" PRIu32 ", CONN %" PRIu32 ", PLANE %" PRIu32 "]: active at %ux%u, %" PRIu64 " mHz", crtc->crtc_id, drmModeConnector->connector_id, plane->plane_id, crtc->width, crtc->height, refresh));

                        _crtc.Load(backendFd, crtc);
                        _primaryPlane.Load(backendFd, Compositor::DRM::object_type::Plane, plane->plane_id);

                        Compositor::DRM::Properties primaryPlane(backendFd, Compositor::DRM::object_type::Plane, plane->plane_id);

                        _frameBuffer.Configure(backendFd, crtc->width, crtc->height, _format);

                        // _refreshRate = crtc->mode.vrefresh;

                        plane = drmModeGetPlane(backendFd, primaryPlane.Id());

                        drmModeFreeCrtc(crtc);
                        drmModeFreePlane(plane);

                        drmModeFreeEncoder(encoder);

                        result = ((_crtc.Id() != Compositor::InvalidIdentifier) && (primaryPlane.Id() != Compositor::InvalidIdentifier));

                    } else {
                        TRACE(Trace::Error, ("Failed to get encoder for id %u", drmModeConnector->connector_id));
                    }

                    break;
                }

                drmModeFreeResources(drmModeResources);
                drmModeFreePlaneResources(drmModePlaneResources);

                return result;
            }

        private:
            Core::ProxyType<BackendImpl> _backend;
            int32_t _x;
            int32_t _y;
            const Compositor::PixelFormat _format;
            Compositor::DRM::Properties _connector;
            Compositor::DRM::CRTCProperties _crtc;
            Compositor::DRM::Properties _primaryPlane;

            FrameBufferImplementation _frameBuffer;

            Compositor::IOutput::ICallback* _feedback;
        };
    }
}
}
