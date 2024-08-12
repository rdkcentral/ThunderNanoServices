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

#include "../include/Module.h"

#include <CECMessage.h>

namespace Thunder {
namespace CEC {
    namespace Message {
        namespace Service {
            class PhysicalAddress : public ServiceType<GIVE_PHYSICAL_ADDR, REPORT_PHYSICAL_ADDR, PhysicalAddress, true> {
                friend ServiceType;
            private:
                static constexpr char EnviroinmentPhysicalAddress[] = "PLUGIN_CEC_PHYSICAL_ADDRESS";
                static constexpr uint8_t PhysicalAddressLength = (32 / 8); // 32 bit ID

            public:
                PhysicalAddress()
                    : ServiceType()
                    , _physicalAddress()
                {
                    memset(_physicalAddress, 0, sizeof(_physicalAddress));
                }

                ~PhysicalAddress() override = default;

            private:
                uint8_t Process(const uint8_t length VARIABLE_IS_NOT_USED, uint8_t buffer[])
                {
                    ASSERT(length >= sizeof(_physicalAddress));

                    memcpy(buffer, _physicalAddress, sizeof(_physicalAddress));

                    return sizeof(_physicalAddress);
                }

            private:
                uint8_t _physicalAddress[4];
            };// class PhysicalAddress
        } //namespace Service

        static Service::PhysicalAddress service_physical_address;
    } // namespace Message
} // namespace CEC
} // namespace Thunder