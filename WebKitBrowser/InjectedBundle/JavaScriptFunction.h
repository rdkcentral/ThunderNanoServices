#ifndef __JAVASCRIPTFUNCTION_H
#define __JAVASCRIPTFUNCTION_H

#include "Module.h"

#include <WPE/WebKit.h>

#include <memory>
#include <cassert>

namespace WPEFramework {
namespace JavaScript {

    class JavaScriptFunction {
    public:
        JSStaticFunction BuildJSStaticFunction() const;

        string GetName() const
        {
            return Name;
        }

        virtual ~JavaScriptFunction() {}

    protected:
        JavaScriptFunction(const string& name, const JSObjectCallAsFunctionCallback callback, bool shouldNotEnum = false);

    private:
        JavaScriptFunction() = delete;

    private:
        string Name;
        const JSObjectCallAsFunctionCallback Callback;
        bool ShouldNotEnum;

        // Temporary string for "char *" version of Name, required by JS API.
        std::string NameString;
    };
} // namespace JavaScript
} // namespace WPEFramework

#endif // __JAVASCRIPTFUNCTION_H
