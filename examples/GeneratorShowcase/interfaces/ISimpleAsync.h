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
    struct ISimpleAsync : public virtual Core::IUnknown {

        enum { ID = IUnknown::ID_OFFSET_CUSTOM + 0x0100 };

        enum state : uint8_t {
            DISCONNECTED,
            CONNECTING,
            CONNECTED,
            CONNECTING_FAILED,
            CONNECTING_TIMED_OUT,
            CONNECTING_ABORTED
        };

        struct ICallback : public virtual Core::IUnknown {

            enum { ID = ISimpleAsync::ID + 1 };

            // Generator will create its implementation of this interface
            // and supply the object to the called async method. The
            // implementation of the completion method will issue a JSON-RPC
            // event.
            //
            // @brief Signals completion of the Connect method
            // @param address Device address (e.g. aa:bb:cc:dd:ee:ff)
            // @param state Result of pairing operation (e.g. CONNECTING_ABORTED)
            virtual void Complete(const Core::OptionalType<std::vector<uint8_t>>& address /* @encode:mac @restrict:6..6  */, const state state)  = 0;
        };

        // @async
        //
        // Asynchronous method. Its result will be delivered via notification.
        // It is expected that async method has one input interface parameter
        // and this interface has a single method that will be used to signal
        // completion. The prototype or name of the signalling method is not
        // predefined. On JSON-RPC level the interface parameter will be replaced
        // with client ID string, that will allow the client to match the call
        // with the async result. It is required that the callback is eventually
        // always called, whether it's success, failure or timeout.
        //
        // @brief Connects to a server
        // @param address Device address (e.g. aa:bb:cc:dd:ee:ff)
        // @param timeout Maximum time allowed for connecting in milliseconds
        // @retval ERROR_INPROGRESS Currently connecting
        // @retval ERROR_ALREADY_CONNECTED Already connected to server
        virtual Core::hresult Connect(const Core::OptionalType<std::vector<uint8_t>>& address /* @encode:mac @restrict:6..6 */, const Core::OptionalType<uint16_t>& timeout /* @default:1000 */, ICallback* const cb) = 0;

        // @brief Aborts connecting to a server
        // @retval ERROR_ILLEGAL_STATE There is no ongoing connection
        virtual Core::hresult Abort() = 0;

        // @brief Disconnects from the server
        // @retval ERROR_INPROGRESS Connecting in progress, abort first
        // @retval ERROR_ALREADY_RELEASED Not connected to server
        virtual Core::hresult Disconnect() = 0;

        // @property
        // @brief Connection status
        // @param address Device address (e.g. 11:22:33:44:55:66)
        virtual Core::hresult Connected(const std::vector<uint8_t>& address /* @index @encode:mac @restrict:6..6 */, bool& result /* @out */) const = 0;


        // BY ARRAY -----------

        // @brief Links a device
        // @param address Device address (e.g. 11:22:33:44:55:66)
        virtual Core::hresult Link(const uint8_t address[6] /* @encode:base64 */) = 0;

        // @brief Unlinks a device
        // @param address Device address (e.g. 11:22:33:44:55:66)
        virtual Core::hresult Unlink(const uint8_t address[6] /* @encode:base64 */) = 0;

        // @property
        // @brief Linked device
        // @param address Device address (e.g. 11:22:33:44:55:66)
        virtual Core::hresult LinkedDevice(uint8_t address[6] /* @encode:base64 @out */) const = 0;

        // @event
        struct INotification : public virtual Core::IUnknown {

            enum { ID = ISimpleAsync::ID + 2 };

            // @brief Signals completion of the Connect method
            // @param address Device address (e.g. [11,22] )
            // @param linked Denotes if device is linked
            virtual void StatusChanged(const uint8_t address[6], const bool linked)  = 0;
        };

        virtual Core::hresult Register(INotification* const notification) = 0;
        virtual Core::hresult Unregister(const INotification* const notification) = 0;

        // @property
        // @brief Device metadata
        // @param address Device address (e.g. 11:22:33:44:55:66)
        virtual Core::hresult Metadata(const uint8_t address[6] /* @index @encode:base64 */, const string& metadata) = 0;
        virtual Core::hresult Metadata(const uint8_t address[6] /* @index @encode:base64 */, string& metadata /* @out */) const = 0;

        // BY MACADDRESS -------------------------

        // @brief Binds a device
        // @param address Device address (e.g. 11:22:33:44:55:66)
        virtual Core::hresult Bind(const Core::MACAddress& address) = 0;

        // @brief Unlinks a device
        // @param address Device address (e.g. 11:22:33:44:55:66)
        virtual Core::hresult Unbind(const Core::MACAddress& address) = 0;

        // @property
        virtual Core::hresult BoundDevice(Core::MACAddress& address /* @out */) const = 0;

        // @property
        // @brief Device metadata
        // @param address Device address (e.g. 11:22:33:44:55:66)
        virtual Core::hresult Type(const Core::MACAddress& address /* @index */, const string& value) = 0;
        virtual Core::hresult Type(const Core::MACAddress& address /* @index */, string& value /* @out */) const = 0;

        // @event
        struct IBindNotification : public virtual Core::IUnknown {

            enum { ID = ISimpleAsync::ID + 3 };

            // @brief Signals completion of the Connect method
            // @param address Device address (e.g. [11,22] )
            // @param linked Denotes if device is linked
            virtual void BindingChanged(const Core::MACAddress& address/* @index */, const bool bound)  = 0;
        };

        virtual Core::hresult Register(IBindNotification* const notification) = 0;
        virtual Core::hresult Unregister(const IBindNotification* const notification) = 0;

    };

} // namespace Exchange

}
