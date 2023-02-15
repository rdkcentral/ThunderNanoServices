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

#include "../Trace.h"

#include <CompositorTypes.h>
#include <DrmCommon.h>
#include <IAllocator.h>

#include <compositorbuffer/IBuffer.h>

#include <IOutput.h>

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#if HAVE_GBM_MODIFIERS
#ifndef GBM_MAX_PLANES
#define GBM_MAX_PLANES 4
#endif
#endif

namespace Compositor {
namespace Backend {
    class LegacyCrtc : virtual public IOutput {

    public:
        LegacyCrtc() = default;
        ~LegacyCrtc() = default;

        uint32_t Commit(const int fd, const uint32_t crtcId, const uint32_t connectorId, const uint32_t frameBufferId, const uint32_t flags, void* userData) override
        {
            uint32_t result(WPEFramework::Core::ERROR_NONE);

            ASSERT((flags & ~DRM_MODE_PAGE_FLIP_FLAGS) == 0); // only allow page flip flags

            if (flags & DRM_MODE_PAGE_FLIP_EVENT) {
                if (drmModePageFlip(fd, crtcId, frameBufferId, flags, userData)) {
                    TRACE(WPEFramework::Trace::Error, ("drmModePageFlip failed"));
                    result = WPEFramework::Core::ERROR_GENERAL;
                }
            }

            return result;
        }

    private:
        signed int _gammaSize;
    };

    class LegacyCrtcFactory : virtual public IOutput::IOutputFactory {
    public:
        LegacyCrtcFactory() = default;
        ~LegacyCrtcFactory() = default;

        std::shared_ptr<IOutput> Create()
        {
            return std::make_shared<LegacyCrtc>();
        }
    };

    IOutput::IOutputFactory* IOutput::IOutputFactory::Instance()
    {
        static LegacyCrtcFactory output;
        return (&output);
    }
} // namespace Backend
} // namespace Compositor