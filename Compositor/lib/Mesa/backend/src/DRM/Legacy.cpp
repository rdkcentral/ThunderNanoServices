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

#include <DRM.h>
#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace Thunder {
namespace Compositor {
    namespace Backend {

        uint32_t Commit(const int fd, const std::vector<IConnector*> connectors, void* userData) 
        {
            static bool doModeSet(true);
            uint32_t result(Core::ERROR_NONE);

            for (auto& connector : connectors) {

                ASSERT(connector != nullptr);
                ASSERT(connector->CrtController() != nullptr);
                ASSERT(connector->Plane() != nullptr);

                const uint32_t ConnectorId(connector->Properties().Id());
                const uint32_t crtcId(connector->CrtController().Id());
                const uint32_t planeId(connector->Plane().Id());

                TRACE_GLOBAL(Trace::Information, ("Commit for connector: %d , CRTC: %d, Plane: %d", ConnectorId, crtcId, planeId));

                uint32_t commitFlags(DRM_MODE_PAGE_FLIP_EVENT);

                int drmResult(0);

                if (doModeSet == true) {
                    std::vector<uint32_t> connectorIds;

                    const drmModeModeInfo* mode(nullptr);

                    if (connector->IsEnabled() == true) {
                        connectorIds.emplace_back(ConnectorId);
                        mode = connector->CrtController();
                    }

                    uint32_t dpms = connector->IsEnabled() ? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF;

                    if ((drmResult = drmModeConnectorSetProperty(fd, ConnectorId, connector->Properties().Id(DRM::property::Dpms), dpms)) != 0) {
                        TRACE_GLOBAL(Trace::Error, ("Failed setting DPMS to %s for connector %d: [%d] %s", connector->IsEnabled() ? "on" : "off", ConnectorId, drmResult, strerror(errno)));
                        return Core::ERROR_GENERAL;
                    }

                    constexpr uint32_t X = 0;
                    constexpr uint32_t Y = 0;

                    /*
                        * Use the same mode as the previous operation on the CRTC and specified connector(s)
                        * New framebuffer Id, x, and y properties will set at vblank.
                        */
                    if ((drmResult = drmModeSetCrtc(fd, crtcId, connector->FrameBufferId(), X, Y, connectorIds.empty() ? nullptr : connectorIds.data(), connectorIds.size(), const_cast<drmModeModeInfoPtr>(mode)) != 0)) {
                        TRACE_GLOBAL(Trace::Error, ("Failed to set CRTC: %d: [%d] %s", crtcId, drmResult, strerror(errno)));
                        return Core::ERROR_INCOMPLETE_CONFIG;
                    }

                    doModeSet = false;
                }

                /*
                    * clear cursor image
                    */
                if ((drmResult = drmModeSetCursor(fd, crtcId, 0, 0, 0)) != 0) {
                    TRACE_GLOBAL(Trace::Error, ("Failed to clear cursor: [%d] %s", drmResult, strerror(errno)));
                }

                if ((drmResult == 0) && ((drmResult = drmModePageFlip(fd, crtcId, connector->FrameBufferId(), commitFlags, userData)) != 0)) {
                    TRACE_GLOBAL(Trace::Error, ("Page flip failed: [%d] %s", drmResult, strerror(errno)));
                    result = Core::ERROR_GENERAL;
                }
            }

            if (result != Core::ERROR_NONE) {
                for (auto connector : connectors) {
                    connector->Presented(0, 0); // notify connector implementation the buffer failed to display.
                }
            }

            return result;
        }

    } // namespace Backend
} // namespace Compositor
} // namespace Thunder
