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

#include <core/core.h>

#include <CECTypes.h>

using namespace Thunder;

namespace Thunder {
namespace CEC {
    // OperationFrame  |[OpCode 1][Parameters 0-13]|
    class OperationFrame : public Core::FrameType<0, false, uint8_t> {
    private:
        using BaseClass = Core::FrameType<0, false, uint8_t>;

    public:
        static constexpr uint8_t MaxLength = 15;
        static constexpr uint8_t OpCodeLength = 1;
        static constexpr uint8_t MaxParameterLength = MaxLength - OpCodeLength;

    public:
        OperationFrame(OperationFrame&) = delete;
        OperationFrame& operator=(const OperationFrame&) = delete;

        OperationFrame()
            : BaseClass(_buffer, sizeof(_buffer), 0)
        {
            memset(_buffer, 0 , sizeof(_buffer));
        }

        OperationFrame(const uint8_t length, const uint8_t raw[])
            : BaseClass(_buffer, sizeof(_buffer), 0)
        {
            ASSERT(length <= MaxLength);

            memset(_buffer, 0 , sizeof(_buffer));

            if ((length > 0) && (length <= MaxLength)) {
                BaseClass::Size(length);
                BaseClass::Copy(0, length, raw);
            }
        }

        OperationFrame(const opcode_t opCode, const uint8_t length, const uint8_t parameters[])
            : BaseClass(_buffer, sizeof(_buffer), 0)
        {
            ASSERT(length > MaxParameterLength);

            memset(_buffer, 0 , sizeof(_buffer));

            if ((length > 0) && (length <= MaxParameterLength)) {
                BaseClass::Size(length + OpCodeLength);
                OpCode(opCode);
                BaseClass::Copy(2, length, parameters);
            }
        }

        ~OperationFrame() = default;

    public:
        opcode_t OpCode() const
        {
            return (Size() >= OpCodeLength) ? static_cast<opcode_t>(BaseClass::Data()[0]) : NO_OPCODE;
        }

        void OpCode(opcode_t opcode)
        {
            if (opcode != NO_OPCODE) {
                BaseClass::Writer writer(*this, 0);
                writer.Number(opcode);
            }
        }

        const uint8_t* Parameter() const
        {
            return (Size() > (OpCodeLength)) ? &BaseClass::Data()[(OpCodeLength)] : nullptr;
        }

        int8_t Parameter(const uint8_t length, const uint8_t data[])
        {
            return BaseClass::Copy(OpCodeLength, length, data);
        }

        uint8_t ParameterLength() const
        {
            return (Size() > (OpCodeLength)) ? Size() - (OpCodeLength) : 0;
        }

        uint8_t* LockParameters()
        {
            return &BaseClass::operator[](OpCodeLength);
        }

        void UnlockParameters(const uint8_t length)
        {
            ASSERT(length <= MaxParameterLength);
            if (length > 0) BaseClass::Size(OpCodeLength + length);
        }

        uint8_t* LockData()
        {
            return &BaseClass::operator[](0);
        }

        void UnlockData(const uint8_t length)
        {
            ASSERT(length <= MaxParameterLength);
            if (length > 0) BaseClass::Size(length);
        }

    private:
        uint8_t _buffer[MaxLength];
    };

} // namespace CEC
} // namespace Thunder
