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
 
#include "../Module.h"

#include "../Core/TestBase.h"
#include "../Core/Trace.h"
#include "TestCategory2.h"
#include <interfaces/ITestController.h>

namespace WPEFramework {

class Test4 : public TestBase {
public:
    Test4(const Test4&) = delete;
    Test4& operator=(const Test4&) = delete;

    Test4()
        : TestBase(TestBase::DescriptionBuilder("Test 4 description"))
    {
        TestCore::TestCategory2::Instance().Register(this);
        ;
    }

    virtual ~Test4()
    {
        TestCore::TestCategory2::Instance().Unregister(this);
    }

public:
    // ICommand methods
    string Execute(const string& params) final
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
    const string _name = _T("Test4");
};

static Exchange::ITestController::ITest* _singleton(Core::Service<Test4>::Create<Exchange::ITestController::ITest>());
} // namespace WPEFramework
