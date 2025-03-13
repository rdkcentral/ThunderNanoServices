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

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace Thunder {
    namespace Compositor {
        namespace Backend {
            class Transaction {
            public:
                Transaction() = delete;
                Transaction(Transaction&&) = delete;
                Transaction(const Transaction&) = delete;
                Transaction& operator=(Transaction&&) = delete;
                Transaction& operator=(const Transaction&) = delete;

                Transaction(const int fd)
                    : _fd(fd)
                    , _request(drmModeAtomicAlloc())
                    , _error(Core::ERROR_NOT_EXIST) {
                    ASSERT(fd > 0);
                    ASSERT(_request != nullptr);
                }
                ~Transaction() {
                    if (_request != nullptr) {
                        drmModeAtomicFree(_request);
                    }
                }

            public:
                bool HasData() const {
                    return (_error == Core::ERROR_NONE);
                }
                void Add (Connector& entry) {
                    // We will schedule the connector for swapping buffers. Tell the connector to
                    // swap. We ask the connector to swap to make sure that if an update on its
                    // framebuffer is still in progress, it will continue to completeion before
                    // swapping the buffers.
                    const uint32_t connectorId(entry.Properties().Id());
                    const uint32_t crtcId(entry.CrtController().Id());
                    const uint32_t planeId(entry.Plane().Id());
                    const Compositor::DRM::Identifier fbId(entry.ActiveFrameBufferId());

                    TRACE(Trace::Backend, ("Commit for connector: %d, CRTC: %d, Plane: %d, Framebuffer: %d", connectorId, crtcId, planeId, fbId));

                    constexpr uint32_t x = 0;
                    constexpr uint32_t y = 0;
                    const uint32_t width(entry.Width());
                    const uint32_t height(entry.Height());

                    if ( (Property(connectorId, entry.Properties().Id(DRM::property::CrtcId), crtcId)           == false) ||
                         (Property(crtcId, entry.CrtController().Id(DRM::property::ModeId), entry.ModeSetId())  == false) ||
                         (Property(crtcId, entry.CrtController().Id(DRM::property::Active), 1)                  == false) ||
                         (Property(planeId, entry.Plane().Id(DRM::property::FbId), fbId)                        == false) ||
                         (Property(planeId, entry.Plane().Id(DRM::property::CrtcId), crtcId)                    == false) ||
                         (Property(planeId, entry.Plane().Id(DRM::property::SrcX), 0)                           == false) ||
                         (Property(planeId, entry.Plane().Id(DRM::property::SrcY), 0)                           == false) ||
                         (Property(planeId, entry.Plane().Id(DRM::property::SrcW), uint64_t(width << 16))       == false) ||
                         (Property(planeId, entry.Plane().Id(DRM::property::SrcH), uint64_t(height << 16))      == false) ||
                         (Property(planeId, entry.Plane().Id(DRM::property::CrtcX), x)                          == false) ||
                         (Property(planeId, entry.Plane().Id(DRM::property::CrtcY), y)                          == false) ||
                         (Property(planeId, entry.Plane().Id(DRM::property::CrtcW), width)                      == false) ||
                         (Property(planeId, entry.Plane().Id(DRM::property::CrtcH), height)                     == false) ) {
                        _error = Core::ERROR_GENERAL;
                    }
                    else if (_error == Core::ERROR_NOT_EXIST) {
                        _error = Core::ERROR_NONE;
                    }
                }
                uint32_t Commit(void* userData) {
                    int drmResult;

                    if ((drmResult = drmModeAtomicCommit(_fd, _request, (DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK), userData)) < 0) {
                        TRACE(Trace::Error, ("Atomic commit failed: [%d] %s", drmResult, strerror(errno)));
                    }

                    return (drmResult == 0) ? Core::ERROR_NONE : Core::ERROR_BAD_REQUEST;
                }
                uint32_t Initial() {
                    int drmResult;
                    // perform test-only atomic commit
	            if ((drmResult = drmModeAtomicCommit(_fd, _request, (DRM_MODE_ATOMIC_TEST_ONLY|DRM_MODE_ATOMIC_ALLOW_MODESET), nullptr) ) < 0) {
                        TRACE(Trace::Error, ("Atomic test commit failed: [%d] %s", drmResult, strerror(errno)));
                        fprintf(stdout, "Atomic test commit failed: [%d] %s and (%d) ", drmResult, strerror(errno), _fd);
                    } 
	            else if ((drmResult = drmModeAtomicCommit(_fd, _request, (DRM_MODE_ATOMIC_ALLOW_MODESET), nullptr) ) < 0) {
                        TRACE(Trace::Error, ("Atomic initial commit failed: [%d] %s", drmResult, strerror(errno)));
                        fprintf(stdout, "Atomic initial commit failed: [%d] %s and (%d) ", drmResult, strerror(errno), _fd);
                    }

                    return (drmResult == 0) ? Core::ERROR_NONE : Core::ERROR_BAD_REQUEST;
                }

            private:
                bool Property(uint32_t objectId, uint32_t propertyId, uint64_t value)
                {
                    int presult;

                    ASSERT(objectId != DRM::InvalidIdentifier);
                    ASSERT(_request != nullptr);

                    TRACE(Trace::Backend, ("ObjectId[%u] Add propertyId %u to %" PRIu64, objectId, propertyId, value));

                    if ((presult = drmModeAtomicAddProperty(_request, objectId, propertyId, value)) < 0) {
                        TRACE(Trace::Error, ("Failed to add atomic DRM property %u: %s", propertyId, strerror(-presult)));
                        return (false);
                    }

                    return true;
                }

            private:
                const int _fd;
                drmModeAtomicReqPtr _request;
                uint32_t _error;
            };
        } // namespace Backend
    } // namespace Compositor
} // namespace Thunder
