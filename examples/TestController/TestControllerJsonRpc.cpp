/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
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
        Register<RunParamsData,Core::JSON::ArrayType<RunResultData>>(_T("run"), &TestController::endpoint_run, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("categories"), &TestController::get_categories, nullptr, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("tests"), &TestController::get_tests, nullptr, this);
        Property<DescriptionData>(_T("description"), &TestController::get_description, nullptr, this);
    }

    void TestController::UnregisterAll()
    {
        Unregister(_T("run"));
        Unregister(_T("description"));
        Unregister(_T("tests"));
        Unregister(_T("categories"));
    }

    Core::JSON::ArrayType<RunResultData> TestController::TestResults(const string& results)
    {
        Core::JSON::ArrayType<RunResultData> testResultsData;

        OverallTestResults overallResults;
        if (overallResults.FromString(results)) {
            auto testResults = overallResults.Results.Elements();
            if (testResults.Count() != 0) {
                while (testResults.Next()) {
                    RunResultData testData;
                    testData.Test = testResults.Current().Name;
                    testData.Status = testResults.Current().OverallStatus;
                    testResultsData.Add(testData);
                }
            }
        }

        return testResultsData;
    }

    // API implementation
    //

    // Method: run - Run all tests - no arguments;
    // A selected category group of test - specify category;
    // A selected test - specify category and test name;
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unknown category/test
    //  - ERROR_BAD_REQUEST: Bad json param data format
    uint32_t TestController::endpoint_run(const RunParamsData& params, Core::JSON::ArrayType<RunResultData>& response)
    {
        uint32_t result = Core::ERROR_NONE;
        string ret = EMPTY_STRING;
        const string& args = params.Args.Value();

        if ((params.Category.IsSet() != true) && (params.Test.IsSet() != true)) {
            ret = RunAll(args);
            response = TestResults(ret);
            if (response.IsSet() != true) {
                result = Core::ERROR_UNAVAILABLE;
            }
        } else if ((params.Category.IsSet() == true) && (params.Test.IsSet() != true)) {
            ret = RunAll(args, params.Category.Value());
            response = TestResults(ret);
            if (response.IsSet() != true) {
                result = Core::ERROR_UNAVAILABLE;
            }
        } else if ((params.Test.IsSet() == true) && (params.Category.IsSet() == true)){
            ret = RunTest(args, params.Category.Value(), params.Test.Value());
            response = TestResults(ret);
            if (response.IsSet() != true) {
                result = Core::ERROR_UNAVAILABLE;
            }
        } else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return result;
    }

    // Property: categories - List of test categories
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t TestController::get_categories(Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        auto const categories = _testControllerImp->Categories();
        ASSERT(categories != nullptr);

        // Get list of Categories
        response = TestCategories(categories);

        return (Core::ERROR_NONE);
    }

    // Property: tests - List of test for selected category
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unknown category
    //  - ERROR_BAD_REQUEST: Bad JSON param data format
    uint32_t TestController::get_tests(const string& index, Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        if (index.empty() == false) {
           auto const category = _testControllerImp->Category(index);
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

    // Property: description - Description of the selected test from particular category
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unknown category/test
    //  - ERROR_BAD_REQUEST: Bad JSON param data format
    uint32_t TestController::get_description(const string& index, DescriptionData& response) const
    {
        uint32_t result = Core::ERROR_NONE;
        bool found = false;

        if (index.empty() == false) {
            auto categories = _testControllerImp->Categories();
            while (categories->Next()) {
                string name;
                name = categories->Category()->Name();
                auto category = _testControllerImp->Category(name);
                if (category != nullptr)
                {
                    auto tests = category->Tests();
                    while (tests->Next()) {
                        string name;
                        name = tests->Test()->Name();
                        if (name == index) {
                            response.Description = tests->Test()->Description();
                            found = true;
                            break;
                        }
                    }
                    if (found == true) {
                        break;
                    }
                }
                else
                {
                    result = Core::ERROR_UNAVAILABLE;
                    break;
                }
            }
            if (found != true) {
                result = Core::ERROR_UNAVAILABLE;
            }
        } else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return result;
    }

} // namespace Plugin

}

