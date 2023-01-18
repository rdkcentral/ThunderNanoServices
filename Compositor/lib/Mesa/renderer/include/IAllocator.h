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

#include "CompositorTypes.h"
#include "compositorbuffer/IBuffer.h"

namespace Compositor {
namespace Interfaces {
    struct EXTERNAL IAllocator {
        virtual ~IAllocator() = default;

        /**
         * @brief A factory for buffers
         *
         * @param identifier ID for this allocator, allows for reuse.
         * @return Core::ProxyType<IAllocator>
         */
        static WPEFramework::Core::ProxyType<IAllocator> Instance(WPEFramework::Core::instance_id identifier);

        /**
         * @brief  Allocate a new buffer.
         *         When the caller is done with the buffer, they must release it.
         *
         *
         * @param width  Width in pixels
         * @param height Height in pixels
         * @param format Pixel layout for this buffer
         *
         * @return Core::ProxyType<IBuffer> The allocated buffer
         */
        virtual WPEFramework::Core::ProxyType<IBuffer> Create(const uint32_t width, const uint32_t height, const PixelFormat& format) = 0;
    }; // struct EXTERNAL IAllocator
} // namespace Interfaces
} // namespace Compositor