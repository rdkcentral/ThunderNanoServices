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

#include <Module.h>

#include "PixelFormat.h"
#include <core/core.h>

using namespace WPEFramework;

namespace Compositor {
namespace Interfaces {
    struct EXTERNAL IBuffer {
        virtual ~IBuffer() = default;

        virtual uint32_t Lock(uint32_t timeout) = 0;
        virtual uint32_t Unlock(uint32_t timeout) = 0;

        virtual WPEFramework::Core::instance_id Accessor() = 0; // Access to the buffer

        virtual int Handle() = 0;

        virtual uint32_t Width() = 0; // Width of the allocated buffer in pixels
        virtual uint32_t Height() = 0; // Height of the allocated buffer in pixels
        virtual uint32_t Format() = 0; // Layout of a pixel according the fourcc format
        virtual uint64_t Modifier() = 0; // Pixel arrangement in the buffer, used to optimize for hardware
        virtual uint8_t Planes() = 0; // The amount of graphic planes available in the buffer.

        virtual uint32_t Stride(const uint8_t PlaneId) = 0; // Bytes per row for a plane [(bit-per-pixel/8) * width]
        virtual uint32_t Offset(const uint8_t PlaneId) = 0; // Offset of the plane from where the pixel data starts in the buffer.
    }; // struct IBuffer
} // namespace Interfaces
} // namespace Compositor