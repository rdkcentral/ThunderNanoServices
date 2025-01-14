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
#include <compositorbuffer/CompositorBufferType.h>
#include <array>

#include <interfaces/IComposition.h>
#include <interfaces/ICompositionBuffer.h>

namespace Thunder {

namespace Compositor {

namespace Rectangle {
    constexpr Exchange::IComposition::Rectangle Default()
    {
        return { 0, 0, 0, 0 };
    }

    constexpr bool IsDefault(const Exchange::IComposition::Rectangle& rectangle)
    {
        return (rectangle.x == 0) && (rectangle.y == 0) && (rectangle.height == 0) && (rectangle.width == 0);
    }
}

using Identifier = uintptr_t;
using Matrix = std::array<float, 9>;
using Color = std::array<float, 4>;

constexpr int InvalidFileDescriptor = -1;
constexpr Identifier InvalidIdentifier = static_cast<Identifier>(~0);

#ifdef __DEBUG__
constexpr uint32_t DefaultTimeoutMs = Core::infinite;
#else
constexpr uint32_t DefaultTimeoutMs = 100;
#endif

/**
 * @brief  A single DRM format, with a set of modifiers attached.
 *
 */

// using FormatRegister = std::map<uint32_t, std::vector<uint64_t>>;

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
        , _modifiers(modifiers, modifiers + nModifiers) {
    }
    PixelFormat(const uint32_t fourcc, std::vector<uint64_t>&& modifiers)
        : _fourcc(fourcc)
        , _modifiers(std::move(modifiers)) {
    }
    PixelFormat(const uint32_t fourcc, const std::vector<uint64_t>& modifiers)
        : _fourcc(fourcc)
        , _modifiers(modifiers) {
    }
    PixelFormat(const uint32_t fourcc)
        : _fourcc(fourcc)
        , _modifiers(1, DefaultModifier)
    {
    }
    PixelFormat(PixelFormat&& move)
        : _fourcc(std::move(move._fourcc))
        , _modifiers(std::move(move._modifiers)) {
    }
    PixelFormat(const PixelFormat& copy)
        : _fourcc(copy._fourcc)
        , _modifiers(copy._modifiers) {
    }
    ~PixelFormat() = default;

    PixelFormat& operator=(PixelFormat&& rhs) {
        _fourcc = std::move(rhs._fourcc);
        _modifiers = std::move(rhs._modifiers);
        return *this;
    }
    PixelFormat& operator=(const PixelFormat& rhs) {
        _fourcc = rhs._fourcc;
        _modifiers = rhs._modifiers;
        return *this;
    }

public:
    uint32_t Type() const {
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
     uint32_t _fourcc; // The actual DRM format, from `drm_fourcc.h`
     std::vector<uint64_t> _modifiers;
}; // class PixelFormat
} // namespace Compositor
} // namespace Thunder
