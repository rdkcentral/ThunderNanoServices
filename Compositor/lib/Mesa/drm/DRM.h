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
#include <map>
#include <vector>
#include <string>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace WPEFramework {

namespace Exchange {
    struct ICompositionBuffer;
}

namespace Compositor {

namespace DRM {
    using Identifier = uintptr_t;
    using Value = uint64_t;
    using PropertyRegister = std::map<std::string, Identifier>;
    using IdentifierRegister = std::vector<Identifier>;

    constexpr Identifier InvalidIdentifier = 0;

    enum class PlaneType : uint8_t {
        Cursor = DRM_PLANE_TYPE_CURSOR,
        Primary = DRM_PLANE_TYPE_PRIMARY,
        Overlay = DRM_PLANE_TYPE_OVERLAY,
    };

    extern std::string PropertyString(const PropertyRegister& properties, const bool pretty = false);
    extern std::string IdentifierString(const std::vector<Identifier>& ids, const bool pretty = false);
    extern uint32_t GetPropertyId(PropertyRegister& registry, const std::string& name);
    extern uint32_t GetProperty(const int cardFd, const Identifier object, const Identifier property, Value& value);
    extern uint16_t GetBlobProperty(const int cardFd, const Identifier object, const Identifier property, const uint16_t blobSize, uint8_t blob[]);

    extern void GetNodes(const uint32_t type, std::vector<std::string>& list);

    /*
     * Re-open the DRM node to avoid GEM handle ref'counting issues.
     * See: https://gitlab.freedesktop.org/mesa/drm/-/merge_requests/110
     *
     */
    extern int ReopenNode(int fd, bool openRenderNode);
    extern uint32_t CreateFrameBuffer(const int cardFd, Exchange::ICompositionBuffer* buffer);
    extern void DestroyFrameBuffer(const int cardFd, const uint32_t frameBufferId);
    extern bool HasNode(const drmDevice* device, const char* deviceName);
    extern int OpenGPU(const std::string& gpuNode);


} // namespace DRM
} // namespace Compositor
} // namespace WPEFramework
