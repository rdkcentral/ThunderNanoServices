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
        class PhysicalAddress : public ExchangeType<GIVE_PHYSICAL_ADDR, REPORT_PHYSICAL_ADDR, PhysicalAddress> {
            friend ExchangeType;
            static constexpr uint8_t VendorIdLength = (32 / 8); // 32 bit ID

        public:
            PhysicalAddress() = default;
            ~PhysicalAddress() override = default;

            uint8_t Value(const uint8_t length, uint8_t buffer[]) const
            {
                ASSERT(length >= sizeof(_physicalAddress));

                memcpy(buffer, _physicalAddress, sizeof(_physicalAddress));

                return sizeof(_physicalAddress);
            }

        private:
            bool DeserializeParameters(const uint8_t length, const uint8_t buffer[])
            {
                ASSERT(sizeof(_physicalAddress) == length);

                memcpy(_physicalAddress, buffer, sizeof(_physicalAddress));

                return true;
            }

        private:
            uint8_t _physicalAddress[VendorIdLength];
        };
    } // namespace Message

} // namespace CEC
} // namespace Thunder