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
 
#pragma once

#include "../Module.h"

#include <qa_interfaces/ITestController.h>

namespace Thunder {
namespace TestCore {

    using TestsContainer = std::map<string, QualityAssurance::ITestController::ITest*>;
    using CategoriesContainer = std::map<string, QualityAssurance::ITestController::ICategory*>;

    class TestIterator : public QualityAssurance::ITestController::ITest::IIterator {
    public:
        TestIterator(const TestIterator&) = delete;
        TestIterator& operator=(const TestIterator&) = delete;

        using IteratorImpl = Core::IteratorMapType<TestsContainer, QualityAssurance::ITestController::ITest*, string>;

        explicit TestIterator(const TestsContainer& tests)
            : QualityAssurance::ITestController::ITest::IIterator()
            , _container(tests)
            , _iterator(_container)
        {
        }

        ~TestIterator() override = default;

        BEGIN_INTERFACE_MAP(TestIterator)
        INTERFACE_ENTRY(QualityAssurance::ITestController::ITest::IIterator)
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

        QualityAssurance::ITestController::ITest* Test() const override
        {
            return *_iterator;
        }

    private:
        TestsContainer _container;
        IteratorImpl _iterator;
    };

    class CategoryIterator : public QualityAssurance::ITestController::ICategory::IIterator {
    public:
        CategoryIterator(const CategoryIterator&) = delete;
        CategoryIterator& operator=(const CategoryIterator&) = delete;

        using IteratorImpl = Core::IteratorMapType<CategoriesContainer, QualityAssurance::ITestController::ICategory*, string>;

        explicit CategoryIterator(const CategoriesContainer& tests)
            : QualityAssurance::ITestController::ICategory::IIterator()
            , _container(tests)
            , _iterator(_container)
        {
        }

        ~CategoryIterator() override = default;

        BEGIN_INTERFACE_MAP(CategoryIterator)
        INTERFACE_ENTRY(QualityAssurance::ITestController::ICategory::IIterator)
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

        QualityAssurance::ITestController::ICategory* Category() const override
        {
            return *_iterator;
        }

    private:
        CategoriesContainer _container;
        IteratorImpl _iterator;
    };

    class TestAdministrator : virtual public Core::IUnknown {
    protected:
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
            for (auto& testCategory : _testsCategories) {

                QualityAssurance::ITestController::ITest::IIterator* existingTests = testCategory.second->Tests();
                while (existingTests->Next()) {
                    existingTests->Test()->Release();
                }
                existingTests->Release();
                testCategory.second->Release();
            }
        }

        void Announce(QualityAssurance::ITestController::ICategory* category);
        void Revoke(QualityAssurance::ITestController::ICategory* category);

        // TestController methods
        QualityAssurance::ITestController::ICategory::IIterator* Categories();
        QualityAssurance::ITestController::ICategory* Category(const string& name);

        BEGIN_INTERFACE_MAP(TestAdministrator)
        END_INTERFACE_MAP
    private:
        mutable Core::CriticalSection _adminLock;
        CategoriesContainer _testsCategories;
    };
} // namespace TestCore
} // namespace Thunder
