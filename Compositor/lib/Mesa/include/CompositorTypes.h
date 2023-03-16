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

#include <core/core.h>
#include <messaging/messaging.h>
#include <array>

namespace WPEFramework {

namespace Compositor {


using Box = struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

using Identifier = uintptr_t;
using Matrix = std::array<float, 9>;
using Color = std::array<float, 4>;

constexpr int InvalidFileDescriptor = -1;
constexpr Identifier InvalidIdentifier = static_cast<Identifier>(~0);

/**
 * @brief  A single DRM format, with a set of modifiers attached.
 *
 */
class PixelFormat {
private:
    static constexpr uint8_t DefaultModifier = 0x00;

    struct modifier {
        uint64_t code;
        bool external;
    };

public:
    using ModifierType = modifier;

    PixelFormat() = delete;

    PixelFormat(const uint32_t fourcc, const uint16_t nModifiers, const uint64_t modifiers[])
        : _fourcc(fourcc)
        , _modifiers(modifiers, modifiers + nModifiers)
    {
    }

    PixelFormat(const uint32_t fourcc, const std::vector<uint64_t>& modifiers)
        : _fourcc(fourcc)
        , _modifiers(modifiers)
    {
    }

    PixelFormat(const uint32_t fourcc)
        : _fourcc(fourcc)
        , _modifiers(1, DefaultModifier)
    {
    }

    const uint32_t& Type() const
    {
        return _fourcc;
    }

    /*
     * For details about the format modifiers see `drm_fourcc.h` 
     */
    const std::vector<uint64_t>& Modifiers() const
    {
        return _modifiers;
    }

private:
    const uint32_t _fourcc; // The actual DRM format, from `drm_fourcc.h`
    const std::vector<uint64_t> _modifiers;
}; // class PixelFormat
} // namespace Compositor
} // namespace WPEFramework
