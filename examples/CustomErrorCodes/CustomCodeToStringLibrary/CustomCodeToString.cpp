
#if defined WIN32 || defined _WINDOWS
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#include <tchar.h>
#define VARIABLE_IS_NOT_USED

#else

#ifdef _UNICODE
#define _T(x) L##x
#define TCHAR wchar_t
#endif

#ifndef _UNICODE
#define _T(x) x
#define TCHAR char
#endif

#endif // __LINUX__

#include <core\ICustomErrorCode.h>

const TCHAR* CustomCodeToSting(int32_t code) {
    const TCHAR* text = nullptr;
    if (code == 100) {
        text = _T("Custom Error code 100");
    } else if (code == -100) {
        text = _T("Custom Error code -100");
    }

    return text;
}

