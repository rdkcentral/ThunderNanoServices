/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological B.V.
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

#include <CompositorTypes.h>
#include <interfaces/IGraphicsBuffer.h>

#include "generated/presentation-time-client-protocol.h"
#include "generated/viewporter-client-protocol.h"
#include "generated/xdg-activation-v1-client-protocol.h"
#include "generated/xdg-decoration-unstable-v1-client-protocol.h"
#include "generated/xdg-shell-client-protocol.h"
#include <wayland-client-protocol.h>

namespace Thunder {
namespace Compositor {
    namespace Backend {
        namespace Wayland {
            struct IBackend : public Core::IReferenceCounted {
                ~IBackend() override = default;

                virtual int RoundTrip() const = 0;
                virtual int Flush() const = 0;

                virtual wl_surface* Surface() const = 0;

                virtual xdg_surface* WindowSurface(wl_surface* surface) const = 0;

                virtual const std::vector<PixelFormat>& Formats() const = 0;

                virtual int RenderNode() const = 0;

                virtual wl_buffer* Buffer(Exchange::IGraphicsBuffer* buffer) const = 0;

                virtual struct wp_presentation_feedback* GetFeedbackInterface(wl_surface* surface) const = 0;

                virtual struct wp_viewport* GetViewportInterface(wl_surface* surface) const = 0;
            }; // struct IBackend
        } //    namespace Wayland
    } //    namespace Backend
} //    namespace Compositor
} //   namespace Thunder
