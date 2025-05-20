/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the License);
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
    struct ISimpleCustomObjects : virtual public Core::IUnknown {

        enum { ID = IUnknown::ID_OFFSET_CUSTOM + 0x0120 };

        // @json @encode:lookup
        struct IAccessory: virtual public Core::IUnknown {

            enum { ID = IUnknown::ID_OFFSET_CUSTOM + 1 };

            // @event
            struct INotification : public Core::IUnknown {

                enum { ID = IUnknown::ID_OFFSET_CUSTOM + 2 };

                // @statuslistener
                // @brief Signals addition of a accessory
                // @param name Name of the accessory (e.g. "mouse")
                virtual void NameChanged(const string& name) = 0;
            };

            virtual Core::hresult Register(INotification* const notification) = 0;
            virtual Core::hresult Unregister(const INotification* const notification) = 0;
    
            // @property
            // @brief Name of the accessory
            // @param name Name of the accessory (e.g. "mouse")
            virtual Core::hresult Name(string& name /* @out */) const = 0;
            virtual Core::hresult Name(const string& name) = 0;

            // @property
            // @brief Pin state
            virtual Core::hresult Pin(const uint8_t pin /* @index */, const bool value) = 0;
            virtual Core::hresult Pin(const uint8_t pin /* @index */, bool& value /* @out */) const = 0;
        };

        // @event
        struct INotification : public Core::IUnknown {

            enum { ID = IUnknown::ID_OFFSET_CUSTOM + 2 };

            // @statuslistener
            // @brief Signals addition of a accessory
            // @param accessory Accessory instance
            virtual void Added(IAccessory* const accessory) = 0;

            // @brief Signals removal of a accessory
            // @param accessory Accessory instance
            virtual void Removed(IAccessory* const accessory) = 0;
        };

        virtual Core::hresult Register(INotification* const notification) = 0;
        virtual Core::hresult Unregister(const INotification* const notification) = 0;

        // @property
        // @brief Accessory by name
        // @param name Name of the accessory to look for (e.g. "mouse")
        // @param accessory Accessory instance
        virtual Core::hresult Accessory(const string& name /* @index */, IAccessory*& accessory /* @out */) const = 0;
    };

} // namespace Exchange

}
