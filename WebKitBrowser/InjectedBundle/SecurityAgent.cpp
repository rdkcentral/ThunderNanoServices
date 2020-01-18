#include "JavaScriptFunctionType.h"
#include "Utils.h"
#include "Tags.h"
#include <securityagent/SecurityToken.h>

namespace WPEFramework {
namespace JavaScript {
    namespace Functions {
        class token {
        public:
            token(const token&) = delete;
            token& operator= (const token&) = delete;
            token() {
            }
            ~token() {
            }

            JSValueRef HandleMessage(JSContextRef context, JSObjectRef,
                JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*) {
                JSValueRef result;

                if (argumentCount != 0) {
                    TRACE_GLOBAL(Trace::Information, (_T("The Token Javascript command, does not take any paramaters!!!")));
                    result = JSValueMakeNull(context);
                }
                else {
                    uint8_t buffer[2 * 1024];

                    std::string url = WebKit::Utils::GetURL();

                    std::string tokenAsString;
                    if (url.length() < sizeof(buffer)) {
                        ::memcpy (buffer, url.c_str(), url.length());

                        int length = GetToken(static_cast<uint16_t>(sizeof(buffer)), url.length(), buffer);
                        if (length > 0) {
                           tokenAsString = std::string(reinterpret_cast<const char*>(buffer), length);
                        }
                    }

                    JSStringRef returnMessage = JSStringCreateWithUTF8CString(tokenAsString.c_str());
                    result = JSValueMakeString(context, returnMessage);

                    JSStringRelease(returnMessage);
                }

                return (result);
            }
        };

        static JavaScriptFunctionType<token> _instance(_T("thunder"));
    }
}
}

