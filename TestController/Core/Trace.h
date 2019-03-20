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
