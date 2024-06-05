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
 
#include "TestAdministrator.h"

namespace Thunder {
namespace TestCore {

    /* static */ TestAdministrator& TestAdministrator::Instance()
    {
        static TestAdministrator* _singleton(Core::ServiceType<TestAdministrator>::Create<TestAdministrator>());;
        return (*_singleton);
    }

    void TestAdministrator::Announce(QualityAssurance::ITestController::ICategory* category)
    {
        ASSERT(category != nullptr);

        _adminLock.Lock();
        auto found = _testsCategories.find(category->Name());
        ASSERT((found == _testsCategories.end()) && "Category already exists!");

        if (found == _testsCategories.end()) {
            _testsCategories[category->Name()] = category;
        }
        _adminLock.Unlock();
    }

    void TestAdministrator::Revoke(QualityAssurance::ITestController::ICategory* category)
    {
        ASSERT(category != nullptr);

        _adminLock.Lock();
        _testsCategories.erase(category->Name());
        _adminLock.Unlock();
    }

    QualityAssurance::ITestController::ICategory::IIterator* TestAdministrator::Categories()
    {
        _adminLock.Lock();
        auto iterator = Core::ServiceType<TestCore::CategoryIterator>::Create<QualityAssurance::ITestController::ICategory::IIterator>(_testsCategories);
        _adminLock.Unlock();
        return iterator;
    }

    QualityAssurance::ITestController::ICategory* TestAdministrator::Category(const string& name)
    {
        QualityAssurance::ITestController::ICategory* result = nullptr;
        _adminLock.Lock();
        auto found = _testsCategories.find(name);
        if (found != _testsCategories.end()) {
            result = found->second;
        }
        _adminLock.Unlock();
        return result;
    }
} // namespace TestCore
} // namespace Thunder
