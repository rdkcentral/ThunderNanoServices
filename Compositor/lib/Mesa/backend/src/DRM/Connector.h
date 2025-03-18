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
            private:
                enum mode : uint8_t {
                    FRAMEBUFFER_INDEX  = 0x01,
                    PENDING_COMMIT     = 0x20,
                    PENDING_PUBLISH    = 0x40,
                    DISABLED           = 0x80
                 };

            public:
                FrameBufferImplementation(FrameBufferImplementation&&) = delete;
                FrameBufferImplementation(const FrameBufferImplementation&&) = delete;
                FrameBufferImplementation& operator=(FrameBufferImplementation&&) = delete;
                FrameBufferImplementation& operator=(const FrameBufferImplementation&&) = delete;

                FrameBufferImplementation(const Core::ProxyType<IRenderer>& renderer)
                    : _swap()
                    , _fd(-1)
                    , _activePlane(~0)
                    , _renderer(renderer)
                {
                    _buffer[0] = Core::ProxyType<Exchange::IGraphicsBuffer>();
                    _buffer[1] = Core::ProxyType<Exchange::IGraphicsBuffer>();
                    _texture[0] = Core::ProxyType<IRenderer::ITexture>();
                    _texture[1] = Core::ProxyType<IRenderer::ITexture>();
                }
                ~FrameBufferImplementation() {
                    Destroy();
                }

            public:
                bool IsValid() const {
                    return (_activePlane != static_cast<uint8_t>(~0));
                }
                void Destroy() {
                    if (_activePlane != static_cast<uint8_t>(~0)) {
                        Compositor::DRM::DestroyFrameBuffer(_fd, _frameId[0]);
                        Compositor::DRM::DestroyFrameBuffer(_fd, _frameId[1]);
                        _buffer[0].Release();
                        _buffer[1].Release();
                        _texture[0].Release();
                        _texture[1].Release();
                        ::close(_fd);
                        _activePlane = ~0;
                    }
                }
                void Configure(const int fd, const uint32_t width, const uint32_t height, const Compositor::PixelFormat& format) {
                    // Configure should only be called once..
                    ASSERT(IsValid() == false)

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
                            if (_renderer.IsValid()) {
                                _texture[0] = _renderer->Texture(_buffer[0]);
                                _texture[1] = _renderer->Texture(_buffer[1]);
                            }
                            _activePlane = 1;
                            _fd = ::dup(fd);
                       }
                    }
                }
                IIterator* Acquire(const uint32_t timeoutMs) {
                    _swap.Lock();
                    return _buffer[_activePlane & mode::FRAMEBUFFER_INDEX]->Acquire(timeoutMs);
                }
                void Relinquish() {
                    _swap.Unlock();
                    _buffer[_activePlane & mode::FRAMEBUFFER_INDEX]->Relinquish();
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
                Exchange::IGraphicsBuffer::DataType Type() const {
                    return (_buffer[0]->Type());
                }
                Core::ProxyType<IRenderer::ITexture> Texture() const {
                    return _texture[_activePlane & mode::FRAMEBUFFER_INDEX];
                }
                Compositor::DRM::Identifier Id() const {
                    return _frameId[(_activePlane & mode::FRAMEBUFFER_INDEX) ^ mode::FRAMEBUFFER_INDEX];
                }
                bool Commit() {
                    bool issueCommit (false);
                    _swap.Lock();
                    if ((_activePlane & mode::PENDING_PUBLISH) != 0) {
                        _activePlane |= mode::PENDING_COMMIT;
                    }
                    else {
                        issueCommit = true;

                        // Make sure another process/client is not writing on this buffer while we need to swap it :-)
                        uint8_t lockBuffer = (_activePlane & mode::FRAMEBUFFER_INDEX);
                        if (_buffer[lockBuffer]->Acquire(50) == nullptr) {
                            TRACE(Trace::Error, ("Could not lock a buffer to swap FrameBuffers"));
                        }
                        else {
                            _activePlane = mode::PENDING_PUBLISH | ((_activePlane & mode::FRAMEBUFFER_INDEX) ^ mode::FRAMEBUFFER_INDEX);
                           _buffer[lockBuffer]->Relinquish();
                        }
                    }
                    _swap.Unlock();
                    return (issueCommit);
                }
                bool Published(bool& commit) {
                    bool publish(false);
                    _swap.Lock();
                    if ((_activePlane & mode::PENDING_COMMIT) != 0) {
                        // Seems we need to swap and keep the publish
                        // Make sure another process/client is not writing on this buffer while we need to swap it :-)
                        uint8_t lockBuffer = (_activePlane & mode::FRAMEBUFFER_INDEX);
                        if (_buffer[lockBuffer]->Acquire(50) == nullptr) {
                            TRACE(Trace::Error, ("Could not lock a buffer to swap FrameBuffers"));
                        }
                        else {
                            _activePlane = mode::PENDING_PUBLISH | ((_activePlane & mode::FRAMEBUFFER_INDEX) ^ mode::FRAMEBUFFER_INDEX);
                            _buffer[lockBuffer]->Relinquish();
                        }
                    }
                    else {
                       commit = false;
                       publish = ((_activePlane & mode::PENDING_PUBLISH) != 0);
                       _activePlane &= ~(mode::PENDING_PUBLISH);
                    }
                    _swap.Unlock();
                    return (publish);
                }

            private:
                Core::CriticalSection _swap;
                int _fd;
                uint8_t _activePlane;
                Core::ProxyType<IRenderer> _renderer;
                Core::ProxyType<Exchange::IGraphicsBuffer> _buffer[2];
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
                    , _connectors() {
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
                uint32_t Commit(Connector&);

            private:
                void PageFlip(const unsigned int sequence, const unsigned sec, const unsigned usec);
                static void PageFlipHandler(int cardFd VARIABLE_IS_NOT_USED, unsigned seq, unsigned sec, unsigned usec, unsigned crtc_id VARIABLE_IS_NOT_USED, void* userData)
                {
                    ASSERT(userData != nullptr);

                    reinterpret_cast<BackendImpl*>(userData)->PageFlip(seq, sec, usec);
                }

        private:
            Core::CriticalSection _adminLock;
            const string _gpuNode;
            int _gpuFd; // GPU opened as master or lease...
            drmEventContext _eventContext;
            Connectors _connectors;
        };

        public:
            Connector() = delete;
            Connector(Connector&&) = delete;
            Connector(const Connector&) = delete;
            Connector& operator=(Connector&&) = delete;
            Connector& operator=(const Connector&) = delete;

            Connector(const string& connectorName, const uint32_t, const uint32_t, const PixelFormat format, const Core::ProxyType<IRenderer>& renderer, IOutput::ICallback* feedback)
                : _backend(BackendImpl::Instance(connectorName))
                , _format(format)
                , _connector(_backend->Descriptor(), Compositor::DRM::object_type::Connector, Compositor::DRM::FindConnectorId(_backend->Descriptor(), connectorName))
                , _crtc()
                , _primaryPlane()
                , _blob()
                , _frameBuffer(renderer)
                , _feedback(feedback) {

                ASSERT(_feedback != nullptr);

                if (Scan() == false) {
                    TRACE(Trace::Backend, (("Connector %s is not in a valid state"), connectorName.c_str()));
                } else {
                    ASSERT(_frameBuffer.IsValid() == true);
                    TRACE(Trace::Backend, ("Connector %s for Id=%u Crtc=%u,", connectorName.c_str(), _connector.Id(), _crtc.Id()));
                    _blob.Load(_backend->Descriptor(), static_cast<const void*>(static_cast<const drmModeModeInfo*>(_crtc)), sizeof(drmModeModeInfo));
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
             * Exchange::IGraphicsBuffer implementation
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
            Exchange::IGraphicsBuffer::DataType Type() const {
                return (_frameBuffer.Type());
            }

            /**
             * IOutput implementation
             *
             * Returning the info of the back buffer because its used to
             * draw a new frame.
             */
            uint32_t Commit() override {
                uint32_t result = Core::ERROR_NONE;
                if (_frameBuffer.Commit() == true) {
                    result = _backend->Commit(*this);
                }
                return (result);
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
            // current framebuffer id to scan out
            Compositor::DRM::Identifier ActiveFrameBufferId() const {
                return (_frameBuffer.Id());
            }
            Compositor::DRM::Identifier ModeSetId() const {
                return (_blob.Id());
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
            bool Published(const unsigned int sequence, const uint64_t stamp) {
                bool commit;
                if ((_frameBuffer.Published(commit) == true) && (_feedback != nullptr)) {
                    _feedback->Presented(this, sequence, stamp);
                }
                return (commit);
            }

        private:
            bool Scan();

        private:
            Core::ProxyType<BackendImpl> _backend;
            const Compositor::PixelFormat _format;
            Compositor::DRM::Properties _connector;
            Compositor::DRM::CRTCProperties _crtc;
            Compositor::DRM::Properties _primaryPlane;
            Compositor::DRM::Property::Blob _blob;

            FrameBufferImplementation _frameBuffer;

            Compositor::IOutput::ICallback* _feedback;
        };
    }
}
}
