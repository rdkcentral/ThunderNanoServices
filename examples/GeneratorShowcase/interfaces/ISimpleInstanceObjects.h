/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Metrological
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
    struct ISimpleInstanceObjects : virtual public Core::IUnknown {

        enum { ID = IUnknown::ID_OFFSET_CUSTOM + 0x0110 };

        enum state : uint8_t {
            ENABLED,
            DISABLED
        };

        // @json
        struct IDevice : virtual public Core::IUnknown {

            enum { ID = ISimpleInstanceObjects::ID + 1 };

            // @event
            // A notification per instance of device
            struct INotification : virtual public Core::IUnknown {

                enum { ID = ISimpleInstanceObjects::ID + 2 };

                // @brief Signals device name changes
                // @param state New name of the device
                virtual void NameChanged(const string& state) = 0;

                // @statuslistener
                // @brief Signals device state changes
                // @param state New state of the device
                virtual void StateChanged(const state state) = 0;

                // @statuslistener
                // @brief Signals pin state changes
                virtual void PinChanged(const uint8_t pin /* @index */, const bool high) = 0;
            };

            virtual Core::hresult Register(INotification* const notification) = 0;
            virtual Core::hresult Unregister(const INotification* const notificaiton) = 0;

            // @property
            // @brief Name of the device
            // @param name Name of the device (e.g. "usb")
            virtual Core::hresult Name(string& name /* @out */) const = 0;
            virtual Core::hresult Name(const string& name) = 0;

            // @brief Enable the device
            // @retval ERROR_ALREADY_CONNECTED The device is already enabled
            virtual Core::hresult Enable() = 0;

            // @brief Disable the device
            // @retval ERROR_ALREADY_RELEASED The device is not enabled
            virtual Core::hresult Disable() = 0;

            // @property
            // @brief A pin
            // @param pin Pin number
            // @retval ERROR_UNAVAILABLE Unknown pin number
            virtual Core::hresult Pin(const uint8_t pin /* @index */ , const bool high) = 0;
            virtual Core::hresult Pin(const uint8_t pin /* @index */, bool& high /* @out */) const = 0;
        };

        // @brief Acquires a device
        // @param name Name of the device to acquire (e.g. "usb")
        // @param device Instance of the acquired device
        // @retval ERROR_UNAVAILABLE The device is not available
        virtual Core::hresult Acquire(const string& name, IDevice*& device /* @out */) = 0;

        // @brief Relinquishes a device
        // @param device Device instance to relinquish
        // @retval ERROR_UNKNOWN_KEY The device is not acquired
        virtual Core::hresult Relinquish(IDevice* const device) = 0;
    };

} // namespace Exchange

}
