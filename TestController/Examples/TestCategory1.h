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
