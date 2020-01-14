#include "JavaScriptFunctionType.h"
#include "Utils.h"
#include "Tags.h"

namespace WPEFramework {
namespace JavaScript {
    namespace Functions {
        class Security {
        public:
            Security(const Security&) = delete;
            Security& operator= (const Security&) = delete;
            Security() {
            }
            ~Security() {
            }

            JSValueRef HandleMessage(JSContextRef context, JSObjectRef,
                JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*) {
                JSValueRef result;

                if (argumentCount != 0) {
                    TRACE_GLOBAL(Trace::Information, (_T("The Token Javascript command, does not take any paramaters!!!")));
                    result = JSValueMakeNull(context);
                }
                else {
                    WKTypeRef returnData; 
                    WKStringRef messageName = WKStringCreateWithUTF8CString(string(Tags::Token));

                    // WKMutableArrayRef messageBody = WKMutableArrayCreate();
                    WKBundlePostSynchronousMessage(WebKit::Utils::GetBundle(), messageName, nullptr /* messageBody */, &returnData);

                    result = JSValueMakeString(context, static_cast<WKStringRef>(returnData));

                    WKRelease(returnData); 
                    WKRelease(messageBody);
                    WKRelease(messageName);
                }

                return (result);
            }
        };

    }

    static JavaScriptFunctionType<Security> _instance(_T("token"));
}
}


