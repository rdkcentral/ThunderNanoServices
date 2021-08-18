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

    struct IAudioCodec {

        static constexpr uint8_t MEDIA_TYPE = 0x00; // audio

        enum type : uint8_t {
            INVALID = 0,
            SBC
        };

        virtual ~IAudioCodec() { }

        virtual type Type() const = 0;

        virtual uint32_t BitRate() const = 0; // bits per second
        virtual uint32_t ClockRate() const = 0; // bits per second
        virtual uint8_t Channels() const = 0; // bits per second

        virtual uint32_t QOS(const int8_t policy) = 0;

        virtual uint32_t Configure(const string& format) = 0;
        virtual void Configuration(string& format) const = 0;

        virtual void SerializeConfiguration(Bluetooth::Buffer& output) const = 0;

        virtual uint32_t Encode(const uint32_t inBufferSize, const uint8_t inBuffer[],
                                uint32_t& outBufferSize, uint8_t outBuffer[]) const = 0;

        virtual uint32_t Decode(const uint32_t inBufferSize, const uint8_t inBuffer[],
                                uint32_t& outBufferSize, uint8_t outBuffer[]) const = 0;
    };

} // namespace A2DP

}