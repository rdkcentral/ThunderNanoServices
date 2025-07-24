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

#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <string>
#include <vector>

namespace Thunder {

namespace Exchange {
    struct IGraphicsBuffer;
}

namespace Compositor {
    namespace DRM {

        using Identifier = uint32_t;
        static constexpr uint32_t InvalidIdentifier = ~0;

        struct ConnectorScanResult {
            bool success = false;
            bool needsModeSet = false;
            bool dimensionsAdjusted = false;
            drmModeModeInfo selectedMode = {};
            std::string gpuNode;

            // DRM object data for initialization
            struct {
                uint32_t crtcId = 0;
                uint32_t planeId = 0;
            } ids;
        };

        extern void GetNodes(const uint32_t type, std::vector<std::string>& list);
        extern Identifier FindConnectorId(const int fd, const std::string& connectorName);
        extern bool HasCapability(const int cardFd, const uint64_t option);

        /*
         * Re-open the DRM node to avoid GEM handle ref'counting issues.
         * See: https://gitlab.freedesktop.org/mesa/drm/-/merge_requests/110
         *
         */
        extern int ReopenNode(const int fd, const bool openRenderNode);
        extern uint32_t CreateFrameBuffer(const int cardFd, Exchange::IGraphicsBuffer* buffer);
        extern void DestroyFrameBuffer(const int cardFd, const uint32_t frameBufferId);
        extern bool HasNode(const drmDevice* device, const char* deviceName);
        extern int OpenGPU(const std::string& gpuNode);
        extern std::string GetNode(const uint32_t type, drmDevice* device);
        extern std::string GetGPUNode(const std::string& connectorName);
        extern bool HasAlpha(const uint32_t drmFormat);
        extern const char* ModifierVendorString(const uint64_t modifier);
        extern const char* FormatToString(const uint32_t format);
        extern bool ModesEqual(const drmModeModeInfo* mode1, const drmModeModeInfo* mode2);
        extern bool SelectBestMode(const drmModeConnector* const connector, const uint32_t requestedWidth, const uint32_t requestedHeight, bool& dimensionsAdjusted, drmModeModeInfo& selectedMode);
        extern ConnectorScanResult ScanConnector(const int backendFd, Thunder::Compositor::DRM::Identifier targetConnectorId, const uint32_t requestedWidth, const uint32_t requestedHeigh);
    } // namespace DRM
} // namespace Compositor
} // namespace Thunder
