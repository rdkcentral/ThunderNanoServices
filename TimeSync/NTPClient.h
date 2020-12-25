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

#ifndef TIMESYNC_NTPCLIENT_H
#define TIMESYNC_NTPCLIENT_H

#include "Module.h"
#include <interfaces/ITimeSync.h>

namespace WPEFramework {
namespace Plugin {

    class NTPClient : public Core::SocketDatagram, public Exchange::ITimeSync, public PluginHost::ISubSystem::ITime {
    public:
        static constexpr uint32_t MilliSeconds = 1000;
        static constexpr uint32_t MicroSeconds = 1000 * MilliSeconds;
        static constexpr uint32_t NanoSeconds = 1000 * MicroSeconds;

        using SourceIterator = Core::JSON::ArrayType<Core::JSON::String>::Iterator;

    private:
        using ServerList = std::vector<string>;
        using ServerIterator = Core::IteratorType<const ServerList, const string&, ServerList::const_iterator>;
        using DataFrame = Core::FrameType<0>;

        // This enum tracks the state for actions begin performed. As the Worker() method is re-entered,
        // we need to keep track of state.
        enum state {
            INITIAL, // Initial state
            SENDREQUEST, // Let send out an NTP request to a legitimate server.
            INPROGRESS, // A request has been sent to a NTP server, waiting for a response
            SUCCESS, // Action succeeded, we received a valid response from an NTP server
            FAILED // Action failed, we did not receive any valid response from any of the NTP servers
        };
        // As this forms the exact package to be sent for NTP, we need to make sure all members are byte
        // aligned
        class NTPPacket {
        public:
            class Timestamp {
            private:
                // Start day of NTP time as days past the imaginary date 12/1/1 BC.
                // (This is the beginning of the Christian Era, or BCE.)
                static constexpr uint32_t DayNTPStarts = 693596;

                // Start day of the UNIX epoch (1970-01-01), also counting from BCE
                static constexpr uint32_t DayUNIXEpochStarts = 719163;

                // Difference in Seconds between UNIX and NTP epoch (25567).
                static constexpr uint32_t SecondsPerMinute = 60;
                static constexpr uint32_t MinutesPerHour = 60;
                static constexpr uint32_t HoursPerDay = 24;
                static constexpr uint32_t SecondsPerHour = SecondsPerMinute * MinutesPerHour;
                static constexpr uint32_t SecondsPerDay = SecondsPerHour * HoursPerDay;

                static constexpr uint32_t NTPToUNIXSeconds = (DayUNIXEpochStarts - DayNTPStarts) * SecondsPerDay;

                static constexpr double NanoSecondFraction = 4294967296.0 /* 2^32 as a double*/ / NanoSeconds;

            public:
                // Initialize default
                Timestamp()
                {
                    _source.tv_sec = NTPToUNIXSeconds;
                    _source.tv_nsec = 0;
                }
                // Initialize using NTP time
                Timestamp(const uint32_t seconds, const uint32_t fraction)
                {
                    // ASSERT(seconds >= NTPToUNIXSeconds);

                    _source.tv_sec = seconds - NTPToUNIXSeconds;
                    _source.tv_nsec = static_cast<uint32_t>((fraction / NanoSecondFraction) + 0.5);

                    Normalize();
                }
                Timestamp(const Timestamp& copy)
                    : _source(copy._source)
                {
                }
                // Initialize using UNIX epoch time
                Timestamp(const timespec& x)
                    : _source(x)
                {
                }
                // Initialize using the generic time methods
                Timestamp(const Core::Time& x)
                {
                    uint64_t ticks = x.Ticks();
                    _source.tv_sec = static_cast<uint32_t>(ticks / (1000 /* MilliSeconds */ * 1000 /* MicroSeconds */));
                    uint32_t microseconds = static_cast<uint32_t>(ticks % (1000 /* MilliSeconds */ * 1000 /* MicroSeconds */));
                    _source.tv_nsec = microseconds * 1000 /* microseconds */;
                }
                ~Timestamp()
                {
                }

