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

#include "Module.h"

namespace Thunder {
namespace Exchange {

    // @json 1.0.0
    struct EXTERNAL IYang : virtual public Core::IUnknown {

        enum { ID = IUnknown::ID_OFFSET_CUSTOM + 0x0020 };

        enum color : uint8_t {
            GRAY,
            DARK_GRAY,
            BLACK
        };

        // @event
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = IYang::ID + 1 };

            // @brief Notifies of yang percentage change
            // @param percentage New percentage of yang in the "universal balance" (e.g. 50)
            virtual void BalanceChanged(const uint8_t percentage) = 0;
        };

        virtual uint32_t Register(INotification* notification) = 0;
        virtual uint32_t Unregister(const INotification* notification) = 0;

        // @property
        // @brief Color of yang
        // @detail Change of color is persistent.
        virtual uint32_t Color(const color& value) = 0;
        virtual uint32_t Color(color& value /* @out */) const = 0;

        // @property
        // @brief Meaning of yang
        // @param meaning (e.g. the dark side)
        virtual uint32_t Etymology(string& meaning /* @out */) const = 0;

        // @property
        // @brief Percentage of yang in the "universal balance" of the system
        // @details The Yin service is additionally required to change the yin/yang balance. 
        // @param percentage (e.g. 50)
        // @retval ERROR_BAD_REQUEST Given percentage value is invalid
        // @retval ERROR_UNAVAILABLE Can't set yang because yin is not available
        virtual uint32_t Balance(const uint8_t& percentage) = 0;
        virtual uint32_t Balance(uint8_t& percentage /* @out */) const = 0;

        // @json:omit
        virtual void Configure(PluginHost::IShell* service) = 0;
    };

} // namespace Exchange

}
