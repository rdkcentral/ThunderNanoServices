/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "Dolby.h"

namespace WPEFramework
{
    namespace Plugin
    {

        class DolbyOutputImplementation : public Exchange::Dolby::IOutput
        {
        public:
            void Mode(const Exchange::Dolby::IOutput::Type value) override
            {
                uint32_t errorCode = set_audio_output_type(value);

                ASSERT(errorCode == Core::ERROR_NONE);
            };

            Exchange::Dolby::IOutput::Type Mode() const override
            {
                uint32_t errorCode = Core::ERROR_NONE;
                Exchange::Dolby::IOutput::Type result = get_audio_output_type(errorCode);

                ASSERT(errorCode == Core::ERROR_NONE);
            };

            BEGIN_INTERFACE_MAP(DolbyOutputImplementation)
                INTERFACE_ENTRY(Exchange::Dolby::IOutput)
            END_INTERFACE_MAP
        };

        SERVICE_REGISTRATION(DolbyOutputImplementation, 1, 0);

    } // namespace Plugin
} // namespace WPEFramework