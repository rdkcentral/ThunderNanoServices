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

namespace Compositor {
namespace Interfaces {
    struct EXTERNAL IBuffer {
        /**
         * @brief   Multi-plane frame buffer interface with hardware optimisation in mind
         *
         *          Good reads:
         *          https://www.collabora.com/news-and-blog/blog/2018/03/20/a-new-era-for-linux-low-level-graphics-part-1/
         *          https://www.collabora.com/news-and-blog/blog/2018/03/23/a-new-era-for-linux-low-level-graphics-part-2/
         *          https://www.linux.com/training-tutorials/optimizing-graphics-memory-bandwidth-compression-and-tiling-notes-drm-format-modifiers/
         *
         *
         */

        static constexpr uint8_t PrimaryPlane = 0;

        virtual ~IBuffer() = default;

        virtual uint32_t Lock(uint32_t timeout) const = 0;
        virtual uint32_t Unlock(uint32_t timeout) const = 0;

        virtual WPEFramework::Core::instance_id Accessor(const uint8_t plane) = 0; // Access to the buffer planes.

        virtual int Handle() = 0;

        virtual uint32_t Width() const = 0; // Width of the allocated buffer in pixels
        virtual uint32_t Height() const = 0; // Height of the allocated buffer in pixels
        virtual uint32_t Format() const = 0; // Layout of a pixel according the fourcc format
        virtual uint64_t Modifier() const = 0; // Pixel arrangement in the buffer, used to optimize for hardware
        virtual uint8_t Planes() const = 0; // The list of id's of graphic planes available in the buffer.

        virtual uint32_t Stride(const uint8_t plane) const = 0; // Bytes per row for a plane [(bit-per-pixel/8) * width]
        virtual uint32_t Offset(const uint8_t plane) const = 0; // Offset of the plane from where the pixel data starts in the buffer.

        /**
         * @brief TODO: Maybe we want the planes to abstracted as well, for now go with a 'flattened' API
         *
         */
        // struct IPlane {
        //     virtual ~IPlane() = default;

        //     virtual WPEFramework::Core::instance_id Accessor() = 0; // Access to the plane.

        //     virtual uint32_t Stride() const = 0; // Bytes per row for a plane [(bit-per-pixel/8) * width]
        //     virtual uint32_t Offset() const = 0; // Offset of the plane from where the pixel data starts in the buffer.
        // };

        // virtual IPlane* Plane(const uint8_t plane) = 0;
    }; // struct IBuffer
} // namespace Interfaces
} // namespace Compositor