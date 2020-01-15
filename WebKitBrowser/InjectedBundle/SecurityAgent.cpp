#include "JavaScriptFunctionType.h"
#include "Utils.h"
#include "Tags.h"
#include <securityagent/SecurityToken.h>

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
                    uint8_t buffer[16 * 1024];

                    WKTypeRef returnData; 
                    WKStringRef messageName = WKStringCreateWithUTF8CString(Tags::URL);

                    // WKMutableArrayRef messageBody = WKMutableArrayCreate();
                    WKBundlePostSynchronousMessage(WebKit::Utils::GetBundle(), messageName, nullptr /* messageBody */, &returnData);
                    std::string url (WebKit::Utils::WKStringToString(static_cast<WKStringRef>(returnData)));
                    std::string tokenAsString;
                    if (url.length() < sizeof(buffer)) {
                        ::memcpy (buffer, url.c_str(), url.length());

                        int length = GetToken(static_cast<uint16_t>(sizeof(buffer)), url.length(), buffer);

                        if (length > 0) {
                           Core::ToString(buffer, static_cast<uint16_t>(length), false, tokenAsString);
                        }
                    }

                    JSStringRef returnMessage = JSStringCreateWithUTF8CString(tokenAsString.c_str());
                    result = JSValueMakeString(context, returnMessage);

                    JSStringRelease(returnMessage);
                    WKRelease(returnData); 
                    WKRelease(messageName);
                }

                return (result);
            }
        };

        static JavaScriptFunctionType<Security> _instance(_T("token"));
    }
}
}


