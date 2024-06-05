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

#include "../Module.h"

#include "interfaces/IKeyHandler.h"

#include <wayland-client.h>

#include "IGlobalRegistry.h"

namespace Thunder {
namespace Compositor {
    namespace Backend {

        class Input : public Wayland::IGlobalRegistry {
            class KeyProducer : public Exchange::IKeyProducer {}; // class KeyProducer
            class WheelProducer : public Exchange::IWheelProducer {}; // class WheelProducer
            class IPointerProducer : public Exchange::IPointerProducer {}; // class IPointerProducer
            class TouchProducer : public Exchange::ITouchProducer {}; // class TouchProducer

        public:
            Input();
            Input(Input&&) = delete;
            Input(const Input&) = delete;
            Input& operator=(const Input&) = delete;

            ~Input();

            void onRegister(struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) override;
            void onUnregister(uint32_t name) override;

        private:
            static void onSeatCapabilities(void* data, struct wl_seat* seat, uint32_t capabilities);
            static void onSeatName(void* data, struct wl_seat* seat, const char* name);

            const struct wl_seat_listener seatListener = {
                .capabilities = onSeatCapabilities,
                .name = onSeatName,
            };

        private:
            wl_seat* _wlSeat;
            std::string _seatId;

            wl_touch* _wlTouch;
            wl_pointer* _wlPointer;
            wl_keyboard* _wlKeyboard;
        }; // class Input
    } //    namespace Backend
} //    namespace Compositor
} //   namespace Thunder
