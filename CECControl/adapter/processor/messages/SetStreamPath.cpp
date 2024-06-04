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
// OpCode: 0x86
// Description: Used by the TV to request a streaming path from the specified Physical Address.
// Parameters: [Physical Address] The Physical Address of the source device.
// Response: Any CEC Switches between the TV and the source device shall switch inputs according
//           to the path defined in [Physical Address]. A CEC device at the new address shall come
//           out of the Standby state, stream its output and broadcast an <Active Source> message.
// Addressing: Broadcast
// Mandatory:
//     Initator: TV with device selection menu
//     Follower: All Sources and CEC Switches

namespace Thunder {
namespace CEC {
    namespace Message {
        namespace Service {
            class SetStreamPath : public ServiceType<SET_STREAM_PATH, NO_OPCODE, SetStreamPath, false> {
                friend ServiceType;
            public:
                SetStreamPath()
                    : ServiceType()
                {
                }

                ~SetStreamPath() override = default;

            private:
               // we are only intersted in the data so for now abuse the IsValid...
                bool IsValid(uint8_t length, const uint8_t buffer[]){
                    string data;

                    Core::ToHexString(buffer, length, data);

                    //TODO: do something usefull with this data. For now just trace it.
                    TRACE(Trace::Information, (("Set stream path parameters[%d]: %s."), length,  data.c_str()));

                    return true;
                }
            };// class SetStreamPath
        } //namespace Service

        static Service::SetStreamPath set_stream_path;
    } // namespace Message
} // namespace CEC
} // namespace Thunder