                Timestamp& operator=(const Timestamp& rhs)
                {

                    _source = rhs._source;

                    return (*this);
                }

            public:
                uint32_t Seconds() const
                {
                    return (static_cast<uint32_t>(_source.tv_sec) + NTPToUNIXSeconds);
                }
                uint32_t Fraction() const
                {
                    return static_cast<uint32_t>((_source.tv_nsec * NanoSecondFraction) + 0.5);
                }

                operator Core::Time() const
                {
                    uint64_t ticks = _source.tv_sec * MicroSeconds;
                    ticks += _source.tv_nsec / (NanoSeconds / MicroSeconds);
                    return Core::Time(ticks);
                }
                inline double TimeSeconds() const
                {
                    return _source.tv_sec + static_cast<double>(_source.tv_nsec) / NanoSeconds;
                }

            private:
                inline void Normalize()
                {
                    if (_source.tv_nsec < 0)
                        do {
                            _source.tv_nsec += NanoSeconds;
                            _source.tv_sec--;
                        } while (_source.tv_nsec < 0);
                    else if (_source.tv_nsec >= static_cast<int32_t>(NanoSeconds))
                        do {
                            _source.tv_nsec -= NanoSeconds;
                            _source.tv_sec++;
                        } while (_source.tv_nsec >= static_cast<int32_t>(NanoSeconds));

                    /*
                        if (_source.tv_nsec < 0) {
                        _source.tv_sec -= ((NanoSeconds - _source.tv_nsec) % NanoSeconds);
                        _source.tv_nsec += (_source.tv_nsec / NanoSeconds) * NanoSeconds;
                        }
                        else {
                        _source.tv_sec += (_source.tv_nsec % NanoSeconds);
                        _source.tv_nsec -= (_source.tv_nsec / NanoSeconds) * NanoSeconds;
                        }
                        */
                }

                timespec _source;
            };

        private:
            NTPPacket(const NTPPacket&) = delete;
            NTPPacket& operator=(const NTPPacket&) = delete;

        public:
            NTPPacket()
                : leapVersionMode()
                , stratum()
                , poll()
                , precision()
                , rootDelay()
                , rootDispersion()
                , referenceID()
                , refTimestamp()
                , origTimestamp()
                , recvTimestamp()
                , xmitTimestamp()
            {
                static_assert(sizeof(uint8_t) == 1, "This only works if the size of the uint8_t is 1 byte");
                static_assert(sizeof(int8_t) == 1, "This only works if the size of the int8_t is 1 byte");
                static_assert(sizeof(uint32_t) == 4, "This only works if the size of the uint32_t is 4 bytes");
                static_assert(sizeof(int32_t) == 4, "This only works if the size of the int32_t is 4 bytes");
            }
            ~NTPPacket()
            {
            }

            static const uint16_t PacketSize = 48;

