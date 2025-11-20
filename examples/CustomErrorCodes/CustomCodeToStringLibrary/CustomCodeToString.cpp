
#if defined WIN32 || defined _WINDOWS
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#include <tchar.h>
#define VARIABLE_IS_NOT_USED

#else

#ifdef _UNICODE
#define _T(x) L##x
#endif

#ifndef _UNICODE
#define _T(x) x
#endif

#endif // __LINUX__

#include <core/ICustomErrorCode.h>

const TCHAR* CustomCodeToString(const int32_t code) {
    const TCHAR* text = nullptr;
    if (code == 100) {
        text = _T("Custom Error code 100");
    } else if (code == -100) {
        text = _T("Custom Error code -100");
    }

    return text;
}

