/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include "AudioCodec.h"

namespace WPEFramework {

namespace A2DP {

    class TransportChannel : public Bluetooth::RTPSocket {
    private:
        static constexpr uint16_t DefaultMTU = 672;
        static constexpr uint16_t OpenTimeout = 2000; // ms
        static constexpr uint16_t CloseTimeout = 5000;
        static constexpr uint16_t PacketTimeout = 100;

    private:
        class TransportFlow {
        public:
            ~TransportFlow() = default;
            TransportFlow() = delete;
            TransportFlow(const TransportFlow&) = delete;
            TransportFlow& operator=(const TransportFlow&) = delete;
            TransportFlow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            explicit TransportFlow(const string& text)
                : _text(Core::ToString(text))
            {
            }

        public:
            const char* Data() const
            {
                return (_text.c_str());
            }
            uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        }; // class TransportFlow

    public:
        TransportChannel() = delete;
        TransportChannel(const TransportChannel&) = delete;
        TransportChannel& operator=(const TransportChannel&) = delete;

        TransportChannel(const uint8_t ssrc, const Core::NodeId& localNode, const Core::NodeId& remoteNode, const uint16_t mtu = DefaultMTU)
            : Bluetooth::RTPSocket(localNode, remoteNode, mtu)
            , _lock()
            , _codec(nullptr)
            , _ssrc(ssrc)
            , _timestamp(0)
            , _sequence(0)
        {
        }
        ~TransportChannel() = default;

    private:
        bool Initialize() override
        {
            return (true);
        }
        void Operational() override
        {
            TRACE(TransportFlow, (_T("Bluetooth A2DP/RTP transport channel is operational")));

            uint32_t flushable = 1;
            if (setsockopt(Handle(), SOL_BLUETOOTH, BT_FLUSHABLE, &flushable, sizeof(flushable)) < 0) {
                TRACE(Trace::Error, (_T("Failed to set the RTP socket flushable")));
            }
        }

    public:
        uint32_t Connect(const Core::NodeId& remoteNode, IAudioCodec* codec)
        {
            RemoteNode(remoteNode);

            uint32_t result = Open(OpenTimeout);
            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Failed to open A2DP/RTP transport socket [%d]"), result));
            } else {
                TRACE(TransportFlow, (_T("Successfully opened A2DP/RTP transport socket")));
                _codec = codec;
                Reset();
            }

            return (result);
        }
        uint32_t Disconnect()
        {
            uint32_t result = Core::ERROR_NONE;

            _codec = nullptr;

            if (IsOpen() == true) {
                result = Close(CloseTimeout);
                if (result != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Failed to close AVDTP/RTP transport socket [%d]"), result));
                } else {
                    TRACE(TransportFlow, (_T("Successfully closed AVDTP/RTP transport socket")));
                }
            }

            return (result);
        }

    public:
        uint32_t Timestamp() const
        {
            return (_timestamp);
        }
        void Timestamp(const uint32_t newTimestamp)
        {
            _timestamp = newTimestamp;
        }
        uint32_t ClockRate() const
        {
            ASSERT(_codec != nullptr);
            return (_codec->ClockRate());
        }
        uint8_t Channels() const
        {
            ASSERT(_codec != nullptr);
            return (_codec->Channels());
        }
        void Reset()
        {
            _timestamp = 0;
            // Ideally the sequence should start with a random value...
            _sequence = (Core::Time::Now().Ticks() % 65535);
        }
        uint32_t Transmit(const uint32_t length /* in bytes! */, const uint8_t data[])
        {
            ASSERT(_codec != nullptr);

            // Limit the packet size to MTU, so it does not have to be fragmented.
            uint8_t scratchPad[SendBufferSize()];

            // Payload type should be a value from the dynamic range (96-127).
            // Typically 96 is chosen for A2DP implementations.
            const uint16_t payloadType = 96;

            uint32_t consumed = 0;

            MediaPacket packet(_ssrc, payloadType, _sequence, _timestamp, sizeof(scratchPad), scratchPad);
            consumed = packet.Ingest([this, length, data](uint8_t buffer[], const uint32_t maxLength, uint32_t& consumed) -> uint32_t {
                uint32_t outSize = maxLength;
                consumed = _codec->Encode(length, data, outSize, buffer);
                return (outSize);
            });

            if (consumed > 0) {
                uint32_t result = Exchange(PacketTimeout, packet);
                if (result != Core::ERROR_NONE) {
                    TRACE_L1("Failed to send out media packet (%d)", result);
                }

                // Timestamp clock frequency is the same as the sampling frequency.
                _timestamp += (consumed / (_codec->Channels() * 2 /* always 16-bit samples! */));

                _sequence++;
            }

            return (consumed);
        }

    private:
        Core::CriticalSection _lock;
        IAudioCodec* _codec;
        uint8_t _ssrc;
        uint32_t _timestamp;
        uint16_t _sequence;
    }; // class TransportChannel

} // namespace A2DP

}


