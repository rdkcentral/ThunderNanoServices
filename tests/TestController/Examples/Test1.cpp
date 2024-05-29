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
 
#include "../Module.h"

#include "../Core/TestBase.h"
#include "../Core/Trace.h"
#include "TestCategory1.h"
#include <qa_interfaces/ITestController.h>

namespace WPEFramework {

class Test1 : public TestBase {
public:
    Test1(const Test1&) = delete;
    Test1& operator=(const Test1&) = delete;

    Test1()
        : TestBase(TestBase::DescriptionBuilder("Test 1 description"))
    {
        TestCore::TestCategory1::Instance().Register(this);
    }

    ~Test1() override
    {
        TestCore::TestCategory1::Instance().Unregister(this);
    }

public:
    // ICommand methods
    string Execute(const string& params VARIABLE_IS_NOT_USED) final
    {
        TestCore::TestResult jsonResult;
        string result;
        TRACE(TestCore::TestStart, (_T("Start execute of test: %s"), _name.c_str()));

        TRACE(TestCore::TestStep, (_T("Do real test execution"), _name.c_str()));
        jsonResult.Name = _name;
        jsonResult.OverallStatus = "Success";

        TRACE(TestCore::TestStart, (_T("End test: %s"), _name.c_str()));
        jsonResult.ToString(result);
        return result;
    }

    string Name() const final
    {
        return _name;
    }

private:
    const string _name = _T("Test1");
};

static QualityAssurance::ITestController::ITest* _singleton(Core::ServiceType<Test1>::Create<QualityAssurance::ITestController::ITest>());
} // namespace WPEFramework
