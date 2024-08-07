/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
 
#include "Module.h"

#include <qa_interfaces/json/JTestController.h>

#include "Core/TestAdministrator.h"

namespace Thunder {
namespace TestCore {

    using StringList = RPC::IteratorType<QualityAssurance::ITestController::IStringIterator>;
    using TestResultIterator = RPC::IteratorType<QualityAssurance::ITestController::ITestResultIterator>;
    using TestResultList = std::list<QualityAssurance::ITestController::TestResult>;

    class TestControllerImp : public QualityAssurance::ITestController {
    public:
        TestControllerImp()
            : _prevCategory(EMPTY_STRING)
	{
	}
        TestControllerImp(const TestControllerImp&) = delete;
        TestControllerImp& operator=(const TestControllerImp&) = delete;

        ~TestControllerImp() override {
            TestCore::TestAdministrator::Instance().Release();
        }

        //  TestControllerImp methods
        // -------------------------------------------------------------------------------------------------------
        void Setup() override {
            //This is overall TestController setup
            //ToDo: Do 'Setup' for ITestController if it is needed
        };
        void TearDown() override {
            //This is overall TestController tear down
            //ToDo: Do 'Tear Down' for ITestController if it is needed
        };

        void TestPreparation(QualityAssurance::ITestController::ICategory* category, const string& categoryName)
        {
            if ((_prevCategory != categoryName) & (_prevCategory != EMPTY_STRING)) {
                const auto prevCategory = TestCore::TestAdministrator::Instance().Category(_prevCategory);
                if (prevCategory != nullptr) {
                    prevCategory->TearDown();
                }
                category->Setup();
                _prevCategory = categoryName;
            }
	}

	inline void Execute(const string& args, QualityAssurance::ITestController::ITest* test, TestResultList& resultList)
	{
            string result = test->Execute(args);
            JsonData::TestController::TestResultData jsonResult;
            if (jsonResult.FromString(result)) {
                QualityAssurance::ITestController::TestResult testResult;
                testResult.test = jsonResult.Test.Value();
   	        testResult.status = jsonResult.Status.Value();
	        resultList.push_back(testResult);
	    }
        }

        uint32_t Description(const string& test, string& description) const override
	{
	    uint32_t result = Core::ERROR_UNAVAILABLE;
            bool found = false;

            if (test.empty() != true) {
                auto categories = TestCore::TestAdministrator::Instance().Categories();
                while (categories->Next()) {
                    string name;
                    name = categories->Category()->Name();
                    auto category = TestCore::TestAdministrator::Instance().Category(name);
                    if (category != nullptr) {
                        auto tests = category->Tests();
                        while (tests->Next()) {
                            string name;
                            name = tests->Test()->Name();
                            if (name == test) {
                                description = tests->Test()->Description();
	                        result = Core::ERROR_NONE;
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
                        break;
                    }
                }
            } else {
                result = Core::ERROR_BAD_REQUEST;
            }

            return result;
	}

        uint32_t Categories(IStringIterator*& categories) const override
	{
            auto const supportdCategaries = TestCore::TestAdministrator::Instance().Categories();
            ASSERT(supportdCategaries != nullptr);

            std::list<string> categoriesList;
            while (supportdCategaries->Next()) {
                categoriesList.push_back(supportdCategaries->Category()->Name());
            }
            if (categoriesList.empty() != true) {
                categories = Core::ServiceType<StringList>::Create<StringList>(categoriesList);
            }

            return (Core::ERROR_NONE);
	}

        uint32_t Tests(const string& categoryName, IStringIterator*& tests) const override
	{
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (categoryName.empty() != true) {
                auto const category = TestCore::TestAdministrator::Instance().Category(categoryName);
                if (category != nullptr) {
                    // Get list of Categories
                    std::list<string> testsList;
                    auto const supportedTests = category->Tests(); 
                    ASSERT(supportedTests != nullptr);
                    if (supportedTests != nullptr) {
                        while (supportedTests->Next()) {
                            testsList.push_back(supportedTests->Test()->Name());
			}
		    }
		    if (testsList.empty() != true) {
                        tests = Core::ServiceType<StringList>::Create<StringList>(testsList);
			result = Core::ERROR_NONE;
		    }
                }
            } else {
                result = Core::ERROR_BAD_REQUEST;
            }

            return result;
	}

        uint32_t Run(const TestInfo& testInfo, QualityAssurance::ITestController::ITestResultIterator*& testResults) override
	{
            uint32_t status = Core::ERROR_UNAVAILABLE;
            TestResultList resultList;

            if (testInfo.category.empty() == true) {
                const auto categories = TestCore::TestAdministrator::Instance().Categories();

                if (categories != nullptr) {
                    while (categories->Next()) {
                        TestPreparation(categories->Category(), categories->Category()->Name());

                        auto tests = categories->Category()->Tests();
                        while (tests->Next()) {
                            Execute(testInfo.args, tests->Test(), resultList);
                        }
                    }
                }
            } else {
                const auto category = TestCore::TestAdministrator::Instance().Category(testInfo.category);
                if (category != nullptr) {
                    TestPreparation(category, testInfo.category);
		    if (testInfo.test.empty() == true) {
                        auto tests = category->Tests();
                        while (tests->Next()) {
                            Execute(testInfo.args, tests->Test(), resultList);
                        }
                    } else {

                        auto test = category->Test(testInfo.test);
                        if (test != nullptr) {
                            Execute(testInfo.args, test, resultList);
                        }
                    }
		}
	    }
	    if (resultList.empty() != true) {
	        testResults = Core::ServiceType<TestResultIterator>::Create<TestResultIterator>(resultList);
                status = Core::ERROR_NONE;
	    }

            return status;
	}

 
        BEGIN_INTERFACE_MAP(TestControllerImp)
        INTERFACE_ENTRY(QualityAssurance::ITestController)
        END_INTERFACE_MAP
    private:
        string _prevCategory;
    };

    SERVICE_REGISTRATION(TestControllerImp, 1, 0)
} // namespace TestCore
} // namespace WPEFramewor
