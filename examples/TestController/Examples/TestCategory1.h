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
 
#pragma once

#include "../Module.h"

#include "../Core/TestAdministrator.h"
#include "../Core/TestCategoryBase.h"
#include <interfaces/ITestController.h>

namespace WPEFramework {
namespace TestCore {

    class TestCategory1 : TestCore::TestCategoryBase {
    protected:
        TestCategory1()
            : TestCategoryBase()
        {
            TestCore::TestAdministrator::Instance().Announce(this);
        }

    public:
        TestCategory1(const TestCategory1&) = delete;
        TestCategory1& operator=(const TestCategory1&) = delete;
        virtual ~TestCategory1() = default;

        static Exchange::ITestController::ICategory& Instance()
        {
            static Exchange::ITestController::ICategory* _singleton(Core::Service<TestCategory1>::Create<Exchange::ITestController::ICategory>());
            return (*_singleton);
        }

        // ITestCategory methods
        string Name() const override
        {
            return _name;
        };

        void Setup() override{
            /*ToDo: Implement Setup for selected category */
        };

        void TearDown() override{
            /*ToDo: Implement TearDown for selected category */
        };

        BEGIN_INTERFACE_MAP(TestCategory1)
        INTERFACE_ENTRY(Exchange::ITestController::ICategory)
        END_INTERFACE_MAP

    private:
        const string _name = _T("TestCategory1");
    };
} // namespace TestCore
} // namespace WPEFramework
