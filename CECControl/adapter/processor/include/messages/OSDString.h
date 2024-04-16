/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include <CECMessage.h>

namespace Thunder {
namespace CEC {
    namespace Message {
        class OSDString : public ExchangeType<SET_OSD_STRING, NO_OPCODE, OSDString, false> {
            friend ExchangeType;

        public:
            OSDString() delete;

            OSDString(const std::string& data)
                : _data(data)
            {
            }

            ~OSDString() override = default;

        private:
            uint8_t SerializeParameters(const uint8_t length, uint8_t buffer[])
            {
                uint8_t result = std::min(length, static_cast<uint8_t>(_data.size()));

                ::memcpy(buffer, reinterpret_cast<const uint8_t*>(_data.c_str()), result)

                return result;
            }

        private:
            std::string _data;
        };
    } // namespace Message

} // namespace CEC
} // namespace Thunder