#ifndef __INJECTEDBUNDLE_UTILS_H
#define __INJECTEDBUNDLE_UTILS_H

#include "Module.h"

#include <WPE/WebKit.h>

#include <vector>
#include <memory>

namespace WPEFramework {
namespace WebKit {
    namespace Utils {
        void AppendStringToWKArray(const string& item, WKMutableArrayRef array);
        string GetStringFromWKArray(WKArrayRef array, unsigned int index);
        string WKStringToString(WKStringRef wkStringRef);
        std::vector<string> ConvertWKArrayToStringVector(WKArrayRef array);
    };
}
}

#endif // __INJECTEDBUNDLE_UTILS_H
