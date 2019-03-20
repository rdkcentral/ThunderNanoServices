#include "TestAdministrator.h"

namespace WPEFramework {
namespace TestCore {

    /* static */ TestAdministrator& TestAdministrator::Instance()
    {
        static TestAdministrator _singleton;
        return (_singleton);
    }

    void TestAdministrator::Announce(Exchange::ITestController::ICategory* category)
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

    void TestAdministrator::Revoke(Exchange::ITestController::ICategory* category)
    {
        ASSERT(category != nullptr);

        _adminLock.Lock();
        _testsCategories.erase(category->Name());
        _adminLock.Unlock();
    }

    Exchange::ITestController::ICategory::IIterator* TestAdministrator::Categories()
    {
        _adminLock.Lock();
        auto iterator = Core::Service<TestCore::CategoryIterator>::Create<Exchange::ITestController::ICategory::IIterator>(_testsCategories);
        _adminLock.Unlock();
        return iterator;
    }

    Exchange::ITestController::ICategory* TestAdministrator::Category(const string& name)
    {
        Exchange::ITestController::ICategory* result = nullptr;
        _adminLock.Lock();
        auto found = _testsCategories.find(name);
        if (found != _testsCategories.end()) {
            result = found->second;
        }
        _adminLock.Unlock();
        return result;
    }
} // namespace TestCore
} // namespace WPEFramework
