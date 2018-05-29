#include <memory.h>
#include "ClassDefinition.h"

namespace WPEFramework {
namespace JavaScript {


    // Constructor: uses identifier to build class and extension name.
    ClassDefinition::ClassDefinition(const string& identifier) 
        : _customFunctions()
        , _className(Core::ToString(identifier))
        , _extName (_className) {
        // Make upper case.
        transform(_className.begin(), _className.end(), _className.begin(), ::toupper);

        // Make lower case.
        transform(_extName.begin(), _extName.end(), _extName.begin(), ::tolower);
    }
    /* static */ ClassDefinition::ClassMap& ClassDefinition::getClassMap() {
        static ClassDefinition::ClassMap singleton;
        return singleton;
    }


    /* static */ ClassDefinition& ClassDefinition::Instance(const string& className) {
        ClassDefinition::ClassMap& _classes = getClassMap();
        ClassDefinition* result = nullptr;
        ClassMap::iterator index (_classes.find(className));

        if (index != _classes.end()) {
            result = &(index->second);
        }
        else {
            TRACE_L1("Before Classdefinition ingest: %s", className.c_str());
            std::pair<ClassMap::iterator, bool> entry (_classes.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(className),
                std::forward_as_tuple(className)));
            TRACE_L1("After Classdefinition ingest - 3: %s", className.c_str());

            result = &(entry.first->second);
        }

        return (*result); 
    }

    // Adds JS function to class.
    void ClassDefinition::Add(const JavaScriptFunction* javaScriptFunction)
    {
        ASSERT(std::find(_customFunctions.begin(), _customFunctions.end(), javaScriptFunction) == _customFunctions.end());

        _customFunctions.push_back(javaScriptFunction);
    }

    // Removes JS function from class.
    void ClassDefinition::Remove(const JavaScriptFunction* javaScriptFunction)
    {
        // Try to find function in class.
        FunctionVector::iterator index (find(_customFunctions.begin(), _customFunctions.end(), javaScriptFunction));

        ASSERT(index != _customFunctions.end());

        if (index != _customFunctions.end()) {
            // Remove function from function vector.
            _customFunctions.erase(index);
        }
    }
}
}
