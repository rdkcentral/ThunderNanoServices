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

namespace WPEFramework {
namespace TestCore {

    class TestStart {
    public:
        TestStart() = delete;
        TestStart(const TestStart& a_Copy) = delete;
        TestStart& operator=(const TestStart& a_RHS) = delete;

        TestStart(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit TestStart(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~TestStart() = default;

    public:
        inline const char* Data() const { return (_text.c_str()); }
        inline uint16_t Length() const { return (static_cast<uint16_t>(_text.length())); }

    private:
        std::string _text;
    };

    class TestEnd {
    private:
        TestEnd() = delete;
        TestEnd(const TestEnd& a_Copy) = delete;
        TestEnd& operator=(const TestEnd& a_RHS) = delete;

    public:
        TestEnd(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit TestEnd(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~TestEnd() = default;

    public:
        inline const char* Data() const { return (_text.c_str()); }
        inline uint16_t Length() const { return (static_cast<uint16_t>(_text.length())); }

    private:
        std::string _text;
    };

    class TestStep {
    private:
        TestStep() = delete;
        TestStep(const TestStep& a_Copy) = delete;
        TestStep& operator=(const TestStep& a_RHS) = delete;

    public:
        TestStep(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit TestStep(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~TestStep() = default;

    public:
        inline const char* Data() const { return (_text.c_str()); }
        inline uint16_t Length() const { return (static_cast<uint16_t>(_text.length())); }

    private:
        std::string _text;
    };

} // namespace TestCore
} // namespace WPEFramework
