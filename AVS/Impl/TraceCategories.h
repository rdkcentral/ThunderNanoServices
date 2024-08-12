 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

namespace Thunder {
namespace Plugin {

    /**
     * Trace category for logs coming directly from the AVS SDK
     */
    class AVSSDK {
    public:
        AVSSDK() = delete;
        AVSSDK(const AVSSDK& a_Copy) = delete;
        AVSSDK& operator=(const AVSSDK& a_RHS) = delete;
        ~AVSSDK() = default;

        explicit AVSSDK(const string& text)
            : _text(Core::ToString(text))
        {
        }

        AVSSDK(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }

        inline const char* Data() const
        {
            return (_text.c_str());
        }

        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };

    /**
     * Trace category for logs coming from the AVS plugin and implementation
    */
    class AVSClient {
    public:
        AVSClient() = delete;
        AVSClient(const AVSClient& a_Copy) = delete;
        AVSClient& operator=(const AVSClient& a_RHS) = delete;
        ~AVSClient() = default;

        explicit AVSClient(const string& text)
            : _text(Core::ToString(text))
        {
        }

        AVSClient(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }

        inline const char* Data() const
        {
            return (_text.c_str());
        }

        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };
}
}
