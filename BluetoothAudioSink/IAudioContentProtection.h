/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "Module.h"

namespace WPEFramework {

namespace A2DP {

    struct IAudioContentProtection {

        enum type {
            INVALID = 0
        };

        virtual ~IAudioContentProtection() { }

        virtual type Type() const;

        virtual uint32_t SerializeConfiguration(Bluetooth::Buffer& output) const = 0;

        virtual uint32_t Protect(const uint16_t inBufferSize, const uint8_t inBuffer[],
                                 const uint16_t maxOutBufferSize, uint8_t outBuffer) const = 0;

        virtual uint32_t Unprotect(const uint16_t inBufferSize, const uint8_t inBuffer[],
                                   const uint16_t maxOutBufferSize, uint8_t outBuffer) const = 0;
    };

} // namespace A2DP

}