            uint8_t LeapIndicator() const { return (leapVersionMode & 0xC0) >> 6; }
            void LeapIndicator(const uint8_t value) { leapVersionMode = (leapVersionMode & 0x3F) | ((value & 0x03) << 6); }
            uint8_t NTPVersion() const { return (leapVersionMode & 0x38) >> 3; }
            void NTPVersion(const uint8_t value) { leapVersionMode = (leapVersionMode & 0xC7) | ((value & 0x07) << 3); }
            uint8_t NTPMode() const { return (leapVersionMode & 0x07) >> 0; }
            void NTPMode(const uint8_t value) { leapVersionMode = (leapVersionMode & 0xF8) | ((value & 0x07) << 0); }
            uint8_t Stratum() const { return stratum; }
            void Stratum(const uint8_t value) { stratum = value; }
            uint8_t Poll() const { return poll; }
            void Poll(const uint8_t value) { poll = value; }
            uint8_t Precision() const { return precision; }
            void Precision(const uint8_t value) { precision = value; }
            uint32_t RootDelay() const { return rootDelay; }
            void RootDelay(const uint32_t value) { rootDelay = value; }
            uint32_t RootDispersion() const { return rootDispersion; }
            void RootDispersion(uint32_t value) { rootDispersion = value; }
            string ReferenceID() const
            {
                string result;
                if (stratum <= 1) {
                    if (referenceID == 0) {
                        result = _T("zero");
                    } else {
                        char name[5];
                        memcpy(name, &referenceID, 4);
                        name[4] = '\0';
                        result = name;
                    }
                } else {
                    TCHAR buffer[20];
                    _stprintf(buffer, "%d.%d.%d.%d",
                        ((referenceID >> 24) & 0xFF),
                        ((referenceID >> 16) & 0xFF),
                        ((referenceID >> 8) & 0xFF),
                        (referenceID & 0xFF));
                    result = buffer;
                }
                return result;
            }
            void ReferenceID(const uint32_t value) { referenceID = value; }
            Timestamp ReferenceTimestamp() const { return refTimestamp; }
            void ReferenceTimestamp(Timestamp const& value) { refTimestamp = value; }
            Timestamp OriginalTimestamp() const { return origTimestamp; }
            void OriginalTimestamp(Timestamp const& value) { origTimestamp = value; }
            Timestamp ReceiveTimestamp() const { return recvTimestamp; }
            void ReceiveTimestamp(Timestamp const& value) { recvTimestamp = value; }
            Timestamp TransmitTimestamp() const { return xmitTimestamp; }
            void TransmitTimestamp(Timestamp const& value) { xmitTimestamp = value; }

            void Serialize(DataFrame::Writer& frame) const
            {
                frame.Number(leapVersionMode);
                frame.Number(stratum);
                frame.Number(poll);
                frame.Number(precision);
                frame.Number(rootDelay);
                frame.Number(rootDispersion);
                frame.Number(referenceID);
                Pack(frame, refTimestamp);
                Pack(frame, origTimestamp);
                Pack(frame, recvTimestamp);
                Pack(frame, xmitTimestamp);
            }

            void Deserialize(const DataFrame::Reader& frame)
            {
                leapVersionMode = frame.Number<uint8_t>();
                stratum = frame.Number<uint8_t>();
                poll = frame.Number<uint8_t>();
                precision = frame.Number<int8_t>();
                rootDelay = frame.Number<uint32_t>();
                rootDispersion = frame.Number<uint32_t>();
                referenceID = frame.Number<uint32_t>();
                Unpack(frame, refTimestamp);
                Unpack(frame, origTimestamp);
                Unpack(frame, recvTimestamp);
                Unpack(frame, xmitTimestamp);
            }

#ifdef __DEBUG__
            void DisplayPacket()
            {
                TRACE(Trace::Information, (_T("Leap indicator:      %d"), LeapIndicator()));
                TRACE(Trace::Information, (_T("NTP version:         %d"), NTPVersion()));
                TRACE(Trace::Information, (_T("NTP mode:            %d"), NTPMode()));
                TRACE(Trace::Information, (_T("Stratum:             %d"), Stratum()));
                TRACE(Trace::Information, (_T("Poll interval:       %d = %d s"), Poll(), 1 << Poll()));
                TRACE(Trace::Information, (_T("Precision:           %d = %lf s"), Precision(), 1.0 / (1 << -Precision())));
                const double Fraction_16_16 = 65536.0;
                uint32_t delay = RootDelay();
                TRACE(Trace::Information, (_T("Root delay:          %d = %lf s"), delay, delay / Fraction_16_16));
                uint32_t dispersion = RootDispersion();
                TRACE(Trace::Information, (_T("Root dispersion:     %d = %lf"), dispersion, dispersion / Fraction_16_16));
                TRACE(Trace::Information, (_T("Reference id:        %s"), ReferenceID().c_str()));
                TRACE(Trace::Information, (_T("Reference timestamp: %s"), Core::Time(ReferenceTimestamp()).ToRFC1123(false).c_str()));
                TRACE(Trace::Information, (_T("Origin timestamp:    %s"), Core::Time(OriginalTimestamp()).ToRFC1123(false).c_str()));
                TRACE(Trace::Information, (_T("Receive timestamp:   %s"), Core::Time(ReceiveTimestamp()).ToRFC1123(false).c_str()));
                TRACE(Trace::Information, (_T("Transmit timestamp:  %s"), Core::Time(TransmitTimestamp()).ToRFC1123(false).c_str()));
            }
#endif

