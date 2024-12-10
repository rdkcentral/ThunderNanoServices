
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

#include <DRMTypes.h>
#include <xf86drmMode.h>

namespace Thunder {
namespace Compositor {
    namespace Backend {
        struct EXTERNAL IGpu {

            struct EXTERNAL IConnector : public Compositor::DRM::IDrmObject {
                virtual ~IConnector() = default;

                // @brief Whenever the output should be displayed
                virtual bool IsEnabled() const = 0;

                virtual Identifier FrameBufferId() const = 0;

                virtual const Core::ProxyType<Thunder::Exchange::ICompositionBuffer> FrameBuffer() const = 0;

                // @brief Current display mode for this connector
                virtual const drmModeModeInfo& ModeInfo() const = 0;

                // @brief Information from the attached CRTC;
                virtual const Compositor::DRM::IDrmObject* CrtController() const = 0;

                // @brief Information from the attached Plane/Buffer;
                virtual const Compositor::DRM::IDrmObject* Plane() const = 0;

                // @brief Callback if the output was presented to a screen.
                // @param pts: Presentation time stamp of the connector, 0 if was not presented.
                virtual void Presented(const uint32_t sequence, const uint64_t pts) = 0;
            };

            static IGpu& Instance();

            virtual ~IGpu() = default;

            /**
             * @brief Commits all pending changes in the framebuffer to the screen.
             *
             * @param fd        The file descriptor of an opened drm node
             * @param connectors The interfaces of the connectors to be scanned out.
             * @param userData  This pointer is returned in the vblank
             *
             * @return uint32_t Core::ERROR_NONE at success, error code otherwise.
             */
            virtual uint32_t Commit(const int fd, const std::vector<IConnector*> connectors, void* userData) = 0;
        };
    } // namespace Backend
} // namespace Compositor
} // namespace Thunder
