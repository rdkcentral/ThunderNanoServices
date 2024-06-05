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
        // CEC 1.1
        // ISO 639.2 Code for the representation of names of languages - Part 2: Alpha 3 code
        // http://www.loc.gov/standards/iso639-2/langhome.html

        class MenuLanguage : public ExchangeType<GET_MENU_LANGUAGE, SET_MENU_LANGUAGE, MenuLanguage> {
            friend ExchangeType;
        public:
            MenuLanguage() = default;
            ~MenuLanguage() override = default;

            string Value() const
            {
                return string(_languageCode, sizeof(_languageCode));
            }

        private:
            bool DeserializeParameters(const uint8_t length, const uint8_t buffer[])
            {
                ASSERT(sizeof(_languageCode) == length);

                memcpy(_languageCode, buffer, sizeof(_languageCode));

                return true;
            }

        private:
            char _languageCode[3];
        };
    } // namespace Message

} // namespace CEC
} // namespace Thunder