
#include <interfaces/json/JsonData_TestController.h>
#include "TestController.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::TestController;

    // Registration
    //

    void TestController::RegisterAll()
    {
        Register<void,Core::JSON::ArrayType<Core::JSON::String>>(_T("categories"), &TestController::endpoint_categories, this);
        Register<TestsParamsData,Core::JSON::ArrayType<Core::JSON::String>>(_T("tests"), &TestController::endpoint_tests, this);
        Register<DescriptionParamsInfo,DescriptionResultData>(_T("description"), &TestController::endpoint_description, this);
        Register<DescriptionParamsInfo,Core::JSON::ArrayType<RunResultData>>(_T("run"), &TestController::endpoint_run, this);
    }

    void TestController::UnregisterAll()
    {
        Unregister(_T("run"));
        Unregister(_T("description"));
        Unregister(_T("tests"));
        Unregister(_T("categories"));
    }

    // API implementation
    //

    // Retrieves the list of test categories.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t TestController::endpoint_categories(Core::JSON::ArrayType<Core::JSON::String>& response)
    {
        auto categories = _testControllerImp->Categories();
        ASSERT(categories != nullptr);

        // Get list of Categories
        response = TestCategories(categories);

        return (Core::ERROR_NONE);
    }

    // Retrieves the list of test for selected category.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unknown category
    //  - ERROR_BAD_REQUEST: Bad JSON param data format
    uint32_t TestController::endpoint_tests(const TestsParamsData& params, Core::JSON::ArrayType<Core::JSON::String>& response)
    {
        uint32_t result = Core::ERROR_NONE;

        if (params.Category.IsSet() == true) {
            auto category = _testControllerImp->Category(params.Category.Value());
           if (category != nullptr)
           {
               // Get list of Categories
               response = Tests(category->Tests());
           } else {
               result = Core::ERROR_UNAVAILABLE;
           }
        } else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return result;
    }

    // Retrieves the description of the selected test from particular category.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unknown category/test
    //  - ERROR_BAD_REQUEST: Bad JSON param data format
    uint32_t TestController::endpoint_description(const DescriptionParamsInfo& params, DescriptionResultData& response)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& category = params.Category.Value();
        const string& test = params.Test.Value();

        TRACE_L1("*** Call endpoint_description ****");

        // response.Description = ...

        return result;
    }

    // Run all tests - no arguments; <br /> A selected category group of test - specify category;<br /> A selected test - specify category and test name;<br />.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unknown category/test
    //  - ERROR_BAD_REQUEST: Bad json param data format
    uint32_t TestController::endpoint_run(const DescriptionParamsInfo& params, Core::JSON::ArrayType<RunResultData>& response)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& category = params.Category.Value();
        const string& test = params.Test.Value();

        TRACE_L1("*** Call endpoint_run ****");

        // response.Test = ...
        // response.Status = ...

        return result;
    }

} // namespace Plugin

}