        private:
            void Pack(DataFrame::Writer& frame, Timestamp const& data) const
            {
                frame.Number(data.Seconds());
                frame.Number(data.Fraction());
            }
            void Unpack(const DataFrame::Reader& frame, Timestamp& data)
            {
                uint32_t seconds = frame.Number<uint32_t>();
                data = Timestamp(seconds, frame.Number<uint32_t>());
            }

            uint8_t leapVersionMode; // LLVVVMMM for LL = Leap Indicator, VVV = NTP version, MMM = mode
                // LL: 0 = No warning, 1 = insert leap second, 2 = delete leap second, 3 = unsynchronized
                // VVV: 1, 2, 3, 4
                // MMM: 0 = reserved, 1 = symmetric active, 2 = symmetric passive, 3 = client
                //      4 = server, 5 = broadcast, 6 = NTP control, 7 = private use
            uint8_t stratum; // Stratum number (0 = invalid)
            uint8_t poll; // Log2 poll interval (seconds)
            int8_t precision; // Log2 precision (seconds)
            uint32_t rootDelay; // Delay fixed precision 16.16 bit
            uint32_t rootDispersion; // Dispersion fixed precision 16.16 bit
            uint32_t referenceID; // Well known reference for stratum 1, IP address (V4) for stratum > 1
            Timestamp refTimestamp; // Timestamp time last set, fixed precision 32.32 bit (NTP time)
            Timestamp origTimestamp; // Timestamp time send request, fixed precision 32.32 bit (NTP time)
            Timestamp recvTimestamp; // Timestamp time receive request, fixed precision 32.32 bit (NTP time)
            Timestamp xmitTimestamp; // Timestamp time transmit request / response, fixed precision 32.32
                // bit (NTP time)
        };

        class Activity : public Core::IDispatchType<void> {
        private:
            Activity() = delete;
            Activity(const Activity&) = delete;
            Activity& operator=(const Activity&) = delete;

        public:
            Activity(NTPClient* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Activity()
            {
            }

        public:
            virtual void Dispatch() override
            {
                _parent.Dispatch();
            }

        private:
            NTPClient& _parent;
        };

    private:
        NTPClient(const NTPClient&) = delete;
        NTPClient& operator=(const NTPClient&) = delete;

    public:
        NTPClient();
        virtual ~NTPClient();

    public:
        void Initialize(SourceIterator& sources, const uint16_t retries, const uint16_t delay);
        virtual void Register(Exchange::ITimeSync::INotification* notification) override;
        virtual void Unregister(Exchange::ITimeSync::INotification* notification) override;

        virtual uint32_t Synchronize() override;
        virtual void Cancel() override;
        virtual string Source() const override;
        virtual uint64_t SyncTime() const override;

        // ITime methods
        virtual uint64_t TimeSync() const override
        {
            return SyncTime();
        }

        BEGIN_INTERFACE_MAP(NTPClient)
        INTERFACE_ENTRY(Exchange::ITimeSync)
        INTERFACE_ENTRY(PluginHost::ISubSystem::ITime)
        END_INTERFACE_MAP

    private:
        // Implement Core::SocketDatagram
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override;

        // Signal a state change, Opened, Closed or Accepted
        virtual void StateChange() override;

        void Update();
        void Dispatch();
        bool FireRequest();

    private:
        Core::CriticalSection _adminLock;
        NTPPacket _packet;
        Core::Time _syncedTimestamp;
        state _state;
        bool _fired;
        uint32_t _WaitForNetwork;
        uint32_t _retryAttempts;
        uint32_t _currentAttempt;
        ServerList _servers;
        ServerIterator _serverIndex;
        Core::ProxyType<Core::IDispatchType<void>> _activity;
        std::list<Exchange::ITimeSync::INotification*> _clients;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // TIMESYNC_NTPCLIENT_H
