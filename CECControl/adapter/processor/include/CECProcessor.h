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

#include <CECTypes.h>

namespace Thunder {
namespace CEC {
    class Service;

    class OperationFrame;

    class Processor {
    public:
        Processor(Processor&) = delete;
        Processor& operator=(const Processor&) = delete;

        Processor()
            : _adminLock()
            , _awnsers()
        {
        }

        ~Processor()
        {
            TRACE(Trace::Information, ("Exit %s", __FUNCTION__));
        }

        static Processor& Instance()
        {
            return Core::SingletonType<Processor>::Instance();
        }

        /**
         * @brief Announce a queriable messages.
         *
         * @param service - A message service to formulate an usfull awnser
         * @return  uint32_t possible error codes:
         *  - ERROR_NONE - All went well.
         */
        uint32_t Announce(Service* service);

        /**
         * @brief Revoke a queriable message.
         *
         * @param service - A message service to be revoked
         * @return  uint32_t possible error codes:
         *  - ERROR_NONE - All went well.
         */
        uint32_t Revoke(Service* service);

        /**
         * @brief Processes a received frame.
         *        An anwser is always formulated in the message provided.
         *        So after calling this method you always will have a awnser
         *        and the orginal @message is overwritten.
         *
         *        Role is for future use, so we can error out on messages that
         *        are not within our capabilaties. For now just awser if we can....
         *
         * @param message - The message that was received.
         */
        void Process(/*const cec_adapter_role_t role,*/ OperationFrame& message, bool& broadcast);

    private:
        mutable Core::CriticalSection _adminLock;
        std::map<uint8_t, Service*> _awnsers;

    }; // class Processor
} // namespace CEC
} // namespace Thunder