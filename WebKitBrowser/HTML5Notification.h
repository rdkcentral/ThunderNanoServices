#ifndef __HTML5NOTIFICATION_H
#define __HTML5NOTIFICATION_H

#include <tracing/tracing.h>

using namespace WPEFramework;

class HTML5Notification {
private:
    HTML5Notification() = delete;
    HTML5Notification(const HTML5Notification& a_Copy) = delete;
    HTML5Notification& operator=(const HTML5Notification& a_RHS) = delete;

public:
    HTML5Notification (const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        Trace::Format(_text, formatter, ap);
        va_end(ap);
    }
    explicit HTML5Notification(const string& text) : _text(Core::ToString(text)) 
    {
    }
    ~HTML5Notification()
    {
    }
public:
    inline const char* Data() const {
        return (_text.c_str());
    }
    inline uint16_t Length() const {
        return (static_cast<uint16_t>(_text.length()));
    }

private:
    std::string _text;
};

#endif // __HTML5NOTIFICATION_H
