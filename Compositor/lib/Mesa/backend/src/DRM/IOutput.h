
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

#include <xf86drmMode.h>

namespace WPEFramework {

namespace Compositor {

namespace Backend {

    struct EXTERNAL IOutput {

        struct EXTERNAL IConnector {
            virtual ~IConnector() = default;


            virtual bool IsEnabled() const = 0; 

            virtual Identifier ConnectorId() const = 0;
            virtual Identifier CtrControllerId() const = 0;
            virtual Identifier PrimaryPlaneId() const = 0;
            virtual Identifier FrameBufferId() const = 0;

            virtual Identifier DpmsPropertyId() const = 0;

            virtual const drmModeModeInfo& ModeInfo() const = 0;
        };

        static IOutput& Instance();

        virtual ~IOutput() = default;

        /**
         * @brief Commits all pending changes in the framebuffer to the screen.
         *
         * @param fd        The file descriptor of an opened drm node
         * @param connector The interface of the connector to be scanned out.
         * @param flags     DRM_MODE_PAGE_FLIP_FLAGS (see drm_mode.h)
         * @param userData  This pointer is returned in the vblank 
         * 
         * @return uint32_t Core::ERROR_NONE at success, error code otherwise.
         */
        virtual uint32_t Commit(const int fd, const IConnector* connector, uint32_t flags, void* userData) = 0;

        /**
         * @brief This will be called when something is changed within the hardware.   
        */
        virtual void Reevaluate() = 0;
    };
} // namespace Backend
} // namespace Compositor
} // namespace WPEFramework
