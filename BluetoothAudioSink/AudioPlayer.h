/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 RDK Management
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
#include "TransportChannel.h"
#include "Tracing.h"

namespace WPEFramework {

namespace Plugin {

    class AudioPlayer : public Core::Thread {
    private:
        static constexpr uint16_t WRITE_AHEAD_THRESHOLD = 10 /* miliseconds */;

    private:
        class ReceiveBuffer : public Core::SharedBuffer {
        public:
            ReceiveBuffer() = delete;
            ReceiveBuffer(const ReceiveBuffer&) = delete;
            ReceiveBuffer& operator=(const ReceiveBuffer&) = delete;

            ReceiveBuffer(const string& name)
                : Core::SharedBuffer(name.c_str())
            {
            }

            ~ReceiveBuffer() = default;

        public:
            uint32_t Get(const uint32_t length, uint8_t buffer[])
            {
                uint32_t result = 0;

                if (IsValid() == true) {
                    if (RequestConsume(100) == Core::ERROR_NONE) {
                        ASSERT(BytesWritten() <= length);
                        const uint32_t size = std::min(BytesWritten(), length);
                        ::memcpy(buffer, Buffer(), size);
                        result = size;
                        Consumed();
                    }
                }

                return (result);
            }
        }; // class ReceiveBuffer

    public:
        AudioPlayer() = delete;
        AudioPlayer(const AudioPlayer&) = delete;
        AudioPlayer& operator=(const AudioPlayer&) = delete;

    public:
        AudioPlayer(A2DP::TransportChannel& transport, const string& connector)
            : _transport(transport)
            , _startTime(0)
            , _offset(0)
            , _available(0)
            , _minFrameSize(0)
            , _maxFrameSize(0)
            , _preferredFrameSize(0)
            , _receiveBuffer(connector)
            , _buffer(nullptr)
            , _readCursor(nullptr)
            , _bufferSize(0)
            , _eos(false)
        {
            _minFrameSize = _transport.MinFrameSize();
            _preferredFrameSize = _transport.PreferredFrameSize();

            ASSERT(_minFrameSize != 0);
            ASSERT(_preferredFrameSize != 0);
            ASSERT(_minFrameSize <= _preferredFrameSize);

            if (_receiveBuffer.IsValid() == true) {
                _maxFrameSize = _receiveBuffer.Size();
                if (_maxFrameSize != 0) {
                    const uint32_t multiplier = (((2 * _preferredFrameSize) / _maxFrameSize) + 1);
                    _bufferSize = multiplier * _maxFrameSize;
                    _buffer = static_cast<uint8_t*>(::malloc(_bufferSize));
                    ASSERT(_buffer != nullptr);
                }
            }

            if (_maxFrameSize == 0) {
                TRACE(Trace::Error, (_T("Shared buffer not available")));
            }
        }
        ~AudioPlayer()
        {
            Stop();
            ::free(_buffer);
        }

    public:
        bool IsValid() const
        {
            return ((_buffer != nullptr) && (_receiveBuffer.IsValid() == true) && (_transport.IsOpen() == true));
        }
        uint32_t Play()
        {
            uint32_t result = Core::ERROR_NONE;

            if (IsValid() == true) {
                _transport.Reset();
                _readCursor = _buffer;
                _available = 0;
                _startTime = Core::Time::Now().Ticks();
                Thread::Run();
            } else {
                TRACE(Trace::Error, (_T("Transport channel is not open!")));
                result = Core::ERROR_ILLEGAL_STATE;
            }

            return (result);
        }
        uint32_t Stop()
        {
            uint32_t result = Core::ERROR_NONE;

            if (IsValid() == true) {
                _eos = true;
                Thread::Wait(BLOCKED, Core::infinite);
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            return (result);
        }
        uint32_t GetTime(uint32_t& time /* milliseconds */) const
        {
            uint32_t result = Core::ERROR_NONE;

            if (IsValid() == true) {
                time = _offset + ((1000ULL * _transport.Timestamp()) / _transport.ClockRate());
            } else {
                result = Core::ERROR_BAD_REQUEST;
                time = 0;
            }

            return (result);
        }
        uint32_t SetTime(const uint32_t time /* milliseconds */)
        {
            _offset = time;
            return (Core::ERROR_NONE);
        }

    private:
        uint32_t PlayTime() const
        {
            return ((1000ULL * _transport.Timestamp()) / _transport.ClockRate());
        }

        uint32_t Worker() override
        {
            uint32_t delay = 0;
            uint32_t transmitted = 0;

            if ((_available < _minFrameSize) && (_eos != true)) {
                // Have to replenish the local buffer...
                // TODO: optimization opportunity
                ::memmove(_buffer, _readCursor, _available);

                // Make sure we have at least the encoder preferred frame size available.
                while (_available < _preferredFrameSize) {
                    ASSERT(_available + _maxFrameSize <= _bufferSize);

                    const uint16_t bytesRead =_receiveBuffer.Get(_maxFrameSize, (_buffer + _available));
                    ASSERT(bytesRead <= _maxFrameSize);

                    _available += bytesRead;

                    if (bytesRead == 0) {
                        // The audio source has problem providing more data at the moment...
                        // We don't have the preferred data available, let's try to play out whatever there is left in the buffer anyway.
                        if (_available < _minFrameSize) {
                            // All data was used and no new is currently available, apparently the source has stalled.
                            _offset += PlayTime();
                            _startTime = Core::Time::Now().Ticks();
                            _transport.Reset();
                        }
                        break;
                    }
                }

                _readCursor = _buffer;
            }

            if (_transport.IsOpen() == true) {
                const uint32_t playTime = PlayTime();
                const uint32_t elapsedTime = ((Core::Time::Now().Ticks() - _startTime) / 1000);

                if ((playTime > WRITE_AHEAD_THRESHOLD) && (playTime - WRITE_AHEAD_THRESHOLD > elapsedTime)) {
                    // We're writing ahead of time, let's wait a bit, so the device's buffer does not overflow.
                    ::SleepMs(playTime - elapsedTime);
                }

                if (_available >= _minFrameSize) {
                    transmitted = _transport.Transmit(_available, _readCursor);

#ifdef __DEBUG__
                    fprintf(stderr, "streaming; time %3i.%03i / %3i.%03i sec; delta %3i ms, frame %i bytes  \r",
                            (playTime / 1000), (playTime % 1000), (elapsedTime / 1000), (elapsedTime % 1000), (playTime - elapsedTime), transmitted);
#endif

                    _readCursor += transmitted;
                    _available -= transmitted;
                }
            } else {
                TRACE(Trace::Error, (_T("Bluetooth transport link failure - terminating audio stream")));
                _eos = true;
            }

            if ((transmitted == 0) && (_eos == true)) {
                // Played out everything in the buffer and end-of-stream was signalled.
                Thread::Block();
                delay = Core::infinite;
            }

            return (delay);
        }

    private:
        A2DP::TransportChannel& _transport;
        uint64_t _startTime;
        uint32_t _offset;
        uint32_t _available;
        uint32_t _minFrameSize;
        uint32_t _maxFrameSize;
        uint32_t _preferredFrameSize;
        ReceiveBuffer _receiveBuffer;
        uint8_t* _buffer;
        uint8_t* _readCursor;
        uint32_t _bufferSize;
        bool _eos;
    };

} // namespace Plugin

}