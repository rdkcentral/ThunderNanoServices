#ifndef __JAVASCRIPTFUNCTIONTYPE_H
#define __JAVASCRIPTFUNCTIONTYPE_H

#include "JavaScriptFunction.h"
#include "ClassDefinition.h"

namespace WPEFramework {

namespace JavaScript {

    // Wrapper for JavaScript handler, takes care of messy function pointer details.
    template <typename ActualJavaScriptFunction>
    class JavaScriptFunctionType : public JavaScriptFunction {
    public:
        // Constructor, also registers to ClassDefinition.
        JavaScriptFunctionType(const string& jsClassName, bool shouldNotEnum = false)
            : JavaScriptFunction(Core::ClassNameOnly(typeid(ActualJavaScriptFunction).name()).Data(), function, shouldNotEnum)
            , JsClassName(jsClassName)
        {
            ClassDefinition::Instance(JsClassName).Add(this);
        }

        // Destructor, also unregisters function.
        ~JavaScriptFunctionType()
        {
            ClassDefinition::Instance(JsClassName).Remove(this);
        }

    private:
        // Callback function.
        static JSValueRef function(JSContextRef context, JSObjectRef function,
            JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
        {
            return Handler.HandleMessage(context, function, thisObject, argumentCount, arguments, exception);
        }

        static ActualJavaScriptFunction Handler;
        string JsClassName;
    };

    template <typename ActualJavaScriptFunction>
    ActualJavaScriptFunction JavaScriptFunctionType<ActualJavaScriptFunction>::Handler;
}
}

#endif // __JAVASCRIPTFUNCTIONTYPE_H
