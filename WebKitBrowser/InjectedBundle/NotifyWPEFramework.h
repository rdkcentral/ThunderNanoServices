#ifndef __NOTIFYH
#define __NOTIFYH

#include "JavaScriptFunctionType.h"

namespace WPEFramework {
namespace JavaScript {
    namespace Functions {

        class NotifyWPEFramework {
        public:
            NotifyWPEFramework();

            JSValueRef HandleMessage(JSContextRef context, JSObjectRef,
                JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*);

            static inline string GetMessageName()
            {
                return Core::ClassNameOnly(typeid(NotifyWPEFramework).name()).Data();
            }
        };
    }
}
}

#endif // __NOTIFYH
