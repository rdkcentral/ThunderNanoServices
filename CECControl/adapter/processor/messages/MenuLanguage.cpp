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

#include <CECOperationFrame.h>
#include <CECMessage.h>

namespace Thunder {
namespace CEC {
    namespace Message {
        namespace Service {
            // CEC 1.1
            // ISO 639.2 Code for the representation of names of languages - Part 2: Alpha 3 code
            // http://www.loc.gov/standards/iso639-2/langhome.html

            class MenuLanguage : public ServiceType<GET_MENU_LANGUAGE, SET_MENU_LANGUAGE, MenuLanguage, true> {
                friend ServiceType;

            public:
                MenuLanguage()
                    : ServiceType()
                {
                }

                ~MenuLanguage() override = default;

            private:
                uint8_t Process(const uint8_t length, uint8_t buffer[])
                {
                    //  ISO 639.2 (alpha-3, bibliographic code) [dut,eng,deu...]
                    char languageCode[3] = { 'e', 'n', 'g' }; // TODO: Get the real language code from somewhere in the system.

                    ASSERT(sizeof(languageCode) < length);

                    uint8_t chars = (length >= sizeof(languageCode)) ? sizeof(languageCode) : CEC::OperationFrame::MaxParameterLength;

                    memcpy(buffer, languageCode, chars);

                    return chars;
                }
            };
        } // namespace Response

        static Service::MenuLanguage service_osd_name;

    } // namespace Message
} // namespace CEC
} // namespace Thunder
