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
            // @param address Device address (e.g. 192.158.1.38)
            // @param state Result of pairing operation (e.g. CONNECTING_ABORTED)
            virtual void Complete(const string& address, const state state)  = 0;
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
        // @param address Server address (e.g. 192.158.1.38)
        // @param timeout Maximum time allowed for connecting in milliseconds
        // @retval ERROR_INPROGRESS Currently connecting
        // @retval ERROR_ALREADY_CONNECTED Already connected to server
        virtual Core::hresult Connect(const string& address, const Core::OptionalType<uint16_t>& timeout /* @default:1000 */, ICallback* const cb) = 0;

        // @brief Aborts connecting to a server
        // @retval ERROR_ILLEGAL_STATE There is no ongoing connection
        virtual Core::hresult Abort() = 0;

        // @brief Disconnects from the server
        // @retval ERROR_INPROGRESS Connecting in progress, abort first
        // @retval ERROR_ALREADY_RELEASED Not connected to server
        virtual Core::hresult Disconnect() = 0;
    };

} // namespace Exchange

}
