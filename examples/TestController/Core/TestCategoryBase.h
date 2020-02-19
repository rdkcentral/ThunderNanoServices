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
#include <interfaces/ITestController.h>

namespace WPEFramework {
namespace TestCore {

    class TestCategoryBase : public Exchange::ITestController::ICategory {
    public:
        TestCategoryBase(const TestCategoryBase&) = delete;
        TestCategoryBase& operator=(const TestCategoryBase&) = delete;

        TestCategoryBase()
            : Exchange::ITestController::ICategory()
            , _adminLock()
            , _tests()
        {
        }

        virtual ~TestCategoryBase()
        {
            ASSERT((_tests.empty() == true) && "Something went wrong");

            for (auto& test : _tests) {
                Unregister(test.second);
            }
            _tests.clear();
        }

        // ITestCategory methods
        void Register(Exchange::ITestController::ITest* test) override
        {
            ASSERT(test != nullptr);

            _adminLock.Lock();
            auto found = _tests.find(test->Name());
            ASSERT((found == _tests.end()) && "Test already exists!");

            if (found == _tests.end()) {
                _tests[test->Name()] = test;
            }
            _adminLock.Unlock();
        }

        void Unregister(Exchange::ITestController::ITest* test) override
        {
            ASSERT(test != nullptr);

            _adminLock.Lock();
            _tests.erase(test->Name());
            _adminLock.Unlock();
        }

        Exchange::ITestController::ITest* Test(const string& name) const override
        {
            Exchange::ITestController::ITest* result = nullptr;
            _adminLock.Lock();
            auto found = _tests.find(name);
            if (found != _tests.end()) {
                result = found->second;
            }
            _adminLock.Unlock();
            return result;
        }

        Exchange::ITestController::ITest::IIterator* Tests(void) const override
        {
            _adminLock.Lock();
            auto iterator = Core::Service<TestCore::TestIterator>::Create<Exchange::ITestController::ITest::IIterator>(_tests);
            _adminLock.Unlock();
            return iterator;
        }

    private:
        mutable Core::CriticalSection _adminLock;
        TestsContainer _tests;
    };
} // namespace TestCore
} // namespace WPEFramework
