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

#include "CECProcessor.h"
#include "Module.h"

#include <CECTypes.h>

namespace Thunder {
namespace CEC {
    struct EXTERNAL IExchange {
        virtual uint8_t Serialize(const uint8_t length, uint8_t buffer[]) const = 0;
        virtual bool Deserialize(const uint8_t length, const uint8_t buffer[]) = 0;
        virtual bool AwnserExpected() const = 0;
        virtual ~IExchange() = default;
    };

    template <opcode_t REQUEST_OPCODE, opcode_t RESPONSE_OPCODE, typename EXCHANGE>
    class ExchangeType : public IExchange {
    public:
        ExchangeType() {};

        ExchangeType(ExchangeType&) = delete;
        ExchangeType& operator=(const ExchangeType&) = delete;

        ~ExchangeType() override = default;

        bool AwnserExpected() const final
        {
            return (RESPONSE_OPCODE != NO_OPCODE);
        }

        uint8_t Serialize(const uint8_t length, uint8_t buffer[]) const
        {
            buffer[0] = REQUEST_OPCODE;
            return (REQUEST_OPCODE != NO_OPCODE) ? __SerializeParameters(length - 1, &(buffer)[1]) + 1 : 0;
        }

        bool Deserialize(const uint8_t length, const uint8_t buffer[])
        {
            return ((buffer[0] == RESPONSE_OPCODE) && (length > 0)) ? __DeserializeParameters(length - 1, &(buffer)[1]) : false;
        }

    private:
        // -----------------------------------------------------
        // Check for Serialize method on Object
        // -----------------------------------------------------
        IS_MEMBER_AVAILABLE(SerializeParameters, hasSerialize);

        template <typename TYPE = EXCHANGE>
        inline typename Core::TypeTraits::enable_if<hasSerialize<const TYPE, uint8_t, const uint8_t, uint8_t[]>::value, uint8_t>::type
        __SerializeParameters(VARIABLE_IS_NOT_USED const uint8_t length, VARIABLE_IS_NOT_USED uint8_t buffer[]) const
        {
            return static_cast<const EXCHANGE&>(*this).SerializeParameters(length, buffer);
        }

        template <typename TYPE = EXCHANGE>
        inline typename Core::TypeTraits::enable_if<!hasSerialize<const TYPE, uint8_t, const uint8_t, uint8_t[]>::value, uint8_t>::type
        __SerializeParameters(VARIABLE_IS_NOT_USED const uint8_t length, VARIABLE_IS_NOT_USED uint8_t buffer[]) const
        {
            return 0;
        }

        // -----------------------------------------------------
        // Check for Deserialize method on Object
        // -----------------------------------------------------
        IS_MEMBER_AVAILABLE(DeserializeParameters, hasDeserialize);

        template <typename TYPE = EXCHANGE>
        inline typename Core::TypeTraits::enable_if<hasDeserialize<TYPE, bool, const uint8_t, const uint8_t[]>::value, bool>::type
        __DeserializeParameters(const uint8_t length, const uint8_t buffer[])
        {
            static_cast<EXCHANGE&>(*this).DeserializeParameters(length, buffer);
            return true;
        }

        template <typename TYPE = EXCHANGE>
        inline typename Core::TypeTraits::enable_if<!hasDeserialize<TYPE, bool, const uint8_t, const uint8_t[]>::value, bool>::type
        __DeserializeParameters(VARIABLE_IS_NOT_USED const uint8_t length, VARIABLE_IS_NOT_USED const uint8_t buffer[])
        {
            return false;
        }
    };

    class Service {
    public:
        Service() = delete;
        Service(Service&) = delete;
        Service& operator=(const Service&) = delete;

        Service(const opcode_t requestOpCode, const opcode_t responseOpCode, const bool isBroadcast, const role_mask_t allowedRoles);
        virtual ~Service();

        virtual uint8_t Handle(const uint8_t maxLength, const uint8_t length, uint8_t buffer[]) = 0;

        inline opcode_t RequestOpCode() const
        {
            return _requestOpCode;
        }
        inline opcode_t ResponseOpCode() const
        {
            return _responseOpCode;
        }
        inline bool IsBroadcast() const
        {
            return _isBroadcast;
        }

        inline bool IsAllowed(const cec_adapter_role_t role) const
        {
            return ((_allowedRoles & role) > 0);
        }

    private:
        const opcode_t _requestOpCode;
        const opcode_t _responseOpCode;
        const bool _isBroadcast;
        const role_mask_t _allowedRoles;
    };

    template <opcode_t REQUEST_OPCODE, opcode_t RESPONSE_OPCODE, typename RESPONSE, bool BROADCAST = false, role_mask_t ALLOWED_ROLES = CEC_DEVICE_ALL>
    class ServiceType : public Service {
    public:
        ServiceType()
            : Service(REQUEST_OPCODE, RESPONSE_OPCODE, BROADCAST, ALLOWED_ROLES)
        {
        }

        virtual ~ServiceType()
        {
        }

        // This will handle a request from an initiator and give you a proper response
        // to send back. This is either the requested info or a feature abort if the
        // request is not supported
        uint8_t Handle(const uint8_t maxLength, const uint8_t length, uint8_t buffer[])
        {
            uint8_t result(~0);

            if (__IsValid(length, buffer)) {
                result = __Process(maxLength, buffer);
            }

            return result;
        }

    private:
        // -----------------------------------------------------
        // Check for IsValid method on Object Check
        // -----------------------------------------------------
        IS_MEMBER_AVAILABLE(IsValid, hasParameters);

        template <typename TYPE = RESPONSE>
        inline typename Core::TypeTraits::enable_if<hasParameters<TYPE, bool, const uint8_t, const uint8_t[]>::value, bool>::type
        __IsValid(const uint8_t length, const uint8_t buffer[])
        {
            return static_cast<RESPONSE&>(*this).IsValid(length, buffer);
        }

        template <typename TYPE = RESPONSE>
        inline typename Core::TypeTraits::enable_if<!hasParameters<TYPE, bool, const uint8_t, const uint8_t[]>::value, bool>::type
        __IsValid(VARIABLE_IS_NOT_USED const uint8_t length, VARIABLE_IS_NOT_USED const uint8_t buffer[])
        {
            return true;
        }

        // -----------------------------------------------------
        // Check for Process method on Object
        // -----------------------------------------------------
        IS_MEMBER_AVAILABLE(Process, hasProcess);

        template <typename TYPE = RESPONSE>
        inline typename Core::TypeTraits::enable_if<hasProcess<TYPE, uint8_t, const uint8_t, uint8_t[]>::value, uint8_t>::type
        __Process(const uint8_t length, uint8_t buffer[])
        {
            return static_cast<RESPONSE&>(*this).Process(length, buffer);
        }

        template <typename TYPE = RESPONSE>
        inline typename Core::TypeTraits::enable_if<!hasProcess<TYPE, uint8_t, const uint8_t, uint8_t[]>::value, uint8_t>::type
        __Process(VARIABLE_IS_NOT_USED const uint8_t length, VARIABLE_IS_NOT_USED uint8_t buffer[])
        {
            return 0;
        }
    };
}
}