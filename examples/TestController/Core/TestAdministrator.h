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

#include <interfaces/ITestController.h>

namespace WPEFramework {
namespace TestCore {

    using TestsContainer = std::map<string, Exchange::ITestController::ITest*>;
    using CategoriesContainer = std::map<string, Exchange::ITestController::ICategory*>;

    class TestIterator : public Exchange::ITestController::ITest::IIterator {
    public:
        TestIterator(const TestIterator&) = delete;
        TestIterator& operator=(const TestIterator&) = delete;

        using IteratorImpl = Core::IteratorMapType<TestsContainer, Exchange::ITestController::ITest*, string>;

        explicit TestIterator(const TestsContainer& tests)
            : Exchange::ITestController::ITest::IIterator()
            , _container(tests)
            , _iterator(_container)
        {
        }

        virtual ~TestIterator() = default;

        BEGIN_INTERFACE_MAP(TestIterator)
        INTERFACE_ENTRY(Exchange::ITestController::ITest::IIterator)
        END_INTERFACE_MAP

        void Reset() override
        {
            _iterator.Reset(0);
        }

        bool IsValid() const override
        {
            return _iterator.IsValid();
        }

        bool Next() override
        {
            return _iterator.Next();
        }

        Exchange::ITestController::ITest* Test() const override
        {
            return *_iterator;
        }

    private:
        TestsContainer _container;
        IteratorImpl _iterator;
    };

    class CategoryIterator : public Exchange::ITestController::ICategory::IIterator {
    public:
        CategoryIterator(const CategoryIterator&) = delete;
        CategoryIterator& operator=(const CategoryIterator&) = delete;

        using IteratorImpl = Core::IteratorMapType<CategoriesContainer, Exchange::ITestController::ICategory*, string>;

        explicit CategoryIterator(const CategoriesContainer& tests)
            : Exchange::ITestController::ICategory::IIterator()
            , _container(tests)
            , _iterator(_container)
        {
        }

        virtual ~CategoryIterator() = default;

        BEGIN_INTERFACE_MAP(CategoryIterator)
        INTERFACE_ENTRY(Exchange::ITestController::ICategory::IIterator)
        END_INTERFACE_MAP

        void Reset() override
        {
            _iterator.Reset(0);
        }

        bool IsValid() const override
        {
            return _iterator.IsValid();
        }

        bool Next() override
        {
            return _iterator.Next();
        }

        Exchange::ITestController::ICategory* Category() const override
        {
            return *_iterator;
        }

    private:
        CategoriesContainer _container;
        IteratorImpl _iterator;
    };

    class TestAdministrator {
    private:
        TestAdministrator()
            : _adminLock()
            , _testsCategories()
        {
        }

    public:
        TestAdministrator(const TestAdministrator&) = delete;
        TestAdministrator& operator=(const TestAdministrator&) = delete;

        static TestAdministrator& Instance();
        ~TestAdministrator()
        {
            ASSERT((_testsCategories.empty() == true) && "Something went wrong");

            for (auto& testCategory : _testsCategories) {
                Exchange::ITestController::ITest::IIterator* existingTests = testCategory.second->Tests();
                while (existingTests->Next()) {
                    testCategory.second->Unregister(existingTests->Test());
                }
                Revoke(testCategory.second);
            }
        }

        void Announce(Exchange::ITestController::ICategory* category);
        void Revoke(Exchange::ITestController::ICategory* category);

        // TestController methods
        Exchange::ITestController::ICategory::IIterator* Categories();
        Exchange::ITestController::ICategory* Category(const string& name);

    private:
        mutable Core::CriticalSection _adminLock;
        CategoriesContainer _testsCategories;
    };
} // namespace TestCore
} // namespace WPEFramework
