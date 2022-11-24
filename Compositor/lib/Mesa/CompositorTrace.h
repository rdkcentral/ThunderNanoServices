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

namespace CompositorTrace {
class GL {
public:
    ~GL() = default;
    GL() = delete;
    GL(const GL&) = delete;
    GL& operator=(const GL&) = delete;
    GL(const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        WPEFramework::Trace::Format(_text, formatter, ap);
        va_end(ap);
    }
    explicit GL(const string& text)
        : _text(WPEFramework::Core::ToString(text))
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
}; // class GL

class Render {
public:
    ~Render() = default;
    Render() = delete;
    Render(const Render&) = delete;
    Render& operator=(const Render&) = delete;
    Render(const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        WPEFramework::Trace::Format(_text, formatter, ap);
        va_end(ap);
    }
    explicit Render(const string& text)
        : _text(WPEFramework::Core::ToString(text))
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
}; // class Render

class Client {
public:
    ~Client() = default;
    Client() = delete;
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        WPEFramework::Trace::Format(_text, formatter, ap);
        va_end(ap);
    }
    explicit Client(const string& text)
        : _text(WPEFramework::Core::ToString(text))
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
}; // class Client

class Context {
public:
    ~Context() = default;
    Context() = delete;
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        WPEFramework::Trace::Format(_text, formatter, ap);
        va_end(ap);
    }
    explicit Context(const string& text)
        : _text(WPEFramework::Core::ToString(text))
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
}; // class Context

class ContextTracer {
private:
    static constexpr char IndentationChar = ' ';

public:
    static uint16_t IndentDepth;

    ContextTracer() = delete;
    ContextTracer(const ContextTracer&) = delete;
    ContextTracer& operator=(const ContextTracer&) = delete;

    ContextTracer(const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        WPEFramework::Trace::Format(_text, formatter, ap);
        va_end(ap);
    }

    ContextTracer(const std::string& text)
        : _text(text)
    {
        std::cout << std::string(IndentDepth, IndentationChar) << (" >> Enter: "), << _text << std::endl));

        ++IndentDepth;
    }
    ~ContextTracer()
    {
        --IndentDepth;
        std::cout << std::string(IndentDepth, IndentationChar) << (" << Exit: "), << _text << std::endl));
        std::fflush
    }

private:
    std::string _text;
}; // class ContextTracer

uint16_t ContextTracer::IndentDepth = 0;

} // namespace CompositorTrace
