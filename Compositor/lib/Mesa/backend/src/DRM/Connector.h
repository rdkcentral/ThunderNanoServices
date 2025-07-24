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
#include "IGpu.h"

#include <IOutput.h>
#include <DRM.h>
#include <DRMTypes.h>

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

                FrameBufferImplementation()
                    : _swap()
                    , _fd(-1)
                    , _activePlane(~0)
                {
                    // Double buffering: 0=current, 1=next, swap via XOR
                    _buffer[0] = Core::ProxyType<Exchange::IGraphicsBuffer>();
                    _buffer[1] = Core::ProxyType<Exchange::IGraphicsBuffer>();
                }
                ~FrameBufferImplementation()
                {
                    if (IsValid() == true) {
                        Compositor::DRM::DestroyFrameBuffer(_fd, _frameId[0]);
                        Compositor::DRM::DestroyFrameBuffer(_fd, _frameId[1]);
                        _frameBuffer[0].Release();
                        _frameBuffer[1].Release();
                        _buffer[0].Release();
                        _buffer[1].Release();
                        ::close(_fd);
                    }
                }

            public:
                bool IsValid() const
                {
                    return (_activePlane != static_cast<uint8_t>(~0));
                }
                void Configure(const int fd, const uint32_t width, const uint32_t height, const Compositor::PixelFormat& format, const Core::ProxyType<IRenderer>& renderer)
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

                            if (renderer.IsValid() == true) {
                                _frameBuffer[0] = renderer->FrameBuffer(_buffer[0]);
                                _frameBuffer[1] = renderer->FrameBuffer(_buffer[1]);
                            }

                            _activePlane = 0;
                            _fd = ::dup(fd);
                        }
                    }
                }

                PUSH_WARNING(DISABLE_WARNING_OVERLOADED_VIRTUALS)
                IIterator* Acquire(const uint32_t timeoutMs)
                {
                    _swap.Lock();
                    return _buffer[_activePlane ^ 1]->Acquire(timeoutMs); // Get next buffer
                }
                void Relinquish()
                {
                    _swap.Unlock();
                    _buffer[_activePlane ^ 1]->Relinquish();
                }
                POP_WARNING()

                uint32_t Width() const
                {
                    return (_buffer[0]->Width()); // Both buffers have same dimensions
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
                Exchange::IGraphicsBuffer::DataType Type() const
                {
                    return (_buffer[0]->Type());
                }
                Compositor::DRM::Identifier Id() const
                {
                    return _frameId[_activePlane]; // Current buffer's frame ID
                }
                Core::ProxyType<Exchange::IGraphicsBuffer> Buffer() const
                {
                    return _buffer[_activePlane];
                }
                void Swap()
                {
                    _swap.Lock();
                    _activePlane ^= 1;
                    _swap.Unlock();
                }

                Core::ProxyType<Compositor::IRenderer::IFrameBuffer> FrameBuffer() const
                {
                    return (_frameBuffer[_activePlane ^ 1]);
                }

            private:
                Core::CriticalSection _swap;
                int _fd;
                uint8_t _activePlane;
                Core::ProxyType<Exchange::IGraphicsBuffer> _buffer[2];
                Compositor::DRM::Identifier _frameId[2];
                Core::ProxyType<Compositor::IRenderer::IFrameBuffer> _frameBuffer[2];
            };

        public:
            Connector() = delete;
            Connector(Connector&&) = delete;
            Connector(const Connector&) = delete;
            Connector& operator=(Connector&&) = delete;
            Connector& operator=(const Connector&) = delete;

            Connector(const Core::ProxyType<Compositor::IBackend>& backend, Compositor::DRM::Identifier connectorId, const uint32_t width, const uint32_t height, const Compositor::PixelFormat format, const Core::ProxyType<IRenderer>& renderer, Compositor::IOutput::ICallback* feedback)
                : _backend(backend)
                , _requestedWidth(width)
                , _requestedHeight(height)
                , _format(format)
                , _connector(_backend->Descriptor(), Compositor::DRM::object_type::Connector, connectorId)
                , _crtc()
                , _frameBuffer()
                , _feedback(feedback)
                , _renderer(renderer)
                , _needsModeSet(false)
                , _selectedMode({})
                , _dimensionsAdjusted(false)
            {
                ASSERT(_feedback != nullptr);

                int backendFd = _backend->Descriptor();

                Compositor::DRM::ConnectorScanResult scanResult = Compositor::DRM::ScanConnector(backendFd, connectorId, width, height);

                if (!scanResult.success) {
                    TRACE(Trace::Backend, ("Connector %d is not in a valid state", connectorId));
                } else {
                    // Apply scan results
                    _needsModeSet = scanResult.needsModeSet;
                    _selectedMode = scanResult.selectedMode;
                    _dimensionsAdjusted = scanResult.dimensionsAdjusted;
                    _gpuNode = scanResult.gpuNode;

                    // Initialize DRM properties using the scanned IDs
                    _crtc.Load(backendFd, Compositor::DRM::object_type::Crtc, scanResult.ids.crtcId);
                    _primaryPlane.Load(backendFd, Compositor::DRM::object_type::Plane, scanResult.ids.planeId);

                    ASSERT((_selectedMode.hdisplay != 0) && (_selectedMode.vdisplay != 0));

                    _frameBuffer.Configure(backendFd, _selectedMode.hdisplay, _selectedMode.vdisplay, _format, _renderer);

                    ASSERT(_frameBuffer.IsValid() == true);
                    TRACE(Trace::Backend, ("Connector %p for Id=%u Crtc=%u, Resolution=%ux%u", this, _connector.Id(), _crtc.Id(), _selectedMode.hdisplay, _selectedMode.vdisplay));

                    if (_dimensionsAdjusted) {
                        TRACE(Trace::Warning, ("Requested dimensions %ux%u adjusted to %ux%u for connector %u", _requestedWidth, _requestedHeight, _selectedMode.hdisplay, _selectedMode.vdisplay, connectorId));
                    }
                }
            }

            ~Connector() override
            {
                TRACE(Trace::Backend, ("Connector %p Destroyed", this));
            }

        public:
            bool IsValid() const
            {
                return (_frameBuffer.IsValid());
            }

            bool NeedsModeSet() const
            {
                return _needsModeSet;
            }

            const drmModeModeInfo* SelectedMode() const
            {
                return &_selectedMode;
            }

            /**
             * Exchange::IGraphicsBuffer implementation
             *
             * Returning the info of the back buffer because its used to
             * draw a new frame.
             */

            PUSH_WARNING(DISABLE_WARNING_OVERLOADED_VIRTUALS)
            IIterator* Acquire(const uint32_t timeoutMs) override
            {
                return (_frameBuffer.Acquire(timeoutMs));
            }
            void Relinquish() override
            {
                _frameBuffer.Relinquish();
            }
            POP_WARNING()

            uint32_t Width() const override
            {
                return (_frameBuffer.Width());
            }
            uint32_t Height() const override
            {
                return (_frameBuffer.Height());
            }
            uint32_t Format() const override
            {
                return (_frameBuffer.Format());
            }
            uint64_t Modifier() const override
            {
                return (_frameBuffer.Modifier());
            }
            Exchange::IGraphicsBuffer::DataType Type() const
            {
                return (_frameBuffer.Type());
            }

            /**
             * Connector methods
             */
            bool IsEnabled() const
            {
                // Todo: this will control switching on or of the display by controlling the Display Power Management Signaling (DPMS)
                return true;
            }
            const Compositor::DRM::Properties& Properties() const
            {
                return _connector;
            }
            const Compositor::DRM::Properties& Plane() const
            {
                return _primaryPlane;
            }
            const Compositor::DRM::Properties& CrtController() const
            {
                return _crtc;
            }
            // current framebuffer id to scan out
            Compositor::DRM::Identifier FrameBufferId() const
            {
                return (_frameBuffer.Id());
            }
            void Presented(const uint32_t sequence, const uint64_t pts)
            {
                if (_feedback != nullptr) {
                    _feedback->Presented(this, sequence, pts);
                }
            }
            void Swap()
            {
                _frameBuffer.Swap();
            }

            // IOutput methods
            uint32_t Commit()
            {
                uint32_t result(Core::ERROR_ILLEGAL_STATE);

                if (IsEnabled() == true) {
                    result = _backend->Commit(_connector.Id());
                }

                return result;
            }

            const string& Node() const
            {
                return _gpuNode;
            }
            Core::ProxyType<Compositor::IRenderer::IFrameBuffer> FrameBuffer() const override
            {
                return _frameBuffer.FrameBuffer();
            }

        private:
            Core::ProxyType<Compositor::IBackend> _backend;
            uint32_t _requestedWidth;
            uint32_t _requestedHeight;
            const Compositor::PixelFormat _format;
            Compositor::DRM::Properties _connector;
            Compositor::DRM::Properties _crtc;
            Compositor::DRM::Properties _primaryPlane;

            string _gpuNode;

            FrameBufferImplementation _frameBuffer;

            Compositor::IOutput::ICallback* _feedback;
            const Core::ProxyType<IRenderer>& _renderer;

            bool _needsModeSet;
            drmModeModeInfo _selectedMode;
            bool _dimensionsAdjusted;
        };
    }
}
}