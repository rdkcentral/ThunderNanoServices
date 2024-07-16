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

#include "Module.h"

#include <interfaces/IMath.h>


namespace Thunder {

namespace Plugin {

    class PolluxImplementation: public Exchange::IMath {
    public:
        PolluxImplementation()
            : IMath()
        {
        }
        ~PolluxImplementation() override
        {
        }

        PolluxImplementation(const PolluxImplementation&) = delete;
        PolluxImplementation& operator=(const PolluxImplementation&) = delete;
        PolluxImplementation(PolluxImplementation&&) = delete;
        PolluxImplementation& operator=(PolluxImplementation&&) = delete;

    public:
        // IMath overrides
        uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const override 
        {
            uint32_t result = Core::ERROR_NONE;
            sum = A + B;
            return (result);
        }
        uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const override 
        {
            uint32_t result = Core::ERROR_NONE;
            sum = A - B;
            return (result);
        }

    public:
        BEGIN_INTERFACE_MAP(PolluxImplementation)
            INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP

    }; // class PolluxImplementation

    SERVICE_REGISTRATION(PolluxImplementation, 1, 0)

} // namespace Plugin

}
