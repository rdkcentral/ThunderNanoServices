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
        enum class Usage : uint8_t {
            USE_FOR_RENDERING = 0x01,
            USE_FOR_DISPLAYING = 0x02,
            USELESS = 0xFF
        };

        enum class Type : uint8_t {
            SharedMemory = 0x01, // Generic (Software)
            DirectRenderingManager = 0x02, // Linux (Hardware)
            GenericBufferManagement = 0x03, // Mesa (Hardware)
        };

        struct EXTERNAL IAllocator {
            virtual ~IAllocator() = default;

            /**
             * @brief A factory for buffers
             *
             * @param flags the @usage for the
             * @return IAllocator*
             */
            static Core::ProxyType<IAllocator> Instance(int drmFd);

            /**
             * @brief  Allocate a new buffer.
             *         When the caller is done with the buffer, they must release it.
             *
             *
             * @param width  Width in pixels
             * @param height Height in pixels
             * @param format Pixel layout for this buffer
             *
             * @return Core::ProxyType<IBuffer*> The allocated buffer
             */
            virtual Core::ProxyType<IBuffer> Create(uint32_t width, uint32_t height, const PixelFormat& format) = 0;
        };

        /**
         * @brief Buffer capabilities.
         *
         * These bits indicate the features supported by a struct wlr_buffer. There is
         * one bit per function in struct wlr_buffer_impl.
         */
        // enum Capability : uint32_t {
        //     DATA_PTR = 1 << 0,
        //     DMABUF = 1 << 1,
        //     SHM = 1 << 2,
        // };

        virtual ~IBuffer() = default;
    }; // struct IBuffer

} // namespace Interfaces
} // namespace Compositor