#ifndef __NOTIFYH
#define __NOTIFYH

#include "JavaScriptFunctionType.h"
#include "Tags.h"

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
                return Tags::Notification;
            }
        };
    }
}
}

#endif // __NOTIFYH
