
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

#include <core/core.h>

#include <interfaces/IComposition.h>

#include "compositorbuffer/IBuffer.h"
#include "CompositorTypes.h"

namespace Compositor {

namespace Interfaces {
    struct IBackend {
        virtual ~IBackend() = default;
        /**
         * @brief  Allocate a new output.
         *         When the callee is done with the output, they must release it.
         *
         *
         * @param connector  Identification of the a output like a connector name 'card1-HDMI-A-1' or 'wayland-0'
         * @param width  Width in pixels
         * @param height Height in pixels
         * @param format Pixel layout for this buffer
         *
         * @return Core::ProxyType<IBuffer> The allocated buffer
         */
        static WPEFramework::Core::ProxyType<IBuffer> Connector(const std::string& connector, const WPEFramework::Exchange::IComposition::ScreenResolution resolution, const Compositor::PixelFormat& format, bool forceResolution);
    };
} // namespace Interfaces
} // namespace Compositor
