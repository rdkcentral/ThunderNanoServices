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

// <Set Stream Path>
// OpCode: 0x64
// Addressing: Direct
// Description: Used to send a text message to output on a TV.
// Parameters: [Display Control] Display timing. [OSD String] Text to be displayed.
// Response: TV displays the message.
// Mandatory:
//     Initator: -
//     Follower: -

namespace Thunder {
namespace CEC {
    namespace Message {
        namespace Service {
            class OSDString : public ServiceType<SET_OSD_STRING, NO_OPCODE, OSDString, false> {
                friend ServiceType;

            public:
                OSDString()
                    : ServiceType()
                {
                }

                ~OSDString() override = default;

            private:
                // we are only intersted in the data so for now abuse the IsValid...
                bool IsValid(uint8_t length, const uint8_t buffer[])
                {
                    string data(reinterpret_cast<const char*>(buffer), length);

                    // TODO: do something usefull with this data. For now just trace it.
                    TRACE(Trace::Information, (("Received OSD string %s."), data.c_str()));

                    return true; // FIXME maybe we need return this: !data.empty();
                }
            };
        } // namespace Service

        static Service::OSDString osd_string;
    } // namespace Message
} // namespace CEC
} // namespace Thunder