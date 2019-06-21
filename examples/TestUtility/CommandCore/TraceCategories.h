#pragma once

namespace WPEFramework {
namespace TestCore {

    class TestLifeCycle {
    public:
        TestLifeCycle() = delete;
        TestLifeCycle(const TestLifeCycle& a_Copy) = delete;
        TestLifeCycle& operator=(const TestLifeCycle& a_RHS) = delete;

    public:
        TestLifeCycle(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit TestLifeCycle(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~TestLifeCycle() = default;

    public:
        inline const char* Data() const { return (_text.c_str()); }
        inline uint16_t Length() const { return (static_cast<uint16_t>(_text.length())); }

    private:
        std::string _text;
    };

    class TestOutput {
    public:
        TestOutput() = delete;
        TestOutput(const TestOutput& a_Copy) = delete;
        TestOutput& operator=(const TestOutput& a_RHS) = delete;

    public:
        TestOutput(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit TestOutput(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~TestOutput() = default;

    public:
        inline const char* Data() const { return (_text.c_str()); }
        inline uint16_t Length() const { return (static_cast<uint16_t>(_text.length())); }

    private:
        std::string _text;
    };

} // namespace TestCore
} // namespace WPEFramework
