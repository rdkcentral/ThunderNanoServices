#include "Milestone.h"

#include "Utils.h"

#include <iostream>
#include <sstream>

// Global handle to this bundle.
extern WKBundleRef g_Bundle;

namespace WPEFramework {

namespace JavaScript {

    namespace Functions {

        Milestone::Milestone()
        {
        }

        JSValueRef Milestone::HandleMessage(JSContextRef context, JSObjectRef,
            JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*)
        {
            const int acceptedArgCount = 3;
            const size_t bufferSize = 1500; // Is limited by UDP package size.
            
            if (argumentCount != acceptedArgCount) {
                std::cerr << "Milestone only accepts 3 string arguments" << std::endl;
                return JSValueMakeNull(context);
            }

            string argStrings[acceptedArgCount];

            for (unsigned int index = 0; index < argumentCount; index++) {
                const JSValueRef& argument = arguments[index];

                if (!JSValueIsString(context, argument)) {
                    std::cerr << "Milestone function only accepts string arguments" << std::endl;
                    return JSValueMakeNull(context);
                }

                JSStringRef jsString = JSValueToStringCopy(context, argument, nullptr);
                char stringBuffer[bufferSize];
                
                // TODO: for now assumption is ASCII, should we also deal with Unicode?
                JSStringGetUTF8CString(jsString, stringBuffer, bufferSize);
                JSStringRelease(jsString);
                
                argStrings[index] = stringBuffer;
            }
            
            std::stringstream ssMessage;
            ssMessage << "TEST TRACE:";
            for (const string& argString : argStrings) {
               ssMessage << " \"" << argString << "\"";
            }
            
            std::cerr << ssMessage.str() << std::endl;
                      
            TRACE_GLOBAL(Trace::Information, (ssMessage.str()));           
            
            return JSValueMakeNull(context);
        }

        static JavaScriptFunctionType<Milestone> _instance(_T("automation"));
    }
}
}

