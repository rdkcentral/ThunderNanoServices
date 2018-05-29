#include "NotifyWPEFramework.h"

#include "Utils.h"

// Global handle to this bundle.
extern WKBundleRef g_Bundle;

namespace WPEFramework {

namespace JavaScript {

    namespace Functions {

        NotifyWPEFramework::NotifyWPEFramework()
        {
        }

        // Implementation of JS function: loops over arguments and passes all strings to WPEFramework.
        JSValueRef NotifyWPEFramework::HandleMessage(JSContextRef context, JSObjectRef,
            JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*)
        {
            WKStringRef messageName = WKStringCreateWithUTF8CString(GetMessageName().c_str());

            // Build message body.
            WKMutableArrayRef messageBody = WKMutableArrayCreate();
            for (unsigned int index = 0; index < argumentCount; index++) {
                const JSValueRef& argument = arguments[index];

                // For now only pass along strings.
                if (!JSValueIsString(context, argument))
                    continue;

                JSStringRef jsString = JSValueToStringCopy(context, argument, nullptr);

                WKStringRef messageLine = WKStringCreateWithJSString(jsString);
                WKArrayAppendItem(messageBody, messageLine);

                WKRelease(messageLine);
                JSStringRelease(jsString);
            }

            WKBundlePostSynchronousMessage(g_Bundle, messageName, messageBody, nullptr);

            WKRelease(messageBody);
            WKRelease(messageName);

            return JSValueMakeNull(context);
        }

        static JavaScriptFunctionType<NotifyWPEFramework> _instance(_T("wpe"));
    }
}
}
