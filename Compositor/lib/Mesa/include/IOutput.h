
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

#include "CompositorTypes.h"

namespace Thunder {

namespace Compositor {
    struct IOutput : Exchange::ICompositionBuffer {

        ~IOutput() override = default;

        struct EXTERNAL ICallback {
            virtual ~ICallback() = default;
            /**
             * @brief Callback when buffer content is presented on the output.
             *
             * @param output Pointer to the output that triggered the callback.
             * @param sequence Commit sequence number that was presented.
             * @param time Presentation time stamp, 0 means never presented/timeout.
             */
            virtual void Presented(const IOutput* output, const uint32_t sequence, const uint64_t time) = 0;
        }; // struct ICallback

        /**
         * @brief  Trigger to start bringing the buffer contents to the output.
         *
         * @return uint32_t Sequence number of the commit.
         */
        virtual uint32_t Commit() = 0;

        /**
         * @brief  Get the node where this output is bound to.
         *
         * @return string  e.g. Wayland display name or DRM node.
         */
        virtual const string& Node() const = 0;

        /**
         * @brief  Get the X position of this output in the complete composition.
         *
         * @return X position in pixels
         */
        virtual int32_t X() const = 0;

        /**
         * @brief  Get the Y position of this output in the complete composition.
         *
         * @return int Y position in pixels.
         */
        virtual int32_t Y() const = 0;
    };

    /**
     * @brief  Allocate a new output.
     *         When the callee is done with the output, they must release it.
     *
     *
     * @param connector  Identification of the a output like a connector name 'card1-HDMI-A-1' or 'wayland-0'
     * @param rectangle  the area that this connector covers in the composition
     * @param format Pixel layout for this buffer
     *
     * @return Core::ProxyType<IOutput> The allocated buffer
     */

    EXTERNAL Core::ProxyType<IOutput> CreateBuffer(
        const string& connector,
        const Exchange::IComposition::Rectangle& rectangle,
        const Compositor::PixelFormat& format,
        IOutput::ICallback* callback);

} // namespace Compositor
} // namespace Thunder
