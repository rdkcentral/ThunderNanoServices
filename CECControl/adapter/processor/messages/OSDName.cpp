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
#include <CECOperationFrame.h>

namespace Thunder {
namespace CEC {
    namespace Message {
        namespace Service {
            class OSDName : public ServiceType<GIVE_OSD_NAME, SET_OSD_NAME, OSDName, false> {
                friend ServiceType;

            public:
                OSDName()
                    : ServiceType()
                {
                }

                ~OSDName() override = default;

            private:
                uint8_t Process(const uint8_t length, uint8_t buffer[])
                {
                    string name("Metrological"); // TODO: Get a real name from somewhere in the system.

                    ASSERT(name.length() < length);

                    uint8_t chars = (length >= name.length()) ? name.length() : CEC::OperationFrame::MaxLength;

                    memcpy(buffer, name.c_str(), chars);

                    return chars;
                }
            };
        } // namespace Response

        static Service::OSDName service_osd_name;

    } // namespace Message
} // namespace CEC
} // namespace Thunder
