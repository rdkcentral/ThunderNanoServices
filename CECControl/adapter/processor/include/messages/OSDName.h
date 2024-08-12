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
        class OSDName : public ExchangeType<GIVE_OSD_NAME, SET_OSD_NAME, OSDName> {
            friend ExchangeType;
        public:
            OSDName() = default;
            ~OSDName() override = default;

            const std::string& Value() const {
                return _name;
            }

        private:
            bool DeserializeParameters(const uint8_t length, const uint8_t buffer[])
            {
                _name = std::string(reinterpret_cast<const char*>(&buffer[0]), length);
                return true;
            }

        private:
            std::string _name;
        };
    } // namespace Message

} // namespace CEC
} // namespace Thunder