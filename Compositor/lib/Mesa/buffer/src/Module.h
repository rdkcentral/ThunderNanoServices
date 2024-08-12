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
#define MODULE_NAME CompositorBuffer
#endif

#include <core/core.h>
#include <messaging/messaging.h>
#include <CompositorTypes.h>
#include <IBuffer.h>

#include <interfaces/ICompositionBuffer.h>

#if HAVE_GBM_MODIFIERS
#ifndef GBM_MAX_PLANES
#define GBM_MAX_PLANES 4
#endif
#endif

namespace Thunder {
namespace Trace {

class Buffer {
public:
    Buffer() = delete;
    Buffer(Buffer&&) = delete;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        Thunder::Trace::Format(_text, formatter, ap);
        va_end(ap);
    }
    explicit Buffer(const string& text)
        : _text(Thunder::Core::ToString(text))
    {
    }
    ~Buffer() = default;

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
}; // class Buffer

} // namespace Trace
} // namespace Thunder
