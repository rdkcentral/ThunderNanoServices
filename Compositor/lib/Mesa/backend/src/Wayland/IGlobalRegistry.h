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
#include <wayland-client.h>

namespace Thunder {
namespace Compositor {
    namespace Backend {
        namespace Wayland {
            struct IGlobalRegistry {
                virtual ~IGlobalRegistry() = default;

                virtual void onRegister(struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) = 0;
                virtual void onUnregister(uint32_t name) = 0;
            }; // struct IGlobalRegistry
        } //    namespace Wayland
    } //    namespace Backend
} //    namespace Compositor
} //   namespace Thunder
