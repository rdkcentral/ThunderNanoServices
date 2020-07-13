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

namespace WPEFramework {
namespace Data {
    struct Window {
        uint32_t X;
        uint32_t Y;
        uint32_t Width;
        uint32_t Height;
    };

    class Geometry : public Core::JSON::Container {
    public:
        Geometry()
            : Core::JSON::Container()
            , X(0)
            , Y(0)
            , Width(~0)
            , Height(~0)
        {
            Add(_T("x"), &X);
            Add(_T("y"), &Y);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);
        }
        Geometry(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height)
            : Core::JSON::Container()
            , X(0)
            , Y(0)
            , Width(~0)
            , Height(~0)
        {
            Add(_T("x"), &X);
            Add(_T("y"), &Y);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);

            X = x;
            Y = y;
            Width = width;
            Height = height;
        }
		Geometry(const Geometry& copy) 
			: Core::JSON::Container() 
            , X(copy.X)
            , Y(copy.Y)
            , Width(copy.Width)
            , Height(copy.Height)
        {
            Add(_T("x"), &X);
            Add(_T("y"), &Y);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);
        }
        ~Geometry()
        {
        }

		Geometry& operator=(const Geometry& rhs) {
            X = rhs.X;
            Y = rhs.Y;
            Width = rhs.Width;
            Height = rhs.Height;
            return (*this);
		}

    public:
        Core::JSON::DecUInt32 X;
        Core::JSON::DecUInt32 Y;
        Core::JSON::DecUInt32 Width;
        Core::JSON::DecUInt32 Height;
    };
    class Time : public Core::JSON::Container {
    private:
        Time(const Time&) = delete;
        Time& operator=(const Time&) = delete;

    public:
        Time()
            : Core::JSON::Container()
            , Hours(0)
            , Minutes(0)
            , Seconds(0)
        {
            Add(_T("hours"), &Hours);
            Add(_T("minutes"), &Minutes);
            Add(_T("seconds"), &Seconds);
        }
        Time(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
            : Core::JSON::Container()
            , Hours(0)
            , Minutes(0)
            , Seconds(0)
        {
            Add(_T("hours"), &Hours);
            Add(_T("minutes"), &Minutes);
            Add(_T("seconds"), &Seconds);
            Hours = hours;
            Minutes = minutes;
            Seconds = seconds;
        }
        virtual ~Time()
        {
        }

    public:
        Core::JSON::DecUInt8 Hours;
        Core::JSON::DecUInt8 Minutes;
        Core::JSON::DecUInt8 Seconds;
    };
    class Parameters : public Core::JSON::Container {
    private:
        Parameters(const Parameters&) = delete;
        Parameters& operator=(const Parameters&) = delete;

    public:
        Parameters()
            : Core::JSON::Container()
            , Location(_T("DefaultValue"))
            , UTC(false)
        {
            Add(_T("location"), &Location);
            Add(_T("utc"), &UTC);
        }
        Parameters(const string& location, const bool utc)
            : Core::JSON::Container()
            , Location(_T("DefaultValue"))
            , UTC(false)
        {
            Add(_T("location"), &Location);
            Add(_T("utc"), &UTC);
            Location = location;
            UTC = utc;
        }
        virtual ~Parameters()
        {
        }

    public:
        Core::JSON::String Location;
        Core::JSON::Boolean UTC;
    };
    class MessageParameters : public Core::JSON::Container {
    private:
        MessageParameters(const MessageParameters&) = delete;
        MessageParameters& operator=(const MessageParameters&) = delete;

    public:
        MessageParameters()
            : Core::JSON::Container()
            , Recipient()
            , Message(false)
        {
            Add(_T("recipient"), &Recipient);
            Add(_T("message"), &Message);
        }
        MessageParameters(const string& recipient, const string& message)
            : Core::JSON::Container()
            , Recipient()
            , Message()
        {
            Add(_T("recipient"), &Recipient);
            Add(_T("message"), &Message);
            Recipient = recipient;
            Message = message;
        }
        virtual ~MessageParameters()
        {
        }

    public:
        Core::JSON::String Recipient;
        Core::JSON::String Message;
    };
    class Response : public Core::JSON::Container {
    private:
        Response(const Response&) = delete;
        Response& operator=(const Response&) = delete;

    public:
        enum state {
            ACTIVE,
            INACTIVE,
            IDLE,
            FAILURE
        };

    public:
        Response()
            : Core::JSON::Container()
            , State(FAILURE)
            , Time(0)
        {
            Add(_T("state"), &State);
            Add(_T("time"), &Time);
        }
        Response(const uint64_t time, const state value)
            : Core::JSON::Container()
            , State(FAILURE)
            , Time(0)
        {
            Add(_T("state"), &State);
            Add(_T("time"), &Time);

			Time = time;
            State = value;
        }
        virtual ~Response()
        {
        }

    public:
        Core::JSON::EnumType<state> State;
        Core::JSON::DecUInt64 Time;
    };

    // The next class describes configuration information for this plugin.
    class JSONDataBuffer : public Core::JSON::Container {
    private:
        JSONDataBuffer(const JSONDataBuffer&) = delete;
        JSONDataBuffer& operator=(const JSONDataBuffer&) = delete;

    public:
        JSONDataBuffer()
            : Core::JSON::Container()
            , Data()
            , Length(0)
            , Duration(0)
        {
            Add(_T("data"), &Data);
            Add(_T("length"), &Length);
            Add(_T("duration"), &Duration);
        }
        ~JSONDataBuffer()
        {
        }

    public:
        Core::JSON::String Data;
        Core::JSON::DecUInt16 Length;
        Core::JSON::DecUInt32 Duration;
    };
}
}
