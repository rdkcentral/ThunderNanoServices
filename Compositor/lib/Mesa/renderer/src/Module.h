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
#define MODULE_NAME CompositorRenderer
#endif

#include <core/core.h>
#include <messaging/messaging.h>

#include <IRenderer.h>
#include <Transformation.h>
#include <interfaces/ICompositionBuffer.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>

namespace Thunder {
namespace Trace {

class Renderer {
public:
    ~Renderer() = default;
    Renderer() = delete;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        Thunder::Trace::Format(_text, formatter, ap);
        va_end(ap);
    }
    explicit Renderer(const string& text)
        : _text(Thunder::Core::ToString(text))
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
}; // class Renderer

class EGL {
public:
    ~EGL() = default;
    EGL() = delete;
    EGL(const EGL&) = delete;
    EGL& operator=(const EGL&) = delete;
    EGL(const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        Thunder::Core::Format(_text, formatter, ap);
        va_end(ap);
    }
    explicit EGL(const string& text)
        : _text(Thunder::Core::ToString(text))
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
}; // class EGL

class GL {
public:
    ~GL() = default;
    GL() = delete;
    GL(const GL&) = delete;
    GL& operator=(const GL&) = delete;
    GL(const TCHAR formatter[], ...) {
        va_list ap;
        va_start(ap, formatter);
        Thunder::Core::Format(_text, formatter, ap);
        va_end(ap);
    }
    explicit GL(const string& text)
        : _text(Thunder::Core::ToString(text)) {
    }

public:
    const char* Data() const {
        return (_text.c_str());
    }
    uint16_t Length() const {
        return (static_cast<uint16_t>(_text.length()));
    }

private:
    std::string _text;
}; // class GL

} // namespace Trace
} // namespace Thunder
