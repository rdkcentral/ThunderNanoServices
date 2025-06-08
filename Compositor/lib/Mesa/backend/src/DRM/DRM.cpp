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
#include "Connector.h"

#ifdef USE_ATOMIC
#include "Atomic.h"
#else
#include "Legacy.h"
#endif

#include <IBuffer.h>
#include <IOutput.h>
#include <interfaces/IComposition.h>

// #define __GBM__

#include <DRM.h>
#include <DRMTypes.h>

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
    static Core::ResourceMonitorType<Core::IResource, Core::Void, 0, 1> _resourceMonitor;

    namespace Backend {
        class DRM : public Compositor::IBackend {
        public:
            enum class State : uint8_t {
                IDLE = 0x00,
                FLIPPING = 0x01,
                PENDING = 0x02,
            };

            DRM() = delete;
            DRM(DRM&&) = delete;
            DRM(const DRM&) = delete;
            DRM& operator=(DRM&&) = delete;
            DRM& operator=(const DRM&) = delete;

            explicit DRM(const string& gpuNode)
                : _state(State::IDLE)
                , _gpuFd(Compositor::DRM::OpenGPU(gpuNode))
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
            ~DRM() override
            {
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
            bool IsValid() const
            {
                return (_gpuFd > 0);
            }
            Core::ProxyType<Connector> GetConnector(const string& connectorName, const uint32_t width, const uint32_t height, const Compositor::PixelFormat& format, const Core::ProxyType<IRenderer>& renderer, Compositor::IOutput::ICallback* feedback)
            {
                return _connectors.Instance<Connector>(connectorName, Core::ProxyType<Compositor::IBackend>(*this, *this), Compositor::DRM::FindConnectorId(_gpuFd, connectorName), width, height, format, renderer, feedback);
            }

            //
            // Core::IResource members
            // -----------------------------------------------------------------
            handle Descriptor() const override
            {
                return (_gpuFd);
            }
            uint16_t Events() override
            {
                return (POLLIN | POLLPRI);
            }
            void Handle(const uint16_t events) override
            {
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
            uint32_t Commit(Compositor::DRM::Identifier connectorId VARIABLE_IS_NOT_USED) override
            {
                uint32_t result(Core::ERROR_GENERAL);

                State expected(State::IDLE);

                if (_state.compare_exchange_strong(expected, State::FLIPPING) == true) {
                    result = Core::ERROR_NONE;

                    static bool doModeSet(true);

                    Backend::Transaction transaction(_gpuFd, doModeSet, this);

                    _connectors.Visit([&](const string& name, const Core::ProxyType<Connector> connector) {
                        connector->Swap();

                        uint32_t outcome;

                        if ((outcome = transaction.Add(*(connector.operator->())) != Core::ERROR_NONE)) {
                            TRACE_GLOBAL(Trace::Error, ("Failed to add connector %s to transaction, error: %d", name.c_str(), outcome));
                        }
                    });

                    if ((result = transaction.Commit()) != Core::ERROR_NONE) {
                        _connectors.Visit([&](const string& /*name*/, const Core::ProxyType<Connector> connector) {
                            connector->Presented(0, 0); // notify connector implementation the buffer failed to display.
                        });

                        TRACE_GLOBAL(Trace::Error, ("Failed to commit connectors, no buffers will be displayed"));
                    }

                    doModeSet = transaction.ModeSet();

                    TRACE_GLOBAL(Trace::Information, ("Committed connectors: %u", result));
                } else {
                    State expected(State::FLIPPING);

                    if (_state.compare_exchange_strong(expected, State::PENDING) == true) {
                        TRACE_GLOBAL(Trace::Error, ("Page flip still in progress, handle this request later to grantee the new content will be displayed"));
                        result = Core::ERROR_INPROGRESS;
                    }
                }

                return result;
            }
            void FinishPageFlip(const Compositor::DRM::Identifier crtc, const unsigned sequence, unsigned seconds, const unsigned useconds)
            {
                _connectors.Visit([&](const string& name, const Core::ProxyType<Connector> connector) {
                    if (connector->CrtController().Id() == crtc) {
                        struct timespec presentationTimestamp;

                        presentationTimestamp.tv_sec = seconds;
                        presentationTimestamp.tv_nsec = useconds * 1000;

                        connector->Presented(sequence, Core::Time(presentationTimestamp).Ticks());
                        // TODO: Check with Bram the intention of the Release()
                        // connector->Release();

                        TRACE(Trace::Backend, ("Pageflip finished for %s", name.c_str()));
                    }
                });

                State expected = State::FLIPPING;

                if (_state.compare_exchange_strong(expected, State::IDLE) == false) {
                    expected = State::PENDING;

                    if (_state.compare_exchange_strong(expected, State::FLIPPING) == true) {
                        Commit(Compositor::DRM::InvalidIdentifier); // re-commit if there was a pending request.
                    }
                }
            }
            static void PageFlipHandler(int cardFd VARIABLE_IS_NOT_USED, unsigned seq, unsigned sec, unsigned usec, unsigned crtc_id, void* userData)
            {
                ASSERT(userData != nullptr);

                DRM* backend = reinterpret_cast<DRM*>(userData);

                backend->FinishPageFlip(crtc_id, seq, sec, usec);
            }

        private:
            std::atomic<State> _state;
            int _gpuFd; // GPU opened as master or lease...
            Core::ProxyMapType<string, Connector> _connectors;
        }; // class DRM

        static Core::ProxyMapType<string, Backend::DRM> _backends;

    } // namespace Backend

    Core::ProxyType<IOutput> CreateBuffer(const string& connectorName,
        const uint32_t width, const uint32_t height, const Compositor::PixelFormat& format,
        const Core::ProxyType<IRenderer>& renderer, Compositor::IOutput::ICallback* feedback)
    {
        ASSERT(drmAvailable() == 1);
        ASSERT(connectorName.empty() == false);

        TRACE_GLOBAL(Trace::Backend, ("Requesting connector '%s'", connectorName.c_str()));
        std::string gpuNodeName(DRM::GetGPUNode(connectorName));
        Core::ProxyType<Backend::DRM> backend = Backend::_backends.Instance<Backend::DRM>(gpuNodeName, gpuNodeName);

        Core::ProxyType<IOutput> connector;

        if (backend.IsValid()) {
            connector = backend->GetConnector(connectorName, width, height, format, renderer, feedback);
        }

        return connector;
    }
} // namespace Compositor
} // Thunder
