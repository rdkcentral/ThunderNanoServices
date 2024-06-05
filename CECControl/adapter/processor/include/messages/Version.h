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
        class Version : public ExchangeType<GET_CEC_VERSION, CEC_VERSION, Version> {
            friend ExchangeType;
        public:
            Version() : _parameter(VERSION_CEC_UNKNOWN){}
            ~Version() override = default;

            version_t Value() const
            {
                return _parameter;
            }

        private:
            bool DeserializeParameters(const uint8_t /*length*/, const uint8_t buffer[])
            {
                _parameter = static_cast<version_t>(buffer[0]);
                return true;
            }

        private:
            version_t _parameter;
        };
    } // namespace Message

} // namespace CEC
} // namespace Thunder