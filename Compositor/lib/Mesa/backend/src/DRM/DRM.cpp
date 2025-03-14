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
#include "Connector.h"

#ifdef USE_ATOMIC
#include "Atomic.h"
#else
#include "Legacy.h"
#endif

#include <interfaces/IComposition.h>

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
            uint32_t Connector::BackendImpl::Commit(Connector& connector) {
                Transaction transaction(_gpuFd);
                transaction.Add(connector);
                return (transaction.Commit(this));
            }

            void Connector::BackendImpl::PageFlip(const unsigned int sequence, const unsigned sec, const unsigned usec) {
                static uint64_t previous = 0;

                struct timespec presentationTimestamp;

                Transaction transaction(_gpuFd);

                presentationTimestamp.tv_sec = sec;
                presentationTimestamp.tv_nsec = usec* 1000;
                uint64_t stamp = Core::Time(presentationTimestamp).Ticks();

                uint32_t elapsed = (stamp - previous) / Core::Time::TicksPerMillisecond;
                float fps = 1 / (elapsed / 1000.0f);
                previous = stamp;

 fprintf(stdout, "------------------------------- %s -------------------- %d ------Elapsed: %d mS ------- FPS [%f]---- \n", __FUNCTION__, __LINE__, elapsed, fps);
                _adminLock.Lock();

                for (Connector*& entry : _connectors) {
                    if (entry->Published(sequence, stamp) == true) {
                        transaction.Add(*entry);
                    }
                }

                _adminLock.Unlock();

                if ( (transaction.HasData() == true) && (transaction.Commit(this) != Core::ERROR_NONE) ) {
                    TRACE(Trace::Error, ("Failed to commit a Flip transaction"));
                }
            }

            bool Connector::Scan() {
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
                        _blob.Load(backendFd, static_cast<const drmModeModeInfo*>(_crtc), sizeof(drmModeModeInfo));
                        _primaryPlane.Load(backendFd, Compositor::DRM::object_type::Plane, plane->plane_id);

                        Compositor::DRM::Properties primaryPlane(backendFd, Compositor::DRM::object_type::Plane, plane->plane_id);

                        _frameBuffer.Configure(backendFd, crtc->width, crtc->height, _format);
                        Transaction transaction(_backend->Descriptor());
                        transaction.Add(*this);
                        ASSERT (transaction.HasData());
                        if (transaction.Initial() != Core::ERROR_NONE) {
                            TRACE(Trace::Error, ("Failed to initialize transaction based display"));
                        }

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
        } 

    Core::ProxyType<IOutput> CreateBuffer(
        const string& connectorName, 
        const Exchange::IComposition::Rectangle& rectangle, 
        const PixelFormat& format, 
        const Core::ProxyType<IRenderer>& renderer, 
        IOutput::ICallback* feedback)
    {
        Core::ProxyType<IOutput> result;

        ASSERT(drmAvailable() == 1);
        ASSERT(connectorName.empty() == false);

        static Core::ProxyMapType<string, Backend::Connector> connectors;

        TRACE_GLOBAL(Trace::Backend, ("Requesting connector '%s'", connectorName.c_str()));

        Core::ProxyType<Backend::Connector> connector = connectors.Instance<Backend::Connector>(connectorName, connectorName, rectangle, format, renderer, feedback);

        if ( (connector.IsValid()) && (connector->IsValid()) ) {
            result = Core::ProxyType<IOutput>(connector);
        }

        return result;
    }

    } // namespace Compositor
} // Thunder
