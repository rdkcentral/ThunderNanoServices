#pragma once

namespace Thunder {
namespace CEC {
    class Processor {
    public:
        Processor() = delete;
        Processor(const Processor& a_Copy) = delete;
        Processor& operator=(const Processor& a_RHS) = delete;

    public:
        Processor(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Processor(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Processor() = default;

    public:
        inline const char* Data() const { return (_text.c_str()); }
        inline uint16_t Length() const { return (static_cast<uint16_t>(_text.length())); }

    private:
        std::string _text;
    };
} // namespace TestCore
} // namespace Thunder
