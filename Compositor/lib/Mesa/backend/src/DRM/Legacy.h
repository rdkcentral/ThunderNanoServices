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

#include <DRM.h>
#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace Thunder {
    namespace Compositor {
        namespace Backend {

            class EXTERNAL Transaction {
            public:
                Transaction() = delete;
                Transaction(Transaction&&) = delete;
                Transaction(const Transaction&) = delete;
                Transaction& operator= (Transaction&&) = delete;
                Transaction& operator= (const Transaction&) = delete;

                Transaction(const int fd) 
                    : _fd(fd) {
                }
                ~Transaction() = default;

            public:
                uint32_t Add (Connector& connector) {

                    uint32_t result = Core::ERROR_NONE;

                    ASSERT(connector.CrtController() != nullptr);

                    uint32_t connectorId(connector.Properties().Id());
                    const uint32_t crtcId(connector.CrtController().Id());

                    TRACE_GLOBAL(Trace::Information, ("Commit for connector: %d , CRTC: %d", connectorId, crtcId));

                    int drmResult(0);

                    const drmModeModeInfo* mode(connector.CrtController());

                    if ((drmResult = drmModeConnectorSetProperty(_fd, connectorId, connector.Properties().Id(Compositor::DRM::property::Dpms), DRM_MODE_DPMS_ON)) != 0) {
                        TRACE_GLOBAL(Trace::Error, ("Failed setting DPMS to %s for connector %d: [%d] %s", connector.IsEnabled() ? "on" : "off", connectorId, drmResult, strerror(errno)));
                        result = Core::ERROR_GENERAL;
                    }
                    else {
                        constexpr uint32_t X = 0;
                        constexpr uint32_t Y = 0;

                        /*
                         * Use the same mode as the previous operation on the CRTC and specified connector(s)
                         * New framebuffer Id, x, and y properties will set at vblank.
                         */
                        if ((drmResult = drmModeSetCrtc(_fd, crtcId, connector.ActiveFrameBufferId(), X, Y, &connectorId, 1, const_cast<drmModeModeInfoPtr>(mode)) != 0)) {
                            TRACE_GLOBAL(Trace::Error, ("Failed to set CRTC: %d: [%d] %s", crtcId, drmResult, strerror(errno)));
                            result = Core::ERROR_INCOMPLETE_CONFIG;
                        }
                    }

                    /*
                    * clear cursor image
                    */

                    if ((result == Core::ERROR_NONE) && (drmResult = drmModeSetCursor(_fd, crtcId, 0, 0, 0)) != 0) {
                        TRACE_GLOBAL(Trace::Error, ("Failed to clear cursor: [%d] %s", drmResult, strerror(errno)));
                        result = Core::ERROR_GENERAL;
                    }
                    else if ((drmResult = drmModePageFlip(_fd, crtcId, connector.FrameBufferId(), DRM_MODE_PAGE_FLIP_EVENT, _userData)) != 0) {
                        TRACE_GLOBAL(Trace::Error, ("Page flip failed: [%d] %s", drmResult, strerror(errno)));
                        result = Core::ERROR_GENERAL;
                    }
                    return (result);
                }
                uint32_t Initial() {
                    return (Core::ERROR_NONE);
                }
                uint32_t Commit(void* /* userData */) {
                    return (Core::ERROR_NONE);
                }

            private:
                int _fd;
            };
            
        } // namespace Backend
    } // namespace Compositor
} // namespace Thunder
