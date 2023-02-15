
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

#include <cstdint>

#include <core/core.h>

namespace Compositor {

namespace Backend {
    struct EXTERNAL IOutput {
        struct EXTERNAL IOutputFactory {
            virtual ~IOutputFactory() = default;

            static IOutputFactory* Instance();

            /**
             * @brief Create a output using a opened drm card/gpu, callee takes ownership.
             *
             * @param _cardFd File descriptor of a opened drm card.
             * @return IOutput*
             */
            virtual std::shared_ptr<IOutput> Create() = 0;
        };

        virtual ~IOutput() = default;

        /**
         * @brief Commits all pending changes in the framebuffer to the screen.
         * 
         * @param fd 
         * @param crtcId 
         * @param connectorId 
         * @param frameBufferId 
         * @param flags DRM_MODE_PAGE_FLIP_FLAGS (see drm_mode.h)
         * @param userData 
         * @return uint32_t uint32_t Core::ERROR_NONE at success, error code otherwise.
         */
        virtual uint32_t Commit(const int fd, const uint32_t crtcId, const uint32_t connectorId, const  uint32_t frameBufferId, const uint32_t flags, void* userData) = 0;
    };
} // namespace Backend
} // namespace Compositor
