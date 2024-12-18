
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

    struct EXTERNAL IOutput {
        struct IFeedback {
            virtual ~IFeedback() = default;
            virtual void Presented(const int fd, const uint32_t sequence, const uint64_t time) = 0;
            virtual void Display(const int fd, const std::string& node) = 0;
        }; // struct IFeedback

        /**
         * @brief  Allocate a new output.
         *         When the callee is done with the output, they must release it.
         *
         *
         * @param connector  Identification of the a output like a connector name 'card1-HDMI-A-1' or 'wayland-0'
         * @param rectangle  the area that this connector covers in the composition
         * @param format Pixel layout for this buffer
         *
         * @return Core::ProxyType<Exchange::ICompositionBuffer> The allocated buffer
         */
        static Core::ProxyType<Exchange::ICompositionBuffer> Instance(
            const string& connector,
            const Exchange::IComposition::Rectangle& rectangle,
            const Compositor::PixelFormat& format,
            IFeedback* callback = nullptr);
    };
} // namespace Compositor
} // namespace Thunder
