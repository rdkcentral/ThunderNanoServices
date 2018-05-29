#ifndef __TESTTRACE_H
#define __TESTTRACE_H

#include "JavaScriptFunctionType.h"

namespace WPEFramework {
namespace JavaScript {
    namespace Functions {
        class Milestone {
        public:
            Milestone();

            JSValueRef HandleMessage(JSContextRef context, JSObjectRef,
                JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*);
        };
    }
}
}

#endif // __TESTTRACE_H
