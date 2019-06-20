#pragma once

#include "../Module.h"

#include "../Core/TestAdministrator.h"
#include "../Core/TestCategoryBase.h"
#include <interfaces/ITestController.h>

namespace WPEFramework {
namespace TestCore {

    class TestCategory2 : TestCore::TestCategoryBase {
    protected:
        TestCategory2()
            : TestCategoryBase()
        {
            TestCore::TestAdministrator::Instance().Announce(this);
        }

    public:
        TestCategory2(const TestCategory2&) = delete;
        TestCategory2& operator=(const TestCategory2&) = delete;
        virtual ~TestCategory2() = default;

        static Exchange::ITestController::ICategory& Instance()
        {
            static Exchange::ITestController::ICategory* _singleton(Core::Service<TestCategory2>::Create<Exchange::ITestController::ICategory>());
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

        BEGIN_INTERFACE_MAP(TestCategory2)
        INTERFACE_ENTRY(Exchange::ITestController::ICategory)
        END_INTERFACE_MAP

    private:
        const string _name = _T("TestCategory2");
    };
} // namespace TestCore
} // namespace WPEFramework
