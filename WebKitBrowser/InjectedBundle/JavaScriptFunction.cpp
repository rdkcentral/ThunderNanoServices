#include "JavaScriptFunction.h"

#include <iostream>

#include "ClassDefinition.h"

namespace WPEFramework {
namespace JavaScript {

    JavaScriptFunction::JavaScriptFunction(const string& name, const JSObjectCallAsFunctionCallback callback,
        bool shouldNotEnum /* = false */)
        : Name(name)
        , Callback(callback)
        , ShouldNotEnum(shouldNotEnum)
    {
        NameString = Core::ToString(Name.c_str());
    }

    // Fills JSStaticFunction struct for this function. For now the name field is valid as long as this
    // instance exists, this might change in the future.
    JSStaticFunction JavaScriptFunction::BuildJSStaticFunction() const
    {
        JSStaticFunction staticFunction;

        // @Zan: How long does "staticFunction.name" need to be valid?
        staticFunction.name = NameString.c_str();
        staticFunction.callAsFunction = Callback;

        // @Zan: assumption is that functions should always be read only and can't be deleted, is this true?
        staticFunction.attributes = kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete;

        // @Zan: does it make sense to make a JS function unenumerable?
        if (ShouldNotEnum)
            staticFunction.attributes |= kJSPropertyAttributeDontEnum;

        return staticFunction;
    }
} // namespace JavaScript
} // namespace WPEFramework
