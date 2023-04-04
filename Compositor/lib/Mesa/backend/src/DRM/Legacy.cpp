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
#include "IOutput.h"

#include <DRM.h>
#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace WPEFramework {
namespace Compositor {
namespace Backend {
    class LegacyCrtc : public IOutput {
    public:
        LegacyCrtc(LegacyCrtc&&) = delete;
        LegacyCrtc(const LegacyCrtc&) = delete;
        LegacyCrtc& operator=(const LegacyCrtc&) = delete;

        LegacyCrtc() = default;
        ~LegacyCrtc() = default;

        uint32_t Commit(const int fd, const IConnector* connector, const uint32_t flags, void* userData) override
        {
            uint32_t result(Core::ERROR_NONE);

            ASSERT(connector != nullptr);

            /*
             * TODO: Before we commit ideally we should do a test if the current mode is matching the mode expected.
             *       If this differs we need ask the kernel to modeset the gpu to the expected mode.
             *
             *       For now always modeset...
             */
            constexpr bool doModeSet = true;

            ASSERT((flags & ~DRM_MODE_PAGE_FLIP_FLAGS) == 0); // only allow page flip flags

            int drmResult(0);

            if (doModeSet == true) {
                std::vector<uint32_t> connectorIds;

                const drmModeModeInfo* mode(nullptr);

                if (connector->IsEnabled() == true) {
                    connectorIds.emplace_back(connector->ConnectorId());
                    mode = &(connector->ModeInfo());
                }

                uint32_t dpms = connector->IsEnabled() ? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF;

                if (drmModeConnectorSetProperty(fd, connector->ConnectorId(), connector->DpmsPropertyId(), dpms) != 0) {
                    TRACE(Trace::Error, ("drmModeSetCrtc failed: %s", strerror(-drmResult)));
                    return false;
                }

                constexpr uint32_t X = 0;
                constexpr uint32_t Y = 0;

                /*
                 * Use the same mode as the previous operation on the CRTC and specified connector(s)
                 * New framebuffer Id, x, and y properties will set at vblank.
                 */
                if (drmResult = drmModeSetCrtc(fd, connector->CtrControllerId(), connector->FrameBufferId(), X, Y, connectorIds.empty() ? nullptr : connectorIds.data(), connectorIds.size(), const_cast<drmModeModeInfoPtr>(mode))) {
                    TRACE(Trace::Error, ("drmModeSetCrtc failed: %s", strerror(-drmResult)));
                }

                /*
                 * clear cursor image
                 */
                if (drmResult = drmModeSetCursor(fd, connector->CtrControllerId(), 0, 0, 0)) {
                    TRACE(Trace::Error, ("drmModeSetCursor failed: %s", strerror(-drmResult)));
                }
            }

            /*
             * Request the kernel to do a page flip. The "DRM_MODE_PAGE_FLIP_EVENT" flags will notify us when the next vblank is active
             */
            if ((drmResult == 0) && ((flags & DRM_MODE_PAGE_FLIP_EVENT) > 0)) {

                if (drmResult = drmModePageFlip(fd, connector->CtrControllerId(), connector->FrameBufferId(), flags, userData)) {
                    TRACE(Trace::Error, ("drmModePageFlip failed: %s", strerror(-drmResult)));
                    result = Core::ERROR_GENERAL;
                }
            }

            return result;
        }

    private:
        signed int _gammaSize;
    };

    /* static */ IOutput* IOutput::Instance()
    {
        static LegacyCrtc transaction;
        return (&transaction);
    }
} // namespace Backend
} // namespace Compositor
} // namespace WPEFramework
