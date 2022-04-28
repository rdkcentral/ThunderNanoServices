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

using namespace WPEFramework;

namespace WPEFramework {
namespace CEC {

    struct la_role_conversion_entry {
        logical_address_t la;
        role_t role;
    };

    static constexpr la_role_conversion_entry _table_la_to_device[] = {
        { .la = CEC_LOGICAL_ADDRESS_TV, .role = CEC_DEVICE_TV },
        { .la = CEC_LOGICAL_ADDRESS_RECORD_1, .role = CEC_DEVICE_RECORDER },
        { .la = CEC_LOGICAL_ADDRESS_RECORD_2, .role = CEC_DEVICE_RECORDER },
        { .la = CEC_LOGICAL_ADDRESS_RECORD_3, .role = CEC_DEVICE_RECORDER },
        { .la = CEC_LOGICAL_ADDRESS_TUNER_1, .role = CEC_DEVICE_TUNER },
        { .la = CEC_LOGICAL_ADDRESS_TUNER_2, .role = CEC_DEVICE_TUNER },
        { .la = CEC_LOGICAL_ADDRESS_TUNER_3, .role = CEC_DEVICE_TUNER },
        { .la = CEC_LOGICAL_ADDRESS_TUNER_4, .role = CEC_DEVICE_TUNER },
        { .la = CEC_LOGICAL_ADDRESS_PLAYBACK_1, .role = CEC_DEVICE_PLAYBACK },
        { .la = CEC_LOGICAL_ADDRESS_PLAYBACK_2, .role = CEC_DEVICE_PLAYBACK },
        { .la = CEC_LOGICAL_ADDRESS_PLAYBACK_3, .role = CEC_DEVICE_PLAYBACK },
        { .la = CEC_LOGICAL_ADDRESS_AUDIOSYSTEM, .role = CEC_DEVICE_AUDIOSYSTEM },
        { .la = CEC_LOGICAL_ADDRESS_BACKUP_1, .role = CEC_DEVICE_VIDEOPROCESSOR },
        { .la = CEC_LOGICAL_ADDRESS_BACKUP_2, .role = CEC_DEVICE_VIDEOPROCESSOR },
        { .la = CEC_LOGICAL_ADDRESS_SPECIFIC, .role = CEC_DEVICE_SWITCH }
    };

    class Message : public Core::FrameType<0, false, uint8_t> {
    private:
        using BaseClass = Core::FrameType<0, false, uint8_t>;

    public:
        static constexpr uint8_t MaxLength = 16;
        static constexpr uint8_t HeaderLength = 1;
        static constexpr uint8_t MaxPayloadLength = MaxLength - HeaderLength;

    public:
        Message(Message&) = delete;
        Message& operator=(const Message&) = delete;

        Message()
            : BaseClass(_buffer, sizeof(_buffer), 0)
        {
            memset(_buffer, 0 , sizeof(_buffer));
        }

        explicit Message(const uint8_t length, const uint8_t raw[])
            : BaseClass(_buffer, sizeof(_buffer), 0)
        {

            memset(_buffer, 0 , sizeof(_buffer));

            ASSERT(length <= MaxLength);

            if ((length > 0) && (length <= MaxLength)) {
                BaseClass::Size(length);
                BaseClass::Copy(0, length, raw);
            }
        }

        explicit Message(const uint8_t initiator, const uint8_t follower)
            : BaseClass(_buffer, sizeof(_buffer), 0)
        {

            memset(_buffer, 0 , sizeof(_buffer));

            BaseClass::Size(1);
            BaseClass::Writer writer(*this, 0);

            writer.Number(uint8_t(((initiator & 0x0f) << 4) | (follower & 0x0f)));
        }

        ~Message() = default;

    public:
        logical_address_t Initiator() const
        {
            return static_cast<logical_address_t>(BaseClass::Data()[0] >> 4);
        }

        void Initiator(logical_address_t initiator)
        {
            BaseClass::Writer writer(*this, 0);
            uint8_t header = (BaseClass::Data()[0] & 0x0f) | ((initiator & 0x0f) << 4);

            writer.Number(header);
        }

        logical_address_t Follower() const
        {
            return static_cast<logical_address_t>(BaseClass::Data()[0] & 0xf);
        }

        void Follower(logical_address_t follower)
        {
            BaseClass::Writer writer(*this, 0);
            uint8_t header = (BaseClass::Data()[0] & 0xf0) | (follower & 0x0f);
            writer.Number(header);
        }

        uint8_t* LockRaw()
        {
            return &BaseClass::operator[](0);
        }

        void UnlockRaw(const uint8_t length)
        {
            ASSERT(length == 0);
            BaseClass::Size(length);
        }

        uint8_t* LockPayload()
        {
            return &BaseClass::operator[](HeaderLength);
        }

        void UnlockPayload(const uint8_t length)
        {
            ASSERT(length <= MaxPayloadLength);
            BaseClass::Size(HeaderLength + length);
        }

        const uint8_t* Payload() const
        {
            return (Size() > HeaderLength) ? &BaseClass::Data()[HeaderLength] : nullptr;
        }

        int8_t Payload(const uint8_t length, const uint8_t data[])
        {
            return BaseClass::Copy(HeaderLength, length, data);
        }

        uint8_t PayloadLength() const
        {
            return (Size() > HeaderLength) ? Size() - HeaderLength : 0;
        }

    private:
        uint8_t _buffer[MaxLength];
    };

} // namespace CEC
} // namespace WPEFramework
