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

#ifndef MODULE_NAME
#define MODULE_NAME CompositorBackend
#endif

#include <core/core.h>
#include <messaging/messaging.h>
#include <CompositorTypes.h>
#include <IBuffer.h>

#if HAVE_GBM_MODIFIERS
#ifndef GBM_MAX_PLANES
#define GBM_MAX_PLANES 4
#endif
#endif

namespace Thunder {

namespace Trace {

    class Backend {
    public:
        Backend() = delete;
        Backend(Backend&&) = delete;
        Backend(const Backend&) = delete;
        Backend& operator=(const Backend&) = delete;

        Backend(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Backend(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Backend() = default;

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
    }; // class Backend

    class Frame {
    public:
        Frame() = delete;
        Frame(Frame&&) = delete;
        Frame(const Frame&) = delete;
        Frame& operator=(const Frame&) = delete;

        Frame(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Frame(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Frame() = default;

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
    }; // class Frame

} // namespace Trace

} // namespace Thunder
